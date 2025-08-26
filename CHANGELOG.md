# Changelog

All notable changes to this project will be documented in this file.

## [v0.1.14] - 2025-08-26
### Added
- Enhanced Dataset roundtrip tests with rigorous validation
- Added test_Dataset_rigorous_roundtrip() function that creates Dataset with actual DependentVariable data
- Improved C test suite to match Python test complexity for better debugging
- Added detailed dictionary structure debugging output in tests

### Fixed
- Improved Dataset roundtrip testing to help isolate Python wrapper vs C library issues
- Enhanced test coverage for Dataset dictionary serialization/deserialization

## [v0.1.11] - 2025-08-23
### Fixed
- Fixed NULL label/description handling in DimensionSetLabel and DimensionSetDescription functions
- Prevents segfaults when RMNpy dimensions are created without explicit label/description parameters
- Functions now create empty strings instead of storing NULL values when inputs are NULL
- Resolves NULL label errors in LinearDimension and other RMNpy dimension classes
- Added proper error checking for empty string creation

## [v0.1.6] - 2025-08-18
### Fixed
- Windows build: Copy DLLs to bin directory for tests
- GitHub Actions test failures with missing RPATH configuration
- Duplicate rpath warnings during linking

## [v0.1.0] - 2025-06-01
### Initial release
- Added core functionality for managing scalar quantities with coordinates.
- Implemented `RMNDatum` type with support for dimensionality, units, and coordinates.
- Integrated with `OCTypes` and `SITypes` libraries for object-oriented and SI unit handling.
- Documentation setup with Sphinx and Doxygen.
- Test suite covering scalar creation, copying, and coordinate management.
- Build system support for Make and CMake (Xcode project generation).
- Continuous integration workflows for Linux, macOS, and Windows.
- ReadTheDocs configuration for online documentation hosting.
