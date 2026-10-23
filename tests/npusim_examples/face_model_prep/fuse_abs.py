"""Turn DEQUANTIZE -> ABS(float) -> QUANTIZE into an int8 ABS (TFLM and TFLite both support int8 ABS).

ai-edge-quantizer leaves ABS in float; MobileFaceNet's PReLU = relu(x) + a*(x-abs(x))/2 uses it 33 times, and the
float intermediates (4x larger) blow up the tensor arena.
"""
import flatbuffers
import numpy as np
from ai_edge_litert import schema_py_generated as S
from ai_edge_litert.interpreter import Interpreter

SRC, DST = "mobilefacenet_int8.tflite", "mobilefacenet_int8_abs.tflite"
buf = bytearray(open(SRC, "rb").read())
m = S.ModelT.InitFromObj(S.Model.GetRootAsModel(buf, 0))
for oc in m.operatorCodes:
    oc.builtinCode = max(oc.builtinCode, oc.deprecatedBuiltinCode)
# The quantizer stores big constants outside the flatbuffer (offset/size); inline them before repacking.
for bf in m.buffers:
    if bf.size and bf.offset:
        bf.data = np.frombuffer(bytes(buf[bf.offset:bf.offset + bf.size]), dtype=np.uint8)
        bf.offset = 0
        bf.size = 0
sg = m.subgraphs[0]
B = S.BuiltinOperator


def code(op):
    return m.operatorCodes[op.opcodeIndex].builtinCode


producer, consumers = {}, {}
for k, op in enumerate(sg.operators):
    for t in op.outputs:
        producer[t] = k
    for t in op.inputs:
        if t >= 0:
            consumers.setdefault(t, []).append(k)

remove = set()
fused = 0
for k, op in enumerate(sg.operators):
    if code(op) != B.ABS:
        continue
    t_in, t_out = op.inputs[0], op.outputs[0]
    d = producer.get(t_in)
    cons = consumers.get(t_out, [])
    if d is None or code(sg.operators[d]) != B.DEQUANTIZE or len(cons) != 1:
        continue
    q = cons[0]
    if code(sg.operators[q]) != B.QUANTIZE:
        continue
    op.inputs = [sg.operators[d].inputs[0]]
    op.outputs = [sg.operators[q].outputs[0]]
    if consumers.get(t_in, []) == [k]:
        remove.add(d)
    remove.add(q)
    fused += 1
sg.operators = [op for k, op in enumerate(sg.operators) if k not in remove]
print("fused", fused, "removed ops", len(remove), "remaining ops", len(sg.operators))

# Drop tensors no op uses any more (TFLM would keep them allocated for the whole run).
used = set(int(t) for t in list(sg.inputs) + list(sg.outputs))
for op in sg.operators:
    used.update(int(t) for t in list(op.inputs) + list(op.outputs) if t >= 0)
remap = {}
kept = []
for i, t in enumerate(sg.tensors):
    if i in used:
        remap[i] = len(kept)
        kept.append(t)
print("tensors", len(sg.tensors), "->", len(kept))
sg.tensors = kept
for op in sg.operators:
    op.inputs = [remap[int(t)] if t >= 0 else t for t in op.inputs]
    op.outputs = [remap[int(t)] for t in op.outputs]
sg.inputs = [remap[int(t)] for t in sg.inputs]
sg.outputs = [remap[int(t)] for t in sg.outputs]
for sd in (m.signatureDefs or []):
    for tm in list(sd.inputs or []) + list(sd.outputs or []):
        tm.tensorIndex = remap[int(tm.tensorIndex)]

b = flatbuffers.Builder(1 << 20)
b.Finish(m.Pack(b), b"TFL3")
open(DST, "wb").write(bytes(b.Output()))

# Compare with the original int8 model on the test inputs.
x = np.load("inputs_int8.npy")


def run(path):
    it = Interpreter(model_path=path)
    it.allocate_tensors()
    i, o = it.get_input_details()[0], it.get_output_details()[0]
    res = []
    for img in x:
        it.set_tensor(i["index"], img[None])
        it.invoke()
        res.append(it.get_tensor(o["index"])[0].copy())
    return np.array(res)


a, c = run(SRC), run(DST)
print("max |orig - fused| (int8 steps):", int(np.abs(a.astype(int) - c.astype(int)).max()))
np.save("ref_int8_abs_raw.npy", c)
