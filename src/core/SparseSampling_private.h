/**
 * @file SparseSampling_private.h
 * @brief Private header for SparseSampling implementation
 *
 * This header contains internal declarations and direct getter functions
 * that should only be used within SparseSampling implementation files.
 * External code should use the copy functions from the public API.
 */
#ifndef SPARSESAMPLING_PRIVATE_H
#define SPARSESAMPLING_PRIVATE_H

// Include the public header for type declarations
#include "SparseSampling.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Private Direct Access Functions
 * These functions return direct references to internal data and should only
 * be used within SparseSampling implementation for performance-critical operations.
 * @{
 */

/**
 * @brief Get direct reference to dimension indexes (internal use only).
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCIndexSetRef of indexes (do not release).
 */
OCIndexSetRef
SparseSamplingGetDimensionIndexes(SparseSamplingRef ss);

/**
 * @brief Get direct reference to sparse grid vertices (internal use only).
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCIndexPairSetRef containing vertex data (do not release).
 */
OCIndexPairSetRef
SparseSamplingGetSparseGridVertexes(SparseSamplingRef ss);

/**
 * @brief Get direct reference to encoding string (internal use only).
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCStringRef encoding value (do not release).
 */
OCStringRef
SparseSamplingGetEncoding(SparseSamplingRef ss);

/**
 * @brief Get direct reference to description (internal use only).
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCStringRef description (do not release).
 */
OCStringRef
SparseSamplingGetDescription(SparseSamplingRef ss);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  // SPARSESAMPLING_PRIVATE_H
