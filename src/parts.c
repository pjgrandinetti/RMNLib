#pragma region SIMonotonicDimension
// ============================================================================
// MARK: - (5) SIMonotonicDimension
// ============================================================================
static OCTypeID kSIMonotonicDimensionID = _kOCNotATypeID;
struct __SIMonotonicDimension {
    //  Dimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;
    //  SIDimension
    OCStringRef quantityName;
    SIScalarRef coordinatesOffset;
    SIScalarRef originOffset;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
    //  SIMonotonicDimension
    SIDimensionRef reciprocal;
    OCMutableArrayRef coordinates;
};
OCTypeID SIMonotonicDimensionGetTypeID(void) {
    if (kSIMonotonicDimensionID == _kOCNotATypeID)
        kSIMonotonicDimensionID = OCRegisterType("SIMonotonicDimension");
    return kSIMonotonicDimensionID;
}
static void __SIMonotonicDimensionFinalize(const void *obj) {
    SIMonotonicDimensionRef dim = (SIMonotonicDimensionRef)obj;
    __SIDimensionFinalize((struct __Dimension *)dim);
    OCRelease(dim->coordinates);
    dim->coordinates = NULL;
}
static OCStringRef __SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<SIMonotonicDimension>");
}
SIMonotonicDimensionRef SIMonotonicDimensionCreateCopy(SIMonotonicDimensionRef src);
static void *__SIMonotonicDimensionDeepCopy(const void *obj) {
    if (!obj)
        return NULL;
    return SIMonotonicDimensionCreateCopy((SIMonotonicDimensionRef)obj);
}
static SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void) {
    SIMonotonicDimensionRef obj = OCTypeAlloc(
        struct __SIMonotonicDimension,
        SIMonotonicDimensionGetTypeID(),
        __SIMonotonicDimensionFinalize,
        NULL,
        __SIMonotonicDimensionCopyFormattingDesc,
        __SIMonotonicDimensionDeepCopy,
        __SIMonotonicDimensionDeepCopy);
    __RMNInitBaseFields((DimensionRef)obj);
    __InitSIDimensionFields((SIDimensionRef)obj);
    obj->coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return obj;
}
SIMonotonicDimensionRef
SIMonotonicDimensionCreate(OCStringRef label, OCStringRef description, OCDictionaryRef metaData, OCArrayRef coordinates, OCStringRef quantityName, SIScalarRef originOffset, SIScalarRef coordinatesOffset, bool periodic, dimensionScaling scaling) {
    // 1) Validate coordinates array
    if (!coordinates) {
        fprintf(stderr, "SIMonotonicDimensionCreate: coordinates array is NULL\n");
        return NULL;
    }
    OCIndex count = OCArrayGetCount(coordinates);
    if (count < 2) {
        fprintf(stderr, "SIMonotonicDimensionCreate: need ≥2 coordinates\n");
        return NULL;
    }
    // 2) Check first coord is real, grab its unit & dimensionality
    SIScalarRef first = OCArrayGetValueAtIndex(coordinates, 0);
    if (SIQuantityIsComplexType((SIQuantityRef)first)) {
        fprintf(stderr, "SIMonotonicDimensionCreate: first coordinate is complex\n");
        return NULL;
    }
    SIDimensionalityRef dim0 = SIQuantityGetUnitDimensionality((SIQuantityRef)first);
    SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)first);
    // 3) Ensure all coords share the same reduced dimensionality & are real
    for (OCIndex i = 1; i < count; ++i) {
        SIScalarRef c = OCArrayGetValueAtIndex(coordinates, i);
        if (SIQuantityIsComplexType((SIQuantityRef)c)) {
            fprintf(stderr, "SIMonotonicDimensionCreate: coordinate %u is complex\n", (unsigned)i);
            return NULL;
        }
        if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)first, (SIQuantityRef)c)) {
            fprintf(stderr, "SIMonotonicDimensionCreate: coordinate %u mismatched dimensionality\n", (unsigned)i);
            return NULL;
        }
    }
    // 4) Validate or infer quantityName
    if (quantityName) {
        OCStringRef err = NULL;
        SIDimensionalityRef qdim = SIDimensionalityForQuantity(quantityName, &err);
        if (!qdim || !SIDimensionalityHasSameReducedDimensionality(qdim, dim0)) {
            fprintf(stderr, "SIMonotonicDimensionCreate: quantityName dimensionality mismatch\n");
            return NULL;
        }
    } else {
        quantityName = SIUnitGuessQuantityName(unit);
        if (!quantityName) {
            fprintf(stderr, "SIMonotonicDimensionCreate: cannot infer quantityName from unit\n");
            return NULL;
        }
    }
    // 5) Allocate
    SIMonotonicDimensionRef dim = SIMonotonicDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "SIMonotonicDimensionCreate: allocation failed\n");
        return NULL;
    }
    // 6) Set base fields
    if (label)
        DimensionSetLabel((DimensionRef)dim, label);
    if (description)
        DimensionSetDescription((DimensionRef)dim, description);
    if (metaData)
        DimensionSetMetaData((DimensionRef)dim, metaData);
    // 7) Set quantityName
    {
        OCStringRef nameCopy = OCStringCreateCopy(quantityName);
        DimensionSetQuantityName((DimensionRef)dim, nameCopy);
        OCRelease(nameCopy);
    }
    // 8) Copy & possibly reduce coordinates
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < count; ++i) {
        SIScalarRef c = OCArrayGetValueAtIndex(coordinates, i);
        SIScalarRef copy = SIDimensionalityEqual(dim0, SIQuantityGetUnitDimensionality((SIQuantityRef)c))
                               ? SIScalarCreateCopy(c)
                               : SIScalarCreateByReducingUnit(c);
        OCArrayAppendValue(dim->coordinates, copy);
        OCRelease(copy);
    }
    // 9) Origin & reference offsets (use provided or default to 0)
    SIScalarRef zero = SIScalarCreateWithDouble(0.0, unit);
    if (!originOffset)
        originOffset = zero;
    if (!coordinatesOffset)
        coordinatesOffset = zero;
    DimensionSetOriginOffset((DimensionRef)dim, originOffset);
    DimensionSetReferenceOffset((DimensionRef)dim, coordinatesOffset);
    if (zero != originOffset)
        OCRelease(zero);
    if (zero != coordinatesOffset)
        OCRelease(zero);
    // 10) Compute & set period = last – first
    SIScalarRef last = OCArrayGetValueAtIndex(coordinates, count - 1);
    SIScalarRef period = SIScalarCreateBySubtracting(last, first, NULL);
    DimensionSetPeriod((DimensionRef)dim, period);
    OCRelease(period);
    // 11) Flags
    DimensionSetPeriodic((DimensionRef)dim, periodic);
    DimensionSetScaling((DimensionRef)dim, scaling);
    return dim;
}
SIMonotonicDimensionRef
SIMonotonicDimensionCreateWithCoordinatesAndQuantity(OCArrayRef coordinates, OCStringRef quantityName) {
    return SIMonotonicDimensionCreate(
        /* label: */ NULL,
        /* description: */ NULL,
        /* metaData: */ NULL,
        /* coordinates: */ coordinates,
        /* quantityName: */ quantityName,
        /* originOffset: */ NULL,
        /* coordinatesOffset: */ NULL,
        /* periodic: */ false,
        /* scaling: */ kDimensionScalingNone);
}
SIMonotonicDimensionRef SIMonotonicDimensionCreateCopy(SIMonotonicDimensionRef src) {
    if (!src) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: source is NULL\n");
        return NULL;
    }
    OCArrayRef originalCoords = SIMonotonicDimensionGetCoordinates(src);
    if (!originalCoords || OCArrayGetCount(originalCoords) < 2) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: invalid or insufficient coordinates\n");
        return NULL;
    }
    const OCIndex count = OCArrayGetCount(originalCoords);
    OCMutableArrayRef copiedCoords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!copiedCoords) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to allocate coordinate array copy\n");
        return NULL;
    }
    for (OCIndex i = 0; i < count; ++i) {
        SIScalarRef coord = OCArrayGetValueAtIndex(originalCoords, i);
        SIScalarRef copy = SIScalarCreateCopy(coord);
        if (!copy) {
            fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to copy scalar at index %ld\n", i);
            OCRelease(copiedCoords);
            return NULL;
        }
        OCArrayAppendValue(copiedCoords, copy);
        OCRelease(copy);
    }
    SIMonotonicDimensionRef copy = SIMonotonicDimensionCreateWithCoordinates(copiedCoords);
    OCRelease(copiedCoords);
    if (!copy) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to create copy with coordinates\n");
        return NULL;
    }
    DimensionRef dst = (DimensionRef)copy;
    DimensionRef srcBase = (DimensionRef)src;
    SIScalarRef origin = SIScalarCreateCopy(DimensionGetOriginOffset(srcBase));
    SIScalarRef reference = SIScalarCreateCopy(DimensionGetReferenceOffset(srcBase));
    if (!origin || !reference) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to copy origin or reference offset\n");
        OCRelease(origin);
        OCRelease(reference);
        OCRelease(copy);
        return NULL;
    }
    if (!DimensionSetOriginOffset(dst, origin) ||
        !DimensionSetReferenceOffset(dst, reference)) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to set origin or reference offset\n");
        OCRelease(origin);
        OCRelease(reference);
        OCRelease(copy);
        return NULL;
    }
    OCRelease(origin);
    OCRelease(reference);
    SIScalarRef srcPeriod = DimensionGetPeriod(srcBase);
    if (srcPeriod) {
        SIScalarRef period = SIScalarCreateCopy(srcPeriod);
        if (!period || !DimensionSetPeriod(dst, period)) {
            fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to copy or set period\n");
            OCRelease(period);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(period);
    }
    if (!DimensionSetPeriodic(dst, DimensionIsPeriodic(srcBase)) ||
        !DimensionSetScaling(dst, DimensionGetScaling(srcBase))) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to set periodic flag or scaling\n");
        OCRelease(copy);
        return NULL;
    }
    OCStringRef label = OCStringCreateCopy(DimensionGetLabel((DimensionRef)src));
    OCStringRef desc = OCStringCreateCopy(DimensionGetDescription((DimensionRef)src));
    if (!DimensionSetLabel((DimensionRef)copy, label) ||
        !DimensionSetDescription((DimensionRef)copy, desc)) {
        fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to set label or description\n");
        OCRelease(label);
        OCRelease(desc);
        OCRelease(copy);
        return NULL;
    }
    OCRelease(label);
    OCRelease(desc);
    OCDictionaryRef meta = DimensionGetMetaData((DimensionRef)src);
    if (meta) {
        OCDictionaryRef metaCopy = OCDictionaryCreateCopy(meta);
        if (!metaCopy || !DimensionSetMetaData((DimensionRef)copy, metaCopy)) {
            fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to copy metadata\n");
            OCRelease(metaCopy);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(metaCopy);
    }
    OCStringRef quantityName = DimensionGetQuantityName(srcBase);
    if (quantityName) {
        OCStringRef nameCopy = OCStringCreateCopy(quantityName);
        if (!nameCopy || !DimensionSetQuantityName(dst, nameCopy)) {
            fprintf(stderr, "SIMonotonicDimensionCreateCopy: failed to copy quantity name\n");
            OCRelease(nameCopy);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(nameCopy);
    }
    return copy;
}
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim) {
    return dim ? dim->coordinates : NULL;
}
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim, OCArrayRef coordinates) {
    if (!dim || !coordinates)
        return false;
    if (dim->coordinates == coordinates)
        return true;
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    return true;
}
#pragma endregion SIMonotonicDimension
#pragma region SILinearDimension
// ============================================================================
// MARK: - (6) SILinearDimension
// ============================================================================
static OCTypeID kSILinearDimensionID = _kOCNotATypeID;
struct __SILinearDimension {
    //  Dimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;
    //  SIDimension
    OCStringRef quantityName;
    SIScalarRef coordinatesOffset;
    SIScalarRef originOffset;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
    //  SILinearDimension
    SIDimensionRef reciprocal;
    OCIndex count;
    SIScalarRef increment;
    SIScalarRef inverseIncrement;
    bool fft;
};
OCTypeID SILinearDimensionGetTypeID(void) {
    if (kSILinearDimensionID == _kOCNotATypeID)
        kSILinearDimensionID = OCRegisterType("SILinearDimension");
    return kSILinearDimensionID;
}
static void __SILinearDimensionFinalize(const void *obj) {
    SILinearDimensionRef dim = (SILinearDimensionRef)obj;
    __SIDimensionFinalize((struct __Dimension *)dim);
    OCRelease(dim->increment);
    dim->increment = NULL;
    OCRelease(dim->inverseIncrement);
    dim->inverseIncrement = NULL;
    dim->count = 0;
    dim->periodic = false;
    dim->fft = false;
}
static OCStringRef __SILinearDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<SILinearDimension>");
}
static void *__SILinearDimensionDeepCopy(const void *obj) {
    if (!obj)
        return NULL;
    DimensionRef src = (DimensionRef)obj;
    return SILinearDimensionCreate(
        DimensionGetCount(src),
        SILinearDimensionGetIncrement((SILinearDimensionRef)src),
        DimensionGetOriginOffset(src),
        DimensionGetReferenceOffset(src),
        DimensionGetPeriod(src),
        DimensionGetQuantityName(src));
}
static SILinearDimensionRef SILinearDimensionAllocate(void) {
    SILinearDimensionRef obj = OCTypeAlloc(struct __SILinearDimension,
                                           SILinearDimensionGetTypeID(),
                                           __SILinearDimensionFinalize,
                                           NULL,
                                           __SILinearDimensionCopyFormattingDesc,
                                           __SILinearDimensionDeepCopy,
                                           __SILinearDimensionDeepCopy);
    __RMNInitBaseFields((DimensionRef)obj);
    __InitSIDimensionFields((SIDimensionRef)obj);
    obj->count = 0;
    obj->increment = NULL;
    obj->inverseIncrement = NULL;
    obj->fft = false;
    return obj;
}
SILinearDimensionRef SILinearDimensionCreate(OCIndex count, SIScalarRef increment, SIScalarRef origin, SIScalarRef coordinatesOffset, SIScalarRef period, OCStringRef quantityName) {
    if (!increment || count <= 1)
        return NULL;
    SILinearDimensionRef dim = SILinearDimensionAllocate();
    if (!dim)
        return NULL;
    dim->count = count;
    dim->increment = OCRetain(increment);
    dim->originOffset = origin ? OCRetain(origin) : NULL;
    dim->coordinatesOffset = coordinatesOffset ? OCRetain(coordinatesOffset) : NULL;
    dim->period = period ? OCRetain(period) : NULL;
    dim->quantityName = quantityName ? OCRetain(quantityName) : NULL;
    return dim;
}
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim) {
    return dim ? dim->increment : NULL;
}
{
    return OCGetTypeID(obj) == SILinearDimensionGetTypeID();
}
#pragma endregion SILinearDimension
#pragma region Dimension Helpers
DimensionRef DimensionCreateDeepCopy(DimensionRef original) {
    if (!original)
        return NULL;
    OCTypeID typeID = OCGetTypeID(original);
    if (typeID == SILinearDimensionGetTypeID()) {
        // use OCRetain from OCTypes
        return (DimensionRef)OCRetain((OCTypeRef)original);
    }
    if (typeID == SIMonotonicDimensionGetTypeID()) {
        return (DimensionRef)SIMonotonicDimensionCreateCopy((SIMonotonicDimensionRef)original);
    }
    if (typeID == LabeledDimensionGetTypeID()) {
        OCArrayRef labels = LabeledDimensionGetCoordinateLabels((LabeledDimensionRef)original);
        if (!labels)
            return NULL;
        return (DimensionRef)LabeledDimensionCreateWithCoordinateLabels(labels);
    }
    if (typeID == SIDimensionGetTypeID()) {
        DimensionRef src = (DimensionRef)original;
        return (DimensionRef)SIDimensionCreate(
            DimensionGetLabel(src),
            DimensionGetDescription(src),
            DimensionGetMetaData(src),
            DimensionGetQuantityName(src),
            DimensionGetReferenceOffset(src),
            DimensionGetOriginOffset(src),
            DimensionGetPeriod(src),
            DimensionIsPeriodic(src),
            DimensionGetScaling(src));
    }
    fprintf(stderr, "DimensionCreateDeepCopy: Unsupported typeID %u\n", typeID);
    return NULL;
}
OCIndex DimensionGetCount(DimensionRef theDimension) {
    if (!theDimension)
        return 0;
    OCTypeID typeID = OCGetTypeID(theDimension);
    if (typeID == LabeledDimensionGetTypeID()) {
        LabeledDimensionRef labeledDim = (LabeledDimensionRef)theDimension;
        return OCArrayGetCount(labeledDim->coordinateLabels);
    } else if (typeID == SILinearDimensionGetTypeID()) {
        SILinearDimensionRef linearDim = (SILinearDimensionRef)theDimension;
        return linearDim->count;
    } else if (typeID == SIMonotonicDimensionGetTypeID()) {
        SIMonotonicDimensionRef monoDim = (SIMonotonicDimensionRef)theDimension;
        return OCArrayGetCount(monoDim->coordinates);
    }
    return 0;
}
bool SIDimensionMultiplyByScalar(SIDimensionRef dim,
                                 SIScalarRef theScalar,
                                 OCStringRef *error) {
    if (!dim) return false;
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID() &&
        typeID != SIMonotonicDimensionGetTypeID() &&
        typeID != SIDimensionGetTypeID()) {
        return false;
    }
    IF_NO_OBJECT_EXISTS_RETURN(theScalar, false);
    if (SIScalarDoubleValue(theScalar) == 0.0)
        return false;
    if (SIQuantityHasDimensionality((SIQuantityRef)theScalar, SIDimensionalityDimensionless()) && SIScalarIsReal(theScalar) && SIScalarDoubleValue(theScalar) == 1)
        return true;
    SIDimensionRef theDimension = (SIDimensionRef)dim;
    SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)theDimension->originOffset, theScalar, error);
    SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)theDimension->coordinatesOffset, theScalar, error);
    if (theDimension->period)
        SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef)theDimension->period, theScalar, error);
    if (theDimension->quantityName)
        OCRelease(theDimension->quantityName);
    OCStringRef quantityName = SIUnitGuessQuantityName(SIQuantityGetUnit((SIQuantityRef)theDimension->originOffset));
    theDimension->quantityName = OCStringCreateCopy(quantityName);
    return true;
}
SIDimensionRef
DimensionGetReciprocal(DimensionRef dim) {
    if (!dim)
        return NULL;
    OCTypeID t = OCGetTypeID(dim);
    if (t == SIMonotonicDimensionGetTypeID()) {
        return ((SIMonotonicDimensionRef)dim)->reciprocal;
    }
    if (t == SILinearDimensionGetTypeID()) {
        return ((SILinearDimensionRef)dim)->reciprocal;
    }
    return NULL;
}
bool DimensionSetReciprocal(DimensionRef dim,
                            SIDimensionRef r) {
    if (!dim) return false;
    OCTypeID t = OCGetTypeID(dim);
    if (t == SIMonotonicDimensionGetTypeID()) {
        SIMonotonicDimensionRef m = (SIMonotonicDimensionRef)dim;
        OCRelease(m->reciprocal);
        m->reciprocal = r
                            ? (SIDimensionRef)OCRetain((OCTypeRef)r)
                            : NULL;
        return true;
    } else if (t == SILinearDimensionGetTypeID()) {
        SILinearDimensionRef l = (SILinearDimensionRef)dim;
        OCRelease(l->reciprocal);
        l->reciprocal = r
                            ? (SIDimensionRef)OCRetain((OCTypeRef)r)
                            : NULL;
        return true;
    }
    return false;
}
SIUnitRef DimensionGetUnit(DimensionRef theDimension) {
    IF_NO_OBJECT_EXISTS_RETURN(theDimension, NULL);
    OCTypeID typeID = OCGetTypeID(theDimension);
    if (typeID != SIMonotonicDimensionGetTypeID())
        return NULL;
    SIMonotonicDimensionRef theDim = (SIMonotonicDimensionRef)theDimension;
    if (OCArrayGetCount(theDim->coordinates) == 0)
        return NULL;
    return SUQuantityGetUnit(OCArrayGetValueAtIndex(theDim->coordinates, 0));
}
SIUnitRef DimensionGetInverseUnit(DimensionRef theDimension) {
    IF_NO_OBJECT_EXISTS_RETURN(theDimension, NULL);
    OCTypeID typeID = OCGetTypeID(theDimension);
    if (typeID != SIMonotonicDimensionGetTypeID())
        return NULL;
    SIMonotonicDimensionRef theDim = (SIMonotonicDimensionRef)theDimension;
    if (OCArrayGetCount(theDim->coordinates) == 0)
        return NULL;
    return SIUnitByRaisingToPower(SIQuantityGetUnit(OCArrayGetValueAtIndex(theDim->coordinates, 0)), -1, NULL, NULL);
}
void DimensionMakeNiceUnits(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SIMonotonicDimensionGetTypeID() &&
        typeID != SILinearDimensionGetTypeID() &&
        typeID != SIDimensionGetTypeID()) {
        return;
    }
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    if (theDimension->inverseIncrement) {
        SIUnitRef unit = SIQuantityGetUnit(theDimension->inverseIncrement);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->inverseIncrement, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->reciprocal->coordinatesOffset, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->reciprocal->originOffset, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->reciprocal->period, unit);
    }
    if (theDimension->increment) {
        SIUnitRef unit = PSQuantityGetUnit(theDimension->increment);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->increment, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->coordinatesOffset, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->originOffset, unit);
        PSScalarBestConversionForUnit((SIMutableScalarRef)theDimension->period, unit);
    }
}
bool SILinearDimensionGetFFT(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, false);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return false;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    return theDimension->fft;
}
void SILinearDimensionSetFFT(DimensionRef dim, bool fft) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, );
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    theDimension->fft = fft;
}
void SILinearDimensionToggleFFT(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, );
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    theDimension->fft = !theDimension->fft;
}
SIScalarRef CreateInverseIncrementFromIncrement(SIScalarRef increment, OCIndex numberOfSamples) {
    IF_NO_OBJECT_EXISTS_RETURN(increment, NULL);
    if (numberOfSamples < 1)
        return NULL;
    SIScalarRef temp = SIScalarCreateByRaisingToPower(increment, -1, NULL);
    if (NULL == temp)
        return NULL;
    long double scaling = (long double)1. / ((long double)numberOfSamples);
    SIScalarRef inverseIncrement = SIScalarCreateByMultiplyingByDimensionlessRealConstant(temp, (double)scaling);
    OCRelease(temp);
    return inverseIncrement;
}
bool SILinearDimensionSetCount(DimensionRef dim, OCIndex count) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, false);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return false;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    if (theDimension->increment) {
        theDimension->count = count;
        theDimension->inverseIncrement = CreateInverseIncrementFromIncrement(theDimension->increment, theDimension->count);
        if (theDimension->reciprocal->quantityName)
            SIScalarBestConversionForQuantityName((SIMutableScalarRef)theDimension->inverseIncrement, theDimension->reciprocal->quantityName);
        return true;
    }
    return false;
}
bool DimensionHasNegativeIncrement(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, false);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return false;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    if (theDimension->increment) {
        if (PSScalarDoubleValue(theDimension->increment) < 0)
            return true;
    }
    return false;
}
SIScalarRef DimensionGetIncrement(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, NULL);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return NULL;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    return theDimension->increment;
}
void DimensionSetIncrement(DimensionRef dim, SIScalarRef increment) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, );
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    IF_NO_OBJECT_EXISTS_RETURN(theDimension->increment, );
    IF_NO_OBJECT_EXISTS_RETURN(increment, );
    if (theDimension->increment == increment || PSQuantityIsComplexType(increment))
        return;
    SIDimensionalityRef theDimensionality = PSDimensionalityForQuantityName(theDimension->quantityName);
    if (!PSDimensionalityHasSameReducedDimensionality(theDimensionality, PSQuantityGetUnitDimensionality(increment)))
        return;
    OCRelease(theDimension->increment);
    theDimension->increment = OCRetain(increment);
    PSScalarSetElementType((SIMutableScalarRef)theDimension->increment, kSINumberFloat64Type);
    SIScalarRef newInverseIncrement = CreateInverseIncrementFromIncrement(theDimension->increment, theDimension->npts);
    if (SIScalarCompare(newInverseIncrement, theDimension->inverseIncrement) != kOCCompareEqualTo) {
        OCRelease(theDimension->inverseIncrement);
        theDimension->inverseIncrement = newInverseIncrement;
        if (theDimension->reciprocal->quantityName)
            PSScalarBestConversionForQuantityName((SIMutableScalarRef)theDimension->inverseIncrement, theDimension->reciprocal->quantityName);
    } else
        OCRelease(newInverseIncrement);
    return;
}
SIScalarRef DimensionGetInverseIncrement(DimensionRef dim) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, NULL);
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return NULL;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    IF_NO_OBJECT_EXISTS_RETURN(theDimension->inverseIncrement, NULL);
    return theDimension->inverseIncrement;
}
void DimensionSetInverseIncrement(DimensionRef dim, SIScalarRef inverseIncrement) {
    IF_NO_OBJECT_EXISTS_RETURN(dim, );
    OCTypeID typeID = OCGetTypeID(dim);
    if (typeID != SILinearDimensionGetTypeID())
        return;
    SILinearDimensionRef theDimension = (SILinearDimensionRef)dim;
    IF_NO_OBJECT_EXISTS_RETURN(theDimension->inverseIncrement, );
    IF_NO_OBJECT_EXISTS_RETURN(inverseIncrement, );
    if (theDimension->inverseIncrement == inverseIncrement)
        return;
    if (SIQuantityGetElementType((SIQuantityRef)inverseIncrement) != kSINumberFloat64Type)
        return;
    OCRelease(theDimension->inverseIncrement);
    theDimension->inverseIncrement = OCRetain(inverseIncrement);
    SIScalarRef newIncrement = CreateInverseIncrementFromIncrement(theDimension->inverseIncrement, theDimension->npts);
    if (PSScalarCompare(newIncrement, theDimension->increment) != kOCCompareEqualTo) {
        OCRelease(theDimension->increment);
        theDimension->increment = newIncrement;
        if (theDimension->quantityName)
            PSScalarBestConversionForQuantityName((SIMutableScalarRef)theDimension->increment, theDimension->quantityName);
    } else
        OCRelease(newIncrement);
    return;
}
#pragma endregion Dimension Helpers
