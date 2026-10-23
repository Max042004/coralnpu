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
"""Run the int8 face-embedding CNN (run_face_embedding.cc) on the MPACT simulator.

  utils/run_py_test.sh tests/npusim_examples/npusim_face_embedding.py \
      --inputs images_int8.npy --out embeddings_int8.npy

--inputs is an (N, 112, 112, 3) int8 array. Each image is run through the
network on the simulated CoralNPU; the 192-element int8 embeddings are saved
to --out and the pairwise cosine similarities are printed.
"""

import argparse
import asyncio

import numpy as np

from coralnpu_test_utils import runfiles
from coralnpu_test_utils.sim_backends.mpact_npusim_test_fixture import (
    MpactNpuSimTestFixture,
)

ELF = "coralnpu_hw/tests/npusim_examples/run_face_embedding.elf"


async def embed(elf_path, image):
    fixture = await MpactNpuSimTestFixture.Create(
        highmem=True, exit_on_ebreak=True
    )
    await fixture.load_elf_and_lookup_symbols(
        elf_path, ["inference_status", "inference_input", "inference_output"]
    )
    await fixture.write("inference_input", image.reshape(-1).astype(np.int8))
    await fixture.run_to_halt()
    status = (await fixture.read("inference_status", size=1, dtype=np.int8))[0]
    if status != 0:
        traps = {}
        for csr in ("mepc", "mcause", "mtval"):
            try:
                traps[csr] = fixture.sim.read_register(csr)
            except Exception:  # CSR not exposed by this build
                pass
        raise RuntimeError(
            f"inference_status = {status}; trap CSRs: {traps}"
        )
    out = await fixture.read(
        "inference_output", size=192, dtype=np.int8, shape=(192, )
    )
    return np.array(out, dtype=np.int8), fixture.get_cycle_count()


def cosine(a, b):
    a, b = a.astype(np.float32), b.astype(np.float32)
    return float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b)))


async def main_async(args):
    elf_path = runfiles.Create().Rlocation(ELF)
    images = np.load(args.inputs)
    embeddings = []
    for i, img in enumerate(images):
        emb, cycles = await embed(elf_path, img)
        print(f"image {i}: {cycles} simulated cycles")
        embeddings.append(emb)
    embeddings = np.array(embeddings)
    np.save(args.out, embeddings)
    for i in range(len(embeddings)):
        for j in range(i + 1, len(embeddings)):
            print(f"cos(emb{i}, emb{j}) = {cosine(embeddings[i], embeddings[j]):.4f}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs", required=True)
    parser.add_argument("--out", required=True)
    asyncio.run(main_async(parser.parse_args()))


if __name__ == "__main__":
    main()
