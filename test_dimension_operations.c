#include <math.h>
#include <stdio.h>
#include "src/RMNLibrary.h"
void test_dimension_multiplication() {
    printf("=== Testing Dimension Multiplication Operations ===\n");
    // Test 1: SILinearDimension multiplication
    printf("\nTest 1: SILinearDimension multiplication by scalar 2.0\n");
    OCStringRef error = NULL;
    // Create a linear dimension: 0-10 with increment 1, in meters
    SIScalarRef start = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("m")));
    SIScalarRef increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("m")));
    OCIndex count = 11;
    SILinearDimensionRef linearDim = SILinearDimensionCreate(
        STR("length"),          // label
        STR("test length"),     // description
        NULL,                   // metadata
        kSIQuantityLength,      // quantity
        start,                  // offset
        NULL,                   // origin
        NULL,                   // period
        false,                  // periodic
        kDimensionScalingNone,  // scaling
        count,                  // count
        increment,              // increment
        false,                  // fft
        NULL,                   // reciprocal
        &error);                // outError
    if (!linearDim) {
        printf("  ❌ Failed to create linear dimension: %s\n",
               error ? OCStringGetCString(error) : "unknown error");
        goto cleanup1;
    }
    // Create scalar multiplier: 2.0 (dimensionless)
    SIScalarRef multiplier = SIScalarCreateWithDouble(2.0, SIUnitWithSymbol(STR("")));
    // Test the multiplication - use the correct function name
    SILinearDimensionRef result = SILinearDimensionCreateByMultiplyingByScalar(linearDim, multiplier, &error);
    if (!result) {
        printf("  ❌ Multiplication failed: %s\n",
               error ? OCStringGetCString(error) : "unknown error");
        goto cleanup1;
    }
    // Check the results using Copy accessors (public API)
    SIScalarRef newIncrement = SILinearDimensionCopyIncrement(result);
    double incrementValue = SIScalarDoubleValue(newIncrement);
    SIScalarRef newOffset = SIDimensionCopyCoordinatesOffset((SIDimensionRef)result);
    double offsetValue = SIScalarDoubleValue(newOffset);
    printf("  ✅ Original increment: 1.0 m, New increment: %.1f m\n", incrementValue);
    printf("  ✅ Original offset: 0.0 m, New offset: %.1f m\n", offsetValue);
    // Verify the scaling worked
    if (fabs(incrementValue - 2.0) < 1e-10 && fabs(offsetValue - 0.0) < 1e-10) {
        printf("  ✅ Linear dimension multiplication: PASSED\n");
    } else {
        printf("  ❌ Linear dimension multiplication: FAILED\n");
    }
    // Clean up the copied values
    if (newIncrement) OCRelease(newIncrement);
    if (newOffset) OCRelease(newOffset);
cleanup1:
    if (start) OCRelease(start);
    if (increment) OCRelease(increment);
    if (linearDim) OCRelease(linearDim);
    if (multiplier) OCRelease(multiplier);
    if (result) OCRelease(result);
    if (error) OCRelease(error);
    error = NULL;
    // Test 2: SIMonotonicDimension multiplication
    printf("\nTest 2: SIMonotonicDimension multiplication by scalar 3.0\n");
    // Create coordinate array: [1, 2, 4, 8] in seconds
    OCMutableArrayRef coords = OCArrayCreateMutable(4, &kOCTypeArrayCallBacks);
    SIScalarRef coord1 = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("s")));
    SIScalarRef coord2 = SIScalarCreateWithDouble(2.0, SIUnitWithSymbol(STR("s")));
    SIScalarRef coord4 = SIScalarCreateWithDouble(4.0, SIUnitWithSymbol(STR("s")));
    SIScalarRef coord8 = SIScalarCreateWithDouble(8.0, SIUnitWithSymbol(STR("s")));
    OCArrayAppendValue(coords, coord1);
    OCArrayAppendValue(coords, coord2);
    OCArrayAppendValue(coords, coord4);
    OCArrayAppendValue(coords, coord8);
    // Create monotonic dimension
    SIMonotonicDimensionRef monotonicDim = SIMonotonicDimensionCreateMinimal(
        STR("time"), coords, NULL, &error);
    if (!monotonicDim) {
        printf("  ❌ Failed to create monotonic dimension: %s\n",
               error ? OCStringGetCString(error) : "unknown error");
        goto cleanup2;
    }
    // Create scalar multiplier: 3.0 (dimensionless)
    SIScalarRef multiplier3 = SIScalarCreateWithDouble(3.0, SIUnitWithSymbol(STR("")));
    // Test the multiplication
    SIMonotonicDimensionRef monotonicResult = SIMonotonicDimensionCreateByMultiplyingByScalar(
        monotonicDim, multiplier3, &error);
    if (!monotonicResult) {
        printf("  ❌ Monotonic multiplication failed: %s\n",
               error ? OCStringGetCString(error) : "unknown error");
        goto cleanup2;
    }
    // Check the results using Copy accessor (public API)
    OCMutableArrayRef newCoords = SIMonotonicDimensionCopyCoordinates(monotonicResult);
    if (newCoords && OCArrayGetCount(newCoords) == 4) {
        SIScalarRef newCoord1 = (SIScalarRef)OCArrayGetValueAtIndex(newCoords, 0);
        SIScalarRef newCoord2 = (SIScalarRef)OCArrayGetValueAtIndex(newCoords, 1);
        double val1 = SIScalarDoubleValue(newCoord1);
        double val2 = SIScalarDoubleValue(newCoord2);
        printf("  ✅ Original coords: [1, 2, 4, 8], New coords: [%.1f, %.1f, ...]\n", val1, val2);
        if (fabs(val1 - 3.0) < 1e-10 && fabs(val2 - 6.0) < 1e-10) {
            printf("  ✅ Monotonic dimension multiplication: PASSED\n");
        } else {
            printf("  ❌ Monotonic dimension multiplication: FAILED\n");
        }
        OCRelease(newCoords);
    } else {
        printf("  ❌ Failed to get new coordinates\n");
    }
cleanup2:
    if (coords) OCRelease(coords);
    if (coord1) OCRelease(coord1);
    if (coord2) OCRelease(coord2);
    if (coord4) OCRelease(coord4);
    if (coord8) OCRelease(coord8);
    if (monotonicDim) OCRelease(monotonicDim);
    if (multiplier3) OCRelease(multiplier3);
    if (monotonicResult) OCRelease(monotonicResult);
    if (error) OCRelease(error);
    // Test 3: Error handling - zero scalar
    printf("\nTest 3: Error handling - multiplication by zero\n");
    SIScalarRef zeroScalar = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("")));
    SIScalarRef testStart = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("m")));
    SIScalarRef testIncrement = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("m")));
    SILinearDimensionRef testDim = SILinearDimensionCreate(
        STR("test"), STR("test dim"), NULL, kSIQuantityLength,
        testStart, NULL, NULL, false, kDimensionScalingNone,
        5, testIncrement, false, NULL, &error);
    if (testDim && zeroScalar) {
        SILinearDimensionRef zeroResult = SILinearDimensionCreateByMultiplyingByScalar(
            testDim, zeroScalar, &error);
        if (!zeroResult && error) {
            printf("  ✅ Zero multiplication correctly rejected: %s\n", OCStringGetCString(error));
            printf("  ✅ Error handling: PASSED\n");
        } else {
            printf("  ❌ Zero multiplication should have failed\n");
        }
        if (zeroResult) OCRelease(zeroResult);
    }
    if (zeroScalar) OCRelease(zeroScalar);
    if (testDim) OCRelease(testDim);
    if (error) OCRelease(error);
    printf("\n=== Dimension Operations Tests Complete ===\n");
}
int main() {
    printf("Testing RMNLib Dimension Operations\n");
    printf("Build date: %s %s\n\n", __DATE__, __TIME__);
    test_dimension_multiplication();
    return 0;
}
