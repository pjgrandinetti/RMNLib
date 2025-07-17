#!/bin/bash
# Test script for Read the Docs documentation build for RMNLib
# This mimics the RTD build environment locally

set -e  # Exit on any error

echo "=== Testing RMNLib Read the Docs documentation build ==="

# Clean previous builds
echo "Cleaning previous build artifacts..."
make clean

# Set RTD environment variable
export READTHEDOCS=True

# Install documentation dependencies
echo "Installing documentation dependencies..."
pip install -r docs/requirements.txt

# Generate Doxygen XML
echo "Generating Doxygen XML documentation..."
make doxygen

# Build documentation
echo "Building Sphinx documentation..."
make html

echo "=== RMNLib documentation build completed successfully! ==="
echo "Open docs/_build/html/index.html in your browser to view the result."
