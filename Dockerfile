FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    libcurl4-openssl-dev \
    libssl-dev \
    pkg-config \
    zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src

RUN cmake -S . -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
  && cmake --build build -j"$(nproc)"

FROM debian:bookworm-slim AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libcurl4 \
    libssl3 \
    zlib1g \
  && rm -rf /var/lib/apt/lists/* \
  && groupadd --system bot \
  && useradd --system --gid bot --create-home --home-dir /app bot

WORKDIR /app

COPY --from=builder /src/build/sarashina_discord_bot /app/sarashina_discord_bot

RUN mkdir -p /app/data && chown -R bot:bot /app

USER bot

ENTRYPOINT ["/app/sarashina_discord_bot"]
