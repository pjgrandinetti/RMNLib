#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "RMNLibrary.h"
#include "test_utils.h"

// ----------------------------------------------------------------------------
// test_SILinearDimensionCreateInverse
// ----------------------------------------------------------------------------
bool test_SILinearDimensionCreateInverse(void) {
    bool ok = false;
    OCStringRef error = NULL;
    SIScalarRef increment = NULL;
    SIScalarRef coordinatesOffset = NULL;
    SIScalarRef originOffset = NULL;
    SIScalarRef period = NULL;
    SILinearDimensionRef originalDim = NULL;
    SILinearDimensionRef inverseDim = NULL;

    printf("Testing SILinearDimensionCreateInverse...\n");
    
    // Create test scalars for dimensions with reciprocal relationship
    increment = SIScalarCreateWithDouble(0.5, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(increment != NULL);
    
    coordinatesOffset = SIScalarCreateWithDouble(100.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(coordinatesOffset != NULL);
    
    originOffset = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(originOffset != NULL);
    
    period = SIScalarCreateWithDouble(1000.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(period != NULL);
    
    // Create original dimension (time domain)
    error = NULL;
    originalDim = SILinearDimensionCreate(
        STR("time"),
        STR("Test time dimension for inverse operation"),
        NULL, // metadata
        kSIQuantityTime,
        coordinatesOffset,
        NULL, // origin
        NULL, // period
        false, // not periodic
        kDimensionScalingNone,  // scaling
        4,   // count (keep it small like working examples)
        increment,
        false, // not complex FFT
        NULL, // no reciprocal for now
        &error
    );
    TEST_ASSERT(originalDim != NULL);
    TEST_ASSERT(error == NULL);
    
    // Test 1: Create inverse dimension - should fail because dimension has no reciprocal
    error = NULL;
    inverseDim = SILinearDimensionCreateInverse(originalDim, &error);
    TEST_ASSERT(inverseDim == NULL);
    TEST_ASSERT(error != NULL);
    printf("  ✓ Dimension without reciprocal correctly rejected: %s\n", OCStringGetCString(error));
    OCRelease(error);
    
    ok = true;
    printf("✅ SILinearDimensionCreateInverse test passed!\n");

cleanup:
    if (increment) OCRelease(increment);
    if (coordinatesOffset) OCRelease(coordinatesOffset);
    if (originOffset) OCRelease(originOffset);
    if (period) OCRelease(period);
    if (originalDim) OCRelease(originalDim);
    if (inverseDim) OCRelease(inverseDim);
    if (error) OCRelease(error);
    
    return ok;
}

// ----------------------------------------------------------------------------
// test_DimensionScalarMultiplication
// ----------------------------------------------------------------------------
bool test_DimensionScalarMultiplication(void) {
    bool ok = false;
    OCStringRef error = NULL;
    SIScalarRef increment = NULL;
    SIScalarRef coordinatesOffset = NULL;
    SIScalarRef multiplier = NULL;
    SILinearDimensionRef originalDim = NULL;
    SILinearDimensionRef resultDim = NULL;
    SIScalarRef newIncrement = NULL;

    printf("Testing dimension scalar multiplication operations...\n");
    
    // Create test dimension
    increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(increment != NULL);
    
    coordinatesOffset = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(coordinatesOffset != NULL);
    
    originalDim = SILinearDimensionCreate(
        STR("length"),
        STR("Test length dimension"),
        NULL, 
        kSIQuantityLength,
        coordinatesOffset, 
        NULL, 
        NULL,
        false, 
        kDimensionScalingNone, 
        11, 
        increment, 
        false, 
        NULL, 
        &error
    );
    TEST_ASSERT(originalDim != NULL);
    TEST_ASSERT(error == NULL);
    
    // Test scalar multiplication by 2.0
    multiplier = SIScalarCreateWithDouble(2.0, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(multiplier != NULL);
    
    error = NULL;
    resultDim = SILinearDimensionCreateByMultiplyingByScalar(originalDim, multiplier, &error);
    TEST_ASSERT(resultDim != NULL);
    TEST_ASSERT(error == NULL);
    
    // Verify the increment was scaled properly
    newIncrement = SILinearDimensionCopyIncrement(resultDim);
    TEST_ASSERT(newIncrement != NULL);
    
    double incrementValue = SIScalarDoubleValue(newIncrement);
    TEST_ASSERT(fabs(incrementValue - 2.0) < 1e-10);
    printf("  ✓ Original increment: 1.0 m, New increment: %.1f m\n", incrementValue);
    
    // Test error case: multiplication by zero
    SIScalarRef zeroScalar = SIScalarCreateWithDouble(0.0, SIUnitDimensionlessAndUnderived());
    error = NULL;
    SILinearDimensionRef zeroResult = SILinearDimensionCreateByMultiplyingByScalar(originalDim, zeroScalar, &error);
    TEST_ASSERT(zeroResult == NULL);
    TEST_ASSERT(error != NULL);
    TEST_ASSERT(error != NULL && OCStringGetLength(error) > 0);
    printf("  ✓ Zero multiplication correctly rejected\n");
    
    ok = true;
    printf("✅ Dimension scalar multiplication test passed!\n");

cleanup:
    if (increment) OCRelease(increment);
    if (coordinatesOffset) OCRelease(coordinatesOffset);
    if (multiplier) OCRelease(multiplier);
    if (originalDim) OCRelease(originalDim);
    if (resultDim) OCRelease(resultDim);
    if (newIncrement) OCRelease(newIncrement);
    if (zeroScalar) OCRelease(zeroScalar);
    if (zeroResult) OCRelease(zeroResult);
    if (error) OCRelease(error);
    
    return ok;
}
