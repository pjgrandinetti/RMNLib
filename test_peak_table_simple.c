#include "RMNLibrary.h"
#include <stdio.h>

// Forward declaration
DatasetRef DatasetImportJCAMPCreateSignalWithData(OCDataRef contents, OCStringRef *error);

int main() {
    printf("Testing PEAK TABLE implementation - simple test...\n");
    
    // Test with the pktab1.jdx file
    const char* testFilePath = "/Users/philip/Github/Software/OCTypes-SITypes/RMNLib/tests/JCAMP/pktab1.jdx";
    
    OCStringRef error = NULL;
    OCDataRef fileData = OCDataCreateWithContentsOfFile(testFilePath, &error);
    
    if (!fileData) {
        printf("Failed to read test file: %s\n", error ? OCStringGetCString(error) : "unknown error");
        if (error) OCRelease(error);
        return 1;
    }
    
    printf("Successfully read test file\n");
    
    // Import the JCAMP data
    DatasetRef dataset = DatasetImportJCAMPCreateSignalWithData(fileData, &error);
    OCRelease(fileData);
    
    if (!dataset) {
        printf("Failed to import JCAMP data: %s\n", error ? OCStringGetCString(error) : "unknown error");
        if (error) OCRelease(error);
        return 1;
    }
    
    printf("Successfully imported PEAK TABLE dataset!\n");
    
    // Check dataset properties
    OCStringRef title = DatasetGetTitle(dataset);
    printf("Dataset title: %s\n", title ? OCStringGetCString(title) : "none");
    
    // Check dimensions
    OCArrayRef dimensions = DatasetGetDimensions(dataset);
    OCIndex numDims = OCArrayGetCount(dimensions);
    printf("Number of dimensions: %ld\n", (long)numDims);
    
    if (numDims > 0) {
        DimensionRef firstDim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, 0);
        OCIndex count = DimensionGetCount(firstDim);
        printf("First dimension count: %ld\n", (long)count);
        
        OCStringRef dimType = DimensionGetType(firstDim);
        printf("Dimension type: %s\n", dimType ? OCStringGetCString(dimType) : "unknown");
        
        // If monotonic, show some coordinates
        if (dimType && OCStringCompare(dimType, STR("monotonic"), 0) == kOCCompareEqualTo) {
            SIMonotonicDimensionRef monoDim = (SIMonotonicDimensionRef)firstDim;
            OCArrayRef coords = SIMonotonicDimensionGetCoordinates(monoDim);
            if (coords) {
                OCIndex coordCount = OCArrayGetCount(coords);
                printf("Coordinate count: %ld\n", (long)coordCount);
                if (coordCount > 0) {
                    SIScalarRef firstCoord = (SIScalarRef)OCArrayGetValueAtIndex(coords, 0);
                    if (firstCoord) {
                        OCStringRef coordStr = SIScalarCreateStringValue(firstCoord);
                        printf("First coordinate: %s\n", coordStr ? OCStringGetCString(coordStr) : "unknown");
                        if (coordStr) OCRelease(coordStr);
                    }
                }
            }
        }
    }
    
    printf("PEAK TABLE test completed successfully!\n");
    
    OCRelease(dataset);
    return 0;
}
