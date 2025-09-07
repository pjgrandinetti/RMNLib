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
 * @returns   OCDataRef containing vertex data (do not release).
 */
OCDataRef
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
/**
 * @brief Get the number of vertices in the sparse grid.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   Number of vertices, or 0 if ss is NULL or empty.
 */
size_t
SparseSamplingGetVertexCount(SparseSamplingRef ss);
/**
 * @brief Get the number of dimensions in the sparse grid.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   Number of dimensions, or 0 if ss is NULL or empty.
 */
OCIndex
SparseSamplingGetDimensionCount(SparseSamplingRef ss);
/**
 * @brief Get vertex coordinates at a specific index.
 *
 * @param ss          SparseSamplingRef object.
 * @param vertexIndex Index of the vertex to retrieve.
 * @param outCoords   Output array to store vertex coordinates (must be pre-allocated).
 * @returns           true on success, false on error or invalid index.
 */
bool SparseSamplingGetVertexAtIndex(SparseSamplingRef ss, OCIndex vertexIndex, OCIndex *outCoords);
/** @} */
#ifdef __cplusplus
}
#endif
#endif  // SPARSESAMPLING_PRIVATE_H
