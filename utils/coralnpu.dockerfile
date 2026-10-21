# syntax=docker/dockerfile:1
# Dockerfile to create a stable CoralNPU build environment (Bender + Make + sbt).
#
# Build command (from the repository root):
#   docker build -t coralnpu -f utils/coralnpu.dockerfile .
#
# Run command:
#   docker run -it -v $PWD:$PWD -w $PWD coralnpu /bin/bash
#   make rtl DESIGN=CoreMiniAxi
#
# The toolchain installation is shared with utils/setup_vm.sh, so a bare Ubuntu
# 22.04 machine (e.g. an OrbStack VM) ends up with the same environment.

FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ARG _UID=1000
ARG _GID=1000
ARG _USERNAME=builder
ENV HOME=/home/${_USERNAME}

# Everything is installed system-wide so any user id can use it.
ENV TOOLS_DIR=/opt/coralnpu/tools
ENV TOOLS=/opt/coralnpu/tools
ENV BIN=/usr/local/bin

COPY utils/setup_vm.sh /opt/coralnpu/setup/utils/setup_vm.sh
COPY requirements.txt /opt/coralnpu/setup/requirements.txt

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked <<EOF
    set -eux -o pipefail
    ln -snf "/usr/share/zoneinfo/${TZ}" /etc/localtime
    echo "${TZ}" > /etc/timezone
    apt-get update
    apt-get install -y -qq sudo ca-certificates curl
    bash /opt/coralnpu/setup/utils/setup_vm.sh
    chmod -R a+rX /opt/coralnpu
    # Linters used by utils/run_linters.sh
    apt-get install -y -qq shellcheck ruby-mdl
    /opt/coralnpu/tools/venv/bin/pip install --no-cache-dir yapf==0.43.0 clang-format==19.1.7
    ln -s /opt/coralnpu/tools/venv/bin/yapf /usr/local/bin/yapf3
    ln -s /opt/coralnpu/tools/venv/bin/clang-format /usr/local/bin/clang-format-19
    ln -s /opt/coralnpu/tools/venv/bin/git-clang-format /usr/local/bin/git-clang-format-19
    # Install verible-verilog-lint / -format
    curl -fL -o verible.tar.gz https://github.com/chipsalliance/verible/releases/download/v0.0-4051-g9fdb4057/verible-v0.0-4051-g9fdb4057-linux-static-x86_64.tar.gz
    echo "f52e5920ef63f70620a6086e09dea8bd778147cd7a9ff827bb7de5d6316b1754  verible.tar.gz" | sha256sum --check -
    mkdir -p verible
    tar -xf verible.tar.gz --strip-components=1 -C verible
    install verible/bin/verible-verilog-* /usr/local/bin
    # Install scalafmt
    curl -fL -o scalafmt.zip https://github.com/scalameta/scalafmt/releases/download/v3.10.7/scalafmt-x86_64-pc-linux.zip
    echo "a238a4e73141538a2a2e4e834f80154906b8a3f654ff14b6752fc3a3feeb5be1  scalafmt.zip" | sha256sum --check -
    unzip scalafmt.zip
    install scalafmt /usr/local/bin
    rm -rf verible verible.tar.gz scalafmt.zip
    # The toolchain's gdb links libmpfr.so.4 (Ubuntu ships .6)
    ln -sf /lib/x86_64-linux-gnu/libmpfr.so.6 /lib/x86_64-linux-gnu/libmpfr.so.4
    # Set up builder user
    echo "${_USERNAME} ALL=(ALL) NOPASSWD:/usr/bin/apt-get" >> /etc/sudoers.d/${_USERNAME}
    echo "${_USERNAME} ALL=(ALL) NOPASSWD:/usr/bin/apt" >> /etc/sudoers.d/${_USERNAME}
    groupadd -f plugdev
    groupadd -f dialout
    groupadd --gid ${_GID} ${_USERNAME}
    useradd \
        --home-dir ${HOME} \
        --comment "" \
        --uid ${_UID} \
        --gid ${_GID} \
        --groups plugdev,dialout \
        ${_USERNAME}
    mkdir -p /home/${_USERNAME}
    chown ${_USERNAME}:${_USERNAME} ${HOME}
EOF
USER ${_USERNAME}
WORKDIR /home/${_USERNAME}/
