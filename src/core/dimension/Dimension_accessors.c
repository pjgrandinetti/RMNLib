#include "Dimension.h"
#include "Dimension_private.h"
/**
 * @file Dimension_accessors.c
 * @brief Property getters and setters for Dimension objects
 *
 * This file implements simple accessor and mutator functions for the Dimension
 * type hierarchy. Only pure getter/setter functions with minimal dependencies
 * are moved here to avoid circular dependencies.
 */
#pragma region Dimension (Base Class) Accessors
OCStringRef DimensionCopyLabel(DimensionRef dim) {
    return (OCStringRef)OCTypeDeepCopy((OCTypeRef)dim->label);
}
OCStringRef DimensionGetLabel(DimensionRef dim) {
    return dim ? dim->label : NULL;
}
bool DimensionSetLabel(DimensionRef dim,
                       OCStringRef label,
                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim) {
        if (outError) *outError = STR("DimensionSetLabel: dim is NULL");
        return false;
    }
    OCStringRef labelCopy = label ? OCStringCreateCopy(label) : STR("");
    if (label && !labelCopy) {
        if (outError)
            *outError = STR("DimensionSetLabel: failed to copy label string");
        return false;
    }
    if (!label && !labelCopy) {
        if (outError)
            *outError = STR("DimensionSetLabel: failed to create default empty label");
        return false;
    }
    OCRelease(dim->label);
    dim->label = labelCopy;
    return true;
}
OCStringRef DimensionCopyDescription(DimensionRef dim) {
    return (OCStringRef)OCTypeDeepCopy((OCTypeRef)dim->description);
}
OCStringRef DimensionGetDescription(DimensionRef dim) {
    return dim ? dim->description : NULL;
}
bool DimensionSetDescription(DimensionRef dim,
                             OCStringRef desc,
                             OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim) {
        if (outError)
            *outError = STR("DimensionSetDescription: dim is NULL");
        return false;
    }
    OCStringRef descCopy = desc ? OCStringCreateCopy(desc) : STR("");
    if (desc && !descCopy) {
        if (outError)
            *outError = STR("DimensionSetDescription: failed to copy description string");
        return false;
    }
    if (!desc && !descCopy) {
        if (outError)
            *outError = STR("DimensionSetDescription: failed to create default empty description");
        return false;
    }
    OCRelease(dim->description);
    dim->description = descCopy;
    return true;
}
OCMutableDictionaryRef DimensionGetApplicationMetaData(DimensionRef dim) {
    return dim ? dim->application : NULL;
}
bool DimensionSetApplicationMetaData(DimensionRef dim,
                                     OCDictionaryRef dict,
                                     OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim) {
        if (outError)
            *outError = STR("DimensionSetApplicationMetaData: dim is NULL");
        return false;
    }
    OCMutableDictionaryRef dictCopy = NULL;
    if (dict) {
        dictCopy = (OCMutableDictionaryRef)OCTypeDeepCopy(dict);
        if (!dictCopy) {
            if (outError)
                *outError = STR("DimensionSetApplicationMetaData: failed to copy metadata dictionary");
            return false;
        }
    } else {
        dictCopy = OCDictionaryCreateMutable(0);
        if (!dictCopy) {
            if (outError)
                *outError = STR("DimensionSetApplicationMetaData: failed to create empty metadata dictionary");
            return false;
        }
    }
    OCRelease(dim->application);
    dim->application = dictCopy;
    return true;
}
#pragma endregion
#pragma region LabeledDimension Accessors
OCArrayRef LabeledDimensionCopyCoordinateLabels(LabeledDimensionRef dim) {
    return (OCArrayRef)OCTypeDeepCopy((OCTypeRef)dim->coordinateLabels);
}
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim) {
    return dim ? (OCArrayRef)dim->coordinateLabels : NULL;
}
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                         OCArrayRef coordinateLabels,
                                         OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim || !coordinateLabels) {
        if (outError)
            *outError = STR("LabeledDimensionSetCoordinateLabels: dim and coordinateLabels must be non-NULL");
        return false;
    }
    // no-op if it's already the same array
    if (dim->coordinateLabels == coordinateLabels)
        return true;
    // need at least two labels
    if (OCArrayGetCount(coordinateLabels) < 2) {
        if (outError)
            *outError = STR("LabeledDimensionSetCoordinateLabels: need ≥2 coordinate labels");
        return false;
    }
    // deep-copy the new labels
    OCMutableArrayRef coordLabelsCopy =
        (OCMutableArrayRef)OCTypeDeepCopy((OCTypeRef)coordinateLabels);
    if (!coordLabelsCopy) {
        if (outError)
            *outError = STR("LabeledDimensionSetCoordinateLabels: failed to deep-copy coordinate labels");
        return false;
    }
    // swap in
    OCRelease(dim->coordinateLabels);
    dim->coordinateLabels = coordLabelsCopy;
    return true;
}
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                               OCIndex index,
                                               OCStringRef label) {
    if (!dim || !dim->coordinateLabels || !label)
        return false;
    OCIndex count = OCArrayGetCount(dim->coordinateLabels);
    if (index < 0 || index >= count)
        return false;
    // Deep-copy the incoming string so we own it
    OCStringRef labelCopy = OCStringCreateCopy(label);
    if (!labelCopy)
        return false;
    // OCArraySetValueAtIndex (with kOCTypeArrayCallBacks) will
    // release the old value and retain ours
    OCArraySetValueAtIndex(dim->coordinateLabels, index, labelCopy);
    // Release our local ownership (the array has retained it)
    OCRelease(labelCopy);
    return true;
}
bool DimensionIsQuantitative(DimensionRef dim) {
    if (!dim) return false;
    OCTypeID tid = OCGetTypeID(dim);
    if (tid == LabeledDimensionGetTypeID())
        return false;
    else if (tid == SIMonotonicDimensionGetTypeID())
        return true;
    else if (tid == SILinearDimensionGetTypeID())
        return true;
    else if (tid == SIDimensionGetTypeID())
        return true;
    else
        return false;
}
OCIndex DimensionGetCount(DimensionRef dim) {
    if (!dim) return 0;
    OCTypeID tid = OCGetTypeID(dim);
    if (tid == SILinearDimensionGetTypeID()) {
        return SILinearDimensionGetCount((SILinearDimensionRef)dim);
    } else if (tid == SIMonotonicDimensionGetTypeID()) {
        OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
        return coords ? OCArrayGetCount(coords) : 0;
    } else if (tid == LabeledDimensionGetTypeID()) {
        OCArrayRef labels = LabeledDimensionGetCoordinateLabels((LabeledDimensionRef)dim);
        return labels ? OCArrayGetCount(labels) : 0;
    }
    // abstract base and any other subclasses default to a single point
    return 1;
}
#pragma endregion
#pragma region SIDimension Accessors
OCStringRef SIDimensionCopyQuantityName(SIDimensionRef dim) {
    return (OCStringRef)OCTypeDeepCopy((OCTypeRef)dim->quantityName);
}
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
bool SIDimensionSetQuantityName(SIDimensionRef dim, OCStringRef name, OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Must have both a dimension and a name
    if (!dim || !name) {
        if (outError) *outError = STR("SIDimensionSetQuantityName: dim and name must be non-NULL");
        return false;
    }
    // 2) Look up the dimensionality for the requested quantityName
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(name, &err);
    if (!nameDim) {
        if (outError) {
            *outError = err
                            ? err
                            : STR("SIDimensionSetQuantityName: unknown quantityName");
        }
        return false;
    }
    OCRelease(err);
    // 3) We need an existing offset to validate against
    SIScalarRef coords = dim->offset;
    if (!coords) {
        if (outError) *outError = STR("SIDimensionSetQuantityName: cannot validate without offset");
        return false;
    }
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    // 4) Compare reduced dimensionalities
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("SIDimensionSetQuantityName: dimensionality mismatch between \"%@\" and existing unit"),
                name);
        }
        return false;
    }
    // 5) All good — replace the old name
    OCRelease(dim->quantityName);
    dim->quantityName = OCStringCreateCopy(name);
    if (!dim->quantityName) {
        if (outError) *outError = STR("SIDimensionSetQuantityName: failed to copy name");
        return false;
    }
    // 6) If we have an origin that no longer matches, reset it to zero
    if (dim->origin) {
        SIDimensionalityRef origDim = SIQuantityGetUnitDimensionality((SIQuantityRef)dim->origin);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->origin);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)coords);
            dim->origin = SIScalarCreateWithDouble(0.0, u);
        }
    }
    // 7) If the period no longer matches, clear it
    if (dim->period) {
        SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)dim->period);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
            SIDimensionSetPeriodToInfinity((SILinearDimensionRef)dim);
        }
    }
    return true;
}
SIScalarRef SIDimensionCopyCoordinatesOffset(SIDimensionRef dim) {
    return (SIScalarRef)OCTypeDeepCopy((OCTypeRef)dim->offset);
}
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim) {
    return dim ? dim->offset : NULL;
}
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim, SIScalarRef val, OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Must have both a dimension and a value
    if (!dim || !val) {
        if (outError) *outError = STR("SIDimensionSetCoordinatesOffset: dim and val must be non-NULL");
        return false;
    }
    // 2) No complex values allowed
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        if (outError) *outError = STR("SIDimensionSetCoordinatesOffset: val must be real-valued");
        return false;
    }
    // 3) Look up the dimensionality for our quantityName
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim =
        SIDimensionalityForQuantity(dim->quantityName, &err);
    if (!nameDim) {
        if (outError) {
            *outError = err
                            ? err
                            : STR("SIDimensionSetCoordinatesOffset: invalid quantityName");
        }
        return false;
    }
    OCRelease(err);
    // 4) Check that val's dimensionality matches
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, valDim)) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("SIDimensionSetCoordinatesOffset: dimensionality mismatch for \"%@\""),
                dim->quantityName);
        }
        return false;
    }
    // 5) Deep-copy & swap in the new offset
    SIScalarRef newCoords = SIScalarCreateCopy(val);
    if (!newCoords) {
        if (outError) *outError = STR("SIDimensionSetCoordinatesOffset: failed to copy scalar");
        return false;
    }
    OCRelease(dim->offset);
    dim->offset = newCoords;
    // 6) If origin no longer matches, reset it to zero
    if (dim->origin) {
        SIDimensionalityRef origDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->origin);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->origin);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)newCoords);
            dim->origin = SIScalarCreateWithDouble(0.0, u);
        }
    }
    // 7) If period no longer matches, clear it
    if (dim->period) {
        SIDimensionalityRef perDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->period);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
            SIDimensionSetPeriodToInfinity((SILinearDimensionRef)dim);
        }
    }
    return true;
}
SIScalarRef SIDimensionCopyOriginOffset(SIDimensionRef dim) {
    return (SIScalarRef)OCTypeDeepCopy((OCTypeRef)dim->origin);
}
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim) {
    return dim ? dim->origin : NULL;
}
bool SIDimensionSetOriginOffset(SIDimensionRef dim, SIScalarRef val, OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Must have both a dimension and a value
    if (!dim || !val) {
        if (outError) *outError = STR("SIDimensionSetOriginOffset: dim and val must be non-NULL");
        return false;
    }
    // 2) Reject complex‐valued scalars
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        if (outError) *outError = STR("SIDimensionSetOriginOffset: val must be real-valued");
        return false;
    }
    // 3) Need a reference offset to validate against
    SIScalarRef coords = dim->offset;
    if (!coords) {
        if (outError) *outError = STR("SIDimensionSetOriginOffset: cannot validate without offset");
        return false;
    }
    // 4) Both must share the same reduced dimensionality
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(refDim, valDim)) {
        if (outError) *outError = STR("SIDimensionSetOriginOffset: dimensionality mismatch with offset");
        return false;
    }
    // 5) Deep‐copy & swap in the new origin
    SIScalarRef copy = SIScalarCreateCopy(val);
    if (!copy) {
        if (outError) *outError = STR("SIDimensionSetOriginOffset: failed to copy scalar");
        return false;
    }
    OCRelease(dim->origin);
    dim->origin = copy;
    return true;
}
SIScalarRef SIDimensionCopyPeriod(SIDimensionRef dim) {
    // if(!dim->periodic) return NULL;
    return SIScalarCreateCopy(dim->period);
}
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim) {
    return dim ? dim->period : NULL;
}
bool SIDimensionSetPeriod(SIDimensionRef dim, SIScalarRef val, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (NULL == val) {
        SIDimensionSetPeriodToInfinity((SILinearDimensionRef)dim);
        return true;
    }
    // 1) Must have both a dimension and a value
    if (!dim || !val) {
        if (outError) *outError = STR("SIDimensionSetPeriod: dim and val must be non-NULL");
        return false;
    }
    // 2) Reject complex‐valued scalars
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        if (outError) *outError = STR("SIDimensionSetPeriod: val must be real-valued");
        return false;
    }
    // 3) Ensure it matches our quantityName dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(dim->quantityName, &err);
    if (!nameDim) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("SIDimensionSetPeriod: invalid quantityName \"%@\""),
                err ? err : STR("<none>"));
        }
        OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 4) Compare reduced dimensionalities
    SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
        if (outError) *outError = STR("SIDimensionSetPeriod: dimensionality mismatch");
        return false;
    }
    // 5) If it's the same object, no change needed (already periodic)
    if (dim->period == val) {
        return true;
    }
    // 6) Convert & deep‐copy into the unit
    SIUnitRef relUnit = SIQuantityGetUnit((SIQuantityRef)dim->offset);
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(val, relUnit, NULL);
    if (!copy) {
        if (outError) *outError = STR("SIDimensionSetPeriod: conversion to relative unit failed");
        return false;
    }
    // 7) Normalize element type
    SIScalarSetNumericType((SIMutableScalarRef)copy, (SINumberType)kOCNumberFloat64Type);
    // 8) Swap in the new period value (dimension becomes periodic)
    OCRelease(dim->period);
    dim->period = copy;
    return true;
}
bool SIDimensionSetPeriodToInfinity(SILinearDimensionRef dim) {
    if (!dim || !dim->increment || dim->count < 2) {
        return false;
    }
    SIScalarRef coordinatesOffset = SIDimensionGetCoordinatesOffset((SIDimensionRef) dim);
    SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)coordinatesOffset);
    SIScalarRef period = SIScalarCreateWithDouble(INFINITY, unit);
    SIDimensionSetPeriod((SIDimensionRef)dim, period, NULL);
    OCRelease(period);
    return true;
}
bool SIDimensionIsPeriodic(SIDimensionRef dim) {
    return dim && SIScalarIsInfinite(SIDimensionGetPeriod(dim)) == false;
}
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim) {
    return dim ? dim->scaling : kDimensionScalingNone;
}
bool SIDimensionSetScaling(SIDimensionRef dim, dimensionScaling scaling) {
    if (!dim) return false;
    dim->scaling = scaling;
    return true;
}
#pragma endregion
#pragma region SIMonotonicDimension Accessors
OCArrayRef SIMonotonicDimensionCopyCoordinates(SIMonotonicDimensionRef dim) {
    return (OCArrayRef)OCTypeDeepCopy((OCTypeRef)dim->coordinates);
}
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim) {
    return dim ? (OCArrayRef)dim->coordinates : NULL;
}
SIDimensionRef SIMonotonicDimensionCopyReciprocal(SIMonotonicDimensionRef dim) {
    return (SIDimensionRef)OCTypeDeepCopy((OCTypeRef)dim->reciprocal);
}
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim, OCArrayRef coords, OCStringRef *outError) {
    if (outError) *outError = NULL;
    
    if (!dim) {
        if (outError) *outError = STR("SIMonotonicDimensionSetCoordinates: dim is NULL");
        return false;
    }
    
    if (!coords || OCArrayGetCount(coords) < 2) {
        if (outError) *outError = STR("SIMonotonicDimensionSetCoordinates: need ≥2 coordinates");
        return false;
    }
    
    // Convert coordinates to SIScalar array (handles OCNumbers)
    OCArrayRef scalarCoords = SIScalarCreateArrayFromMixedTypeArray(coords, outError);
    if (!scalarCoords) {
        if (outError && !*outError) *outError = STR("SIMonotonicDimensionSetCoordinates: failed to convert coordinates to SIScalar array");
        return false;
    }
    
    // Derive dimensionality from first coordinate
    SIScalarRef first = (SIScalarRef)OCArrayGetValueAtIndex(scalarCoords, 0);
    SIDimensionalityRef baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)first);
    
    // Validate that all coordinates have the same dimensionality
    OCStringRef err = NULL;
    if (!SIQuantityValidateMixedArrayForDimensionality(scalarCoords, baseDim, &err)) {
        if (outError) *outError = err;
        else if (err) OCRelease(err);
        OCRelease(scalarCoords);
        return false;
    }
    
    // Validate dimensionality matches the dimension's quantity
    SIDimensionalityRef dimDim = SIQuantityGetUnitDimensionality((SIQuantityRef)SIDimensionGetCoordinatesOffset((SIDimensionRef)dim));
    if (!SIDimensionalityHasSameReducedDimensionality(baseDim, dimDim)) {
        if (outError) *outError = STR("SIMonotonicDimensionSetCoordinates: coordinate dimensionality does not match dimension's quantity");
        OCRelease(scalarCoords);
        return false;
    }
    
    // All validation passed - update coordinates
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(scalarCoords);
    OCRelease(scalarCoords);
    
    return dim->coordinates != NULL;
}
bool SIMonotonicDimensionSetReciprocal(
    SIMonotonicDimensionRef dim,
    SIDimensionRef r,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim) {
        if (outError) *outError = STR("SIMonotonicDimensionSetReciprocal: dim is NULL");
        return false;
    }
    // no-op if already set to the same object
    if (dim->reciprocal == r) {
        return true;
    }
    // validate that r is truly the reciprocal (if non-NULL)
    if (r) {
        OCStringRef err = NULL;
        if (!impl_SIDimensionIsReciprocalOf((SIDimensionRef)dim, r, &err)) {
            if (outError) {
                *outError = err;  // take ownership of the error string
            } else if (err) {
                OCRelease(err);
            }
            return false;
        }
    }
    // swap in the new reciprocal (NULL is allowed, it just clears)
    OCRelease(dim->reciprocal);
    dim->reciprocal = r;
    if (r) OCRetain(r);
    return true;
}
OCArrayRef SIMonotonicDimensionCreateAbsoluteCoordinates(SIMonotonicDimensionRef dim) {
    if (!dim) return NULL;
    // Get regular coordinates
    OCArrayRef regular_coords = SIMonotonicDimensionGetCoordinates(dim);
    if (!regular_coords) return NULL;
    // Get origin offset
    SIScalarRef origin_offset = SIDimensionGetOriginOffset((SIDimensionRef)dim);
    if (!origin_offset) {
        // If no origin offset, return a copy of regular coordinates
        return OCArrayCreateCopy(regular_coords);
    }
    // Check if origin offset is zero (no transformation needed)
    double offset_value = SIScalarDoubleValue(origin_offset);
    if (fabs(offset_value) < 1e-15) {
        // Origin offset is effectively zero, return copy of regular coordinates
        return OCArrayCreateCopy(regular_coords);
    }
    // Create absolute coordinates array
    OCIndex count = OCArrayGetCount(regular_coords);
    OCMutableArrayRef abs_coords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!abs_coords) return NULL;
    // Apply formula: X^abs_k = X_k + o_k
    for (OCIndex i = 0; i < count; i++) {
        SIScalarRef regular_coord = (SIScalarRef)OCArrayGetValueAtIndex(regular_coords, i);
        if (!regular_coord) {
            OCRelease(abs_coords);
            return NULL;
        }
        // Add origin offset to regular coordinate
        SIScalarRef abs_coord = SIScalarCreateByAdding(regular_coord, origin_offset, NULL);
        if (!abs_coord) {
            OCRelease(abs_coords);
            return NULL;
        }
        // Add to absolute coordinates array
        OCArrayAppendValue(abs_coords, abs_coord);
        OCRelease(abs_coord);
    }
    return (OCArrayRef)abs_coords;
}
#pragma endregion
#pragma region SILinearDimension Accessors
OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim) {
    return dim ? dim->count : 0;
}
bool SILinearDimensionSetCount(SILinearDimensionRef dim, OCIndex count) {
    if (!dim || count < 2) return false;
    dim->count = count;
    // if you want to keep reciprocalIncrement in sync, you could recompute it here…
    return true;
}
SIScalarRef SILinearDimensionCopyIncrement(SILinearDimensionRef dim) {
    return (SIScalarRef)OCTypeDeepCopy((OCTypeRef)dim->increment);
}
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim) {
    return dim ? dim->increment : NULL;
}
bool SILinearDimensionSetIncrement(SILinearDimensionRef dim, SIScalarRef inc) {
    if (!dim || !inc) return false;
    // ensure real & same dimensionality as offset/origin
    SIUnitRef offsetUnit =
        SIQuantityGetUnit((SIQuantityRef)SIDimensionGetCoordinatesOffset((SIDimensionRef)dim));
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(inc, offsetUnit, NULL);
    if (!copy) return false;
    OCRelease(dim->increment);
    dim->increment = copy;
    return true;
}
SIDimensionRef SILinearDimensionCopyReciprocal(SILinearDimensionRef dim) {
    return (SIDimensionRef)OCTypeDeepCopy((OCTypeRef)dim->reciprocal);
}
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}
bool SILinearDimensionSetReciprocal(SILinearDimensionRef dim, SIDimensionRef rec, OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Must have a valid dimension
    if (!dim) {
        if (outError) *outError = STR("SILinearDimensionSetReciprocal: dim is NULL");
        return false;
    }
    // 2) No-op if already the same
    if (dim->reciprocal == rec) {
        return true;
    }
    // 3) If non-NULL, validate the incoming reciprocal
    if (rec) {
        OCStringRef vErr = NULL;
        if (!SIDimensionValidate(rec, &vErr)) {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("SILinearDimensionSetReciprocal: invalid reciprocal: %@"),
                    vErr);
            }
            OCRelease(vErr);
            return false;
        }
        OCRelease(vErr);
    }
    // 4) Swap in (NULL clears)
    OCRelease(dim->reciprocal);
    dim->reciprocal = rec;
    if (rec) OCRetain(rec);
    return true;
}
bool SILinearDimensionGetComplexFFT(SILinearDimensionRef dim) {
    return dim ? dim->fft : false;
}
bool SILinearDimensionSetComplexFFT(SILinearDimensionRef dim, bool fft) {
    if (!dim) return false;
    dim->fft = fft;
    return true;
}
bool SILinearDimensionSetPeriodToWindow(SILinearDimensionRef dim) {
    if (!dim || !dim->increment || dim->count < 2) {
        return false;
    }
    SIMutableScalarRef window = SIScalarCreateMutableCopy(dim->increment);
    if (!window) return false;
    SIScalarMultiplyByDimensionlessRealConstant(window, (double)dim->count);
    SIDimensionSetPeriod((SIDimensionRef)dim, (SIScalarRef)window, NULL);
    OCRelease(window);
    return true;
}
SIScalarRef SILinearDimensionCreateReciprocalIncrement(SILinearDimensionRef dim) {
    if (!dim || !dim->increment || dim->count < 2) {
        return NULL;
    }
    // Make a mutable copy of the increment
    SIMutableScalarRef rec = SIScalarCreateMutableCopy(dim->increment);
    if (!rec) return NULL;
    // Check if multiply by count succeeds
    if (!SIScalarMultiplyByDimensionlessRealConstant(rec, (double)dim->count)) {
        OCRelease(rec);
        return NULL;
    }
    // rec = 1 / rec
    OCStringRef error = NULL;
    if (!SIScalarRaiseToAPowerWithoutReducingUnit(rec, -1, &error)) {
        // Handle the error case
        if (rec) OCRelease(rec);
        if (error) OCRelease(error);
        return NULL;
    }
    return rec;
}
OCArrayRef SILinearDimensionCreateCoordinates(SILinearDimensionRef dim) {
    if (!dim) return NULL;
    // Get dimension properties
    OCIndex count = dim->count;
    if (count == 0) return NULL;
    SIScalarRef increment = dim->increment;
    if (!increment) return NULL;
    SIScalarRef coordinates_offset = SIDimensionGetCoordinatesOffset((SIDimensionRef)dim);
    if (!coordinates_offset) return NULL;
    bool complex_fft = dim->fft;
    // Calculate Z_k according to CSDM specification
    OCIndex Z_k = 0;
    if (complex_fft) {
        OCIndex T_k = (count % 2 == 0) ? count : (count - 1);
        Z_k = T_k / 2;
    }
    // Create coordinate array
    OCMutableArrayRef coords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!coords) return NULL;
    // For each index j in [0, 1, 2, ..., count-1]
    for (OCIndex j = 0; j < count; j++) {
        // Calculate (j - Z_k)
        double index_offset = (double)((long)j - (long)Z_k);
        // Create scalar for increment * (j - Z_k)
        SIMutableScalarRef term1 = SIScalarCreateMutableCopy(increment);
        if (!term1) {
            OCRelease(coords);
            return NULL;
        }
        // Multiply increment by (j - Z_k)
        SIScalarMultiplyByDimensionlessRealConstant(term1, index_offset);
        // Add coordinates_offset: term1 += coordinates_offset
        SIScalarRef result = SIScalarCreateByAdding((SIScalarRef)term1, coordinates_offset, NULL);
        OCRelease(term1);
        if (!result) {
            OCRelease(coords);
            return NULL;
        }
        // Add to coordinate array
        OCArrayAppendValue(coords, result);
        OCRelease(result);
    }
    return (OCArrayRef)coords;
}
SIScalarRef SILinearDimensionCreateCoordinateAtIndex(SILinearDimensionRef dim, OCIndex index) {
    if (!dim) return NULL;
    // Get dimension properties
    OCIndex count = dim->count;
    if (count == 0) return NULL;
    SIScalarRef increment = dim->increment;
    if (!increment) return NULL;
    SIScalarRef coordinates_offset = SIDimensionGetCoordinatesOffset((SIDimensionRef)dim);
    if (!coordinates_offset) return NULL;
    bool complex_fft = dim->fft;
    // Calculate Z_k according to CSDM specification
    OCIndex Z_k = 0;
    if (complex_fft) {
        OCIndex T_k = (count % 2 == 0) ? count : (count - 1);
        Z_k = T_k / 2;
    }
    // Create coordinate array
    OCMutableArrayRef coords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!coords) return NULL;
    // For each index in [0, 1, 2, ..., count-1]
    // Calculate (index - Z_k)
    double index_offset = (double)((long)index - (long)Z_k);
    // Create scalar for increment * (index - Z_k)
    SIMutableScalarRef term1 = SIScalarCreateMutableCopy(increment);
    if (!term1) {
        OCRelease(coords);
        return NULL;
    }
    // Multiply increment by (index - Z_k)
    SIScalarMultiplyByDimensionlessRealConstant(term1, index_offset);
    // Add coordinates_offset: term1 += coordinates_offset
    SIScalarRef result = SIScalarCreateByAdding((SIScalarRef)term1, coordinates_offset, NULL);
    OCRelease(term1);
    if (!result) {
        OCRelease(coords);
        return NULL;
    }
    return(result);
}
OCArrayRef SILinearDimensionCreateAbsoluteCoordinates(SILinearDimensionRef dim) {
    if (!dim) return NULL;
    // Get regular coordinates first
    OCArrayRef regular_coords = SILinearDimensionCreateCoordinates(dim);
    if (!regular_coords) return NULL;
    // Get origin offsetcoordinate
    SIScalarRef origin_offset = SIDimensionGetOriginOffset((SIDimensionRef)dim);
    if (!origin_offset) {
        // If no origin offset, just return regular coordinates
        return regular_coords;
    }
    // Check if origin offset is zero (no transformation needed)
    double offset_value = SIScalarDoubleValue(origin_offset);
    if (fabs(offset_value) < 1e-15) {
        // Origin offset is effectively zero, return regular coordinates
        return regular_coords;
    }
    // Create absolute coordinates array
    OCIndex count = OCArrayGetCount(regular_coords);
    OCMutableArrayRef abs_coords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!abs_coords) {
        OCRelease(regular_coords);
        return NULL;
    }
    // Apply formula: X^abs_k = X_k + o_k
    for (OCIndex i = 0; i < count; i++) {
        SIScalarRef regular_coord = (SIScalarRef)OCArrayGetValueAtIndex(regular_coords, i);
        if (!regular_coord) {
            OCRelease(abs_coords);
            OCRelease(regular_coords);
            return NULL;
        }
        // Add origin offset to regular coordinate
        SIScalarRef abs_coord = SIScalarCreateByAdding(regular_coord, origin_offset, NULL);
        if (!abs_coord) {
            OCRelease(abs_coords);
            OCRelease(regular_coords);
            return NULL;
        }
        // Add to absolute coordinates array
        OCArrayAppendValue(abs_coords, abs_coord);
        OCRelease(abs_coord);
    }
    // Clean up regular coordinates
    OCRelease(regular_coords);
    return (OCArrayRef)abs_coords;
}
#pragma endregion
#pragma region Dimension Type Information and Utilities
OCStringRef DimensionGetType(DimensionRef dim) {
    if (!dim) return NULL;
    OCTypeID tid = OCGetTypeID(dim);
    if (tid == LabeledDimensionGetTypeID())
        return STR("labeled");
    else if (tid == SIMonotonicDimensionGetTypeID())
        return STR("monotonic");
    else if (tid == SILinearDimensionGetTypeID())
        return STR("linear");
    else if (tid == SIDimensionGetTypeID())
        return STR("si_dimension");
    else
        return STR("dimension");
}
OCStringRef DimensionCreateAxisLabel(DimensionRef dim, OCIndex index) {
    if (!dim)
        return NULL;
    OCTypeID tid = OCGetTypeID(dim);
    OCStringRef label = DimensionGetLabel(dim);
    // Use quantity name as fallback label
    OCStringRef quantityName = NULL;
    if (!label || OCStringGetLength(label) == 0) {
        if (tid == SIDimensionGetTypeID() ||
            tid == SIMonotonicDimensionGetTypeID() ||
            tid == SILinearDimensionGetTypeID()) {
            quantityName = SIDimensionGetQuantityName((SIDimensionRef)dim);
        }
        label = (quantityName && OCStringGetLength(quantityName) > 0)
                    ? quantityName
                    : STR("(unlabeled)");
    }
    // LabeledDimension: just label-<index>
    if (tid == LabeledDimensionGetTypeID()) {
        return OCStringCreateWithFormat(STR("%@-%ld"), label, (long)index);
    }
    // SIDimension or subclasses: label-<index>/<unit>
    OCStringRef unitStr = NULL;
    if (tid == SIDimensionGetTypeID() ||
        tid == SIMonotonicDimensionGetTypeID() ||
        tid == SILinearDimensionGetTypeID()) {
        SIScalarRef offset = SIDimensionGetCoordinatesOffset((SIDimensionRef)dim);
        if (offset)
            unitStr = SIScalarCopyUnitSymbol(offset);
    }
    // Defensive: fallback if unitStr is NULL
    if (!unitStr)
        unitStr = STR("(no unit)");
    OCStringRef out = OCStringCreateWithFormat(
        STR("%@-%ld/%@"), label, (long)index, unitStr);
    if (unitStr && unitStr != STR("(no unit)")) {
        OCRelease(unitStr);
    }
    return out;
}

OCTypeRef DimensionCopyCoordinateAtIndex(DimensionRef dim, double index)
{
    IF_NO_OBJECT_EXISTS_RETURN(dim,NULL);
        if (!dim)
        return NULL;
    OCTypeID tid = OCGetTypeID(dim);

    if (tid == SILinearDimensionGetTypeID()) {
        return (OCTypeRef)SILinearDimensionCreateCoordinateAtIndex((SILinearDimensionRef) dim, index);
    }
    if (tid == SIMonotonicDimensionGetTypeID()) {
        OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
        return (OCTypeRef)SIScalarCreateCopy(OCArrayGetValueAtIndex(coords, index));
    }
    if (tid == LabeledDimensionGetTypeID()) {
        OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
        return (OCTypeRef)OCStringCreateCopy(OCArrayGetValueAtIndex(coords, index));
    }
    return NULL;
}


OCTypeRef DimensionCreateInterpolatedCoordinateAtIndex(SILinearDimensionRef dim, double dIndex) {
    // Input validation
    if (!dim) return NULL;
    if (dim->count == 0) return NULL;
    
    // Handle bounds consistently
    if (dIndex <= 0.0) {
        return DimensionCopyCoordinateAtIndex((DimensionRef)dim, 0);
    }
    if (dIndex >= (double)(dim->count - 1)) {
        return DimensionCopyCoordinateAtIndex((DimensionRef)dim, dim->count - 1);
    }
    
    OCIndex index_before = (OCIndex)floor(dIndex);
    OCIndex index_after = (OCIndex)ceil(dIndex);
    OCIndex index_closest = (OCIndex)round(dIndex);
    OCTypeID tid = OCGetTypeID(dim);
    if (tid == LabeledDimensionGetTypeID()) {
        OCArrayRef coords = SIMonotonicDimensionGetCoordinates((SIMonotonicDimensionRef)dim);
        return (OCTypeRef)OCStringCreateCopy(OCArrayGetValueAtIndex(coords, index_closest));
    }
    if (tid != SILinearDimensionGetTypeID() && tid != SIMonotonicDimensionGetTypeID()) {
        return NULL;
    }

    // For exact indices, no interpolation needed
    if (index_before == index_after) {
        return DimensionCopyCoordinateAtIndex((DimensionRef)dim, index_before);
    }
    
    // Get coordinate values for interpolation
    SIScalarRef before = (SIScalarRef)DimensionCopyCoordinateAtIndex((DimensionRef)dim, index_before);
    SIScalarRef after = (SIScalarRef)DimensionCopyCoordinateAtIndex((DimensionRef)dim, index_after);
    if (!before || !after) {
        if (before) OCRelease(before);
        if (after) OCRelease(after);
        return NULL;
    }
    
    // Calculate interpolated value: before + fraction * (after - before)
    double fraction = dIndex - (double)index_before;
    SIMutableScalarRef diff = SIScalarCreateMutableCopy(after);
    if (!diff) {
        OCRelease(before);
        OCRelease(after);
        return NULL;
    }
    
    SIScalarSubtract(diff, before, NULL);
    SIScalarMultiplyByDimensionlessRealConstant(diff, fraction);
    SIScalarRef result = SIScalarCreateByAdding(before, (SIScalarRef)diff, NULL);
    
    // Clean up resources
    OCRelease(before);
    OCRelease(after);
    OCRelease(diff);
    
    return (OCTypeRef) result;
}


#pragma endregion
