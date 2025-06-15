// RMNGridUtils.h
#ifndef RMNGRIDUTILS_H
#define RMNGRIDUTILS_H

#include "RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute the total number of samples implied by an array of DimensionRef.
 *
 * For each dimension in `dimensions`, multiplies together:
 *  - SILinearDimension → count
 *  - SIMonotonicDimension → number of coordinates
 *  - LabeledDimension → number of labels
 *  - all other Dimension subclasses → 1
 *
 * @param dimensions  An OCArrayRef of DimensionRef
 * @return            The product of all per-dimension sample counts.
 */
OCIndex
RMNCalculateSizeFromDimensions(OCArrayRef dimensions);

/**
 * @brief Like RMNCalculateSizeFromDimensions, but skip any dimensions
 *        whose index is present in `ignored`.
 *
 * @param dimensions  An OCArrayRef of DimensionRef
 * @param ignored     An OCIndexSetRef of dimension indices to skip
 * @return            The product of sample counts over the non-ignored dimensions.
 */
OCIndex
RMNCalculateSizeFromDimensionsIgnoring(OCArrayRef    dimensions,
                                       OCIndexSetRef ignored);

/**
 * @brief Compute the stride (flat‐index increment) along a given dimension.
 *
 * stride = product(npts[0..dimensionIndex-1]), or 1 if dimensionIndex == 0.
 *
 * @param npts              Array of length `dimensionsCount` with per-dimension sizes.
 * @param dimensionsCount   Number of dimensions (length of `npts`).
 * @param dimensionIndex    Which dimension’s stride to compute.
 * @return                  The linear stride for that dimension.
 */
OCIndex
strideAlongDimensionIndex(const OCIndex *npts,
                          const OCIndex  dimensionsCount,
                          const OCIndex  dimensionIndex);

/**
 * @brief Convert a multi‐dimensional index vector into a single memory offset.
 *
 * Wraps each index into [0..npts[idim]) before computing:
 *   offset = Σ_{k=0..D-1} ( index[k] * ∏_{j<k} npts[j] )
 *
 * @param indexes           In/out array of length `dimensionsCount`; on entry, may be out‐of-range;
 *                          on exit, each index is wrapped into [0..npts[idim]).
 * @param dimensionsCount   Number of dimensions.
 * @param npts              Per-dimension counts (length `dimensionsCount`).
 * @return                  The single linear offset.
 */
OCIndex
memOffsetFromIndexes(OCIndex       *indexes,
                     const OCIndex  dimensionsCount,
                     const OCIndex  *npts);

/**
 * @brief Convert a single memory offset into a full index-vector.
 *
 * index[k] = (offset / ∏_{j<k} npts[j]) % npts[k]
 *
 * @param memOffset         The linear offset.
 * @param indexes           Output array of length `dimensionsCount`.
 * @param dimensionsCount   Number of dimensions.
 * @param npts              Per-dimension counts.
 */
void
setIndexesForMemOffset(const OCIndex  memOffset,
                       OCIndex        indexes[],
                       const OCIndex  dimensionsCount,
                       const OCIndex  *npts);

/**
 * @brief Like setIndexesForMemOffset, but skip one dimension.
 *
 * Fills `indexes[idim]` for all idim != ignoredDimension.
 *
 * @param memOffset           The linear offset.
 * @param indexes             Output array of length `dimensionsCount`.
 * @param dimensionsCount     Number of dimensions.
 * @param npts                Per-dimension counts.
 * @param ignoredDimension    The one dimension index to skip.
 */
void
setIndexesForReducedMemOffsetIgnoringDimension(const OCIndex  memOffset,
                                               OCIndex        indexes[],
                                               const OCIndex  dimensionsCount,
                                               const OCIndex  *npts,
                                               const OCIndex  ignoredDimension);

/**
 * @brief Like setIndexesForMemOffset, but skip any in a set.
 *
 * Fills `indexes[idim]` for all idim not in `dimensionIndexSet`.
 *
 * @param memOffset           The linear offset.
 * @param indexes             Output array of length `dimensionsCount`.
 * @param dimensionsCount     Number of dimensions.
 * @param npts                Per-dimension counts.
 * @param dimensionIndexSet   Indices of dimensions to skip.
 */
void
setIndexesForReducedMemOffsetIgnoringDimensions(const OCIndex       memOffset,
                                                OCIndex             indexes[],
                                                const OCIndex       dimensionsCount,
                                                const OCIndex       *npts,
                                                OCIndexSetRef       dimensionIndexSet);

#ifdef __cplusplus
}
#endif

#endif /* RMNGRIDUTILS_H */
