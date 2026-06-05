FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gcc \
    g++ \
    git \
    libconfig-dev \
    libelf-dev \
    elfutils \
    libevent-dev \
    libboost-all-dev \
    libnuma-dev \
    libyaml-cpp-dev \
    libattr1-dev \
    clang-format \
    cppcheck \
    valgrind \
    libgtest-dev \
    libgmock-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /mnemosyne

COPY src/ src/
COPY tests/ tests/

# Optional: run clang-format dry-run as a non-fatal style check
RUN find /mnemosyne/src -name "*.c" -o -name "*.cc" -o -name "*.h" -o -name "*.hh" \
    | xargs clang-format --dry-run -Werror || true

WORKDIR /mnemosyne/src/build

RUN cmake .. \
      -DCMAKE_BUILD_TYPE=Debug \
      -DTARGET_ARCH_MEM=CC-NUMA \
    && make -j$(nproc) 2>&1 | tee /mnemosyne/build.log \
    && mkdir -p /dev/shm/psegments \
    && (cd /mnemosyne/src && MNEMOSYNE_PHEAP_SIZE_MB=32 LD_LIBRARY_PATH=build:$LD_LIBRARY_PATH build/examples/prime/prime || true) \
    && ctest --output-on-failure -E "_valgrind" \
    && ctest --output-on-failure -R "_valgrind$"

# NOTE: the persistent segments live under /dev/shm. Docker's default tmpfs is
# 64 MiB; if the test step above hits a Bus error, build with a larger shm:
#   docker build --shm-size=2g ...
# (the integration tests cap the heap region at 32 MiB to stay within limits).

CMD ["/bin/bash"]
