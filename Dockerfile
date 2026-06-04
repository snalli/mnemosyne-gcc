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
    && ctest --output-on-failure -E "_valgrind" \
    && ctest --output-on-failure -R "_valgrind$"

CMD ["/bin/bash"]
