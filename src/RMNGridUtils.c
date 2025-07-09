// RMNDUtilities.c
#include "RMNLibrary.h"
OCIndex RMNCalculateSizeFromDimensions(OCArrayRef dimensions) {
    OCIndex size = 1;
    if (!dimensions) return size;
    OCIndex nDims = OCArrayGetCount(dimensions);
    for (OCIndex i = 0; i < nDims; i++) {
        DimensionRef dim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
        OCTypeID tid = OCGetTypeID(dim);
        OCIndex npts = 1;
        if (tid == SILinearDimensionGetTypeID()) {
            npts = SILinearDimensionGetCount((SILinearDimensionRef)dim);
        } else if (tid == SIMonotonicDimensionGetTypeID()) {
            OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
            npts = coords ? OCArrayGetCount(coords) : 1;
        } else if (tid == LabeledDimensionGetTypeID()) {
            OCArrayRef labels = LabeledDimensionGetCoordinateLabels((LabeledDimensionRef)dim);
            npts = labels ? OCArrayGetCount(labels) : 1;
        }
        // any other Dimension subclasses default to 1
        size *= npts;
    }
    return size;
}
OCIndex RMNCalculateSizeFromDimensionsIgnoring(OCArrayRef dimensions,
                                       OCIndexSetRef ignored) {
    OCIndex size = 1;
    if (!dimensions) return size;
    OCIndex nDims = OCArrayGetCount(dimensions);
    for (OCIndex i = 0; i < nDims; i++) {
        if (ignored && OCIndexSetContainsIndex(ignored, i))
            continue;
        DimensionRef dim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
        OCTypeID tid = OCGetTypeID(dim);
        OCIndex npts = 1;
        if (tid == SILinearDimensionGetTypeID()) {
            npts = SILinearDimensionGetCount((SILinearDimensionRef)dim);
        } else if (tid == SIMonotonicDimensionGetTypeID()) {
            OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
            npts = coords ? OCArrayGetCount(coords) : 1;
        } else if (tid == LabeledDimensionGetTypeID()) {
            OCArrayRef labels = LabeledDimensionGetCoordinateLabels((LabeledDimensionRef)dim);
            npts = labels ? OCArrayGetCount(labels) : 1;
        }
        size *= npts;
    }
    return size;
}
OCIndex RMNGridMemOffsetFromIndexes(OCArrayRef dimensions,const OCIndex indexes[]) {
    if (!dimensions || !indexes) return (OCIndex)-1;
    OCIndex nDims = OCArrayGetCount(dimensions);
    if (nDims == 0) return 0;
    // start with the last dimension
    OCIndex memOffset;
    {
        DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, nDims - 1);
        OCIndex npts = DimensionGetCount(d);
        OCIndex idx = indexes[nDims - 1] % npts;
        if (idx < 0) idx += npts;
        memOffset = idx;
    }
    // then fold in each earlier dimension
    for (OCIndex i = nDims - 1; i-- > 0;) {
        DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
        OCIndex npts = DimensionGetCount(d);
        memOffset *= npts;
        OCIndex idx = indexes[i] % npts;
        if (idx < 0) idx += npts;
        memOffset += idx;
    }
    return memOffset;
}
OCIndex
RMNGridCoordinateIndexFromMemOffset(OCArrayRef dimensions,
                                    OCIndex memOffset,
                                    OCIndex dimensionIndex) {
    if (!dimensions) return (OCIndex)-1;
    OCIndex nDims = OCArrayGetCount(dimensions);
    if (dimensionIndex >= nDims) return (OCIndex)-1;
    OCIndex hyper = 1;
    for (OCIndex i = 0; i <= dimensionIndex; ++i) {
        DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
        OCIndex npts = DimensionGetCount(d);
        OCIndex coord = (memOffset / hyper) % npts;
        if (i == dimensionIndex) {
            return coord;
        }
        hyper *= npts;
    }
    return (OCIndex)-1;  // should never happen
}
OCIndex strideAlongDimensionIndex(const OCIndex *npts, const OCIndex dimensionsCount, const OCIndex dimensionIndex) {
    if (dimensionIndex == 0) return 1;
    OCIndex stride = 1;
    for (OCIndex idim = 0; idim < dimensionIndex; idim++) {
        stride *= npts[idim];
    }
    return stride;
}
OCIndex memOffsetFromIndexes(OCIndex *indexes, const OCIndex dimensionsCount, const OCIndex *npts) {
    // npts is an array containing the total number of samples along each dimension
    // First alias all indexes back into valid range.
    for (OCIndex idim = 0; idim < dimensionsCount; idim++) {
        indexes[idim] = indexes[idim] % npts[idim];
        if (indexes[idim] < 0) indexes[idim] += npts[idim];
    }
    OCIndex memOffset = indexes[dimensionsCount - 1];
    for (OCIndex idim = dimensionsCount - 2; idim >= 0; idim--) {
        memOffset *= npts[idim];
        memOffset += indexes[idim];
    }
    return memOffset;
}
void setIndexesForMemOffset(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts) {
    OCIndex hyperVolume = 1;
    for (OCIndex idim = 0; idim < dimensionsCount; idim++) {
        indexes[idim] = (memOffset / hyperVolume) % (npts[idim]);
        hyperVolume *= npts[idim];
    }
}
void setIndexesForReducedMemOffsetIgnoringDimension(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts, const OCIndex ignoredDimension) {
    OCIndex hyperVolume = 1;
    for (OCIndex idim = 0; idim < dimensionsCount; idim++) {
        if (idim != ignoredDimension) {
            indexes[idim] = (memOffset / hyperVolume) % (npts[idim]);
            hyperVolume *= npts[idim];
        }
    }
}
void setIndexesForReducedMemOffsetIgnoringDimensions(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts, OCIndexSetRef dimensionIndexSet) {
    OCIndex hyperVolume = 1;
    for (OCIndex idim = 0; idim < dimensionsCount; idim++) {
        if (!OCIndexSetContainsIndex(dimensionIndexSet, idim)) {
            indexes[idim] = (memOffset / hyperVolume) % (npts[idim]);
            hyperVolume *= npts[idim];
        }
    }
}
