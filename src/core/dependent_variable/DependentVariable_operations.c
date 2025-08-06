#include "DependentVariable.h"
#include "DependentVariable_private.h"
/**
 * @brief Convert all component data in a dependent variable to a new unit.
 *        Integer‐typed dependent variables cannot be converted and will error.
 * @param dv      The dependent variable to convert.
 * @param unit    The target unit (must have the same reduced dimensionality).
 * @param error   On failure, receives an OCStringRef describing the problem; may be NULL.
 *                   Caller should release *error if non-NULL.
 * @return true on success, false on error.
 * @ingroup RMNLib
 */
bool DependentVariableConvertToUnit(DependentVariableRef dv,
                                    SIUnitRef unit,
                                    OCStringRef *error) {
    /* Bail if caller passed in an existing error string */
    if (error && *error) return false;
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    /* Retrieve components and ensure there is data to convert */
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t count = OCArrayGetCount(comps);
    if (count == 0) {
        if (error) {
            *error = STR("Convert to Unit failed: no data components present.");
        }
        return false;
    }
    /* Determine element type and reject integer types */
    OCNumberType etype = DependentVariableGetElementType(dv);
    switch (etype) {
        case kOCNumberSInt8Type:
        case kOCNumberSInt16Type:
        case kOCNumberSInt32Type:
        case kOCNumberSInt64Type:
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            if (error) {
                *error = STR("Invalid operation: cannot convert integer-typed dependent variable to another unit.");
            }
            return false;
        default:
            break;
    }
    /* Check dimensionality compatibility */
    SIUnitRef oldUnit = SIQuantityGetUnit((SIQuantityRef)dv);
    SIDimensionalityRef oldDim = SIUnitGetDimensionality(oldUnit);
    SIDimensionalityRef newDim = SIUnitGetDimensionality(unit);
    if (!SIDimensionalityHasSameReducedDimensionality(oldDim, newDim)) {
        if (error) {
            *error = STR("Convert to Unit failed: incompatible dimensionalities.");
        }
        return false;
    }
    /* Compute conversion factor and update stored unit */
    double factor = SIUnitConversion(oldUnit, unit);
    SIQuantitySetUnit((SIMutableQuantityRef)dv, unit);
    /* Scale each component in a simple loop */
    uint64_t size = DependentVariableGetSize(dv);
    for (uint64_t ci = 0; ci < count; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (etype) {
            case kOCNumberFloat32Type: {
                float *arr = (float *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= (float)factor;
                }
                break;
            }
            case kOCNumberFloat64Type: {
                double *arr = (double *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= factor;
                }
                break;
            }
            case kOCNumberComplex64Type: {
                float complex *arr = (float complex *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= (float)factor;
                }
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *arr = (double complex *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= factor;
                }
                break;
            }
            default:
                /* All integer types handled above; no action here */
                break;
        }
    }
    return true;
}
bool DependentVariableSetValuesToZero(DependentVariableRef dv, int64_t componentIndex) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0) return false;
    // Determine the range of components to clear
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        if ((uint64_t)componentIndex >= nComps) return false;
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    // For each selected component, just memset its entire byte buffer to zero
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        uint64_t byteCount = OCDataGetLength(data);
        memset(bytes, 0, byteCount);
    }
    return true;
}
bool DependentVariableZeroPartInRange(DependentVariableRef dv,
                                      OCIndex componentIndex,
                                      OCRange range,
                                      complexPart part) {
    if (!dv) return false;
    OCArrayRef comps = dv->components;
    OCIndex nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 || componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }
    OCIndex totalSize = DependentVariableGetSize(dv);
    if (range.location < 0 || range.length < 0 || range.location + range.length > totalSize) {
        return false;
    }
    OCIndex startComp = componentIndex;
    OCIndex endComp = componentIndex + 1;
    for (OCIndex ci = startComp; ci < endComp; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *ptr = OCDataGetMutableBytes(data);
        OCNumberType etype = DependentVariableGetElementType(dv);
        switch (etype) {
            case kOCNumberFloat32Type:
                if (part == kSIRealPart || part == kSIMagnitudePart) {
                    /* scale the subvector at &ptr[range.location] by 0.0f */
                    cblas_sscal((int)range.length,
                                0.0f,
                                (float *)ptr + range.location,
                                1);
                }
                break;
            case kOCNumberFloat64Type:
                if (part == kSIRealPart || part == kSIMagnitudePart) {
                    cblas_dscal((int)range.length,
                                0.0,
                                (double *)ptr + range.location,
                                1);
                }
                break;
            case kOCNumberComplex64Type: {
                float complex *cptr = (float complex *)ptr;
                switch (part) {
                    case kSIRealPart:
                        /* zero real entries, keep imag: real at stride=2 offset 0 */
                        cblas_sscal((int)range.length,
                                    0.0f,
                                    (float *)cptr + 2 * range.location,
                                    2);
                        break;
                    case kSIImaginaryPart:
                        /* zero imag entries: offset 1, stride=2 */
                        cblas_sscal((int)range.length,
                                    0.0f,
                                    (float *)cptr + 2 * range.location + 1,
                                    2);
                        break;
                    case kSIMagnitudePart:
                        /* zero both real & imag: treat as 2*length contiguous */
                        cblas_sscal((int)(2 * range.length),
                                    0.0f,
                                    (float *)cptr + 2 * range.location,
                                    1);
                        break;
                    case kSIArgumentPart:
                        /* must compute abs and zero imag by hand */
                        for (OCIndex i = range.location; i < range.location + range.length; ++i) {
                            float complex v = cptr[i];
                            float m = cabsf(v);
                            cptr[i] = m + 0.0f * I;
                        }
                        break;
                }
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *cptr = (double complex *)ptr;
                switch (part) {
                    case kSIRealPart:
                        cblas_dscal((int)range.length,
                                    0.0,
                                    (double *)cptr + 2 * range.location,
                                    2);
                        break;
                    case kSIImaginaryPart:
                        cblas_dscal((int)range.length,
                                    0.0,
                                    (double *)cptr + 2 * range.location + 1,
                                    2);
                        break;
                    case kSIMagnitudePart:
                        cblas_dscal((int)(2 * range.length),
                                    0.0,
                                    (double *)cptr + 2 * range.location,
                                    1);
                        break;
                    case kSIArgumentPart:
                        for (OCIndex i = range.location; i < range.location + range.length; ++i) {
                            double complex v = cptr[i];
                            double m = cabs(v);
                            cptr[i] = m + 0.0 * I;
                        }
                        break;
                }
                break;
            }
            default:
                /* integer types or unsupported */
                return false;
        }
    }
    return true;
}
bool DependentVariableTakeAbsoluteValue(DependentVariableRef dv,
                                        int64_t componentIndex) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0 ||
        (componentIndex >= 0 && (uint64_t)componentIndex >= nComps)) {
        return false;
    }
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    uint64_t size = DependentVariableGetSize(dv);
    OCNumberType origEtype = DependentVariableGetElementType(dv);
    OCNumberType newEtype = origEtype;
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (origEtype) {
            case kOCNumberSInt8Type: {
                int8_t *arr = (int8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt16Type: {
                int16_t *arr = (int16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt32Type: {
                int32_t *arr = (int32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt64Type: {
                int64_t *arr = (int64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberUInt8Type:
            case kOCNumberUInt16Type:
            case kOCNumberUInt32Type:
            case kOCNumberUInt64Type:
                /* Unsigned types are already non-negative */
                break;
            case kOCNumberFloat32Type: {
                float *arr = (float *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = fabsf(arr[i]);
                break;
            }
            case kOCNumberFloat64Type: {
                double *arr = (double *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = fabs(arr[i]);
                break;
            }
            case kOCNumberComplex64Type: {
                float complex *arr = (float complex *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = cabsf(arr[i]); /* real=|z|, imag=0 */
                newEtype = kOCNumberFloat32Type;
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *arr = (double complex *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = cabs(arr[i]); /* real=|z|, imag=0 */
                newEtype = kOCNumberFloat64Type;
                break;
            }
            default:
                return false;
        }
    }
    if (newEtype != origEtype) {
        DependentVariableSetElementType(dv, newEtype);
    }
    return true;
}
bool DependentVariableMultiplyValuesByDimensionlessComplexConstant(DependentVariableRef dv,
                                                                   int64_t componentIndex,
                                                                   double complex constant) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0 ||
        (componentIndex >= 0 && (uint64_t)componentIndex >= nComps)) {
        return false;
    }
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    uint64_t size = DependentVariableGetSize(dv);
    OCNumberType etype = DependentVariableGetElementType(dv);
    /* Prepare BLAS scalars */
    float scalar_f32 = (float)creal(constant);
    double scalar_f64 = creal(constant);
    float scalar_c32[2] = {(float)creal(constant), (float)cimag(constant)};
    double scalar_c64[2] = {creal(constant), cimag(constant)};
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (etype) {
            case kOCNumberSInt8Type: {
                int8_t *arr = (int8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt16Type: {
                int16_t *arr = (int16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt32Type: {
                int32_t *arr = (int32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt64Type: {
                int64_t *arr = (int64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int64_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt8Type: {
                uint8_t *arr = (uint8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt16Type: {
                uint16_t *arr = (uint16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt32Type: {
                uint32_t *arr = (uint32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt64Type: {
                uint64_t *arr = (uint64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint64_t)(arr[i] * constant);
                break;
            }
            case kOCNumberFloat32Type:
                cblas_sscal((int)size, scalar_f32, (float *)bytes, 1);
                break;
            case kOCNumberFloat64Type:
                cblas_dscal((int)size, scalar_f64, (double *)bytes, 1);
                break;
            case kOCNumberComplex64Type:
                cblas_cscal((int)size,
                            (const void *)scalar_c32,
                            (float _Complex *)bytes,
                            1);
                break;
            case kOCNumberComplex128Type:
                cblas_zscal((int)size,
                            (const void *)scalar_c64,
                            (double _Complex *)bytes,
                            1);
                break;
            default:
                /* Unsupported element type */
                return false;
        }
    }
    return true;
}
/**
 * @brief Extracts a specific complex component (real, imaginary, magnitude, or argument)
 *        from a DependentVariable’s data, replacing each value accordingly.
 *
 * @param dv               The DependentVariable to modify.
 * @param componentIndex   Index of the component to operate on (0-based).
 *                         If negative, the operation applies to all components in sequence.
 * @param part             Which part of each element to retain:
 *                         - kSIRealPart      : keep real part, zero imaginary
 *                         - kSIImaginaryPart : keep imaginary part, zero real
 *                         - kSIMagnitudePart : replace with magnitude (abs)
 *                         - kSIArgumentPart  : replace with argument (phase)
 *
 * @return true if the data was successfully transformed; false if inputs are invalid
 *         or the variable’s numeric type does not support the requested component.
 *
 * @ingroup DependentVariable
 *
 * @code
 * // Convert component 1 of myDV to its magnitude values:
 * bool ok = DependentVariableTakeComplexPart(myDV, 1, kSIMagnitudePart);
 * @endcode
 */
bool DependentVariableTakeComplexPart(DependentVariableRef dv,
                                      OCIndex componentIndex,
                                      complexPart part) {
    if (!dv) return false;
    OCArrayRef comps = dv->components;
    OCIndex nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps) {
        return false;
    }
    OCIndex size = DependentVariableGetSize(dv);
    OCRange fullRange = {.location = 0, .length = size};
    switch (dv->numericType) {
        case kOCNumberFloat32Type:
        case kOCNumberFloat64Type:
            switch (part) {
                case kSIRealPart:
                    return true;
                case kSIImaginaryPart:
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIRealPart);
                    break;
                case kSIMagnitudePart:
                    DependentVariableTakeAbsoluteValue(dv, componentIndex);
                    break;
                case kSIArgumentPart:
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIRealPart);
                    break;
            }
            break;
        case kOCNumberComplex64Type:
            switch (part) {
                case kSIRealPart:
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
                    break;
                case kSIImaginaryPart:
                    DependentVariableMultiplyValuesByDimensionlessComplexConstant(dv, componentIndex, -I);
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
                    break;
                case kSIMagnitudePart:
                    DependentVariableTakeAbsoluteValue(dv, componentIndex);
                    break;
                case kSIArgumentPart: {
                    OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, componentIndex);
                    float complex *buf = (float complex *)OCDataGetMutableBytes(data);
                    for (OCIndex i = 0; i < size; ++i) {
                        buf[i] = cargf(buf[i]);
                    }
                    break;
                }
            }
            break;
        case kOCNumberComplex128Type:
            switch (part) {
                case kSIRealPart:
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
                    break;
                case kSIImaginaryPart:
                    DependentVariableMultiplyValuesByDimensionlessComplexConstant(dv, componentIndex, -I);
                    DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
                    break;
                case kSIMagnitudePart:
                    DependentVariableTakeAbsoluteValue(dv, componentIndex);
                    break;
                case kSIArgumentPart: {
                    OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, componentIndex);
                    double complex *buf = (double complex *)OCDataGetMutableBytes(data);
                    for (OCIndex i = 0; i < size; ++i) {
                        buf[i] = carg(buf[i]);
                    }
                    break;
                }
            }
            break;
        default:
            // integer or unsupported types
            return false;
    }
    if (componentIndex < 0) {
        if (dv->numericType == kOCNumberComplex64Type) {
            DependentVariableSetElementType(dv, kOCNumberFloat32Type);
        } else if (dv->numericType == kOCNumberComplex128Type) {
            DependentVariableSetElementType(dv, kOCNumberFloat64Type);
        }
    }
    return true;
}
bool DependentVariableConjugate(DependentVariableRef dv,
                                OCIndex componentIndex) {
    if (!dv) return false;
    OCArrayRef comps = dv->components;
    OCIndex nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 || componentIndex >= nComps) {
        return false;
    }
    size_t totalSize = DependentVariableGetSize(dv);
    OCIndex lo = componentIndex >= 0 ? componentIndex : 0;
    OCIndex hi = componentIndex >= 0 ? componentIndex + 1 : nComps;
    for (OCIndex ci = lo; ci < hi; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        switch (dv->numericType) {
            case kOCNumberFloat32Type:
            case kOCNumberFloat64Type:
                // real‐only, nothing to do
                break;
            case kOCNumberComplex64Type: {
                // interleaved [r0,i0, r1,i1, …]
                float complex *buf = (float complex *)OCDataGetMutableBytes(data);
                float *imag = ((float *)buf) + 1;
                // negate each imaginary entry:
                //      imag[j] = -imag[j],  j=0..totalSize-1, stride=2
                cblas_sscal((int)totalSize, -1.0f, imag, 2);
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *buf = (double complex *)OCDataGetMutableBytes(data);
                double *imag = ((double *)buf) + 1;
                cblas_dscal((int)totalSize, -1.0, imag, 2);
                break;
            }
            default:
                return false;  // integers & others not supported
        }
    }
    return true;
}
bool DependentVariableMultiplyValuesByDimensionlessRealConstant(DependentVariableRef dv,
                                                                OCIndex componentIndex,
                                                                double constant) {
    if (!dv) return false;
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 ||
        (componentIndex >= 0 && (uint64_t)componentIndex >= nComps)) {
        return false;
    }
    uint64_t lower = (componentIndex >= 0 ? componentIndex : 0);
    uint64_t upper = (componentIndex >= 0 ? lower + 1 : nComps);
    uint64_t size = DependentVariableGetSize(dv);
    OCNumberType type = DependentVariableGetElementType(dv);
    /* prepare BLAS scalars */
    float alpha_f = (float)constant;
    double alpha_d = constant;
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        void *bytes = OCDataGetMutableBytes(data);
        switch (type) {
            /* 8/16/32/64-bit signed ints */
            case kOCNumberSInt8Type: {
                int8_t *arr = (int8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt16Type: {
                int16_t *arr = (int16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt32Type: {
                int32_t *arr = (int32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt64Type: {
                int64_t *arr = (int64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int64_t)(arr[i] * constant);
                break;
            }
            /* 8/16/32/64-bit unsigned ints */
            case kOCNumberUInt8Type: {
                uint8_t *arr = (uint8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt16Type: {
                uint16_t *arr = (uint16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt32Type: {
                uint32_t *arr = (uint32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt64Type: {
                uint64_t *arr = (uint64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint64_t)(arr[i] * constant);
                break;
            }
            /* 32-bit float */
            case kOCNumberFloat32Type:
                cblas_sscal((int)size, alpha_f, (float *)bytes, 1);
                break;
            /* 64-bit float */
            case kOCNumberFloat64Type:
                cblas_dscal((int)size, alpha_d, (double *)bytes, 1);
                break;
            /* single-precision complex: scale by real constant */
            case kOCNumberComplex64Type:
                cblas_csscal((int)size, alpha_f, (void *)bytes, 1);
                break;
            /* double-precision complex: scale by real constant */
            case kOCNumberComplex128Type:
                cblas_zdscal((int)size, alpha_d, (void *)bytes, 1);
                break;
            default:
                /* unsupported type */
                return false;
        }
    }
    return true;
}
#pragma endregion Conversion and Manipulation
// Enumeration for arithmetic operations
typedef enum {
    kArithmeticAdd,
    kArithmeticSubtract,
    kArithmeticMultiply,
    kArithmeticDivide
} ArithmeticOperation;
// Unified helper function for arithmetic operations with potential type conversion
// Helpers to load any element as a double:
// Load the real part of element i as a double, from any type
static inline double load_double(const void *src, OCNumberType t, OCIndex i) {
    switch (t) {
        case kOCNumberSInt8Type:
            return (double)((const int8_t *)src)[i];
        case kOCNumberSInt16Type:
            return (double)((const int16_t *)src)[i];
        case kOCNumberSInt32Type:
            return (double)((const int32_t *)src)[i];
        case kOCNumberSInt64Type:
            return (double)((const int64_t *)src)[i];
        case kOCNumberUInt8Type:
            return (double)((const uint8_t *)src)[i];
        case kOCNumberUInt16Type:
            return (double)((const uint16_t *)src)[i];
        case kOCNumberUInt32Type:
            return (double)((const uint32_t *)src)[i];
        case kOCNumberUInt64Type:
            return (double)((const uint64_t *)src)[i];
        case kOCNumberFloat32Type:
            return (double)((const float *)src)[i];
        case kOCNumberFloat64Type:
            return ((const double *)src)[i];
        case kOCNumberComplex64Type:
            return (double)crealf(((const float complex *)src)[i]);
        case kOCNumberComplex128Type:
            return creal(((const double complex *)src)[i]);
    }
    return 0.0;  // unreachable
}
// Store a double back into element i of any type (real or complex real part)
static inline void store_double(
    void *dst, OCNumberType t, OCIndex i, double v) {
    switch (t) {
        case kOCNumberSInt8Type:
            ((int8_t *)dst)[i] = (int8_t)v;
            break;
        case kOCNumberSInt16Type:
            ((int16_t *)dst)[i] = (int16_t)v;
            break;
        case kOCNumberSInt32Type:
            ((int32_t *)dst)[i] = (int32_t)v;
            break;
        case kOCNumberSInt64Type:
            ((int64_t *)dst)[i] = (int64_t)v;
            break;
        case kOCNumberUInt8Type:
            ((uint8_t *)dst)[i] = (uint8_t)v;
            break;
        case kOCNumberUInt16Type:
            ((uint16_t *)dst)[i] = (uint16_t)v;
            break;
        case kOCNumberUInt32Type:
            ((uint32_t *)dst)[i] = (uint32_t)v;
            break;
        case kOCNumberUInt64Type:
            ((uint64_t *)dst)[i] = (uint64_t)v;
            break;
        case kOCNumberFloat32Type:
            ((float *)dst)[i] = (float)v;
            break;
        case kOCNumberFloat64Type:
            ((double *)dst)[i] = v;
            break;
        case kOCNumberComplex64Type:
            ((float complex *)dst)[i] = (float)v + 0.0f * I;
            break;
        case kOCNumberComplex128Type:
            ((double complex *)dst)[i] = v + 0.0 * I;
            break;
    }
}
static bool perform_arithmetic_with_conversion(
    void *dest,
    OCNumberType dest_type,
    const void *src,
    OCNumberType src_type,
    OCIndex size,
    ArithmeticOperation op,
    double u)  // unit_multiplier
{
    // 1) SAME-TYPE fast paths
    if (dest_type == src_type) {
        switch (dest_type) {
            case kOCNumberFloat32Type: {
                float *restrict d = (float *restrict)dest;
                const float *restrict s = (const float *restrict)src;
                const float f = (float)u;
                if (op == kArithmeticAdd) {
                    cblas_saxpy((int)size, 1.0f, s, 1, d, 1);
                } else if (op == kArithmeticSubtract) {
                    cblas_saxpy((int)size, -1.0f, s, 1, d, 1);
                } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] = fmaf(d[i], s[i], 0.0f) * f;
                } else {  // Divide
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= f / s[i];
                }
                return true;
            }
            case kOCNumberFloat64Type: {
                double *restrict d = (double *restrict)dest;
                const double *restrict s = (const double *restrict)src;
                const double f = u;
                if (op == kArithmeticAdd) {
                    cblas_daxpy((int)size, 1.0, s, 1, d, 1);
                } else if (op == kArithmeticSubtract) {
                    cblas_daxpy((int)size, -1.0, s, 1, d, 1);
                } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] = fma(d[i], s[i], 0.0) * f;
                } else {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= f / s[i];
                }
                return true;
            }
            case kOCNumberComplex64Type: {
                float complex *restrict d = (float complex *restrict)dest;
                const float complex *restrict s = (const float complex *restrict)src;
                const float complex f = (float)u + 0.0f * I;
                if (op == kArithmeticAdd) {
                    float complex α = 1.0f + 0.0f * I;
                    cblas_caxpy((int)size, &α, s, 1, d, 1);
                } else if (op == kArithmeticSubtract) {
                    float complex α = -1.0f + 0.0f * I;
                    cblas_caxpy((int)size, &α, s, 1, d, 1);
                } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= s[i] * f;
                } else {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= f / s[i];
                }
                return true;
            }
            case kOCNumberComplex128Type: {
                double complex *restrict d = (double complex *restrict)dest;
                const double complex *restrict s = (const double complex *restrict)src;
                const double complex f = u + 0.0 * I;
                if (op == kArithmeticAdd) {
                    double complex α = 1.0 + 0.0 * I;
                    cblas_zaxpy((int)size, &α, s, 1, d, 1);
                } else if (op == kArithmeticSubtract) {
                    double complex α = -1.0 + 0.0 * I;
                    cblas_zaxpy((int)size, &α, s, 1, d, 1);
                } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= s[i] * f;
                } else {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
                    for (OCIndex i = 0; i < size; i++)
                        d[i] *= f / s[i];
                }
                return true;
            }
            default:
                break;
        }
    }
    // 2) Fast float32↔float64 cross-paths
    if (dest_type == kOCNumberFloat64Type && src_type == kOCNumberFloat32Type) {
        double *restrict d = (double *restrict)dest;
        const float *restrict s = (const float *restrict)src;
        const double f = u;
        if (op == kArithmeticAdd) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++) d[i] += (double)s[i];
        } else if (op == kArithmeticSubtract) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++) d[i] -= (double)s[i];
        } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++)
                d[i] = fma(d[i], (double)s[i], 0.0) * f;
        } else {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++)
                d[i] *= f / (double)s[i];
        }
        return true;
    }
    if (dest_type == kOCNumberFloat32Type && src_type == kOCNumberFloat64Type) {
        float *restrict d = (float *restrict)dest;
        const double *restrict s = (const double *restrict)src;
        const float f = (float)u;
        if (op == kArithmeticAdd) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++) d[i] += (float)s[i];
        } else if (op == kArithmeticSubtract) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++) d[i] -= (float)s[i];
        } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++)
                d[i] = fmaf(d[i], (float)s[i], 0.0f) * f;
        } else {
#if HAVE_OPENMP
#pragma omp parallel for simd aligned(d, s : 32)
#endif
            for (OCIndex i = 0; i < size; i++)
                d[i] *= f / (float)s[i];
        }
        return true;
    }
    // 3) GENERIC fallback for every other type combination
    if (op == kArithmeticAdd) {
#if HAVE_OPENMP
#pragma omp parallel for simd
#endif
        for (OCIndex i = 0; i < size; i++) {
            double dv = load_double(dest, dest_type, i);
            double sv = load_double(src, src_type, i);
            store_double(dest, dest_type, i, dv + sv);
        }
    } else if (op == kArithmeticSubtract) {
#if HAVE_OPENMP
#pragma omp parallel for simd
#endif
        for (OCIndex i = 0; i < size; i++) {
            double dv = load_double(dest, dest_type, i);
            double sv = load_double(src, src_type, i);
            store_double(dest, dest_type, i, dv - sv);
        }
    } else if (op == kArithmeticMultiply) {
#if HAVE_OPENMP
#pragma omp parallel for simd
#endif
        for (OCIndex i = 0; i < size; i++) {
            double dv = load_double(dest, dest_type, i);
            double sv = load_double(src, src_type, i);
            store_double(dest, dest_type, i, dv * (sv * u));
        }
    } else {  // kArithmeticDivide
#if HAVE_OPENMP
#pragma omp parallel for simd
#endif
        for (OCIndex i = 0; i < size; i++) {
            double dv = load_double(dest, dest_type, i);
            double sv = load_double(src, src_type, i);
            store_double(dest, dest_type, i, dv * (u / sv));
        }
    }
    return true;
}
bool DependentVariableAdd(DependentVariableRef input1,
                          DependentVariableRef input2,
                          OCStringRef *error) {
    if (error && *error) return false;
    if (!input1 || !input2) return false;
    OCIndex componentsCount1 = OCArrayGetCount(input1->components);
    OCIndex componentsCount2 = OCArrayGetCount(input2->components);
    if (componentsCount1 == 0 || componentsCount2 == 0) return false;
    if (componentsCount1 != componentsCount2 && componentsCount1 != 1) return false;
    // Handle self-addition case (input1 + input1 = 2*input1)
    if (input1 == input2) {
        DependentVariableMultiplyValuesByDimensionlessRealConstant(input1, -1, 2.);
        return true;
    }
    // Perform validation checks once, outside the loop
    OCIndex size = DependentVariableGetSize(input1);
    if (size != DependentVariableGetSize(input2)) {
        if (error) {
            *error = STR("Incompatible component sizes for addition.");
        }
        return false;
    }
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)input1, (SIQuantityRef)input2)) {
        if (error) {
            *error = STR("DV Add, Incompatible Dimensionalities.");
        }
        return false;
    }
    // Process each component using unified helper functions
    for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
        OCMutableDataRef input1Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input1->components, componentIndex);
        OCMutableDataRef input2Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input2->components, componentIndex * (componentsCount2 != 1));
        void *dest = OCDataGetMutableBytes(input1Values);
        const void *src = OCDataGetMutableBytes(input2Values);
        // Perform arithmetic with conversion
        perform_arithmetic_with_conversion(dest, input1->numericType, src, input2->numericType, size, kArithmeticAdd, 1.0);
    }
    return true;
}
bool DependentVariableSubtract(DependentVariableRef input1,
                               DependentVariableRef input2,
                               OCStringRef *error) {
    if (error && *error) return false;
    if (!input1 || !input2) return false;
    OCIndex componentsCount1 = OCArrayGetCount(input1->components);
    OCIndex componentsCount2 = OCArrayGetCount(input2->components);
    if (componentsCount1 == 0 || componentsCount2 == 0) return false;
    if (componentsCount1 != componentsCount2 && componentsCount1 != 1) return false;
    // Handle self-subtraction case (input1 - input1 = 0)
    if (input1 == input2) {
        DependentVariableSetValuesToZero(input1, -1);
        return true;
    }
    // Perform validation checks once, outside the loop
    OCIndex size = DependentVariableGetSize(input1);
    if (size != DependentVariableGetSize(input2)) {
        if (error) {
            *error = STR("Incompatible component sizes for subtraction.");
        }
        return false;
    }
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)input1, (SIQuantityRef)input2)) {
        if (error) {
            *error = STR("DV Sub, Incompatible Dimensionalities.");
        }
        return false;
    }
    // Process each component using unified helper functions
    for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
        OCMutableDataRef input1Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input1->components, componentIndex);
        OCMutableDataRef input2Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input2->components, componentIndex * (componentsCount2 != 1));
        void *dest = OCDataGetMutableBytes(input1Values);
        const void *src = OCDataGetMutableBytes(input2Values);
        // Perform arithmetic with conversion
        perform_arithmetic_with_conversion(dest, input1->numericType, src, input2->numericType, size, kArithmeticSubtract, 1.0);
    }
    return true;
}
bool DependentVariableMultiply(DependentVariableRef input1,
                               DependentVariableRef input2,
                               OCStringRef *error) {
    if (error && *error) return false;
    if (!input1 || !input2) return false;
    OCIndex componentsCount1 = OCArrayGetCount(input1->components);
    OCIndex componentsCount2 = OCArrayGetCount(input2->components);
    if (componentsCount1 == 0 || componentsCount2 == 0) return false;
    if (componentsCount1 != componentsCount2 && componentsCount1 != 1) return false;
    // Handle self-multiplication case (input1 * input1 = input1^2)
    if (input1 == input2) {
        // For now, handle as regular multiplication - could be optimized further
        // TODO: Implement dedicated squaring operation for better performance
    }
    // Perform validation checks once, outside the loop
    OCIndex size = DependentVariableGetSize(input1);
    if (size != DependentVariableGetSize(input2)) {
        if (error) {
            *error = STR("Incompatible component sizes for multiplication.");
        }
        return false;
    }
    // Handle unit multiplication
    double unit_multiplier = 1.0;
    SIUnitRef newUnit = SIUnitByMultiplying(SIQuantityGetUnit((SIQuantityRef)input1),
                                            SIQuantityGetUnit((SIQuantityRef)input2),
                                            &unit_multiplier, error);
    if (!newUnit) return false;
    SIQuantitySetUnit((SIMutableQuantityRef)input1, newUnit);
    // Process each component using unified helper functions
    for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
        OCMutableDataRef input1Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input1->components, componentIndex);
        OCMutableDataRef input2Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input2->components, componentIndex * (componentsCount2 != 1));
        void *dest = OCDataGetMutableBytes(input1Values);
        const void *src = OCDataGetBytesPtr(input2Values);
        // Perform arithmetic with conversion
        perform_arithmetic_with_conversion(dest, input1->numericType, src, input2->numericType, size, kArithmeticMultiply, unit_multiplier);
    }
    return true;
}
bool DependentVariableDivide(DependentVariableRef input1,
                             DependentVariableRef input2,
                             OCStringRef *error) {
    if (error && *error) return false;
    if (!input1 || !input2) return false;
    OCIndex componentsCount1 = OCArrayGetCount(input1->components);
    OCIndex componentsCount2 = OCArrayGetCount(input2->components);
    if (componentsCount1 == 0 || componentsCount2 == 0) return false;
    if (componentsCount1 != componentsCount2 && componentsCount1 != 1) return false;
    // Handle self-division case (input1 / input1 = 1, assuming same units)
    if (input1 == input2) {
        // Set to dimensionless unit
        SIUnitRef dimensionlessUnit = SIUnitDimensionlessAndUnderived();
        if (dimensionlessUnit) {
            SIQuantitySetUnit((SIMutableQuantityRef)input1, dimensionlessUnit);
        }
        // Set all values to 1.0 (dimensionless)
        DependentVariableSetValuesToZero(input1, -1);
        for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
            OCMutableDataRef input1Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input1->components, componentIndex);
            OCIndex size = DependentVariableGetSize(input1);
            void *bytes = OCDataGetMutableBytes(input1Values);
            switch (input1->numericType) {
                case kOCNumberFloat32Type: {
                    float *data = (float *)bytes;
                    for (OCIndex i = 0; i < size; i++) data[i] = 1.0f;
                    break;
                }
                case kOCNumberFloat64Type: {
                    double *data = (double *)bytes;
                    for (OCIndex i = 0; i < size; i++) data[i] = 1.0;
                    break;
                }
                case kOCNumberComplex64Type: {
                    float complex *data = (float complex *)bytes;
                    for (OCIndex i = 0; i < size; i++) data[i] = 1.0f + 0.0f * I;
                    break;
                }
                case kOCNumberComplex128Type: {
                    double complex *data = (double complex *)bytes;
                    for (OCIndex i = 0; i < size; i++) data[i] = 1.0 + 0.0 * I;
                    break;
                }
                default:
                    break;
            }
        }
        return true;
    }
    // Perform validation checks once, outside the loop
    OCIndex size = DependentVariableGetSize(input1);
    if (size != DependentVariableGetSize(input2)) {
        if (error) {
            *error = STR("Incompatible component sizes for division.");
        }
        return false;
    }
    // Handle unit division
    double unit_multiplier = 1.0;
    SIUnitRef newUnit = SIUnitByDividing(SIQuantityGetUnit((SIQuantityRef)input1),
                                         SIQuantityGetUnit((SIQuantityRef)input2),
                                         &unit_multiplier, error);
    if (!newUnit) return false;
    SIQuantitySetUnit((SIMutableQuantityRef)input1, newUnit);
    // Process each component using unified helper functions
    for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
        OCMutableDataRef input1Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input1->components, componentIndex);
        OCMutableDataRef input2Values = (OCMutableDataRef)OCArrayGetValueAtIndex(input2->components, componentIndex * (componentsCount2 != 1));
        void *dest = OCDataGetMutableBytes(input1Values);
        const void *src = OCDataGetBytesPtr(input2Values);
        // Perform arithmetic with conversion
        perform_arithmetic_with_conversion(dest, input1->numericType, src, input2->numericType, size, kArithmeticDivide, unit_multiplier);
    }
    return true;
}
bool DependentVariableCombineMagnitudeWithArgument(DependentVariableRef magnitude,
                                                   DependentVariableRef argument) {
    if (!magnitude || !argument) return false;
    OCIndex componentsCount1 = OCArrayGetCount(magnitude->components);
    OCIndex componentsCount2 = OCArrayGetCount(argument->components);
    if (componentsCount1 == 0 || componentsCount2 == 0) return false;
    if (componentsCount1 != componentsCount2 && componentsCount1 != 1) return false;
    if (SIQuantityIsComplexType((SIQuantityRef)magnitude) || SIQuantityIsComplexType((SIQuantityRef)argument)) return false;
    // Determine the best complex type to use
    OCNumberType finalType;
    if (magnitude->numericType == kOCNumberFloat64Type || argument->numericType == kOCNumberFloat64Type) {
        finalType = kOCNumberComplex128Type;
    } else {
        finalType = kOCNumberComplex64Type;
    }
    DependentVariableSetElementType(magnitude, finalType);
    OCIndex size = DependentVariableGetSize(magnitude);
    for (OCIndex componentIndex = 0; componentIndex < componentsCount1; componentIndex++) {
        OCMutableDataRef resultValues = (OCMutableDataRef)OCArrayGetValueAtIndex(magnitude->components, componentIndex);
        OCMutableDataRef argumentValues = (OCMutableDataRef)OCArrayGetValueAtIndex(argument->components, componentIndex * (componentsCount2 != 1));
        if (argument->numericType == kOCNumberFloat32Type) {
            float *phase = (float *)OCDataGetBytesPtr(argumentValues);
            if (finalType == kOCNumberComplex64Type) {
                float complex *bytes = (float complex *)OCDataGetMutableBytes(resultValues);
                for (OCIndex memOffset = 0; memOffset < size; memOffset++) {
                    bytes[memOffset] = bytes[memOffset] * (cosf(phase[memOffset]) + I * sinf(phase[memOffset]));
                }
            } else {
                double complex *bytes = (double complex *)OCDataGetMutableBytes(resultValues);
                for (OCIndex memOffset = 0; memOffset < size; memOffset++) {
                    bytes[memOffset] = bytes[memOffset] * (cosf(phase[memOffset]) + I * sinf(phase[memOffset]));
                }
            }
        } else {
            double *phase = (double *)OCDataGetBytesPtr(argumentValues);
            if (finalType == kOCNumberComplex64Type) {
                float complex *bytes = (float complex *)OCDataGetMutableBytes(resultValues);
                for (OCIndex memOffset = 0; memOffset < size; memOffset++) {
                    bytes[memOffset] = bytes[memOffset] * ((float)cos(phase[memOffset]) + I * (float)sin(phase[memOffset]));
                }
            } else {
                double complex *bytes = (double complex *)OCDataGetMutableBytes(resultValues);
                for (OCIndex memOffset = 0; memOffset < size; memOffset++) {
                    bytes[memOffset] = bytes[memOffset] * (cos(phase[memOffset]) + I * sin(phase[memOffset]));
                }
            }
        }
    }
    return true;
}
bool DependentVariableAppend(DependentVariableRef dv, DependentVariableRef appendedDV, OCStringRef *outError) {
    // 0) if caller already has an error, bail
    if (outError && *outError) return false;
    // 1) sanity
    if (!dv || !appendedDV) return false;
    // 2) must have matching physical dimensionality
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)dv,
                                                (SIQuantityRef)appendedDV)) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Append Error: Incompatible dimensionalities."));
        }
        return false;
    }
    // 3) must have the same element type
    OCNumberType et1 = DependentVariableGetElementType(dv);
    OCNumberType et2 = DependentVariableGetElementType(appendedDV);
    if (et1 != et2) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Append Error: Incompatible numeric types."));
        }
        return false;
    }
    // 4) component‐count checks
    OCIndex n1 = DependentVariableGetComponentCount(dv);
    OCIndex n2 = DependentVariableGetComponentCount(appendedDV);
    if (n1 == 0 || n2 == 0) return false;
    // allow either exactly matching or a single “broadcast” component on dv
    if (n1 != n2 && n1 != 1) return false;
    // 5) for each component, append raw bytes
    for (OCIndex ci = 0; ci < n1; ++ci) {
        // dest is always mutable
        OCMutableDataRef dest = (OCMutableDataRef)
            DependentVariableGetComponentAtIndex(dv, ci);
        // pick matching source, or 0 if appendedDV has exactly one component
        OCIndex srcIdx = (n2 != 1 ? ci : 0);
        OCDataRef src = DependentVariableGetComponentAtIndex(appendedDV, srcIdx);
        OCDataAppendBytes(
            dest,
            OCDataGetBytesPtr(src),
            OCDataGetLength(src));
    }
    return true;
}
