#include "RMNLibrary.h"

OCIndex strideAlongDimensionIndex(const OCIndex *npts, const OCIndex dimensionsCount, const OCIndex dimensionIndex)
{
    if(dimensionIndex==0) return 1;
    OCIndex stride = 1;
    for(OCIndex idim = 0; idim<dimensionIndex; idim++) {
        stride *= npts[idim];
    }
    return stride;
}

OCIndex memOffsetFromIndexes(OCIndex *indexes, const OCIndex dimensionsCount, const OCIndex *npts)
{
    // npts is an array containing the total number of samples along each dimension
    
    // First alias all indexes back into valid range.
   for(OCIndex idim = 0;idim<dimensionsCount; idim++) {
        indexes[idim] = indexes[idim]%npts[idim];
        if(indexes[idim] < 0) indexes[idim] += npts[idim];
    }

    OCIndex memOffset = indexes[dimensionsCount-1];
    for(OCIndex idim = dimensionsCount-2;idim >= 0; idim--) {
        memOffset *= npts[idim];
        memOffset += indexes[idim];
    }
    return memOffset;
}

void setIndexesForMemOffset(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts)
{
    OCIndex hyperVolume = 1;
    for(OCIndex idim = 0; idim<dimensionsCount; idim++) {
        indexes[idim] = (memOffset/hyperVolume)%(npts[idim]);
        hyperVolume *= npts[idim];
    }
}

void setIndexesForReducedMemOffsetIgnoringDimension(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts, const OCIndex ignoredDimension)
{
    OCIndex hyperVolume = 1;
    for(OCIndex idim = 0; idim<dimensionsCount; idim++) {
        if(idim!=ignoredDimension) {
            indexes[idim] = (memOffset/hyperVolume)%(npts[idim]);
            hyperVolume *= npts[idim];
        }
    }
}

void setIndexesForReducedMemOffsetIgnoringDimensions(const OCIndex memOffset, OCIndex indexes[], const OCIndex dimensionsCount, const OCIndex *npts, OCIndexSetRef dimensionIndexSet)
{
    OCIndex hyperVolume = 1;
    for(OCIndex idim = 0; idim<dimensionsCount; idim++) {
        if(!OCIndexSetContainsIndex(dimensionIndexSet, idim)) {
            indexes[idim] = (memOffset/hyperVolume)%(npts[idim]);
            hyperVolume *= npts[idim];
        }
    }
}

