#!/usr/bin/env python3
"""One-off migration helper: Bazel firmware targets -> mk/sw_targets.mk + mk/gen_sources.json.

BUILD and .bzl files are valid Python, so each BUILD file is *executed* with
stubbed Bazel rules that merely record their arguments. That makes macros,
list comprehensions and `template_rule(...)` expand exactly as Bazel would.

Outputs
  mk/sw_targets.mk    one block per `coralnpu_v2_binary`:
      <id>_PKG/_NAME/_SRCS/_COPTS/_DEFS/_LDS/_SEMIHOSTING/_NOCRT
      <id>_GENSRCS   generated sources (paths relative to the build dir)
  mk/gen_sources.json templates to expand (see utils/expand_templates.py)
Targets with deps that are not local header/source libraries (FreeRTOS,
TFLite, ...) are listed as UNSUPPORTED.
"""
import ast
import json
import os
import pathlib
import sys
import types

ROOT = pathlib.Path(__file__).resolve().parent.parent
SKIP = {"third_party", "rules", "build", "target", "node_modules", "vendor", ".bender"}

RECORDS = {}            # kind -> [kwargs]
GENERATED = {}          # (pkg, target name) -> repo-relative generated file
GEN_SPECS = []          # template expansion jobs
CUR_PKG = [""]


class _Attr:
    """Stand-in for Bazel's `attr` module."""

    def __getattr__(self, kind):
        def mk(*a, **kw):
            return {"kind": kind, **kw}
        return mk


class _Rule:
    """Result of `rule(...)`: calling it runs the implementation with a fake ctx."""

    def __init__(self, implementation, attrs, **_):
        self.impl = implementation
        self.attrs = attrs

    def __call__(self, name, **kw):
        pkg = CUR_PKG[0]
        attrs = {}
        for k, spec in self.attrs.items():
            if k in kw:
                attrs[k] = kw[k]
            elif isinstance(spec, dict) and "default" in spec:
                attrs[k] = spec["default"]
        attrs["name"] = name
        files = {}
        for k, v in attrs.items():
            if isinstance(v, str) and (v.startswith(":") or v.startswith("//")) and k.startswith("_"):
                p = v.split(":")[-1] if v.startswith("//") else v[1:]
                files[k] = str(pathlib.Path(pkg) / p)
        outputs = {}
        for k, spec in self.attrs.items():
            if isinstance(spec, dict) and spec.get("kind") == "output" and k in kw:
                outputs[k] = str(pathlib.Path(pkg) / kw[k])
        recorded = {}

        class Actions:
            def expand_template(self, template, output, substitutions):
                recorded.update(template=template, output=output, substitutions=substitutions)

        ctx = types.SimpleNamespace(
            attr=types.SimpleNamespace(**attrs),
            file=types.SimpleNamespace(**files),
            outputs=types.SimpleNamespace(**outputs),
            actions=Actions(), label=types.SimpleNamespace(name=name))
        self.impl(ctx)
        if recorded:
            GEN_SPECS.append(recorded)
            GENERATED[(pkg, name)] = recorded["output"]


def record(kind):
    def f(**kw):
        kw["_pkg"] = CUR_PKG[0]
        RECORDS.setdefault(kind, []).append(kw)
    return f


def template_rule(rule, name_map, **kwargs):
    for name, over in name_map.items():
        kw = dict(kwargs)
        kw.update(over)
        rule(name=name, **kw)


def _ignored(*a, **k):
    return None


def _tflm_path(label, pkg):
    """Label of a data file -> repo-relative path (external TFLM maps to vendor/)."""
    if label.startswith("@tflite_micro//"):
        d, f = label[len("@tflite_micro//"):].split(":")
        return f"vendor/tflite_micro/{d}/{f}"
    if label.startswith("//"):
        d, f = label[2:].split(":")
        return f"{d}/{f}"
    return f"{pkg}/{label.lstrip(':')}"


def generate_cc_arrays(name, src, out, tags=None, extdata=False, section=None):
    pkg = CUR_PKG[0]
    rel = f"genfiles/{pkg}/{out}"
    GEN_SPECS.append({"tool": "cc_array", "src": _tflm_path(src, pkg), "output": rel,
                      "section": "extdata" if extdata else (section or "")})
    GENERATED[(pkg, name)] = rel


SIM_TESTS = []   # coralnpu_v2_sim_test(...) macro calls


def coralnpu_v2_sim_test(name, srcs, sim_timeout=30, trace=False, highmem=False,
                         loader="backdoor", size="large", tags=None, **kwargs):
    """rules/coralnpu_v2_sim_test.bzl: a firmware binary + a run on the SoC simulation."""
    if highmem:
        kwargs.setdefault("itcm_size_kbytes", 1024)
        kwargs.setdefault("dtcm_size_kbytes", 1024)
    RECORDS.setdefault("coralnpu_v2_binary", []).append(
        dict(kwargs, name=name + "_binary", srcs=srcs, _pkg=CUR_PKG[0]))
    SIM_TESTS.append(dict(name=name, pkg=CUR_PKG[0], binary=name + "_binary",
                          timeout=sim_timeout, highmem=highmem, loader=loader,
                          itcm=kwargs.get("itcm_size_kbytes", 1024 if highmem else 8),
                          dtcm=kwargs.get("dtcm_size_kbytes", 1024 if highmem else 32)))


STUBS = {
    "coralnpu_v2_sim_test": coralnpu_v2_sim_test,
    "package": _ignored, "filegroup": _ignored, "exports_files": _ignored,
    "py_binary": _ignored, "py_library": _ignored, "py_test": _ignored,
    "genrule": _ignored, "sh_test": _ignored, "sh_binary": _ignored,
    "cc_binary": _ignored, "cc_test": _ignored, "pkg_zip": _ignored,
    "glob": lambda *a, **k: [], "select": lambda d, **k: next(iter(d.values()), []),
    "rule": _Rule, "attr": _Attr(), "native": types.SimpleNamespace(
        package_name=lambda: CUR_PKG[0], existing_rules=lambda: {},
        filegroup=_ignored, genrule=_ignored, sh_test=_ignored),
    "template_rule": template_rule,
    "generate_cc_arrays": generate_cc_arrays,
    "requirement": lambda n: n,
    "Label": lambda s: s,
}
RECORDED_KINDS = ["coralnpu_v2_binary", "cc_library"]
for _k in RECORDED_KINDS:
    STUBS[_k] = record(_k)


def make_env():
    env = dict(STUBS)
    return env


_BZL_CACHE = {}


def load_bzl(label, pkg):
    """Execute a repo-local .bzl and return its namespace (None if not local)."""
    if label.startswith("@") and not label.startswith("@coralnpu_hw"):
        return None
    label = label.replace("@coralnpu_hw", "")
    if label.startswith("//"):
        d, f = label[2:].split(":")
    elif label.startswith(":"):
        d, f = pkg, label[1:]
    else:
        return None
    if d.startswith("rules") or d.startswith("third_party"):
        return None
    path = ROOT / d / f
    if not path.exists():
        return None
    key = str(path)
    if key not in _BZL_CACHE:
        _BZL_CACHE[key] = None
        _BZL_CACHE[key] = run_file(path, d)
    return _BZL_CACHE[key]


def run_file(path, pkg):
    """exec a BUILD/.bzl file with stubs; returns its namespace."""
    tree = ast.parse(path.read_text())
    env = make_env()
    body = []
    for node in tree.body:
        if (isinstance(node, ast.Expr) and isinstance(node.value, ast.Call)
                and getattr(node.value.func, "id", "") == "load"):
            args = node.value.args
            label = args[0].value
            ns = load_bzl(label, pkg)
            for a in args[1:]:
                name = a.value if isinstance(a, ast.Constant) else None
                if name is None:
                    continue
                if ns is not None and name in ns:
                    env[name] = ns[name]
                elif name not in env:
                    env[name] = record("__" + name)
            for kw in node.value.keywords:   # load(..., alias = "sym")
                src = kw.value.value
                if ns is not None and src in ns:
                    env[kw.arg] = ns[src]
            continue
        body.append(node)
    saved = CUR_PKG[0]
    CUR_PKG[0] = pkg
    code = compile(ast.Module(body=body, type_ignores=[]), str(path), "exec")
    try:
        # Unknown rules (alias, constraint_setting, ...) are stubbed on demand;
        # re-running is safe because stubs are the only side effects, but the
        # recorders must not double-count, so snapshot and restore them.
        for _ in range(200):
            snap = {k: list(v) for k, v in RECORDS.items()}
            gsnap, nspecs = dict(GENERATED), len(GEN_SPECS)
            try:
                exec(code, env)
                break
            except NameError as e:
                missing = getattr(e, "name", None)
                if not missing or missing in env:
                    raise
                RECORDS.clear(); RECORDS.update(snap)
                GENERATED.clear(); GENERATED.update(gsnap)
                del GEN_SPECS[nspecs:]
                env[missing] = _ignored
    except Exception as e:
        sys.stderr.write(f"warning: {path}: {type(e).__name__}: {e}\n")
    finally:
        CUR_PKG[0] = saved
    return env


# Third-party firmware libraries that used to be Bazel external repos. They are
# now Bender vendor_packages under vendor/ (see Bender.yml).
_FRT = "vendor/freertos"
EXTERNAL = {
    "@freertos": {
        "srcs": [f"{_FRT}/{s}" for s in (
            "croutine.c", "event_groups.c", "list.c", "queue.c", "stream_buffer.c",
            "tasks.c", "timers.c", "portable/GCC/RISC-V/port.c",
            "portable/GCC/RISC-V/portASM.S", "portable/MemMang/heap_4.c")],
        "copts": [f"-I{_FRT}/include", f"-I{_FRT}/portable/GCC/RISC-V"],
    },
    "//third_party/freertos:config": {"copts": ["-Ithird_party/freertos/config"]},
}


def norm_label(dep, pkg):
    if dep.startswith(":"):
        return f"//{pkg}:{dep[1:]}"
    if dep.startswith("//") and ":" not in dep:
        return f"{dep}:{dep.split('/')[-1]}"
    return dep


def main():
    builds = sorted(set(ROOT.glob("**/BUILD")) | set(ROOT.glob("**/BUILD.bazel")))
    for b in builds:
        keep = "third_party/waveshare_display" in str(b.parent)
        if not b.is_file() or (SKIP & set(b.parts) and not keep):
            continue
        if b.name == "BUILD.bazel" and (b.parent / "BUILD").exists():
            continue
        run_file(b, str(b.parent.relative_to(ROOT)))

    libs = {}
    for kw in RECORDS.get("cc_library", []):
        libs[f"//{kw['_pkg']}:{kw['name']}"] = kw

    TFLM_MARK = "@TFLM@"

    def resolve(deps, pkg):
        """Flatten local cc_library deps -> (srcs, gens, defines, copts) or None if unsupported."""
        srcs, gens, defs, copts = [], [], [], []
        for d in deps:
            lbl = norm_label(d, pkg)
            if lbl.startswith("//toolchain/crt"):
                continue
            if lbl.startswith("@tflite_micro//"):
                copts.append(TFLM_MARK)
                continue
            if lbl in EXTERNAL:
                ext = EXTERNAL[lbl]
                srcs += ext.get("srcs", [])
                copts += ext.get("copts", [])
                continue
            lib = libs.get(lbl)
            if lib is None:
                return None
            for s_ in lib.get("srcs", []):
                if (lib["_pkg"], s_.lstrip(":")) in GENERATED:
                    gens.append(GENERATED[(lib["_pkg"], s_.lstrip(":"))])
                else:
                    srcs.append(f"{lib['_pkg']}/{s_}")
            defs += lib.get("defines", [])
            copts += lib.get("copts", [])
            for inc in lib.get("includes", []):
                d = os.path.normpath(f"{lib['_pkg']}/{inc}")
                copts.append(f"-I{d}")
            sub = resolve(lib.get("deps", []), lib["_pkg"])
            if sub is None:
                return None
            srcs += sub[0]; gens += sub[1]; defs += sub[2]; copts += sub[3]
        return srcs, gens, defs, copts

    out, unsupported = [], []
    for kw in RECORDS.get("coralnpu_v2_binary", []):
        pkg, name = kw["_pkg"], kw["name"]
        tid = (pkg.replace("/", "_") + "_" + name).replace("-", "_")
        crt_srcs = []
        if kw.get("crt"):
            crt_res = resolve([kw["crt"]], pkg)
            if crt_res is None:
                unsupported.append((pkg, name, f"crt={kw.get('crt')}"))
                continue
            crt_srcs = crt_res[0]
        ld_script = ""
        if kw.get("linker_script"):
            lsp = kw["linker_script"]
            if lsp.startswith((":", "//", "@")):
                unsupported.append((pkg, name, f"ld={lsp}"))
                continue
            ld_script = f"{pkg}/{lsp}"
        res = resolve(kw.get("deps") or [], pkg)
        if res is None:
            unsupported.append((pkg, name, f"deps={kw.get('deps')}"))
            continue
        srcs, gens = [], list(res[1])
        bad = False
        for s in kw.get("srcs", []):
            if (pkg, s.lstrip(":")) in GENERATED:
                gens.append(GENERATED[(pkg, s.lstrip(":"))])
            elif s.startswith(("//", "@")) or (s.startswith(":") and (pkg, s[1:]) not in GENERATED):
                bad = True
            elif s.endswith((".h", ".hh", ".hpp", ".inc")):
                pass          # headers listed in srcs are not compiled
            else:
                srcs.append(f"{pkg}/{s}")
        if bad:
            unsupported.append((pkg, name, f"srcs={kw.get('srcs')}"))
            continue
        out.append((tid, pkg, name, kw, srcs + crt_srcs, gens, res, ld_script))

    lines = ["# Generated by utils/extract_sw.py from the former Bazel targets.",
             "# Firmware built with the CoralNPU toolchain; see mk/sw.mk.",
             "# _SRCS are repo-relative; _GENSRCS are generated under $(BUILD)/gen (mk/gen_sources.json).", ""]
    for tid, pkg, name, kw, srcs, gens, (dsrcs, dgens, ddefs, dcopts), ld_script in out:
        lines.append(f"SW_TARGETS += {tid}")
        lines.append(f"{tid}_PKG  := {pkg}")
        lines.append(f"{tid}_NAME := {name}")
        lines.append(f"{tid}_SRCS := {' '.join(dict.fromkeys(srcs + dsrcs))}")
        if gens:
            lines.append(f"{tid}_GENSRCS := {' '.join(dict.fromkeys(gens))}")
        copts = (kw.get("copts") or []) + dcopts
        if TFLM_MARK in copts:
            lines.append(f"{tid}_TFLM := 1")
            copts = [c for c in copts if c != TFLM_MARK]
        if copts:
            lines.append(f"{tid}_COPTS := {' '.join(copts)}")
        defs = (kw.get("defines") or []) + ddefs
        if defs:
            lines.append(f"{tid}_DEFS := {' '.join(defs)}")
        lds = (kw.get("itcm_size_kbytes", 8), kw.get("dtcm_size_kbytes", 32),
               kw.get("stack_size_bytes", 128), kw.get("heap_size", "") or "-",
               kw.get("heap_location", "DTCM"))
        lines.append(f"{tid}_LDS := {' '.join(map(str, lds))}")
        if kw.get("crt") is not None and kw.get("crt") != "None":
            lines.append(f"{tid}_NOCRT := 1")   # empty or custom crt replaces the default runtime
        if ld_script:
            lines.append(f"{tid}_LDSCRIPT := {ld_script}")
        if kw.get("semihosting"):
            lines.append(f"{tid}_SEMIHOSTING := 1")
        lines.append("")
    lines.append("# ---- not yet ported (need extra libraries / custom runtime) ----")
    for pkg, name, why in unsupported:
        lines.append(f"# UNSUPPORTED {pkg}:{name}  {why}")
    (ROOT / "mk/sw_targets.mk").write_text("\n".join(lines) + "\n")

    ft = ["# Generated by utils/extract_sw.py: FPGA SoC simulation tests (see mk/fpga.mk).", ""]
    seen_tests = set()
    for t in SIM_TESTS:
        if (t["pkg"], t["name"]) in seen_tests:
            continue   # BUILD files are re-executed when a stub is missing
        seen_tests.add((t["pkg"], t["name"]))
        tid = t["pkg"].replace("/", "_") + "_" + t["name"]
        ft += [f"FPGA_SIM_TESTS += {tid}",
               f"{tid}_ELF := {t['pkg']}/{t['binary']}",
               f"{tid}_TIMEOUT := {t['timeout']}",
               f"{tid}_HIGHMEM := {1 if t['highmem'] else 0}",
               f"{tid}_LOADER := {t['loader']}",
               f"{tid}_ITCM := {t['itcm']}",
               f"{tid}_DTCM := {t['dtcm']}", ""]
    (ROOT / "mk/fpga_tests.mk").write_text("\n".join(ft) + "\n")

    specs = [s_ if "tool" in s_ else
             {"template": s_["template"], "output": s_["output"], "substitutions": s_["substitutions"]}
             for s_ in GEN_SPECS]
    (ROOT / "mk/gen_sources.json").write_text(json.dumps(specs, indent=1, sort_keys=True) + "\n")
    sys.stderr.write(f"{len(out)} firmware targets, {len(unsupported)} unsupported, "
                     f"{len(specs)} generated sources\n")


if __name__ == "__main__":
    main()
