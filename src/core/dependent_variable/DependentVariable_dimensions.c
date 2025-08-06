#include "DependentVariable.h"
#include "DependentVariable_private.h"
DependentVariableRef DependentVariableCreateCrossSection(DependentVariableRef dv, OCArrayRef dimensions, OCIndexPairSetRef indexPairs, OCStringRef *outError) {
    // 0) bail if caller already has an error
    if (outError && *outError) return NULL;
    if (!dv) return NULL;
    OCIndex allDimsCount = OCArrayGetCount(dimensions);
    OCIndex fixedCount = OCIndexPairSetGetCount(indexPairs);
    OCIndex freeCount = allDimsCount - fixedCount;
    if (freeCount < 0 || freeCount > allDimsCount) return NULL;
    // 1) no dims fixed? just copy
    if (freeCount == allDimsCount) {
        return DependentVariableCopy(dv);
    }
    // 2) build an OCIndexSet of the *fixed* dimension‐indices
    OCIndexSetRef fixedDims =
        OCIndexPairSetCreateIndexSetOfIndexes(indexPairs);
    // 3) compute cross‐section length = product of all *free* dims
    OCIndex crossSize =
        RMNCalculateSizeFromDimensionsIgnoring(dimensions, fixedDims);
    // 4) allocate output DV of the right size
    DependentVariableRef outDV =
        DependentVariableCreateWithSize(
            DependentVariableGetName(dv),
            DependentVariableGetDescription(dv),
            dv->unit,
            DependentVariableGetQuantityName(dv),
            DependentVariableGetQuantityType(dv),
            DependentVariableGetNumericType(dv),
            DependentVariableGetComponentLabels(dv),
            crossSize,
            /* owner */ NULL);
    if (!outDV) {
        OCRelease(fixedDims);
        return NULL;
    }
    // 5) prepare coord & npts arrays
    OCIndex *coords = calloc((size_t)allDimsCount, sizeof(OCIndex));
    OCIndex *npts = calloc((size_t)allDimsCount, sizeof(OCIndex));
    for (OCIndex d = 0; d < allDimsCount; d++) {
        DimensionRef dim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, d);
        // fixed dims get their specified coordinate; others start at 0
        coords[d] = OCIndexPairSetContainsIndex(indexPairs, d)
                        ? OCIndexPairSetValueForIndex(indexPairs, d)
                        : 0;
        // record the total length along each dimension
        npts[d] = DimensionGetCount(dim);
    }
    // 6) for each component buffer, walk the cross‐section
    OCNumberType elemType = DependentVariableGetNumericType(dv);
    OCIndex nComps = DependentVariableGetComponentCount(dv);
    for (OCIndex ci = 0; ci < nComps; ci++) {
        OCDataRef srcBlob = DependentVariableGetComponentAtIndex(dv, ci);
        OCMutableDataRef dstBlob = (OCMutableDataRef)
            DependentVariableGetComponentAtIndex(outDV, ci);
        const void *srcPtr = OCDataGetBytesPtr(srcBlob);
        void *dstPtr = OCDataGetMutableBytes(dstBlob);
        if (freeCount == 0) {
            // single‐point slice
            OCIndex memOff = RMNGridMemOffsetFromIndexes(dimensions, coords);
            switch (elemType) {
                case kOCNumberFloat32Type:
                    ((float *)dstPtr)[0] = ((float *)srcPtr)[memOff];
                    break;
                case kOCNumberFloat64Type:
                    ((double *)dstPtr)[0] = ((double *)srcPtr)[memOff];
                    break;
                case kOCNumberComplex64Type:
                    ((float complex *)dstPtr)[0] =
                        ((float complex *)srcPtr)[memOff];
                    break;
                case kOCNumberComplex128Type:
                    ((double complex *)dstPtr)[0] =
                        ((double complex *)srcPtr)[memOff];
                    break;
                default:
                    break;
            }
        } else {
            // full slice: for each output offset, decode the *free* dims
            for (OCIndex outOff = 0; outOff < crossSize; outOff++) {
                setIndexesForReducedMemOffsetIgnoringDimensions(outOff, coords, allDimsCount, npts, fixedDims);
                OCIndex memOff =
                    RMNGridMemOffsetFromIndexes(dimensions, coords);
                switch (elemType) {
                    case kOCNumberFloat32Type:
                        ((float *)dstPtr)[outOff] = ((float *)srcPtr)[memOff];
                        break;
                    case kOCNumberFloat64Type:
                        ((double *)dstPtr)[outOff] = ((double *)srcPtr)[memOff];
                        break;
                    case kOCNumberComplex64Type:
                        ((float complex *)dstPtr)[outOff] =
                            ((float complex *)srcPtr)[memOff];
                        break;
                    case kOCNumberComplex128Type:
                        ((double complex *)dstPtr)[outOff] =
                            ((double complex *)srcPtr)[memOff];
                        break;
                    default:
                        break;
                }
            }
        }
    }
    free(coords);
    free(npts);
    OCRelease(fixedDims);
    return outDV;
}
OCArrayRef DependentVariableCreatePackedSparseComponentsArray(DependentVariableRef dv, OCArrayRef dimensions) {
    if (!dv || !dimensions) return NULL;
    SparseSamplingRef ss = DependentVariableGetSparseSampling(dv);
    OCIndexSetRef idxs = SparseSamplingGetDimensionIndexes(ss);
    OCArrayRef verts = SparseSamplingGetSparseGridVertexes(ss);
    if (!idxs || !verts) return NULL;
    OCIndex nVerts = OCArrayGetCount(verts);
    // 1) Build an “output” DV of exactly the right size (nVerts)
    DependentVariableRef outDV =
        DependentVariableCreateWithSize(
            DependentVariableGetName(dv),
            DependentVariableGetDescription(dv),
            dv->unit,
            DependentVariableGetQuantityName(dv),
            DependentVariableGetQuantityType(dv),
            DependentVariableGetNumericType(dv),
            DependentVariableGetComponentLabels(dv),
            nVerts,
            /* owner */ NULL);
    if (!outDV) return NULL;
    OCStringRef err = NULL;
    // 2) Cross‐section each sparse vertex into outDV
    for (OCIndex iVert = 0; iVert < nVerts; ++iVert) {
        OCIndexPairSetRef pairs = (OCIndexPairSetRef)OCArrayGetValueAtIndex(verts, iVert);
        DependentVariableRef slice =
            DependentVariableCreateCrossSection(dv, dimensions, pairs, &err);
        if (slice) {
            DependentVariableAppend(outDV, slice, &err);
            OCRelease(slice);
        }
    }
    // 3) Extract & return a _deep_ mutable copy of the packed components
    OCArrayRef packed =
        OCArrayCreateMutableCopy(outDV->components);
    OCRelease(outDV);
    return packed;
}
OCDataRef DependentVariableCreateCSDMComponentsData(DependentVariableRef dv,
                                                    OCArrayRef dimensions) {
    if (!dv) return NULL;
    // 1) Allocate the output buffer
    OCMutableDataRef buffer = OCDataCreateMutable(0);
    if (!buffer) return NULL;
    // 2) Decide whether we're packing just the sparse points...
    OCArrayRef sourceArray = NULL;
    bool ownsArray = false;
    SparseSamplingRef ss = DependentVariableGetSparseSampling(dv);
    if (ss) {
        OCIndexSetRef idxs = SparseSamplingGetDimensionIndexes(ss);
        OCArrayRef verts = SparseSamplingGetSparseGridVertexes(ss);
        if (idxs && OCIndexSetGetCount(idxs) > 0 && verts) {
            sourceArray = DependentVariableCreatePackedSparseComponentsArray(dv, dimensions);
            ownsArray = true;  // we'll need to release it
        }
    }
    // 3) …otherwise just concatenate ALL components
    if (!sourceArray) {
        sourceArray = dv->components;
    }
    // 4) Append every chunk in sourceArray to our buffer
    OCIndex n = OCArrayGetCount(sourceArray);
    for (OCIndex i = 0; i < n; ++i) {
        OCDataRef chunk = (OCDataRef)OCArrayGetValueAtIndex(sourceArray, i);
        const uint8_t *ptr = OCDataGetBytesPtr(chunk);
        uint64_t len = OCDataGetLength(chunk);
        if (!OCDataAppendBytes(buffer, ptr, len)) {
            goto fail;
        }
    }
    // 5) Clean up and return
    if (ownsArray) OCRelease(sourceArray);
    return (OCDataRef)buffer;
fail:
    if (ownsArray) OCRelease(sourceArray);
    OCRelease(buffer);
    return NULL;
}
