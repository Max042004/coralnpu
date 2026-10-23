"""Post-training int8 quantization of the float MobileFaceNet (calibration with synthetic images)."""
import numpy as np
from ai_edge_litert.interpreter import Interpreter
from ai_edge_quantizer import quantizer, recipe

SRC = "mobilefacenet_sig.tflite"
DST = "mobilefacenet_int8.tflite"

it = Interpreter(model_path=SRC)
inp = it.get_input_details()[0]
name = it.get_signature_list()["serving_default"]["inputs"][0]
shape = tuple(inp["shape"])
print("input", name, shape)

rng = np.random.default_rng(0)
# Smooth random images in [-1, 1] (face-like statistics are not available here).
samples = []
for _ in range(64):
    img = rng.normal(0, 0.5, size=shape).astype(np.float32)
    samples.append({name: np.clip(img, -1, 1)})

qt = quantizer.Quantizer(SRC)
qt.load_quantization_recipe(recipe.static_wi8_ai8())
sigs = list(it.get_signature_list().keys()) or ["serving_default"]
print("signatures", sigs)
cal = qt.calibrate({sigs[0]: samples})
res = qt.quantize(cal)
res.export_model(DST, overwrite=True)

it2 = Interpreter(model_path=DST)
it2.allocate_tensors()
print("int8 inputs ", [(d["shape"].tolist(), d["dtype"].__name__, d["quantization"]) for d in it2.get_input_details()])
print("int8 outputs", [(d["shape"].tolist(), d["dtype"].__name__, d["quantization"]) for d in it2.get_output_details()])
print("ops", sorted({o["op_name"] for o in it2._get_ops_details()}))
