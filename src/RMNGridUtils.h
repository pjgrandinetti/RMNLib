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
 * @return            The product of all per-dimension sample counts (or 1 if NULL).
 */
OCIndex RMNCalculateSizeFromDimensions(OCArrayRef dimensions);
/**
 * @brief Like RMNCalculateSizeFromDimensions, but skip any dimensions
 *        whose index appears in `ignored`.
 *
 * @param dimensions  An OCArrayRef of DimensionRef
 * @param ignored     An OCIndexSetRef of dimension‐indices to skip
 * @return            The product of sample counts over the non-ignored dimensions.
 */
OCIndex RMNCalculateSizeFromDimensionsIgnoring(OCArrayRef dimensions, OCIndexSetRef ignored);
/**
 * @brief Convert a multi‐dimensional index vector into a single memory offset.
 *
 * Wraps each index into [0..npts−1] before computing:
 *   offset = Σ_{k=0..D−1} ( index[k] * ∏_{j<k} npts[j] )
 *
 * @param dimensions   An OCArrayRef of DimensionRef
 * @param indexes      Array of length D giving each coordinate (may be out of range)
 * @return             The flat memory offset, or (OCIndex)−1 on error.
 */
OCIndex RMNGridMemOffsetFromIndexes(OCArrayRef dimensions, const OCIndex indexes[]);
/**
 * @brief Recover a single coordinate along one dimension from a flat offset.
 *
 * coord = (offset / ∏_{j<dim} npts[j]) % npts[dim]
 *
 * @param dimensions      An OCArrayRef of DimensionRef
 * @param memOffset       The flat offset
 * @param dimensionIndex  Which dimension to extract
 * @return                The wrapped coordinate, or (OCIndex)−1 on error.
 */
OCIndex
RMNGridCoordinateIndexFromMemOffset(OCArrayRef dimensions, OCIndex memOffset, OCIndex dimensionIndex);
/**
 * @brief Compute the stride (flat‐index increment) along a given dimension.
 *
 * stride = ∏_{j<dimensionIndex} npts[j], or 1 if dimensionIndex == 0.
 *
 * @param npts              Array of per-dimension sizes (length D)
 * @param dimensionsCount   Number of dimensions (D)
 * @param dimensionIndex    Which dimension’s stride to compute
 * @return                  The linear stride for that dimension.
 */
OCIndex
strideAlongDimensionIndex(const OCIndex *npts, OCIndex dimensionsCount, OCIndex dimensionIndex);
/**
 * @brief Convert a full index‐vector into a flat offset, wrapping out‐of‐range indexes.
 *
 * On exit, each indexes[i] has been reduced modulo npts[i].
 * offset = Σ_{k=0..D−1} ( indexes[k] * ∏_{j<k} npts[j] )
 *
 * @param indexes           In/out array of length D; on entry may be arbitrary,
 *                          on exit each entry is wrapped into valid range.
 * @param dimensionsCount   Number of dimensions (D)
 * @param npts              Per-dimension sizes (length D)
 * @return                  The flat offset.
 */
OCIndex
memOffsetFromIndexes(OCIndex *indexes, OCIndex dimensionsCount, const OCIndex *npts);
/**
 * @brief Convert a flat offset into a full index‐vector.
 *
 * index[k] = (offset / ∏_{j<k} npts[j]) % npts[k]
 *
 * @param memOffset         The flat offset.
 * @param indexes           Output array of length D.
 * @param dimensionsCount   Number of dimensions (D).
 * @param npts              Per-dimension sizes (length D).
 */
void setIndexesForMemOffset(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts);
/**
 * @brief Like setIndexesForMemOffset, but skip one dimension.
 *
 * Fills `indexes[idim]` only for idim ≠ ignoredDimension; the array must be pre-initialized.
 *
 * @param memOffset           The flat offset.
 * @param indexes             Output array of length D.
 * @param dimensionsCount     Number of dimensions (D).
 * @param npts                Per-dimension sizes (length D).
 * @param ignoredDimension    The one dimension index to skip.
 */
void setIndexesForReducedMemOffsetIgnoringDimension(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts, OCIndex ignoredDimension);
/**
 * @brief Like setIndexesForMemOffset, but skip any dimensions in a set.
 *
 * Fills `indexes[idim]` for all idim ∉ dimensionIndexSet; the array must be pre-initialized.
 *
 * @param memOffset            The flat offset.
 * @param indexes              Output array of length D.
 * @param dimensionsCount      Number of dimensions (D).
 * @param npts                 Per-dimension sizes (length D).
 * @param dimensionIndexSet    Set of dimension‐indices to skip.
 */
void setIndexesForReducedMemOffsetIgnoringDimensions(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts, OCIndexSetRef dimensionIndexSet);
#ifdef __cplusplus
}
#endif
#endif /* RMNGRIDUTILS_H */
