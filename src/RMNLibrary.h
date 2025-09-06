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
/* LAPACKE C interface to LAPACK - only if available */
#if __has_include(<lapacke.h>)
#include <lapacke.h>
#define HAVE_LAPACKE 1
#else
#define HAVE_LAPACKE 0
#endif
#endif
/*
 -------------------------------------------------------------
 OpenMP — conditionally include for parallel processing
 -------------------------------------------------------------
 OpenMP provides parallel processing capabilities for loops and other
 constructs. Support varies by compiler and platform:
   GCC      : -fopenmp flag required (includes MinGW on Windows)
   Clang    : -fopenmp flag + OpenMP runtime library
   MSVC     : /openmp flag
   MinGW    : -fopenmp flag (typically uses GCC toolchain)
   Others   : may not be supported

 If OpenMP is not available, the pragma directives are safely ignored
 and the code runs sequentially.
 -------------------------------------------------------------
*/
#ifdef _OPENMP
#include <omp.h>
#define HAVE_OPENMP 1
#else
#define HAVE_OPENMP 0
#endif
// Include the core OCTypes definitions and utilities
#include <OCTypes.h>
// Include the core SITypes definitions and utilities
#include <SITypes.h>
/** @cond INTERNAL */

typedef struct impl_RMNBase {
      void *(*copyAsDictionary)(const void *);
} RMNBase;

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
// Import/Export headers
#include "importers/Image.h"
#include "importers/JCAMP.h"
#include "importers/Tecmag.h"
// Spectroscopy headers
#include "spectroscopy/NMRSpectroscopy.h"


/**
 * @defgroup UniversalAccessors Universal Property Accessors
 * @brief Universal getter and setter functions for RMNLib object properties.
 *
 * These functions provide a unified interface for accessing common properties
 * across different RMNLib types, automatically dispatching to the appropriate
 * type-specific implementation based on the object's type ID.
 * @{
 */

/**
 * @brief Universal getter for description property across RMNLib types.
 *
 * @param theType The RMNLib object to get description from
 * @param outError Optional error output parameter
 * @return The description string, or NULL if not supported or on error
 *
 * @note Supported types: Dimension, DependentVariable, SparseSampling, Dataset
 * @note Not supported: Datum, GeographicCoordinate
 */
OCStringRef RMNLibGetDescription(OCTypeRef theType, OCStringRef *outError);

/**
 * @brief Universal setter for description property across RMNLib types.
 *
 * @param theType The RMNLib object to set description on
 * @param description The description string to set
 * @param outError Optional error output parameter
 * @return true on success, false on error
 *
 * @note Supported types: Dimension, DependentVariable, SparseSampling, Dataset
 * @note Not supported: Datum, GeographicCoordinate
 */
bool RMNLibSetDescription(OCTypeRef theType, OCStringRef description, OCStringRef *outError);

/**
 * @brief Universal getter for application metadata across RMNLib types.
 *
 * @param theType The RMNLib object to get metadata from
 * @param outError Optional error output parameter
 * @return The application metadata dictionary, or NULL if not supported or on error
 *
 * @note Supported types: All RMNLib types except Datum
 */
OCDictionaryRef RMNLibGetApplicationMetaData(OCTypeRef theType, OCStringRef *outError);

/**
 * @brief Universal setter for application metadata across RMNLib types.
 *
 * @param theType The RMNLib object to set metadata on
 * @param metadata The metadata dictionary to set (can be NULL)
 * @param outError Optional error output parameter
 * @return true on success, false on error
 *
 * @note Supported types: All RMNLib types except Datum
 */
bool RMNLibSetApplicationMetaData(OCTypeRef theType, OCDictionaryRef metadata, OCStringRef *outError);

/** @} */  // end of UniversalAccessors group

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
/** @} */  // end of LibraryManagement group
#endif     /* RMNLIBRARY_H */
