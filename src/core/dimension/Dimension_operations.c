#include "../RMNLibrary.h"
#include "Dimension_private.h"

/**
 * @brief Multiply an SILinearDimension by a scalar, updating all dimension properties accordingly.
 * 
 * This operation scales the dimension's increment, offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar.
 * 
 * @param dim The SILinearDimension to multiply (must be mutable)
 * @param scalar The scalar to multiply by
 * @param outError Optional error output parameter
 * @return true if successful, false on error
 */
bool SILinearDimensionMultiplyByScalar(SILinearDimensionRef dim, SIScalarRef scalar, OCStringRef *outError) {
    if (!dim || !scalar) {
        if (outError) *outError = STR("Dimension or scalar is NULL");
        return false;
    }
    
    // Check for zero scalar
    if (SIScalarIsReal(scalar)) {
        double value = SIScalarDoubleValue(scalar);
        if (value == 0.0) {
            if (outError) *outError = STR("Cannot multiply by zero scalar");
            return false;
        }
        // Optimization: if multiplying by dimensionless 1, do nothing
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)scalar);
        if (value == 1.0 && SIUnitIsDimensionless(unit)) {
            return true;
        }
    }
    
    // Get internal structure
    struct impl_SILinearDimension *linear = (struct impl_SILinearDimension *)dim;
    
    // Multiply the increment (spacing between points)
    if (linear->increment) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)linear->increment, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply increment");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    // Multiply offset and origin in the base SIDimension
    if (linear->_super.offset) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)linear->_super.offset, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply offset");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    if (linear->_super.origin) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)linear->_super.origin, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply origin");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    // Multiply period if it exists
    if (linear->_super.period) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)linear->_super.period, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply period");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    // Update the quantity name based on the new unit
    if (linear->increment) {
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)linear->increment);
        OCStringRef quantityName = SIUnitCreateQuantityNameGuess(unit);
        if (quantityName) {
            // Release old quantity name and set new one
            if (linear->_super.quantityName) {
                OCRelease(linear->_super.quantityName);
            }
            linear->_super.quantityName = quantityName;
        }
    }
    
    // Handle reciprocal dimension if it exists
    if (linear->reciprocal) {
        // Create inverse scalar for reciprocal operations
        OCStringRef error = NULL;
        SIScalarRef inverseScalar = SIScalarCreateByRaisingToPower(scalar, -1, &error);
        if (!inverseScalar) {
            if (outError) *outError = error ? error : STR("Failed to create inverse scalar");
            else if (error) OCRelease(error);
            return false;
        }
        
        // Recursively multiply the reciprocal dimension by the inverse scalar
        bool success = true;
        if (SILinearDimensionGetTypeID() == OCGetTypeID(linear->reciprocal)) {
            success = SILinearDimensionMultiplyByScalar((SILinearDimensionRef)linear->reciprocal, inverseScalar, &error);
        } else if (SIMonotonicDimensionGetTypeID() == OCGetTypeID(linear->reciprocal)) {
            success = SIMonotonicDimensionMultiplyByScalar((SIMonotonicDimensionRef)linear->reciprocal, inverseScalar, &error);
        }
        
        OCRelease(inverseScalar);
        
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply reciprocal dimension");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Multiply an SIMonotonicDimension by a scalar, updating all dimension properties accordingly.
 * 
 * This operation scales all coordinates, offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar.
 * 
 * @param dim The SIMonotonicDimension to multiply (must be mutable)
 * @param scalar The scalar to multiply by
 * @param outError Optional error output parameter
 * @return true if successful, false on error
 */
bool SIMonotonicDimensionMultiplyByScalar(SIMonotonicDimensionRef dim, SIScalarRef scalar, OCStringRef *outError) {
    if (!dim || !scalar) {
        if (outError) *outError = STR("Dimension or scalar is NULL");
        return false;
    }
    
    // Check for zero scalar
    if (SIScalarIsReal(scalar)) {
        double value = SIScalarDoubleValue(scalar);
        if (value == 0.0) {
            if (outError) *outError = STR("Cannot multiply by zero scalar");
            return false;
        }
        // Optimization: if multiplying by dimensionless 1, do nothing
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)scalar);
        if (value == 1.0 && SIUnitIsDimensionless(unit)) {
            return true;
        }
    }
    
    // Get internal structure
    struct impl_SIMonotonicDimension *monotonic = (struct impl_SIMonotonicDimension *)dim;
    
    // Multiply all coordinates
    if (monotonic->coordinates) {
        OCIndex count = OCArrayGetCount(monotonic->coordinates);
        for (OCIndex i = 0; i < count; i++) {
            SIScalarRef coordinate = (SIScalarRef)OCArrayGetValueAtIndex(monotonic->coordinates, i);
            if (coordinate) {
                OCStringRef error = NULL;
                bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)coordinate, scalar, &error);
                if (!success) {
                    if (outError) *outError = error ? error : STR("Failed to multiply coordinate");
                    else if (error) OCRelease(error);
                    return false;
                }
            }
        }
    }
    
    // Multiply offset and origin in the base SIDimension
    if (monotonic->_super.offset) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)monotonic->_super.offset, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply offset");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    if (monotonic->_super.origin) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)monotonic->_super.origin, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply origin");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    // Multiply period if it exists
    if (monotonic->_super.period) {
        OCStringRef error = NULL;
        bool success = SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)monotonic->_super.period, scalar, &error);
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply period");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    // Update the quantity name based on the new unit
    if (monotonic->coordinates && OCArrayGetCount(monotonic->coordinates) > 0) {
        SIScalarRef firstCoord = (SIScalarRef)OCArrayGetValueAtIndex(monotonic->coordinates, 0);
        if (firstCoord) {
            SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)firstCoord);
            OCStringRef quantityName = SIUnitCreateQuantityNameGuess(unit);
            if (quantityName) {
                // Release old quantity name and set new one
                if (monotonic->_super.quantityName) {
                    OCRelease(monotonic->_super.quantityName);
                }
                monotonic->_super.quantityName = quantityName;
            }
        }
    }
    
    // Handle reciprocal dimension if it exists
    if (monotonic->reciprocal) {
        // Create inverse scalar for reciprocal operations
        OCStringRef error = NULL;
        SIScalarRef inverseScalar = SIScalarCreateByRaisingToPower(scalar, -1, &error);
        if (!inverseScalar) {
            if (outError) *outError = error ? error : STR("Failed to create inverse scalar");
            else if (error) OCRelease(error);
            return false;
        }
        
        // Recursively multiply the reciprocal dimension by the inverse scalar
        bool success = true;
        if (SILinearDimensionGetTypeID() == OCGetTypeID(monotonic->reciprocal)) {
            success = SILinearDimensionMultiplyByScalar((SILinearDimensionRef)monotonic->reciprocal, inverseScalar, &error);
        } else if (SIMonotonicDimensionGetTypeID() == OCGetTypeID(monotonic->reciprocal)) {
            success = SIMonotonicDimensionMultiplyByScalar((SIMonotonicDimensionRef)monotonic->reciprocal, inverseScalar, &error);
        }
        
        OCRelease(inverseScalar);
        
        if (!success) {
            if (outError) *outError = error ? error : STR("Failed to multiply reciprocal dimension");
            else if (error) OCRelease(error);
            return false;
        }
    }
    
    return true;
}

