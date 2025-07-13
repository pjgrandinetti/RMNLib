RMNLib Documentation
====================

RMNLib is a C library for reading, writing, and manipulating Core Scientific Dataset Model (CSDM) files.
It provides a comprehensive API for working with multidimensional scientific datasets, including sparse sampling,
geographic coordinates, and various dimension types.

Requirements
~~~~~~~~~~~~

Ensure you have installed:

- A C compiler (e.g., clang or gcc)
- Make
- Doxygen
- Python 3 with ``sphinx`` and ``breathe`` (``pip install -r docs/requirements.txt``)

Building the Library
~~~~~~~~~~~~~~~~~~~~

Compile the static library::

    make

This produces ``libRMNLib.a``.

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/Dataset
   api/Dimension
   api/DependentVariable
   api/Datum
   api/SparseSampling
   api/GeographicCoordinate
   api/RMNGridUtils
   api/RMNLibrary

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
