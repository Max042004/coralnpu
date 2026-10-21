#!/usr/bin/env python3
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""One-off migration helper: translate the Bazel C++ build graph of the MPACT
stack (vendor/mpact-sim, vendor/mpact-riscv, vendor/coralnpu-mpact) into Make.

BUILD files are valid Python, so they are *executed* with stub rules that record
their arguments (the same approach as utils/extract_sw.py); the repositories'
own macros (.bzl: mpact_isa_decoder, antlr4_cc_parser, ...) run unmodified and
expand into genrule + cc_library. The closure of the requested root targets is
emitted as explicit Make rules:

    mk/mpact_rules.mk          objects, archives, executables, genrule rules
    mk/mpact_cmds/<id>.sh      one shell script per genrule (Bazel `cmd` with the
                               location/SRCS/@D variables substituted)

External dependencies are mapped to system / Bender-built libraries in
mk/mpact.mk (abseil, re2, protobuf, antlr4 runtime, elfio, googletest).

    bazel2make.py <root label> [<root label> ...]
"""
import ast
import glob as _glob
import os
import pathlib
import re
import sys
import types

ROOT = pathlib.Path(__file__).resolve().parent.parent

# alias -> (canonical repo name, directory relative to ROOT)
REPOS = {}
for alias in ("mpact-sim", "com_google_mpact-sim"):
    REPOS[alias] = ("mpact-sim", "vendor/mpact-sim")
for alias in ("mpact-riscv", "com_google_mpact-riscv"):
    REPOS[alias] = ("mpact-riscv", "vendor/mpact-riscv")
for alias in ("coralnpu_mpact", "coralnpu-mpact"):
    REPOS[alias] = ("coralnpu-mpact", "vendor/coralnpu-mpact")
REPO_DIRS = {name: d for name, d in REPOS.values()}


class Label:
    def __init__(self, repo, pkg, name):
        self.repo, self.pkg, self.name = repo, pkg, name

    def key(self):
        return (self.repo, self.pkg, self.name)

    def __repr__(self):
        return f"@{self.repo}//{self.pkg}:{self.name}"

    def __hash__(self):
        return hash(self.key())

    def __eq__(self, o):
        return isinstance(o, Label) and self.key() == o.key()


def parse_label(s, repo, pkg):
    """Parse a label string; external (non-source) repos keep their alias as `repo`."""
    if s.startswith("@"):
        r, rest = s[1:].split("//", 1) if "//" in s else (s[1:], "")
        if r == "":
            r = repo
        elif r in REPOS:
            r = REPOS[r][0]
        else:
            if ":" in rest:
                p, n = rest.split(":", 1)
            else:
                p, n = rest, rest.split("/")[-1] if rest else r
            return Label("@" + r, p, n)
        repo = r
        s = "//" + rest
    if s.startswith("//"):
        body = s[2:]
        if ":" in body:
            p, n = body.split(":", 1)
        else:
            p, n = body, body.split("/")[-1]
        return Label(repo, p, n)
    if s.startswith(":"):
        return Label(repo, pkg, s[1:])
    return Label(repo, pkg, s)  # bare file name in this package


# ---------------------------------------------------------------- evaluator

TARGETS = {}          # Label -> dict(kind, kw)
OUT_INDEX = {}        # (repo, pkg, out file) -> genrule Label
LOADED_PKGS = set()
CUR = {"repo": "", "pkg": ""}
BZL_CACHE = {}


def record(kind):
    def f(**kw):
        name = kw.get("name")
        if name is None:
            return
        lbl = Label(CUR["repo"], CUR["pkg"], name)
        TARGETS[lbl] = {"kind": kind, "kw": kw, "label": lbl}
        if kind == "genrule":
            for o in kw.get("outs", []):
                OUT_INDEX[(CUR["repo"], CUR["pkg"], o)] = lbl
    return f


def ignored(*a, **k):
    return None


def make_glob(pkgdir):
    def glob(include=(), exclude=(), exclude_directories=1, allow_empty=True):
        out = []
        for pat in include:
            for m in _glob.glob(str(pkgdir / pat), recursive=True):
                p = pathlib.Path(m)
                if p.is_file():
                    out.append(str(p.relative_to(pkgdir)))
        ex = set()
        for pat in exclude:
            for m in _glob.glob(str(pkgdir / pat), recursive=True):
                ex.add(str(pathlib.Path(m).relative_to(pkgdir)))
        return sorted(set(out) - ex)
    return glob


def template_ns():
    ns = {}
    for k in ("cc_library", "cc_binary", "cc_test", "genrule", "filegroup", "proto_library",
              "cc_proto_library", "alias", "exports_files"):
        ns[k] = record(k)
    ns["exports_files"] = ignored
    for k in ("package", "license", "licenses", "py_library", "py_binary", "py_test",
              "bzl_library", "cc_static_library", "sh_test", "config_setting", "package_group",
              "cc_import", "java_library", "load"):
        ns[k] = ignored
    ns["select"] = lambda d, **kw: d.get("//conditions:default", [])
    ns["Label"] = lambda s: s
    ns["fail"] = lambda *a, **k: (_ for _ in ()).throw(RuntimeError("fail: %s" % (a,)))
    ns["native"] = types.SimpleNamespace(
        genrule=ns["genrule"], filegroup=ns["filegroup"], cc_library=ns["cc_library"],
        cc_binary=ns["cc_binary"], cc_test=ns["cc_test"], exports_files=ignored,
        package_name=lambda: CUR["pkg"], repository_name=lambda: "@" + CUR["repo"],
        alias=ns["alias"], package=ignored, glob=None)
    return ns


def run_code(path, repo, pkg, is_bzl=False):
    tree = ast.parse(path.read_text())
    env = template_ns()
    pkgdir = ROOT / REPO_DIRS[repo] / pkg
    env["glob"] = make_glob(pkgdir)
    env["native"].glob = env["glob"]
    body = []
    for node in tree.body:
        if (isinstance(node, ast.Expr) and isinstance(node.value, ast.Call)
                and getattr(node.value.func, "id", "") == "load"):
            args = node.value.args
            label = args[0].value
            ns = load_bzl(label, repo, pkg)
            for a in args[1:]:
                if isinstance(a, ast.Constant) and ns is not None and a.value in ns:
                    env[a.value] = ns[a.value]
            continue
        body.append(node)
    saved = dict(CUR)
    CUR.update(repo=repo, pkg=pkg)
    code = compile(ast.Module(body=body, type_ignores=[]), str(path), "exec")
    for _ in range(100):
        try:
            exec(code, env)
            break
        except NameError as e:
            if not getattr(e, "name", None) or e.name in env:
                raise
            env[e.name] = ignored
    CUR.clear(); CUR.update(saved)
    return env


def load_bzl(label, repo, pkg):
    lbl = parse_label(label, repo, pkg)
    if lbl.repo.startswith("@") or lbl.repo not in REPO_DIRS:
        return None            # rules_cc, skylib, ... : stubs already in the namespace
    path = ROOT / REPO_DIRS[lbl.repo] / lbl.pkg / lbl.name
    if not path.exists():
        return None
    key = str(path)
    if key not in BZL_CACHE:
        BZL_CACHE[key] = None
        BZL_CACHE[key] = run_code(path, lbl.repo, lbl.pkg, is_bzl=True)
    return BZL_CACHE[key]


def ensure_pkg(repo, pkg):
    if (repo, pkg) in LOADED_PKGS or repo not in REPO_DIRS:
        return
    LOADED_PKGS.add((repo, pkg))
    d = ROOT / REPO_DIRS[repo] / pkg
    for name in ("BUILD", "BUILD.bazel"):
        if (d / name).is_file():
            run_code(d / name, repo, pkg)
            return


def get_target(lbl):
    ensure_pkg(lbl.repo, lbl.pkg)
    return TARGETS.get(lbl)


# ---------------------------------------------------------------- graph

def src_path(lbl):
    """Repo-relative path of a source file label."""
    return f"{REPO_DIRS[lbl.repo]}/{lbl.pkg}/{lbl.name}".replace("//", "/").replace("/./", "/")


def gen_path(lbl, out=None):
    return f"$(MPACT_GEN)/{lbl.repo}/{lbl.pkg}/{out or lbl.name}"


class Graph:
    def __init__(self):
        self.libs = {}       # Label -> info
        self.genrules = {}   # Label -> info
        self.file_to_gen = {}  # path -> genrule label (outputs)
        self.order = []

    def tid(self, lbl):
        return re.sub(r"[^A-Za-z0-9_]", "_", f"{lbl.repo}_{lbl.pkg}_{lbl.name}")

    # resolve a src/hdr/tool string to ('src'|'gen'|'ext'|'target', value)
    def resolve_file(self, s, cur):
        lbl = parse_label(s, cur.repo, cur.pkg)
        if lbl.repo.startswith("@"):
            return ("ext", lbl)
        t = get_target(lbl)
        if t is None:
            gen = OUT_INDEX.get((lbl.repo, lbl.pkg, lbl.name))
            if gen is not None:
                return ("genout", (gen, lbl.name))
            p = ROOT / REPO_DIRS[lbl.repo] / lbl.pkg / lbl.name
            if p.is_file():
                return ("src", src_path(lbl))
            return ("missing", lbl)
        return ("target", lbl)

    def outputs_of(self, lbl):
        """Files produced by a target (as Make paths), or [] for libs."""
        t = get_target(lbl)
        k = t["kind"]
        if k == "genrule":
            return [gen_path(lbl.__class__(lbl.repo, lbl.pkg, lbl.name), o) for o in t["kw"].get("outs", [])] \
                if False else [f"$(MPACT_GEN)/{lbl.repo}/{lbl.pkg}/{o}" for o in t["kw"].get("outs", [])]
        if k == "filegroup":
            out = []
            for s in t["kw"].get("srcs", []):
                kind, v = self.resolve_file(s, lbl)
                if kind == "src":
                    out.append(v)
                elif kind == "target":
                    out += self.outputs_of(v)
            return out
        if k == "alias":
            return self.outputs_of(parse_label(t["kw"]["actual"], lbl.repo, lbl.pkg))
        if k == "cc_binary":
            return [f"$(MPACT_OUT)/bin/{self.tid(lbl)}"]
        return []

    def add_genrule(self, lbl):
        if lbl in self.genrules:
            return
        t = get_target(lbl)
        kw = t["kw"]
        info = {"label": lbl, "kw": kw, "srcs": [], "tools": [], "deps_gen": set()}
        self.genrules[lbl] = info
        for s in kw.get("srcs", []):
            kind, v = self.resolve_file(s, lbl)
            if kind == "src":
                info["srcs"].append(v)
            elif kind == "genout":
                gl, out = v
                self.add_genrule(gl)
                info["srcs"].append(f"$(MPACT_GEN)/{gl.repo}/{gl.pkg}/{out}")
                info["deps_gen"].add(f"$(MPACT_GEN)/{gl.repo}/{gl.pkg}/{out}")
            elif kind == "target":
                info["srcs"] += self.outputs_of(v)
                self.visit_target(v, info["deps_gen"])
        for s in kw.get("tools", []):
            kind, v = self.resolve_file(s, lbl)
            if kind == "target":
                info["tools"] += self.outputs_of(v)
                self.visit_target(v, info["deps_gen"])
            elif kind == "ext":
                info["tools"].append(("ext", v))
        self.order.append(("gen", lbl))

    def visit_target(self, lbl, gens):
        """Register a referenced target; collect generated-file prerequisites into `gens`."""
        t = get_target(lbl)
        if t is None:
            return
        k = t["kind"]
        if k == "genrule":
            self.add_genrule(lbl)
            gens.update(self.outputs_of(lbl))
        elif k in ("cc_library", "cc_binary"):
            self.add_lib(lbl)
            gens.update(self.libs[lbl]["gens"])
        elif k == "filegroup":
            for s in t["kw"].get("srcs", []):
                kind, v = self.resolve_file(s, lbl)
                if kind == "target":
                    self.visit_target(v, gens)
        elif k == "alias":
            self.visit_target(parse_label(t["kw"]["actual"], lbl.repo, lbl.pkg), gens)
        elif k == "cc_proto_library":
            self.add_proto(lbl)
            gens.update(self.libs[lbl]["gens"])
        elif k == "proto_library":
            pass

    def add_proto(self, lbl):
        if lbl in self.libs:
            return
        t = get_target(lbl)
        deps = t["kw"].get("deps", [])
        protos = []
        for d in deps:
            pl = parse_label(d, lbl.repo, lbl.pkg)
            pt = get_target(pl)
            if pt and pt["kind"] == "proto_library":
                for s in pt["kw"].get("srcs", []):
                    protos.append((pl.repo, pl.pkg, s))
        info = {"label": lbl, "kind": "proto", "protos": protos, "deps": [], "ext": [Label("@protobuf", "", "protobuf")],
                "gens": set(), "srcs": [], "hdrs": [], "copts": [], "defines": [], "includes": [],
                "alwayslink": False, "linkopts": []}
        self.libs[lbl] = info
        for repo, pkg, s in protos:
            base = s[:-6]
            info["gens"].add(f"$(MPACT_GEN)/{repo}/{pkg}/{base}.pb.h")
            info["gens"].add(f"$(MPACT_GEN)/{repo}/{pkg}/{base}.pb.cc")
            info["srcs"].append(("gen", f"$(MPACT_GEN)/{repo}/{pkg}/{base}.pb.cc"))
        self.order.append(("lib", lbl))

    def add_lib(self, lbl):
        if lbl in self.libs:
            return
        t = get_target(lbl)
        kw = t["kw"]
        info = {"label": lbl, "kind": t["kind"], "srcs": [], "hdrs": [], "deps": [], "ext": [],
                "gens": set(), "copts": [], "defines": kw.get("defines", []),
                "includes": kw.get("includes", []), "alwayslink": kw.get("alwayslink", False),
                "linkopts": kw.get("linkopts", []), "kw": kw}
        self.libs[lbl] = info
        for field in ("srcs", "hdrs", "textual_hdrs"):
            for s in kw.get(field, []):
                kind, v = self.resolve_file(s, lbl)
                if kind == "src":
                    if field == "srcs":
                        info["srcs"].append(("src", v))
                elif kind == "genout":
                    gl, out = v
                    self.add_genrule(gl)
                    path = f"$(MPACT_GEN)/{gl.repo}/{gl.pkg}/{out}"
                    info["gens"].add(path)
                    if field == "srcs":
                        info["srcs"].append(("gen", path))
                elif kind == "target":
                    outs = self.outputs_of(v)
                    self.visit_target(v, info["gens"])
                    if field == "srcs":
                        info["srcs"] += [("gen", o) for o in outs]
                elif kind == "ext":
                    pass
        for c in kw.get("copts", []):
            if "external/" in c:
                continue
            info["copts"].append(c)
        for d in kw.get("deps", []) + kw.get("implementation_deps", []):
            kind, v = self.resolve_file(d, lbl)
            if kind == "ext":
                info["ext"].append(v)
            elif kind == "target":
                tt = get_target(v)
                if tt["kind"] == "alias":
                    v = parse_label(tt["kw"]["actual"], v.repo, v.pkg)
                    tt = get_target(v)
                if tt["kind"] in ("cc_library", "cc_proto_library"):
                    self.visit_target(v, set())
                    info["deps"].append(v)
                    info["gens"] |= self.libs[v]["gens"]
                elif tt["kind"] == "filegroup":
                    pass
        if t["kind"] == "cc_library" or t["kind"] == "cc_binary":
            self.order.append(("lib", lbl))


def closure(g, lbl, seen=None):
    seen = seen if seen is not None else []
    if lbl in seen:
        return seen
    for d in g.libs[lbl]["deps"]:
        closure(g, d, seen)
    seen.append(lbl)
    return seen


# ---------------------------------------------------------------- emit

def write_script(g, info):
    lbl = info["label"]
    kw = info["kw"]
    outs = [f"build/mpact/gen/{lbl.repo}/{lbl.pkg}/{o}" for o in kw.get("outs", [])]
    outdir = f"build/mpact/gen/{lbl.repo}/{lbl.pkg}"
    cmd = kw.get("cmd", "")

    def fix(p):
        return p.replace("$(MPACT_GEN)", "build/mpact/gen").replace("$(MPACT_OUT)", "build/mpact")

    srcs = [fix(s) for s in info["srcs"]]

    def loc(m):
        s = m.group(1)
        kind, v = g.resolve_file(s, lbl)
        if kind == "src":
            return v
        if kind == "genout":
            gl, out = v
            return f"build/mpact/gen/{gl.repo}/{gl.pkg}/{out}"
        if kind == "ext":
            return '"$ANTLR_JAR"' if "antlr" in v.repo else f"EXTERNAL_{v.repo}"
        if kind == "target":
            o = [fix(x) for x in g.outputs_of(v)]
            return " ".join(o)
        return s

    cmd = re.sub(r"\$\(locations?\s+([^)]+)\)", loc, cmd)
    # Run in a private scratch directory and copy the declared outputs out afterwards:
    # several genrules share one grammar and write the same file names (Bazel's
    # sandbox keeps them apart; here they would race with `make -j`).
    tmp = f"build/mpact/tmp/{g.tid(lbl)}"
    tmp_outs = [f"{tmp}/{o}" for o in kw.get("outs", [])]
    cmd = cmd.replace("$(SRCS)", " ".join(srcs)).replace("$(OUTS)", " ".join(tmp_outs))
    cmd = cmd.replace("$(@D)", tmp).replace("$@", tmp_outs[0] if tmp_outs else "")
    cmd = cmd.replace("$$", "$")
    copies = "\n".join(f"cp {t} {o}" for t, o in zip(tmp_outs, outs))
    script = ("#!/bin/bash\n# Generated by utils/bazel2make.py from genrule %s\nset -euo pipefail\n"
              "rm -rf %s && mkdir -p %s %s\n%s\n%s\n") % (lbl, tmp, tmp, outdir, cmd, copies)
    d = ROOT / "mk/mpact_cmds"
    d.mkdir(exist_ok=True)
    p = d / (g.tid(lbl) + ".sh")
    p.write_text(script)
    return f"mk/mpact_cmds/{g.tid(lbl)}.sh", outs


def emit(g, roots):
    L = ["# Generated by utils/bazel2make.py -- do not edit.", "# Roots: " + ", ".join(map(str, roots)), ""]
    # genrules
    for lbl, info in g.genrules.items():
        script, outs = write_script(g, info)
        outs_mk = [o.replace("build/mpact/gen", "$(MPACT_GEN)") for o in outs]
        prereq = list(info["srcs"]) + [t for t in info["tools"] if isinstance(t, str)]
        prereq += sorted(info["deps_gen"])
        L.append(f"{' '.join(outs_mk)} &: {script} {' '.join(dict.fromkeys(prereq))}")
        L.append(f"\t@mkdir -p $(dir $(firstword {' '.join(outs_mk)}))")
        L.append(f"\tANTLR_JAR=$(ANTLR_JAR) bash {script}")
        L.append("")
    # libs
    for lbl, info in g.libs.items():
        tid = g.tid(lbl)
        objs = []
        cl = closure(g, lbl)
        defs, incs = [], []
        for c in cl:
            ci = g.libs[c]
            defs += ci["defines"]
            for inc in ci["includes"]:
                rd = REPO_DIRS.get(c.repo)
                if rd:
                    base = f"{c.pkg}/{inc}".strip("/").replace("/./", "/").rstrip("/.")
                    incs += [f"-Ivendor/{rd.split('/')[-1]}/{base}", f"-I$(MPACT_GEN)/{c.repo}/{base}"]
        flags = " ".join(dict.fromkeys(info["copts"] + ["-D" + d for d in dict.fromkeys(defs)] + incs))
        order_only = " ".join(sorted(info["gens"]))
        for kind, path in info["srcs"]:
            if not path.endswith((".cc", ".cpp", ".c", ".S")):
                continue
            flat = re.sub(r"[^A-Za-z0-9_.]", "_", path.replace("$(MPACT_GEN)/", "gen/"))
            obj = f"$(MPACT_OUT)/obj/{tid}/{flat}.o"
            objs.append(obj)
            comp = "$(MPACT_CC) $(MPACT_CFLAGS)" if path.endswith((".c", ".S")) else "$(MPACT_CXX) $(MPACT_CXXFLAGS)"
            L.append(f"{obj}: {path}" + (f" | {order_only}" if order_only else ""))
            L.append(f"\t@mkdir -p $(dir $@)")
            L.append(f"\t{comp} {flags} -MMD -MP -c $< -o $@")
        if info["kind"] == "cc_binary":
            libs = []
            for c in reversed(cl):
                if c == lbl:
                    continue
                libs.append(c)
            archives = []
            for c in libs:
                ci = g.libs[c]
                if any(p.endswith((".cc", ".cpp", ".c", ".S")) for _, p in ci["srcs"]):
                    a = f"$(MPACT_OUT)/lib/{g.tid(c)}.a"
                    archives.append(("whole:" if ci["alwayslink"] else "") + a)
            ext = sorted({e.repo for c in cl for e in g.libs[c]["ext"]})
            L.append(f"$(MPACT_OUT)/bin/{tid}: {' '.join(objs)} {' '.join(a.replace('whole:', '') for a in archives)}")
            L.append("\t@mkdir -p $(dir $@)")
            lnk = " ".join((f"-Wl,--whole-archive {a[6:]} -Wl,--no-whole-archive" if a.startswith("whole:") else a) for a in archives)
            lopts = " ".join(dict.fromkeys(o for c in cl for o in g.libs[c]["linkopts"]))
            L.append(f"\t$(MPACT_CXX) {' '.join(objs)} -Wl,--start-group {lnk} -Wl,--end-group $(MPACT_LDLIBS) {lopts} -o $@")
        else:
            if objs:
                L.append(f"$(MPACT_OUT)/lib/{tid}.a: {' '.join(objs)}")
                L.append("\t@mkdir -p $(dir $@)")
                L.append(f"\trm -f $@ && $(AR) rcs $@ $^")
        L.append("")
        # protobuf generation
        if info["kind"] == "proto":
            for repo, pkg, s in info["protos"]:
                base = s[:-6]
                src = f"{REPO_DIRS[repo]}/{pkg}/{s}"
                outs = f"$(MPACT_GEN)/{repo}/{pkg}/{base}.pb.h $(MPACT_GEN)/{repo}/{pkg}/{base}.pb.cc"
                L.append(f"{outs} &: {src}")
                L.append(f"\t@mkdir -p $(MPACT_GEN)/{repo}")
                L.append(f"\tprotoc -I{REPO_DIRS[repo]} --cpp_out=$(MPACT_GEN)/{repo} {pkg}/{s}")
                L.append("")
    # Per-library link recipe pieces, so other targets (python module, ...) can link
    # a library with all of its dependencies:
    #   MPACT_ARCHIVES_<id> : the .a files (prerequisites)
    #   MPACT_LINK_<id>     : the link arguments (whole-archive for alwayslink libs)
    L.append("# link pieces per library")
    for lbl, info in g.libs.items():
        if info["kind"] == "cc_binary":
            continue
        cl = closure(g, lbl)
        archives = []
        for c in reversed(cl):
            ci = g.libs[c]
            if any(p.endswith((".cc", ".cpp", ".c", ".S")) for _, p in ci["srcs"]):
                archives.append((f"$(MPACT_OUT)/lib/{g.tid(c)}.a", ci["alwayslink"]))
        tid = g.tid(lbl)
        L.append(f"MPACT_ARCHIVES_{tid} := {' '.join(a for a, _ in archives)}")
        lnk = " ".join(f"-Wl,--whole-archive {a} -Wl,--no-whole-archive" if w else a for a, w in archives)
        L.append(f"MPACT_LINK_{tid} := -Wl,--start-group {lnk} -Wl,--end-group")
    return "\n".join(L) + "\n"


def main():
    roots = [parse_label(a, "coralnpu-mpact", "") for a in sys.argv[1:]]
    g = Graph()
    for r in roots:
        gens = set()
        # `@repo//pkg:name` form in argv already handled by parse_label
        g.visit_target(r, gens)
    text = emit(g, roots)
    (ROOT / "mk/mpact_rules.mk").write_text(text)
    sys.stderr.write(f"{len(g.libs)} cc targets, {len(g.genrules)} genrules\n")
    ext = sorted({e.repo for i in g.libs.values() for e in i["ext"]})
    sys.stderr.write("external repos used: " + ", ".join(ext) + "\n")


if __name__ == "__main__":
    main()
