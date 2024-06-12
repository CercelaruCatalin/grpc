# Utilizăm imaginea de bază Ubuntu
FROM ubuntu:latest

RUN apt-get update && apt-get install -y cmake build-essential git

WORKDIR /deps
#taken from gRPC page
RUN git clone --recurse-submodules -b v1.62.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

RUN mkdir -p /deps/grpc/build && cd /deps/grpc/build && \
    cmake -DgRPC_install=ON \
        -DgRPC_BUILD_TESTS=OFF \
        .. && \
        make -j8 install

# Instalăm cmake, build-essential, git și alte dependințe necesare pentru proiect
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    git \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc-dev \
    libtinyxml2-dev \
    libcjson-dev \
    sqlite3 \
    && rm -rf /var/lib/apt/lists/*