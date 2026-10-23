"""Add a serving_default SignatureDef to a signature-less .tflite so ai-edge-quantizer can calibrate it."""
import flatbuffers
from ai_edge_litert import schema_py_generated as schema

SRC, DST = "mobilefacenet.tflite", "mobilefacenet_sig.tflite"
buf = bytearray(open(SRC, "rb").read())
model = schema.ModelT.InitFromObj(schema.Model.GetRootAsModel(buf, 0))
sg = model.subgraphs[0]
# Old models only set deprecatedBuiltinCode; builtinCode would default to 0 (ADD) after repacking.
for oc in model.operatorCodes:
    oc.builtinCode = max(oc.builtinCode, oc.deprecatedBuiltinCode)
assert len(sg.inputs) == 1 and len(sg.outputs) == 1, (sg.inputs, sg.outputs)


def tmap(idx):
    m = schema.TensorMapT()
    m.name = sg.tensors[idx].name.decode() if isinstance(sg.tensors[idx].name, bytes) else sg.tensors[idx].name
    m.tensorIndex = int(idx)
    return m


sd = schema.SignatureDefT()
sd.signatureKey = "serving_default"
sd.subgraphIndex = 0
sd.inputs = [tmap(sg.inputs[0])]
sd.outputs = [tmap(sg.outputs[0])]
model.signatureDefs = [sd]

b = flatbuffers.Builder(1024)
b.Finish(model.Pack(b), b"TFL3")
open(DST, "wb").write(bytes(b.Output()))
print("wrote", DST, "input", sd.inputs[0].name, "output", sd.outputs[0].name)
