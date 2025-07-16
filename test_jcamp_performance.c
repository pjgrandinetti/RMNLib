#include "src/RMNLibrary.h"
#include "src/DatasetJCAMP.h"
#include <sys/time.h>
#include <stdio.h>

double getTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int main() {
    const char* filename = "tests/JCAMP/GC/guava.jdx";
    
    printf("Testing JCAMP parsing performance for %s\n", filename);
    
    // Read file
    OCStringRef error = NULL;
    OCDataRef fileData = OCDataCreateWithContentsOfFile(filename, &error);
    
    if (!fileData) {
        printf("Error: Could not read file %s", filename);
        if (error) {
            printf(": %s", OCStringGetCString(error));
            OCRelease(error);
        }
        printf("\n");
        return 1;
    }
    
    printf("File size: %llu bytes\n", (unsigned long long)OCDataGetLength(fileData));
    
    // Time the parsing
    double start = getTimestamp();
    
    error = NULL;
    DatasetRef dataset = DatasetImportJCAMPCreateSignalWithData(fileData, &error);
    
    double end = getTimestamp();
    double elapsed = end - start;
    
    OCRelease(fileData);
    
    if (!dataset) {
        printf("Error parsing JCAMP");
        if (error) {
            printf(": %s", OCStringGetCString(error));
            OCRelease(error);
        }
        printf("\n");
        return 1;
    }
    
    printf("Parsing completed successfully in %.3f seconds\n", elapsed);
    printf("Dataset has %llu dimension(s)\n", (unsigned long long)OCArrayGetCount(DatasetGetDimensions(dataset)));
    
    OCArrayRef dimensions = DatasetGetDimensions(dataset);
    if (OCArrayGetCount(dimensions) > 0) {
        DimensionRef firstDim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, 0);
        OCIndex count = DimensionGetCount(firstDim);
        printf("First dimension size: %ld points\n", (long)count);
        printf("Performance: %.1f points/second\n", count / elapsed);
    }
    
    OCRelease(dataset);
    if (error) OCRelease(error);
    
    return 0;
}
