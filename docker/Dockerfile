# syntax=docker/dockerfile:1.6

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        python3 \
        python3-pip \
        python3-venv \
        git && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo && \
    cmake --build build && \
    cmake --install build --prefix /opt/trdp

RUN python3 -m pip install --upgrade pip build && \
    python3 -m build --wheel --outdir /tmp/dist

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        libstdc++6 \
        ca-certificates \
        python3 \
        python3-pip && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/trdp /opt/trdp
COPY --from=builder /tmp/dist /tmp/dist

RUN python3 -m pip install --no-cache-dir /tmp/dist/*.whl && \
    rm -rf /tmp/dist

ENV PATH="/opt/trdp/bin:${PATH}"

ENTRYPOINT ["trdp_sim_cli"]
CMD ["demo-scenario"]
