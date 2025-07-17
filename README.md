# RMNLib

> **🚧 DEVELOPMENT STATUS: ALPHA - NOT READY FOR USE 🚧**
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

Ensure you have installed:

- A C compiler (e.g., clang or gcc)
- Make
- Doxygen
- Python 3 with `sphinx` and `breathe` (`pip install -r docs/requirements.txt`)

## Building the Library

Compile the static library:

```bash
make
```

This produces `libRMN.a`.

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
