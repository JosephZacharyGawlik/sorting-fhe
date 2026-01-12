# OpenFHE Installation & Environment Setup Log
This document records the configuration of the OpenFHE development environment on a Google Cloud n2-standard-8 instance.

## 1. System Specifications
**Machine Type:** n2-standard-8 (8 vCPUs, 32 GB RAM).

**Processor Architecture:** x86_64 with AVX-512 vector instruction sets.

**Operating System:** Ubuntu 22.04 LTS.

## 2. Prerequisites & Dependencies
To support advanced math backends and version control, the following system packages were installed:

```bash

sudo apt update
sudo apt install -y git build-essential cmake autoconf libgmp-dev libntl-dev
```

**GMP/NTL**: High-performance libraries for multi-precision arithmetic, essential for FHE polynomial math.

**Autoconf**: Required for building external dependencies within the OpenFHE ecosystem.

## 3. OpenFHE Source Configuration
We cloned the official development repository and configured the build specifically for the N2 hardware.

### Clone
```bash

git clone https://github.com/openfheorg/openfhe-development.git
cd openfhe-development
mkdir build && cd build
```
### Build Configuration (CMake)
We used specific flags to optimize performance and include external math libraries:

```bash

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DWITH_NATIVEOPT=ON \
  -DWITH_NTL=ON \
  -DWITH_TCM=OFF
```

**WITH_NATIVEOPT=ON:** Critical flag that triggers -march=native. This allows the compiler to use the AVX-512 instructions available on the N2 instance.

**WITH_NTL=ON:** Enables the Number Theory Library backend for faster Lattice-based operations.

**WITH_TCM=OFF:** Explicitly disabled during setup to bypass a linking error regarding libtcmalloc_minimal.so, opting for the standard system allocator for stability.

## 4. Compilation and Installation
To maximize the 8-core CPU, we used parallel compilation:

**1. Compile:** make -j8 (assigned 1 thread per vCPU).

**2. System Install:** sudo make install (moved headers to /usr/local/include and libs to /usr/local/lib).

**3. Linker Refresh:** sudo ldconfig (updated the runtime linker cache to recognize OpenFHE shared objects).

## 5. Verification
The installation was verified by checking the presence of the core headers:

```bash

ls /usr/local/include/openfhe/core/config_core.h
```

### Key Troubleshooting Notes
**Git Missing:** The minimal cloud image required manual installation of git.

**Permissions:** make install requires sudo because it writes to protected system directories (/usr/local).

**Shared Library Errors:** If the system cannot find libOPENFHEcore.so, ensure sudo ldconfig has been run to refresh the library paths.