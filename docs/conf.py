# docs/conf.py

# -- Path setup --------------------------------------------------------------

import os
import sys

# If your extensions (or modules to document with autodoc) are in
# the project root, add that directory to sys.path here:
#    sys.path.insert(0, os.path.abspath('..'))

# -- Project information -----------------------------------------------------

project = 'RMNLib'
author = 'Philip J. Grandinetti'
# The full version, including alpha/beta/rc tags
release = '0.1.0'
# The short X.Y version
version = release

# -- General configuration ---------------------------------------------------

# Sphinx extensions
extensions = [
    'breathe',
]

# Paths that contain templates, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files to ignore.
exclude_patterns = []

# Suppress duplicate C declaration warnings from Breathe
suppress_warnings = ['duplicate_declaration', 'duplicate_declaration.c']

# -- Breathe configuration --------------------------------------------------

# Tell Breathe where the Doxygen XML lives (relative to this conf.py).
# This should match OUTPUT_DIRECTORY = doxygen and XML_OUTPUT = xml in Doxyfile,
# resulting in docs/doxygen/xml.
breathe_projects = {
    "RMNLib": os.path.abspath(
        os.path.join(os.path.dirname(__file__), "doxygen", "xml")
    ),
}
breathe_default_project = 'RMNLib'

# Ensure that .c/.h files use the C domain
breathe_domain_by_extension = {
    "c": "c",
    "h": "c",
}
primary_domain = 'c'

# -- Options for HTML output -------------------------------------------------

html_theme = 'sphinx_rtd_theme'
html_static_path = []  # clear if you have no _static directory
