"""Host reference for the face-embedding demo: build test inputs, run float and int8 models with LiteRT."""
import numpy as np
from ai_edge_litert.interpreter import Interpreter

FLOAT, INT8 = "mobilefacenet.tflite", "mobilefacenet_int8.tflite"


def run(path, x):
    it = Interpreter(model_path=path)
    it.allocate_tensors()
    i, o = it.get_input_details()[0], it.get_output_details()[0]
    outs = []
    for img in x:
        it.set_tensor(i["index"], img[None])
        it.invoke()
        outs.append(it.get_tensor(o["index"])[0].copy())
    return np.array(outs), i, o


def cos(a, b):
    return float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b)))


rng = np.random.default_rng(1)
# Three synthetic "faces": A, A with small perturbation (same identity analogue), and B.
base = lambda: np.clip(rng.normal(0, 0.4, (7, 7, 3)).repeat(16, 0).repeat(16, 1), -1, 1)
A = base()
Ap = np.clip(A + rng.normal(0, 0.03, A.shape), -1, 1)
B = base()
xf = np.stack([A, Ap, B]).astype(np.float32)

_, qi, qo = run(INT8, xf[:0]) if False else (None, None, None)
it = Interpreter(model_path=INT8)
it.allocate_tensors()
qi = it.get_input_details()[0]
qo = it.get_output_details()[0]
s_in, z_in = qi["quantization"]
s_out, z_out = qo["quantization"]
xq = np.clip(np.round(xf / s_in) + z_in, -128, 127).astype(np.int8)

ef, _, _ = run(FLOAT, xf)
eq_raw, _, _ = run(INT8, xq)
eq = (eq_raw.astype(np.float32) - z_out) * s_out

np.save("inputs_int8.npy", xq)
np.save("ref_int8_raw.npy", eq_raw)
print("input quant", s_in, z_in, "output quant", s_out, z_out)
for n, e in (("float", ef), ("int8 ", eq)):
    print(f"{n}: cos(A,A')={cos(e[0], e[1]):.4f}  cos(A,B)={cos(e[0], e[2]):.4f}")
print("float vs int8 per-image cosine:", [round(cos(ef[k], eq[k]), 4) for k in range(3)])
