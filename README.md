# RMNLib

> # This repository is shared for development purposes only. Check back later for stable releases.

![CI Status](https://github.com/pjgrandinetti/RMNLib/actions/workflows/ci.yml/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/RMN/badge/?version=latest)](https://RMN.readthedocs.io/en/latest/?badge=latest)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/pjgrandinetti/RMNLib)](https://github.com/pjgrandinetti/RMNLib/releases/latest)
[![License](https://img.shields.io/github/license/pjgrandinetti/RMNLib)](https://github.com/pjgrandinetti/RMNLib/blob/main/LICENSE)
[![GitHub issues](https://img.shields.io/github/issues/pjgrandinetti/RMNLib)](https://github.com/pjgrandinetti/RMNLib/issues)
[![GitHub last commit](https://img.shields.io/github/last-commit/pjgrandinetti/RMNLib)](https://github.com/pjgrandinetti/RMNLib/commits/main)
[![Platform Support](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-blue)](https://github.com/pjgrandinetti/RMNLib#requirements) DEVELOPMENT STATUS: ALPHA - NOT READY FOR USE 🚧**
> 
> **⚠️ WARNING: This project is in early development and is NOT suitable for production use.**
> 
> - API is unstable and subject to major changes
> - Many features are incomplete or untested
> - Documentation may be outdated or incorrect
> - Breaking changes will occur without notice
> - **DO NOT USE** in any production environment
> 
> This repository is shared for development purposes only. Check back later for stable releases.

![CI Status](https://github.com/pjgrandinetti/RMNLib/actions/workflows/ci.yml/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/RMN/badge/?version=latest)](https://RMN.readthedocs.io/en/latest/?badge=latest)

---

A library for multi-dimensional signal processing.

## Requirements

### Core Dependencies

- **C Compiler**: clang or gcc
- **Build Tools**: Make, flex, bison
- **Documentation**: Doxygen, Python 3 with `sphinx` and `breathe`

### Platform-Specific Dependencies

#### macOS
```bash
# Install LLVM and OpenMP for parallel processing
brew install llvm libomp

# Install BLAS/LAPACK (uses Accelerate framework by default)
# Install curl for dependency fetching
brew install curl

# For documentation
pip install -r docs/requirements.txt
```

#### Ubuntu/Linux
```bash
# Core build dependencies
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  make \
  flex \
  bison \
  curl

# OpenMP and math libraries
sudo apt-get install -y \
  libomp-dev \
  libopenblas-dev \
  liblapacke-dev

# For dependency fetching
sudo apt-get install -y libcurl4-openssl-dev

# For documentation
pip install -r docs/requirements.txt
```

#### Windows (MinGW/MSYS2)
```bash
# Install via MSYS2
pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-curl \
          mingw-w64-x86_64-openblas \
          mingw-w64-x86_64-lapack \
          mingw-w64-x86_64-openmp \
          make flex bison
```

## Building the Library

### Quick Start
```bash
make clean
make all
```

### macOS with OpenMP Support
For optimal performance on macOS, use LLVM clang with OpenMP:

```bash
export CC="/opt/homebrew/opt/llvm/bin/clang"
export LDFLAGS="-L/opt/homebrew/opt/libomp/lib -L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/libomp/include -I/opt/homebrew/opt/llvm/include"

make clean
make all
```

### Build Targets

- `make all` - Build library and fetch dependencies
- `make libRMN.a` - Static library only  
- `make install-shared` - Build shared library (.dylib/.so/.dll)
- `make fetchlibs` - Download OCTypes and SITypes dependencies
- `make clean` - Remove build artifacts

**Output**: Produces `libRMN.a` (static) and optionally shared libraries in `install/lib/`.

### Dependencies

RMNLib automatically downloads its dependencies (OCTypes and SITypes) from GitHub releases during the build process. The `make fetchlibs` target:

1. Downloads `libOCTypes-{platform}.zip` and `libOCTypes-headers.zip`
2. Downloads `libSITypes-{platform}.zip` and `libSITypes-headers.zip`  
3. Extracts libraries to `third_party/lib/`
4. Extracts headers to `third_party/include/`

**Note**: Requires internet connection for initial build. Dependencies are cached in `third_party/` for subsequent builds.

## Documentation

To build the API docs (after the library is built):

```bash
make docs  # from project root
```

The HTML output appears in `docs/_build/html`.

### Online Documentation

The latest documentation is hosted on Read the Docs: https://RMN.readthedocs.io/en/latest/

A Read the Docs configuration file (`.readthedocs.yml`) is included at the project root to automate the documentation build.

## Running the Test Suite

After building the library, run:

```bash
make test        # build and run all tests
make test-debug  # run under LLDB
make test-asan   # with AddressSanitizer
```

## Troubleshooting

### OpenMP Issues

**Problem**: Build shows "OpenMP not found - using sequential processing"

**Solution**: 
- **macOS**: Install LLVM and set compiler paths as shown above
- **Linux**: Install `libomp-dev` package
- **Windows**: Install `mingw-w64-x86_64-openmp` package

### Dependency Download Failures

**Problem**: "Cannot find zipfile directory" or download errors

**Solutions**:
- Check internet connection
- Verify OCTypes/SITypes releases are available on GitHub
- Try `make fetchlibs` separately before `make all`

### Missing Math Libraries

**Problem**: Linker errors for BLAS/LAPACK functions

**Solutions**:
- **macOS**: Uses Accelerate framework (built-in)
- **Linux**: Install `libopenblas-dev liblapacke-dev`
- **Windows**: Install `mingw-w64-x86_64-openblas mingw-w64-x86_64-lapack`

### Flex/Bison Issues

**Problem**: "flex: command not found" or "yacc: command not found"

**Solutions**:
- **macOS**: `brew install flex bison`
- **Linux**: `sudo apt-get install flex bison`
- **Windows**: Install via MSYS2 as shown above
# SITypes artifacts now use consistent naming with OCTypes
