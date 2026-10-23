# Preparing the face-embedding demo model

`make elf T=tests/npusim_examples/run_face_embedding` needs
`tests/npusim_examples/models/face_embedding_int8.tflite` (git-ignored; the
source model's licence is unclear, so it is not redistributed).

Starting point: a float MobileFaceNet `mobilefacenet.tflite` (112x112x3 float32
in, 192-d float32 out), e.g. `assets/mobilefacenet.tflite` of
github.com/MCarlomagno/FaceRecognitionAuth. In a venv with
`pip install ai-edge-litert ai-edge-quantizer numpy`, run in this order from the
directory holding the model:

```bash
python addsig.py      # add a SignatureDef (needed by the quantizer)  -> mobilefacenet_sig.tflite
python quantize.py    # static int8 PTQ, synthetic calibration images   -> mobilefacenet_int8.tflite
python host_ref.py    # test inputs + LiteRT reference (inputs_int8.npy)
python fuse_abs.py    # DEQUANTIZE-ABS-QUANTIZE -> int8 ABS, drop dead tensors
cp mobilefacenet_int8_abs.tflite <repo>/tests/npusim_examples/models/face_embedding_int8.tflite
```

`fuse_abs.py` matters: the quantizer leaves the 33 `ABS` ops of the PReLU
decomposition in float32, whose activations need > 5 MB of tensor arena.

Then:

```bash
make mpact-pybind elf T=tests/npusim_examples/run_face_embedding
utils/run_py_test.sh tests/npusim_examples/npusim_face_embedding.py \
    --inputs inputs_int8.npy --out embeddings.npy
```

The calibration images are synthetic noise; for a real deployment calibrate
with representative aligned face crops. One image takes about 4.3 billion
simulated cycles because the depthwise convolutions run on the reference
kernel (the RVV one faults on this model).
