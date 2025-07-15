# RMNLib

![CI Status](https://github.com/pjgrandinetti/RMNLib/actions/workflows/ci.yml/badge.svg)
[![Documentation Status](https://readthedocs.org/projects/RMNLib/badge/?version=latest)](https://RMN.readthedocs.io/en/latest/?badge=latest)

> **⚠️ Development Status**: RMNLib is currently under active development and is not ready for production use. APIs may change without notice and functionality may be incomplete.

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

This produces `libRMNLib.a`.

## Documentation

To build the API docs (after the library is built):

```bash
make docs  # from project root
```

The HTML output appears in `docs/_build/html`.

### Online Documentation

The latest documentation is hosted on Read the Docs: https://RMNLib.readthedocs.io/en/latest/

A Read the Docs configuration file (`.readthedocs.yml`) is included at the project root to automate the documentation build.

## Running the Test Suite

After building the library, run:

```bash
make test        # build and run all tests
make test-debug  # run under LLDB
make test-asan   # with AddressSanitizer
```
