/**

@file RMNLibrary.h

@brief Core definitions and includes for the RMN measurement library
This header centralizes project-wide includes and dependencies for the
RMN library, wrapping both OCTypes and SITypes core headers as well
as local modules (Datum, Dimension, Dataset).
*/
#ifndef RMNLIBRARY_H
#define RMNLIBRARY_H
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* 
 -------------------------------------------------------------
 BLAS & LAPACK — pick the right include for each platform
 -------------------------------------------------------------
 macOS      : use Apple’s Accelerate.framework, which bundles CBLAS + LAPACK
 Linux      : assume a system-installed CBLAS + LAPACKE (e.g. OpenBLAS, Netlib)
 Windows    : assume user has installed OpenBLAS or Intel MKL with CBLAS + LAPACKE
 -------------------------------------------------------------
 Linker flags you will typically need:
   macOS    : -framework Accelerate
   Linux    : -lcblas -llapacke  (or -lopenblas -llapacke)
   Windows  : link against OpenBLAS (e.g. openblas.lib) or MKL import libs
 -------------------------------------------------------------
*/
#if defined(__APPLE__)
  /* All BLAS & LAPACK lives in Accelerate.framework on macOS */
  #include <Accelerate/Accelerate.h>
#else
  /* CBLAS interface */
  #include <cblas.h>
  /* LAPACKE C interface to LAPACK */
  #include <lapacke.h>
#endif

// Include the core OCTypes definitions and utilities
#include <OCLibrary.h>
// Include the core SITypes definitions and utilities
#include <SILibrary.h>
/** @cond INTERNAL */
// Centralized Ref typedefs
typedef struct impl_GeographicCoordinate *GeographicCoordinateRef;
typedef struct impl_Datum *DatumRef;
typedef struct impl_SparseSampling *SparseSamplingRef;
typedef struct impl_DependentVariable *DependentVariableRef;
typedef struct impl_Dimension *DimensionRef;
typedef struct impl_LabeledDimension *LabeledDimensionRef;
typedef struct impl_SIDimension *SIDimensionRef;
typedef struct impl_SIMonotonicDimension *SIMonotonicDimensionRef;
typedef struct impl_SILinearDimension *SILinearDimensionRef;
typedef struct impl_Dataset *DatasetRef;
/** @endcond */
#define DependentVariableComponentsFileName STR("dependent_variable-%ld.data")

// Core module headers
#include "core/Dataset.h"
#include "core/Datum.h"
#include "core/DependentVariable.h"
#include "core/Dimension.h"
#include "core/GeographicCoordinate.h"
#include "core/SparseSampling.h"

// Utility headers
#include "utils/RMNGridUtils.h"

// Import/Export headers
#include "importers/JCAMP.h"
#include "importers/Tecmag.h"
#include "importers/Image.h"

// Spectroscopy headers
#include "spectroscopy/NMRSpectroscopy.h"

/**
 * @defgroup MetadataJSON JSON Metadata Functions
 * @brief Functions for converting metadata dictionaries to/from JSON format.
 * 
 * These functions provide serialization and deserialization capabilities for 
 * metadata stored in OCDictionary objects, enabling data persistence and 
 * exchange with external systems.
 * @{
 */

/**
 * @brief Converts an OCDictionary containing metadata to a JSON object.
 *
 * This function serializes metadata stored in an OCDictionary to a cJSON object
 * following the RMNLib metadata schema. The resulting JSON can be used for data
 * persistence, network transmission, or integration with other systems that
 * consume JSON-formatted metadata.
 *
 * The function handles various OCTypes objects within the dictionary, including:
 * - OCString objects (converted to JSON strings)
 * - OCNumber objects (converted to appropriate JSON numeric types)
 * - OCArray objects (converted to JSON arrays)
 * - Nested OCDictionary objects (converted to JSON objects recursively)
 * - Other OCTypes objects (serialized using their copyJSON virtual method)
 *
 * @param dict The OCDictionary containing metadata to serialize. If NULL, 
 *             returns a JSON null object.
 *
 * @return A newly allocated cJSON object representing the metadata dictionary.
 *         The caller is responsible for freeing this object with cJSON_Delete().
 *         Returns a JSON null object if the input dictionary is NULL or if
 *         serialization fails.
 *
 * @note The returned cJSON object must be freed by the caller using cJSON_Delete().
 * @note This function does not modify the input dictionary.
 * @note Complex objects may be converted to string representations if they
 *       don't have specialized JSON serialization support.
 *
 * @see OCMetadataCreateFromJSON() for the inverse operation
 * @see cJSON_Delete() for proper cleanup of the returned object
 *
 * @ingroup MetadataJSON
 */
cJSON *OCMetadataCopyJSON(OCDictionaryRef dict);

/**
 * @brief Creates an OCDictionary from a JSON object containing metadata.
 *
 * This function deserializes a cJSON object into an OCDictionary, reconstructing
 * the metadata according to the RMNLib metadata schema. It performs the inverse
 * operation of OCMetadataCopyJSON(), enabling round-trip serialization of
 * metadata objects.
 *
 * The function automatically detects and converts JSON types to appropriate
 * OCTypes objects:
 * - JSON strings → OCString objects
 * - JSON numbers → OCNumber objects (integer or floating-point as appropriate)
 * - JSON arrays → OCArray objects containing converted elements
 * - JSON objects → OCDictionary objects (recursively processed)
 * - JSON booleans → OCBoolean objects
 * - JSON null → NULL entries (skipped in the dictionary)
 *
 * @param json The cJSON object to deserialize. If NULL, returns an empty dictionary.
 * @param outError Optional pointer to an OCStringRef that will receive an error
 *                 description if deserialization fails. Pass NULL if error details
 *                 are not needed. If an error occurs, this will contain a human-readable
 *                 description of what went wrong. The caller is responsible for
 *                 releasing this string with OCRelease().
 *
 * @return A newly created OCDictionary containing the deserialized metadata,
 *         or NULL if deserialization fails. The returned dictionary has a
 *         retain count of 1 and must be released with OCRelease() when no
 *         longer needed.
 *
 * @note The returned OCDictionary must be released by the caller using OCRelease().
 * @note This function does not modify the input JSON object.
 * @note If outError is provided and an error occurs, the caller must release
 *       the error string with OCRelease().
 * @note Unrecognized JSON structures may be skipped or converted to string
 *       representations as a fallback.
 *
 * @see OCMetadataCopyJSON() for the inverse operation
 * @see OCRelease() for proper cleanup of the returned dictionary and error string
 *
 * @ingroup MetadataJSON
 */
OCDictionaryRef OCMetadataCreateFromJSON(cJSON *json, OCStringRef *outError);

/** @} */ // end of MetadataJSON group

/**
 * @defgroup LibraryManagement Library Management Functions
 * @brief Functions for managing the lifecycle of the RMNLib library.
 * 
 * These functions handle initialization and cleanup of library-wide resources,
 * ensuring proper memory management and resource cleanup.
 * @{
 */

/**
 * @brief Shuts down the RMNLib library and releases all internal resources.
 *
 * This function performs comprehensive cleanup of all RMNLib internal state,
 * including type registrations, cached objects, and any global resources
 * allocated during library operation. It should be called near the end of
 * the program to ensure proper cleanup and accurate leak detection.
 *
 * The function performs the following cleanup operations:
 * - Releases all registered RMNLib type definitions
 * - Clears internal caches and static instances
 * - Frees memory pools and temporary allocations
 * - Calls OCTypesShutdown() and SITypesShutdown() for underlying libraries
 * - Resets all global state to initial conditions
 *
 * After calling this function:
 * - No RMNLib functions should be called except for re-initialization
 * - All RMNLib objects should have been released by the caller beforehand
 * - Memory leak detection tools should report accurate results
 * - The library can be safely re-initialized if needed
 *
 * @warning This function should only be called when no RMNLib objects are
 *          still in use. Accessing RMNLib objects after calling this function
 *          results in undefined behavior.
 * @warning This function is not thread-safe. Ensure all RMNLib operations
 *          have completed before calling this function.
 *
 * @note This function is optional but recommended for applications that want
 *       clean shutdown and accurate leak detection.
 * @note Calling this function multiple times is safe (subsequent calls are no-ops).
 * @note This function does not free objects created by the application - those
 *       must be released by the caller using OCRelease().
 *
 * @see OCTypesShutdown() for cleanup of the underlying OCTypes library
 * @see SITypesShutdown() for cleanup of the underlying SITypes library
 *
 * @ingroup LibraryManagement
 */
void RMNLibTypesShutdown(void);

/** @} */ // end of LibraryManagement group

#endif /* RMNLIBRARY_H */
