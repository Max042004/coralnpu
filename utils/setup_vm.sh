#!/bin/bash
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

# Provision an Ubuntu 22.04 (x86_64) machine -- e.g. an OrbStack VM -- with the
# tools the Bender/Make flow needs. Everything except apt packages goes into
# $TOOLS (default ~/tools); versions are pinned below.
#
#   orb -m <vm> bash utils/setup_vm.sh [step...]
#
# Steps: apt bender sbt firtool verilator   (default: all)
set -euo pipefail

TOOLS="${TOOLS:-$HOME/tools}"
BENDER_VERSION="${BENDER_VERSION:-0.28.1}"
SBT_VERSION="${SBT_VERSION:-1.10.7}"
FIRTOOL_VERSION="${FIRTOOL_VERSION:-1.114.0}"   # llvm-firtool jar on Maven
VERILATOR_VERSION="${VERILATOR_VERSION:-5.052}"

mkdir -p "$TOOLS" "$HOME/bin"

step_apt() {
  # Finish any half-configured packages left by an interrupted earlier install.
  sudo DEBIAN_FRONTEND=noninteractive dpkg --configure -a || true
  sudo apt-get update
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -f -y
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential git curl unzip ca-certificates \
    openjdk-17-jdk-headless \
    flex bison autoconf automake libtool help2man perl \
    libfl2 libfl-dev zlib1g-dev ccache \
    cmake ninja-build clang \
    python3-venv python3-pip
}

step_bender() {
  local tmp; tmp="$(mktemp -d)"
  curl -fsSL -o "$tmp/bender.tgz" \
    "https://github.com/pulp-platform/bender/releases/download/v${BENDER_VERSION}/bender-${BENDER_VERSION}-x86_64-linux-gnu-ubuntu22.04.tar.gz"
  tar xzf "$tmp/bender.tgz" -C "$tmp"
  install -m755 "$tmp/bender" "$HOME/bin/bender"
  "$HOME/bin/bender" --version
}

step_sbt() {
  local tmp; tmp="$(mktemp -d)"
  curl -fsSL -o "$tmp/sbt.tgz" \
    "https://github.com/sbt/sbt/releases/download/v${SBT_VERSION}/sbt-${SBT_VERSION}.tgz"
  rm -rf "$TOOLS/sbt"
  tar xzf "$tmp/sbt.tgz" -C "$TOOLS"
  ln -sf "$TOOLS/sbt/bin/sbt" "$HOME/bin/sbt"
}

step_firtool() {
  local tmp; tmp="$(mktemp -d)"
  curl -fsSL -o "$tmp/firtool.jar" \
    "https://repo1.maven.org/maven2/org/chipsalliance/llvm-firtool/${FIRTOOL_VERSION}/llvm-firtool-${FIRTOOL_VERSION}.jar"
  rm -rf "$TOOLS/firtool"; mkdir -p "$TOOLS/firtool"
  unzip -q -o "$tmp/firtool.jar" 'org.chipsalliance/llvm-firtool/linux-x64/bin/firtool' -d "$TOOLS/firtool"
  ln -sf "$TOOLS/firtool/org.chipsalliance/llvm-firtool/linux-x64/bin/firtool" "$HOME/bin/firtool"
  chmod +x "$HOME/bin/firtool"
  "$HOME/bin/firtool" --version | head -2
}

step_verilator() {
  local tmp; tmp="$(mktemp -d)"
  curl -fsSL -o "$tmp/v.tgz" \
    "https://github.com/verilator/verilator/archive/refs/tags/v${VERILATOR_VERSION}.tar.gz"
  tar xzf "$tmp/v.tgz" -C "$tmp"
  ( cd "$tmp/verilator-${VERILATOR_VERSION}" \
    && autoconf && ./configure --prefix="$TOOLS/verilator-${VERILATOR_VERSION}" \
    && make -j"$(nproc)" && make install )
  ln -sfn "$TOOLS/verilator-${VERILATOR_VERSION}" "$TOOLS/verilator"
  ln -sf "$TOOLS/verilator/bin/verilator" "$HOME/bin/verilator"
  ln -sf "$TOOLS/verilator/bin/verilator_bin" "$HOME/bin/verilator_bin"
  "$HOME/bin/verilator" --version
}

STEPS=("$@"); [ ${#STEPS[@]} -gt 0 ] || STEPS=(apt bender sbt firtool verilator)
for s in "${STEPS[@]}"; do echo "=== $s ==="; "step_$s"; done
echo 'Add to your shell profile:  export PATH="$HOME/bin:$PATH"'
