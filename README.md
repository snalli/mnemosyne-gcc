# Mnemosyne

Lightweight Persistent Memory (PM) library. Mnemosyne lets programmers declare
persistent global variables with the `persistent` keyword and allocate persistent
heap objects dynamically. Consistent updates are provided through a lightweight
software transactional memory (STM) mechanism built on GCC's `-fgnu-tm` extension.

## Authors

- Haris Volos — hvolos@cs.wisc.edu
- Andres Jaan Tack — tack@cs.wisc.edu
- Sanketh Nalli — nalli@wisc.edu

## Architecture

```
libmnemosyne.so
├── mcore   — persistent memory core (segment management, logging, PCM emulation)
├── mtm     — GCC TM ABI implementation (transactions, write-set, commit/abort)
└── pmalloc — persistent heap allocator (built on ALPS layer-based allocator)
```

All three components are compiled as static libraries and combined into a single
`libmnemosyne.so` shared library.

## Build

### Docker (recommended)

```bash
docker build -t mnemosyne .
docker run -it mnemosyne
```

The image is based on Ubuntu 24.04 and builds natively on both x86-64 and ARM64.

### Native (Linux)

**Dependencies**

```bash
# Ubuntu / Debian
sudo apt-get install -y \
    build-essential cmake gcc g++ \
    libconfig-dev libelf-dev elfutils \
    libboost-all-dev libnuma-dev \
    libyaml-cpp-dev libattr1-dev
```

**Build**

```bash
cd src
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DTARGET_ARCH_MEM=CC-NUMA
make -j$(nproc)
```

Produces:
- `src/build/libmnemosyne.so` — combined library
- `src/build/examples/simple/simple` — persistent-flag example

**Run tests**

```bash
ctest --output-on-failure   # from src/build/
```

## Usage

### Persistent variables

```c
#include <mnemosyne.h>
#include <mtm.h>

MNEMOSYNE_PERSISTENT int my_counter = 0;

__transaction_relaxed {
    my_counter++;
}
```

### Persistent heap allocation

```c
#include <pmalloc.h>

long *p;
PTx { p = pmalloc(sizeof(long) * 1024); }
```

### Run the simple example

```bash
./build/examples/simple/simple
# persistent flag: 0 --> 1

./build/examples/simple/simple
# persistent flag: 1 --> 0
```

## Development

**Code style**

```bash
clang-format -i $(find src -name "*.c" -o -name "*.h")
```

**Static analysis**

```bash
cmake --build src/build --target analysis
```

**Requirements:** GCC 13+ with `-fgnu-tm`, CMake 3.12+

## Project layout

```
mnemosyne-gcc/
├── src/                   Library source
│   ├── library/mcore/     Persistent memory core
│   ├── library/mtm/       Transaction manager (GCC TM ABI)
│   ├── library/pmalloc/   Persistent allocator + ALPS
│   └── examples/simple/   Hello-world example
├── tests/unit/            Portability unit tests (CTest)
├── Dockerfile
└── .clang-format
```

## License

GPL-2.0 — see `src/COPYING`.
