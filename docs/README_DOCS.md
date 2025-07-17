# RMNLib Documentation

This directory contains the documentation for RMNLib, built with Sphinx, Breathe, and Doxygen, and hosted on Read the Docs.

## ⚠️ DEVELOPMENT STATUS WARNING

**This documentation is for a project in ALPHA development and NOT ready for use.**

- API functions may not work as documented
- Memory management is not guaranteed to be safe
- Functions and structures will change without notice
- **DO NOT** use this library in any production code

## Building Documentation Locally

### Prerequisites

1. Install system dependencies:
   ```bash
   # On Ubuntu/Debian:
   sudo apt-get install doxygen graphviz
   
   # On macOS:
   brew install doxygen graphviz
   ```

2. Install Python dependencies:
   ```bash
   pip install -r requirements.txt
   ```

### Building

```bash
# Generate Doxygen XML
make doxygen

# Build Sphinx documentation
make html
```

The built documentation will be in `_build/html/`. Open `_build/html/index.html` in your browser.

### Other Build Targets

- `make clean` - Clean all build artifacts
- `make doxygen` - Generate only Doxygen XML
- `make html` - Build only Sphinx HTML (requires Doxygen XML)

## Read the Docs Integration

This project is configured for automatic building on Read the Docs:

- **Configuration**: `.readthedocs.yaml` in the project root
- **Requirements**: `requirements.txt` in this directory
- **Build Process**: Custom commands that run Doxygen then Sphinx

### RTD Build Process

1. RTD detects changes to the repository
2. Installs system dependencies (doxygen, graphviz)
3. Installs Python dependencies from `requirements.txt`
4. Runs `make doxygen` to generate XML from C source code
5. Runs `make html` to build Sphinx documentation with Breathe integration
6. Copies result to RTD hosting

### Testing RTD Build Locally

Use the provided test script:

```bash
./test_rtd_build.sh
```

This script mimics the RTD environment by:
- Setting `READTHEDOCS=True` environment variable
- Installing dependencies exactly as RTD does
- Running the same build commands as RTD

## Documentation Architecture

### Technology Stack
- **Doxygen**: Extracts API documentation from C source code comments
- **Breathe**: Sphinx extension that imports Doxygen XML into Sphinx
- **Sphinx**: Builds the final HTML documentation with cross-references

### File Structure

```
docs/
├── index.rst              # Main entry point
├── api/                   # API documentation files
│   ├── Dataset.rst        # Dataset API reference
│   ├── Dimension.rst      # Dimension types
│   ├── DependentVariable.rst
│   └── ...                # Other API modules
├── doxygen/              # Generated Doxygen output
│   └── xml/              # XML files used by Breathe
├── _build/               # Generated Sphinx output
├── conf.py               # Sphinx configuration
├── requirements.txt      # Python dependencies
└── Doxyfile              # Doxygen configuration
```

## Writing Documentation

### C Source Code Documentation

Use Doxygen-style comments in C source files:

```c
/**
 * @brief Create a new dataset with the specified properties.
 * 
 * @param title The dataset title
 * @param description Optional description (can be NULL)
 * @return Pointer to the created dataset, or NULL on error
 * 
 * @note This function allocates memory that must be freed with DatasetDestroy()
 * @warning This API is unstable and will change
 */
Dataset* DatasetCreate(const char* title, const char* description);
```

### RST Documentation

- Use RestructuredText format for all `.rst` files
- Link to C functions using `:c:func:` directive
- Include code examples in `.. code-block:: c` blocks

### Adding New API Documentation

1. Ensure C source has proper Doxygen comments
2. Create or update corresponding `.rst` file in `docs/api/`
3. Add to the toctree in `index.rst`
4. Test with `make html`

## Troubleshooting

### Common Issues

1. **Doxygen XML Missing**: Run `make doxygen` before `make html`
2. **Breathe Import Errors**: Check that Doxygen XML exists in `doxygen/xml/`
3. **Missing Functions**: Ensure C source has proper `/** */` comments
4. **RTD Build Failures**: Check the RTD build logs for specific errors

### Build Dependencies

- **Doxygen >= 1.8**: For C documentation extraction
- **Graphviz**: For generating diagrams  
- **Python 3.8+**: For Sphinx and Breathe
- **Make**: For build automation

### Environment Variables

- `READTHEDOCS=True` - Indicates building on RTD
- `READTHEDOCS_OUTPUT` - RTD output directory

## Links

- [Live Documentation](https://RMN.readthedocs.io/) (⚠️ Development version)
- [Doxygen Documentation](https://doxygen.nl/manual/)
- [Breathe Documentation](https://breathe.readthedocs.io/)
- [Sphinx Documentation](https://www.sphinx-doc.org/)
