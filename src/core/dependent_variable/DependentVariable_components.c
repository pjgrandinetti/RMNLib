#include "DependentVariable.h"
#include "DependentVariable_private.h"
OCIndex DependentVariableGetComponentCount(DependentVariableRef dv) {
    if (!dv) return 0;
    return OCArrayGetCount(dv->components);
}
OCMutableArrayRef DependentVariableGetComponents(DependentVariableRef dv) {
    return dv ? dv->components : NULL;
}
OCArrayRef DependentVariableGetComponentLabels(DependentVariableRef dv) {
    if (!dv) return NULL;
    return (OCArrayRef)dv->componentLabels;
}
bool DependentVariableSetComponentLabels(DependentVariableRef dv, OCArrayRef labels) {
    if (!dv) return false;
    // release old labels
    OCRelease(dv->componentLabels);
    if (labels) {
        // deep‐mutable copy of the incoming array
        dv->componentLabels = OCArrayCreateMutableCopy(labels);
    } else {
        // no labels → empty mutable array
        dv->componentLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    }
    return dv->componentLabels != NULL;
}
OCStringRef DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv) return NULL;
    OCStringRef name = dv->name;
    OCArrayRef labels = (OCArrayRef)dv->componentLabels;
    OCStringRef componentName = NULL;
    if (labels && componentIndex >= 0 && componentIndex < OCArrayGetCount(labels)) {
        componentName = OCArrayGetValueAtIndex(labels, componentIndex);
    }
    // If we have both a non‐empty name and a componentName, join them:
    if (name && OCStringGetLength(name) > 0 && componentName) {
        return OCStringCreateWithFormat(
            STR("%@ : %@"),
            name,
            componentName);
    }
    // Otherwise just copy the component label (or fall back to empty string).
    if (componentName) {
        return OCStringCreateCopy(componentName);
    } else {
        return STR("");
    }
}
OCStringRef DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv) return NULL;
    OCArrayRef labels = (OCArrayRef)dv->componentLabels;
    if (!labels ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(labels)) {
        return NULL;
    }
    return OCArrayGetValueAtIndex(labels, componentIndex);
}
bool DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv, OCStringRef newLabel, OCIndex componentIndex) {
    if (!dv || !newLabel) return false;
    OCArrayRef comps = (OCArrayRef)dv->components;
    OCMutableArrayRef labels = dv->componentLabels;
    OCIndex count = comps ? OCArrayGetCount(comps) : 0;
    if (componentIndex < 0 || componentIndex >= count) {
        return false;
    }
    // Just overwrite the slot; OCArray uses retain/release automatically.
    OCArraySetValueAtIndex(labels, componentIndex, newLabel);
    return true;
}
bool DependentVariableSetComponents(DependentVariableRef dv, OCArrayRef newComponents) {
    if (!dv || !newComponents) return false;
    OCIndex count = OCArrayGetCount(newComponents);
    if (count == 0) return false;
    // Validate each component is OCDataRef and has matching size
    uint64_t expectedLength = 0;
    for (OCIndex i = 0; i < count; ++i) {
        OCTypeRef obj = OCArrayGetValueAtIndex(newComponents, i);
        if (OCGetTypeID(obj) != OCDataGetTypeID()) {
            return false;
        }
        OCDataRef data = (OCDataRef)obj;
        uint64_t len = OCDataGetLength(data);
        if (i == 0) {
            expectedLength = len;
        } else if (len != expectedLength) {
            return false;  // mismatched component sizes
        }
    }
    // Install the new components (retain or copy, depending on usage needs)
    OCMutableArrayRef newArray = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < count; ++i) {
        OCArrayAppendValue(newArray, OCArrayGetValueAtIndex(newComponents, i));
    }
    OCRelease(dv->components);
    dv->components = newArray;
    // Rebuild or adjust component labels
    OCRelease(dv->componentLabels);
    dv->componentLabels = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < count; ++i) {
        OCStringRef lbl = OCStringCreateWithFormat(STR("component-%ld"), (long)i);
        OCArrayAppendValue(dv->componentLabels, lbl);
        OCRelease(lbl);
    }
    // Update quantityType if necessary
    OCRelease(dv->quantityType);
    if (count == 1) {
        dv->quantityType = STR("scalar");
    } else {
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    }
    return true;
}
OCMutableArrayRef DependentVariableCopyComponents(DependentVariableRef dv) {
    if (!dv || !dv->components) return NULL;
    OCIndex n = OCArrayGetCount(dv->components);
    OCMutableArrayRef copy =
        OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < n; i++) {
        OCDataRef buf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, i);
        // Deep‐copy the data buffer:
        OCDataRef bufCopy = (OCDataRef)OCTypeDeepCopyMutable(buf);
        if (bufCopy) {
            OCArrayAppendValue(copy, bufCopy);
            OCRelease(bufCopy);
        }
    }
    return copy;
}
OCDataRef DependentVariableGetComponentAtIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv || !dv->components ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(dv->components))
        return NULL;
    return (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
}
bool DependentVariableSetComponentAtIndex(DependentVariableRef dv, OCDataRef newBuf, OCIndex componentIndex) {
    if (!dv || !dv->components || !newBuf) return false;
    OCIndex n = OCArrayGetCount(dv->components);
    if (componentIndex < 0 || componentIndex >= n) return false;
    OCDataRef oldBuf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    if (OCDataGetLength(newBuf) != OCDataGetLength(oldBuf)) return false;
    OCArraySetValueAtIndex(dv->components, componentIndex, newBuf);
    return true;
}
static void updateForComponentCountChange(DependentVariableRef dv) {
    OCIndex count = OCArrayGetCount(dv->components);
    const char *qt = OCStringGetCString(dv->quantityType);
    // "scalar" stays scalar
    if (strncmp(qt, "vector", 6) == 0) {
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    } else if (strncmp(qt, "pixel", 5) == 0) {
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("pixel_%ld"), (long)count);
    } else if (strncmp(qt, "matrix", 6) == 0 ||
               strncmp(qt, "symmetric_matrix", 16) == 0) {
        // PS fell back to vector for nonuniform grids
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    }
}
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv, OCDataRef component, OCIndex idx) {
    if (!dv || !component) return false;
    OCMutableArrayRef comps = dv->components;
    if (!comps) return false;
    OCIndex count = OCArrayGetCount(comps);
    // valid positions are 0…count
    if (idx < 0 || idx > count) return false;
    // if there's already at least one component, enforce matching byte‐length
    if (count > 0) {
        OCDataRef first = (OCDataRef)OCArrayGetValueAtIndex(comps, 0);
        if (OCDataGetLength(component) != OCDataGetLength(first)) {
            return false;
        }
    }
    OCArrayInsertValueAtIndex(comps, idx, component);
    updateForComponentCountChange(dv);
    return true;
}
bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv, OCIndex idx) {
    if (!dv) return false;
    OCIndex count = OCArrayGetCount(dv->components);
    if (idx >= count || count <= 1)
        return false;
    OCArrayRemoveValueAtIndex(dv->components, idx);
    OCArrayRemoveValueAtIndex(dv->componentLabels, idx);
    updateForComponentCountChange(dv);
    return true;
}
OCIndex DependentVariableGetSize(DependentVariableRef dv) {
    if (!dv) return 0;
    OCIndex componentsCount = OCArrayGetCount(dv->components);
    if (componentsCount == 0) return 0;
    OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(dv->components, 0);
    size_t byteLength = OCDataGetLength(blob);
    // element size in bytes for this variable’s unit/quantity
    size_t eltSize = SIQuantityElementSize((SIQuantityRef)dv);
    if (eltSize == 0) return 0;
    return (OCIndex)(byteLength / eltSize);
}
bool DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize) {
    if (!dv) return false;
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0) return false;
    OCIndex oldSize = DependentVariableGetSize(dv);
    // pick up byte‐size per element
    size_t elemSize;
    switch (dv->numericType) {
        // 8-bit integers
        case kOCNumberSInt8Type:
        case kOCNumberUInt8Type:
            elemSize = 1;
            break;
        // 16-bit integers
        case kOCNumberSInt16Type:
        case kOCNumberUInt16Type:
            elemSize = 2;
            break;
        // 32-bit integers
        case kOCNumberSInt32Type:
        case kOCNumberUInt32Type:
            elemSize = 4;
            break;
        // 64-bit integers
        case kOCNumberSInt64Type:
        case kOCNumberUInt64Type:
            elemSize = 8;
            break;
        // IEEE floats
        case kOCNumberFloat32Type:
            elemSize = sizeof(float);
            break;
        case kOCNumberFloat64Type:
            elemSize = sizeof(double);
            break;
        // Complex (real+imaginary)
        case kOCNumberComplex64Type:
            elemSize = 2 * sizeof(float);
            break;
        case kOCNumberComplex128Type:
            elemSize = 2 * sizeof(double);
            break;
        default:
            // Should never happen if numericType is valid
            elemSize = 0;
            break;
    }
    OCIndex newByteLen = newSize * elemSize;
    // if shrinking, just cut each buffer down
    if (newSize < oldSize) {
        for (OCIndex i = 0; i < nComps; i++) {
            OCMutableDataRef buf =
                (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, i);
            OCDataSetLength(buf, newByteLen);
        }
        return true;
    }
    // else we must grow: copy-and-replace each buffer, then zero its tail
    for (OCIndex i = 0; i < nComps; i++) {
        OCDataRef oldBuf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, i);
        // deep-mutable copy
        OCMutableDataRef newBuf =
            (OCMutableDataRef)OCTypeDeepCopyMutable(oldBuf);
        if (!newBuf) return false;
        // resize to the new byte length
        OCDataSetLength(newBuf, newByteLen);
        // install it
        OCArraySetValueAtIndex(dv->components, i, newBuf);
        // zero out the newly-appended region
        uint8_t *bytes = (uint8_t *)OCDataGetMutableBytes(newBuf);
        size_t offset = (size_t)oldSize * elemSize;
        size_t count = (size_t)(newSize - oldSize) * elemSize;
        memset(bytes + offset, 0, count);
        OCRelease(newBuf);
    }
    return true;
}
OCNumberType DependentVariableGetElementType(DependentVariableRef dv) {
    if (!dv) return kOCNumberTypeInvalid;
    return dv->numericType;
}
bool DependentVariableSetElementType(DependentVariableRef dv, OCNumberType newType) {
    if (!dv) return false;
    OCNumberType oldType = dv->numericType;
    if (oldType == newType) return true;
    OCMutableArrayRef comps = dv->components;
    if (!comps) return false;
    OCIndex nComps = OCArrayGetCount(comps);
    OCIndex nElems = DependentVariableGetSize(dv);
    size_t newBytes = nElems * OCNumberTypeSize((OCNumberType)newType);
    for (OCIndex ci = 0; ci < nComps; ci++) {
        OCMutableDataRef oldData = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *oldPtr = OCDataGetMutableBytes(oldData);
        OCMutableDataRef newData = OCDataCreateMutable(0);
        void *tmpBuf = malloc(newBytes);
        // copy/convert element-by-element:
        for (OCIndex ei = 0; ei < nElems; ei++) {
            switch (oldType) {
                case kOCNumberFloat32Type: {
                    float *src = (float *)oldPtr;
                    float f = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = f;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberFloat64Type: {
                    double *src = (double *)oldPtr;
                    double d = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)d;
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = d;
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = (float)d;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = d;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberComplex64Type: {
                    float complex *src = (float complex *)oldPtr;
                    float complex z = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = z;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = z;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberComplex128Type: {
                    double complex *src = (double complex *)oldPtr;
                    double complex z = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)creal(z);
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = creal(z);
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = (float)creal(z) + (float)cimag(z) * I;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = z;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        // append and swap in
        OCDataAppendBytes(newData, tmpBuf, newBytes);
        free(tmpBuf);
        OCArraySetValueAtIndex(comps, ci, newData);
        OCRelease(newData);
    }
    dv->numericType = newType;
    return true;
}
bool DependentVariableSetValues(DependentVariableRef dv, OCIndex componentIndex, OCDataRef values) {
    // NULL‐check
    if (!dv) return false;
    // Bounds check
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0 || componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }
    // Ensure the new data blob is the same length as the existing components
    {
        OCDataRef first = (OCDataRef)OCArrayGetValueAtIndex(dv->components, 0);
        size_t oldLen = OCDataGetLength(first);
        size_t newLen = OCDataGetLength(values);
        if (oldLen != newLen) return false;
    }
    // If it’s literally the same object, nothing to do
    OCMutableDataRef oldValues = (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    if (oldValues == values) return true;
    // Otherwise make a mutable copy and swap it in
    OCIndex length = OCDataGetLength(values);
    OCMutableDataRef newValues = OCDataCreateMutableCopy(length, values);
    OCArraySetValueAtIndex(dv->components, componentIndex, newValues);
    OCRelease(newValues);
    return true;
}
// Enumeration for value return types
typedef enum {
    kValueReturnFloat,
    kValueReturnDouble,
    kValueReturnFloatComplex,
    kValueReturnDoubleComplex
} ValueReturnType;
// Unified value accessor function with type conversion and complex part handling
static void get_value_at_offset_unified(DependentVariableRef dv, OCIndex componentIndex,
                                        OCIndex memOffset, int part,
                                        ValueReturnType returnType, void *result) {
    // Common validation
    if (!dv || !result) {
        switch (returnType) {
            case kValueReturnFloat:
                *(float *)result = NAN;
                return;
            case kValueReturnDouble:
                *(double *)result = NAN;
                return;
            case kValueReturnFloatComplex:
                *(float complex *)result = NAN + NAN * I;
                return;
            case kValueReturnDoubleComplex:
                *(double complex *)result = NAN + NAN * I;
                return;
        }
        return;
    }
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 || componentIndex < 0 || componentIndex >= nComps) {
        switch (returnType) {
            case kValueReturnFloat:
                *(float *)result = NAN;
                return;
            case kValueReturnDouble:
                *(double *)result = NAN;
                return;
            case kValueReturnFloatComplex:
                *(float complex *)result = NAN + NAN * I;
                return;
            case kValueReturnDoubleComplex:
                *(double complex *)result = NAN + NAN * I;
                return;
        }
        return;
    }
    // Wrap negative or out-of-bounds offsets
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    // Extract values based on source type
    double realv = 0.0, imagv = 0.0;
    switch (dv->numericType) {
        case kOCNumberSInt8Type:
            realv = (double)((int8_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt8Type:
            realv = (double)((uint8_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt16Type:
            realv = (double)((int16_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt16Type:
            realv = (double)((uint16_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt32Type:
            realv = (double)((int32_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt32Type:
            realv = (double)((uint32_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt64Type:
            realv = (double)((int64_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt64Type:
            realv = (double)((uint64_t *)bytes)[memOffset];
            break;
        case kOCNumberFloat32Type:
            realv = (double)((float *)bytes)[memOffset];
            break;
        case kOCNumberFloat64Type:
            realv = ((double *)bytes)[memOffset];
            break;
        case kOCNumberComplex64Type: {
            float complex val = ((float complex *)bytes)[memOffset];
            realv = (double)crealf(val);
            imagv = (double)cimagf(val);
            break;
        }
        case kOCNumberComplex128Type: {
            double complex val = ((double complex *)bytes)[memOffset];
            realv = creal(val);
            imagv = cimag(val);
            break;
        }
        default:
            switch (returnType) {
                case kValueReturnFloat:
                    *(float *)result = NAN;
                    return;
                case kValueReturnDouble:
                    *(double *)result = NAN;
                    return;
                case kValueReturnFloatComplex:
                    *(float complex *)result = NAN + NAN * I;
                    return;
                case kValueReturnDoubleComplex:
                    *(double complex *)result = NAN + NAN * I;
                    return;
            }
            return;
    }
    // Apply complex part selection if needed
    double final_real = realv, final_imag = imagv;
    if (part >= 0) {  // Only apply part selection if a valid part is specified
        switch (part) {
            case kSIRealPart:
                final_real = realv;
                final_imag = 0.0;
                break;
            case kSIImaginaryPart:
                final_real = imagv;
                final_imag = 0.0;
                break;
            case kSIMagnitudePart:
                final_real = hypot(realv, imagv);
                final_imag = 0.0;
                break;
            case kSIArgumentPart:
                final_real = atan2(imagv, realv);
                final_imag = 0.0;
                break;
            default:
                // Invalid part - keep original values
                break;
        }
    } else {
        // No part specified - for complex return types, keep both real and imaginary
        // For real return types, just use the real part (which is the default behavior)
        if (returnType == kValueReturnFloat || returnType == kValueReturnDouble) {
            final_real = realv;  // Use real part for real return types
            final_imag = 0.0;
        }
        // For complex return types, keep both components as-is
    }
    // Convert to requested return type
    switch (returnType) {
        case kValueReturnFloat:
            *(float *)result = (float)final_real;
            break;
        case kValueReturnDouble:
            *(double *)result = final_real;
            break;
        case kValueReturnFloatComplex:
            *(float complex *)result = (float)final_real + (float)final_imag * I;
            break;
        case kValueReturnDoubleComplex:
            *(double complex *)result = final_real + final_imag * I;
            break;
    }
}
float DependentVariableGetFloatValueAtMemOffset(DependentVariableRef dv,
                                                OCIndex componentIndex,
                                                OCIndex memOffset) {
    float result;
    get_value_at_offset_unified(dv, componentIndex, memOffset, -1, kValueReturnFloat, &result);
    return result;
}
double DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv,
                                                  OCIndex componentIndex,
                                                  OCIndex memOffset) {
    double result;
    get_value_at_offset_unified(dv, componentIndex, memOffset, -1, kValueReturnDouble, &result);
    return result;
}
float complex DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    float complex result;
    get_value_at_offset_unified(dv, componentIndex, memOffset, -1, kValueReturnFloatComplex, &result);
    return result;
}
double complex DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    double complex result;
    get_value_at_offset_unified(dv, componentIndex, memOffset, -1, kValueReturnDoubleComplex, &result);
    return result;
}
double DependentVariableGetDoubleValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex componentIndex,
    OCIndex memOffset,
    complexPart part) {
    double result = NAN;
    get_value_at_offset_unified(dv, componentIndex, memOffset, part, kValueReturnDouble, &result);
    return result;
}
float DependentVariableGetFloatValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex componentIndex,
    OCIndex memOffset,
    complexPart part) {
    float result = NAN;
    get_value_at_offset_unified(dv, componentIndex, memOffset, part, kValueReturnFloat, &result);
    return result;
}
SIScalarRef DependentVariableCreateValueFromMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    if (!dv) return NULL;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NULL;
    }
    // wrap memOffset into [0..size)
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    // grab the raw bytes
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float v = ((float *)bytes)[memOffset];
            return SIScalarCreateWithFloat(v, dv->unit);
        }
        case kOCNumberFloat64Type: {
            double v = ((double *)bytes)[memOffset];
            return SIScalarCreateWithDouble(v, dv->unit);
        }
        case kOCNumberComplex64Type: {
            float complex v = ((float complex *)bytes)[memOffset];
            return SIScalarCreateWithFloatComplex(v, dv->unit);
        }
        case kOCNumberComplex128Type: {
            double complex v = ((double complex *)bytes)[memOffset];
            return SIScalarCreateWithDoubleComplex(v, dv->unit);
        }
        default:
            return NULL;
    }
}
bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset, SIScalarRef value, OCStringRef *error) {
    // if caller already set *error, bail
    if (error && *error) return false;
    if (!dv) return false;
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }
    // dimensionality check
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)dv,
                                                (SIQuantityRef)value)) {
        if (error) {
            *error = STR("DependentVariableSetValueAtMemOffset: Incompatible dimensionalities");
        }
        return false;
    }
    OCIndex size = DependentVariableGetSize(dv);
    if (size == 0) return false;
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    // grab a mutable pointer into the data
    OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    void *bytes = OCDataGetMutableBytes(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float v = SIScalarFloatValueInUnit(value, dv->unit, NULL);
            ((float *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberFloat64Type: {
            double v = SIScalarDoubleValueInUnit(value, dv->unit, NULL);
            ((double *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberComplex64Type: {
            float complex v = SIScalarFloatComplexValueInUnit(value, dv->unit, NULL);
            ((float complex *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberComplex128Type: {
            double complex v = SIScalarDoubleComplexValueInUnit(value, dv->unit, NULL);
            ((double complex *)bytes)[memOffset] = v;
            break;
        }
        default:
            return false;
    }
    return true;
}
