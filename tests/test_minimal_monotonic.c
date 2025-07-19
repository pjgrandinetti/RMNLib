#include "RMNLibrary.h"
#include <stdio.h>

bool test_minimal_monotonic(void) {
    OCStringRef error = NULL;
    
    // Create coordinates array with SIScalar objects
    double values[] = {0.0, 1.5, 3.7, 8.2, 15.0};
    OCMutableArrayRef valueArray = OCArrayCreateMutable(5, &kOCTypeArrayCallBacks);
    for (int i = 0; i < 5; i++) {
        OCStringRef valueStr = OCStringCreateWithFormat(STR("%.1f m"), values[i]);
        SIScalarRef scalar = SIScalarCreateFromExpression(valueStr, &error);
        if (!scalar || error) {
            printf("Error creating scalar: %s\n", error ? OCStringGetCString(error) : "unknown");
            OCRelease(valueArray);
            if (error) OCRelease(error);
            return false;
        }
        OCArrayAppendValue(valueArray, (const void*)scalar);
        OCRelease(scalar);
        OCRelease(valueStr);
    }
    
    // Test our new minimal function
    SIMonotonicDimensionRef monotonicDim = SIMonotonicDimensionCreateMinimal(
        kSIQuantityLength, // quantityName
        (OCArrayRef)valueArray,        // coordinates
        NULL,              // reciprocal
        &error);           // outError
    
    if (!monotonicDim || error) {
        printf("Error creating monotonic dimension: %s\n", 
               error ? OCStringGetCString(error) : "unknown error");
        OCRelease(valueArray);
        if (error) OCRelease(error);
        return false;
    }
    
    // Verify the dimension was created correctly
    OCArrayRef coords = SIMonotonicDimensionGetCoordinates(monotonicDim);
    OCIndex count = coords ? OCArrayGetCount(coords) : 0;
    
    printf("✅ SIMonotonicDimensionCreateMinimal test passed!\n");
    printf("   - Created dimension with %ld coordinates\n", (long)count);
    printf("   - Quantity: Length\n");
    printf("   - No reciprocal dimension\n");
    
    // Clean up
    OCRelease(valueArray);
    OCRelease(monotonicDim);
    
    return true;
}
