#include "../RMNLibrary.h"
#include "Dimension_private.h"
/**
 * @brief Multiply an SIDimension by a scalar, updating all dimension properties accordingly.
 *
 * This operation scales the dimension's offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar.
 *
 * @param dim The SIDimension to multiply
 * @param scalar The scalar to multiply by
 * @param outError Optional error output parameter
 * @return A new SIDimensionRef with scaled properties, or NULL on error
 */
SIDimensionRef SIDimensionCreateByMultiplyingByScalar(SIDimensionRef dim,
                                                      SIScalarRef scalar,
                                                      OCStringRef *outError) {
    if (!dim || !scalar) {
        if (outError) *outError = STR("Cannot multiply when dimension or scalar is NULL");
        return NULL;
    }
    // Check for zero scalar
    if (SIScalarIsReal(scalar)) {
        double value = SIScalarDoubleValue(scalar);
        if (value == 0.0) {
            if (outError) *outError = STR("Cannot multiply by zero scalar");
            return NULL;
        }
        // Optimization: if multiplying by dimensionless 1, do nothing
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)scalar);
        if (value == 1.0 && SIUnitEqual(unit, SIUnitDimensionlessAndUnderived())) {
            return dim;
        }
    }
    // Create a deep mutable copy of the dimension - use OCTypeDeepCopy to copy
    // all types of dimensions correctly
    // This ensures we handle both linear and monotonic dimensions correctly
    SIDimensionRef copiedDim = (SIDimensionRef)OCTypeDeepCopy((OCTypeRef)dim);
    if (!copiedDim) {
        if (outError) *outError = STR("Failed to create a copy of the dimension");
        return NULL;
    }
    // Multiply coordinates offset
    SIScalarRef coordinates_offset = SIDimensionGetCoordinatesOffset(copiedDim);
    bool success = SIScalarMultiply((SIMutableScalarRef)coordinates_offset, scalar, outError);
    if (!success) {
        OCRelease(copiedDim);
        return NULL;
    }
    // Update the quantity name based on the new unit
    SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)coordinates_offset);
    OCStringRef quantityNameGuess = SIUnitCreateQuantityNameGuess(unit);
    if (quantityNameGuess) {
        // Access the internal structure to modify the quantityName ivar directly
        struct impl_SIDimension *dimImpl = (struct impl_SIDimension *)copiedDim;
        if (dimImpl->quantityName) {
            OCRelease(dimImpl->quantityName);
        }
        dimImpl->quantityName = OCRetain(quantityNameGuess);
        OCRelease(quantityNameGuess);
    }
    // Multiply origin offset
    SIScalarRef origin_offset = SIDimensionGetOriginOffset(copiedDim);
    success = SIScalarMultiply((SIMutableScalarRef)origin_offset, scalar, outError);
    if (!success) {
        OCRelease(copiedDim);
        return NULL;
    }
    // Update the quantity name for origin_offset
    SIUnitRef origin_unit = SIQuantityGetUnit((SIQuantityRef)origin_offset);
    OCStringRef originQuantityNameGuess = SIUnitCreateQuantityNameGuess(origin_unit);
    if (originQuantityNameGuess) {
        // Can't use setter function since it won't pass validation.
        // Need to access the internal structure directly
        struct impl_SIDimension *dimImpl = (struct impl_SIDimension *)copiedDim;
        if (dimImpl->quantityName) {
            OCRelease(dimImpl->quantityName);
        }
        dimImpl->quantityName = OCRetain(originQuantityNameGuess);
        OCRelease(originQuantityNameGuess);
    }
    // Multiply period
    SIScalarRef period = SIDimensionGetPeriod((SIDimensionRef)copiedDim);
    success = SIScalarMultiply((SIMutableScalarRef)period, scalar, outError);
    if (!success) {
        OCRelease(copiedDim);
        return NULL;
    }
    // Update the quantity name for period
    SIUnitRef period_unit = SIQuantityGetUnit((SIQuantityRef)period);
    OCStringRef periodQuantityNameGuess = SIUnitCreateQuantityNameGuess(period_unit);
    if (periodQuantityNameGuess) {
        // Use setter function if available, or add necessary private header includes
        // SIScalarSetQuantityName((SIMutableScalarRef)period, periodQuantityNameGuess);
        OCRelease(periodQuantityNameGuess);
    }
    return copiedDim;
}
SILinearDimensionRef SILinearDimensionCreateByMultiplyingByScalar(SILinearDimensionRef dim,
                                                                  SIScalarRef scalar,
                                                                  OCStringRef *outError) {
    if (!dim || !scalar) {
        if (outError) *outError = STR("Cannot multiply when dimension or scalar is NULL");
        return NULL;
    }
    // Check for zero scalar
    if (SIScalarIsReal(scalar)) {
        double value = SIScalarDoubleValue(scalar);
        if (value == 0.0) {
            if (outError) *outError = STR("Cannot multiply by zero scalar");
            return NULL;
        }
        // Optimization: if multiplying by dimensionless 1, create a copy anyway for consistency
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)scalar);
        if (value == 1.0 && SIUnitEqual(unit, SIUnitDimensionlessAndUnderived())) {
            return (SILinearDimensionRef)OCTypeDeepCopy((OCTypeRef)dim);
        }
    }
    SILinearDimensionRef copiedDim = (SILinearDimensionRef)SIDimensionCreateByMultiplyingByScalar((SIDimensionRef)dim,
                                                                                                  scalar,
                                                                                                  outError);
    if (!copiedDim) {
        if (outError) *outError = STR("Failed to create a copy of the dimension");
        return NULL;
    }
    SIScalarRef increment = SILinearDimensionGetIncrement(copiedDim);
    // Multiply the increment (spacing between points)
    bool success = SIScalarMultiply((SIMutableScalarRef)increment, scalar, outError);
    if (!success) {
        OCRelease(copiedDim);
        return NULL;
    }
    // Update the quantity name based on the new unit
    SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)increment);
    OCStringRef quantityName = SIUnitCreateQuantityNameGuess(unit);
    if (quantityName) {
        // Access the internal structure to modify the quantityName ivar directly
        struct impl_SIDimension *dimImpl = (struct impl_SIDimension *)copiedDim;
        if (dimImpl->quantityName) {
            OCRelease(dimImpl->quantityName);
        }
        dimImpl->quantityName = OCRetain(quantityName);
        OCRelease(quantityName);
    }
    SIScalarRef inverseScalar = SIScalarCreateByRaisingToPower(scalar, -1, outError);
    if (!inverseScalar) {
        OCRelease(copiedDim);
        return NULL;
    }
    SIDimensionRef reciprocal = SILinearDimensionGetReciprocal(copiedDim);
    SIDimensionRef copiedReciprocalDim = SIDimensionCreateByMultiplyingByScalar((SIDimensionRef)reciprocal,
                                                                                inverseScalar,
                                                                                outError);
    SILinearDimensionSetReciprocal(copiedDim, copiedReciprocalDim, outError);
    OCRelease(inverseScalar);
    OCRelease(copiedReciprocalDim);
    return copiedDim;
}
/**
 * @brief Multiply an SIMonotonicDimension by a scalar, updating all dimension properties accordingly.
 *
 * This operation scales all coordinates, offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar.
 *
 * @param dim The SIMonotonicDimension to multiply
 * @param scalar The scalar to multiply by
 * @param outError Optional error output parameter
 * @return A new SIMonotonicDimensionRef with scaled properties, or NULL on error
 */
SIMonotonicDimensionRef SIMonotonicDimensionCreateByMultiplyingByScalar(SIMonotonicDimensionRef dim, SIScalarRef scalar, OCStringRef *outError) {
    if (!dim || !scalar) {
        if (outError) *outError = STR("Cannot multiply when dimension or scalar is NULL");
        return NULL;
    }
    // Check for zero scalar
    if (SIScalarIsReal(scalar)) {
        double value = SIScalarDoubleValue(scalar);
        if (value == 0.0) {
            if (outError) *outError = STR("Cannot multiply by zero scalar");
            return NULL;
        }
        // Optimization: if multiplying by dimensionless 1, create a copy anyway for consistency
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)scalar);
        if (value == 1.0 && SIUnitEqual(unit, SIUnitDimensionlessAndUnderived())) {
            return (SIMonotonicDimensionRef)OCTypeDeepCopy((OCTypeRef)dim);
        }
    }
    SIMonotonicDimensionRef copiedDim = (SIMonotonicDimensionRef)SIDimensionCreateByMultiplyingByScalar((SIDimensionRef)dim,
                                                                                                        scalar,
                                                                                                        outError);
    if (!copiedDim) {
        if (outError) *outError = STR("Failed to create a copy of the dimension");
        return NULL;
    }
    OCMutableArrayRef coordinates = (OCMutableArrayRef)SIMonotonicDimensionCopyCoordinates(copiedDim);
    if (!coordinates) {
        OCRelease(copiedDim);
        if (outError) *outError = STR("Failed to get coordinates from the dimension");
        return NULL;
    }
    // Multiply each coordinate by the scalar
    OCIndex count = OCArrayGetCount(coordinates);
    for (OCIndex i = 0; i < count; ++i) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coordinates, i);
        if (!coord) {
            OCRelease(copiedDim);
            OCRelease(coordinates);
            if (outError) *outError = STR("Failed to get coordinate at index");
            return NULL;
        }
        // Multiply the coordinate by the scalar
        bool success = SIScalarMultiply((SIMutableScalarRef)coord, scalar, outError);
        if (!success) {
            OCRelease(copiedDim);
            OCRelease(coordinates);
            return NULL;
        }
    }
    struct impl_SIMonotonicDimension *dimImpl = (struct impl_SIMonotonicDimension *)copiedDim;
    OCRelease(dimImpl->coordinates);
    dimImpl->coordinates = coordinates;  // take ownership of the modified coordinates array
    // Update the quantity name based on the new unit
    SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coordinates, 0);
    SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)coord);
    OCStringRef quantityName = SIUnitCreateQuantityNameGuess(unit);
    if (quantityName) {
        // Access the internal structure to modify the quantityName ivar directly
        struct impl_SIDimension *dimImpl = (struct impl_SIDimension *)copiedDim;
        if (dimImpl->quantityName) {
            OCRelease(dimImpl->quantityName);
        }
        dimImpl->quantityName = OCRetain(quantityName);
        OCRelease(quantityName);
    }
    SIScalarRef inverseScalar = SIScalarCreateByRaisingToPower(scalar, -1, outError);
    if (!inverseScalar) {
        OCRelease(copiedDim);
        return NULL;
    }
    SIDimensionRef reciprocal = SIMonotonicDimensionGetReciprocal(copiedDim);
    SIDimensionRef copiedReciprocalDim = SIDimensionCreateByMultiplyingByScalar((SIDimensionRef)reciprocal,
                                                                                inverseScalar,
                                                                                outError);
    SIMonotonicDimensionSetReciprocal(copiedDim, copiedReciprocalDim, outError);
    OCRelease(inverseScalar);
    OCRelease(copiedReciprocalDim);
    return copiedDim;
}
