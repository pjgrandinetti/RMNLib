// ============================================================================
// Dimension.c
//
// All Dimension types (base + Labeled, Quantitative, Monotonic, Linear)
// are defined and implemented in one file, grouped by type.
//
// (1)  Shared utilities & macros
// (2)  Dimension (abstract base)
// (3)  LabeledDimension
// (4)  SIDimension
// (5)  SIMonotonicDimension
// (6)  SILinearDimension
// ============================================================================
#include "RMNLibrary.h"
#pragma region Dimension
// ============================================================================
// MARK: - (1) Dimension (Abstract Base)
// ============================================================================
static OCTypeID kDimensionID = _kOCNotATypeID;
struct __Dimension {
    //  Dimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;
};
static void __InitBaseDimensionFields(DimensionRef dim) {
    dim->label = STR("");
    dim->description = STR("");
    dim->metaData = OCDictionaryCreateMutable(0);
}
OCTypeID DimensionGetTypeID(void) {
    if (kDimensionID == _kOCNotATypeID)
        kDimensionID = OCRegisterType("Dimension");
    return kDimensionID;
}
static bool __DimensionEqual(const void *a, const void *b) {
    const DimensionRef dimA = (const DimensionRef)a;
    const DimensionRef dimB = (const DimensionRef)b;
    if (!dimA || !dimB) return false;
    return OCEqual(dimA->label, dimB->label) &&
           OCEqual(dimA->description, dimB->description) &&
           OCEqual(dimA->metaData, dimB->metaData);
}
static void __DimensionFinalize(const void *obj) {
    DimensionRef dim = (DimensionRef)obj;
    if (!dim) return;
    OCRelease(dim->label);
    OCRelease(dim->description);
    OCRelease(dim->metaData);
    dim->label = NULL;
    dim->description = NULL;
    dim->metaData = NULL;
}
static OCStringRef __DimensionCopyFormattingDesc(OCTypeRef cf) {
    DimensionRef dim = (DimensionRef)cf;
    if (!dim) return OCStringCreateWithCString("<Dimension: NULL>");
    OCStringRef label = dim->label ? dim->label : STR("(no label)");
    OCStringRef description = dim->description ? dim->description : STR("(no description)");
    return OCStringCreateWithFormat("<Dimension label=\"%@\" description=\"%@\">", label, description);
}
static void *__DimensionDeepCopy(const void *obj) {
    return DimensionDeepCopy((DimensionRef)obj);
}
static DimensionRef DimensionAllocate(void) {
    DimensionRef dim = OCTypeAlloc(
        struct __Dimension,
        DimensionGetTypeID(),
        __DimensionFinalize,
        __DimensionEqual,
        __DimensionCopyFormattingDesc,
        __DimensionDeepCopy,
        __DimensionDeepCopy);
    if (!dim) return NULL;
    __InitBaseDimensionFields(dim);
    return dim;
}
DimensionRef DimensionDeepCopy(DimensionRef original) {
    if (!original) return NULL;
    // Allocate a new Dimension instance
    DimensionRef copy = DimensionAllocate();
    if (!copy) return NULL;
    // Copy each field using the setters (which perform safe copying)
    if (!DimensionSetLabel(copy, original->label) ||
        !DimensionSetDescription(copy, original->description) ||
        !DimensionSetMetaData(copy, original->metaData)) {
        OCRelease(copy);
        return NULL;
    }
    return copy;
}
OCStringRef DimensionGetLabel(DimensionRef dim) {
    return dim ? dim->label : NULL;
}
bool DimensionSetLabel(DimensionRef dim, OCStringRef label) {
    if (!dim) return false;
    OCStringRef labelCopy = label ? OCStringCreateCopy(label) : NULL;
    if (label && !labelCopy) {
        fprintf(stderr, "DimensionSetLabel: failed to copy label string\n");
        return false;
    }
    OCRelease(dim->label);
    dim->label = labelCopy;
    return true;
}
OCStringRef DimensionGetDescription(DimensionRef dim) {
    return dim ? dim->description : NULL;
}
bool DimensionSetDescription(DimensionRef dim, OCStringRef desc) {
    if (!dim)
        return false;
    OCStringRef descCopy = desc ? OCStringCreateCopy(desc) : NULL;
    if (desc && !descCopy) {
        fprintf(stderr, "DimensionSetDescription: failed to copy description string\n");
        return false;
    }
    OCRelease(dim->description);
    dim->description = descCopy;
    return true;
}
OCDictionaryRef DimensionGetMetaData(DimensionRef dim) {
    return dim ? dim->metaData : NULL;
}
bool DimensionSetMetaData(DimensionRef dim, OCDictionaryRef dict) {
    if (!dim)
        return false;
    OCDictionaryRef dictCopy = dict ? OCTypeDeepCopy(dict) : NULL;
    if (dict && !dictCopy) {
        fprintf(stderr, "DimensionSetMetaData: failed to copy metadata dictionary\n");
        return false;
    }
    OCRelease(dim->metaData);
    dim->metaData = dictCopy;
    return true;
}
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    OCStringRef label = OCTypeCast(OCStringRef, OCDictionaryGetValue(dict, STR("label")));
    OCStringRef description = OCTypeCast(OCStringRef, OCDictionaryGetValue(dict, STR("description")));
    OCDictionaryRef metaData = OCTypeCast(OCDictionaryRef, OCDictionaryGetValue(dict, STR("metaData")));
    DimensionRef dim = DimensionAllocate();
    if (!dim) return NULL;
    if (label && !DimensionSetLabel(dim, label)) {
        OCRelease(dim);
        return NULL;
    }
    if (description && !DimensionSetDescription(dim, description)) {
        OCRelease(dim);
        return NULL;
    }
    if (metaData && !DimensionSetMetaData(dim, metaData)) {
        OCRelease(dim);
        return NULL;
    }
    return dim;
}
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim) {
    if (!dim) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    OCStringRef label = DimensionGetLabel(dim);
    if (label) OCDictionarySetValue(dict, STR("label"), label);
    OCStringRef description = DimensionGetDescription(dim);
    if (description) OCDictionarySetValue(dict, STR("description"), description);
    OCDictionaryRef metaData = DimensionGetMetaData(dim);
    if (metaData) OCDictionarySetValue(dict, STR("metaData"), metaData);
    return dict;
}
#pragma endregion Dimension
#pragma region LabeledDimension
// ============================================================================
// MARK: - (3) LabeledDimension
// ============================================================================
static OCTypeID kLabeledDimensionID = _kOCNotATypeID;
typedef struct __LabeledDimension {
    struct __Dimension _super;  // <-- inherit all base fields
    OCMutableArrayRef coordinateLabels;
} *LabeledDimensionRef;
OCTypeID LabeledDimensionGetTypeID(void) {
    if (kLabeledDimensionID == _kOCNotATypeID)
        kLabeledDimensionID = OCRegisterType("LabeledDimension");
    return kLabeledDimensionID;
}
static bool __LabeledDimensionEqual(const void *a, const void *b) {
    const LabeledDimensionRef dimA = (const LabeledDimensionRef)a;
    const LabeledDimensionRef dimB = (const LabeledDimensionRef)b;
    if (!dimA || !dimB)
        return false;
    // Compare base Dimension fields via the embedded _super
    if (!__DimensionEqual((const DimensionRef)&dimA->_super,
                          (const DimensionRef)&dimB->_super))
        return false;
    // Compare LabeledDimension-specific field
    return OCEqual(dimA->coordinateLabels, dimB->coordinateLabels);
}
static void __LabeledDimensionFinalize(const void *obj) {
    const LabeledDimensionRef dim = (const LabeledDimensionRef)obj;
    // Finalize only the base part:
    __DimensionFinalize((DimensionRef)&dim->_super);
    // Then clean up subclass fields:
    OCRelease(dim->coordinateLabels);
    /* dim->coordinateLabels = NULL;  // not strictly needed after finalize */
}
static OCStringRef __LabeledDimensionCopyFormattingDesc(OCTypeRef cf) {
    const LabeledDimensionRef dim = (const LabeledDimensionRef)cf;
    if (!dim) {
        return OCStringCreateWithCString("<LabeledDimension: NULL>");
    }
    // Use the base‐class getters rather than poking at _super directly:
    OCStringRef label = DimensionGetLabel((DimensionRef)dim) ?: STR("(no label)");
    OCStringRef description = DimensionGetDescription((DimensionRef)dim) ?: STR("(no description)");
    OCIndex count = dim->coordinateLabels
                        ? OCArrayGetCount(dim->coordinateLabels)
                        : 0;
    return OCStringCreateWithFormat(
        "<LabeledDimension label=\"%@\" description=\"%@\" coordinateLabelCount=%ld>",
        label,
        description,
        (long)count);
}
static void *__LabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    const LabeledDimensionRef original = (const LabeledDimensionRef)obj;
    // 1) Allocate a fresh LabeledDimension (correct type & vtable already set)
    LabeledDimensionRef copy = LabeledDimensionAllocate();
    if (!copy) return NULL;
    // 2) Copy all base‐class fields via the Dimension API
    if (!DimensionSetLabel((DimensionRef)copy, DimensionGetLabel((DimensionRef)original)) ||
        !DimensionSetDescription((DimensionRef)copy, DimensionGetDescription((DimensionRef)original)) ||
        !DimensionSetMetaData((DimensionRef)copy, DimensionGetMetaData((DimensionRef)original))) {
        OCRelease(copy);
        return NULL;
    }
    // 3) Copy the coordinate‐labels array via the setter, which will deep‐copy it
    OCArrayRef origLabels = LabeledDimensionGetCoordinateLabels(original);
    if (!LabeledDimensionSetCoordinateLabels(copy, origLabels)) {
        OCRelease(copy);
        return NULL;
    }
    return copy;
}
static LabeledDimensionRef LabeledDimensionAllocate(void) {
    LabeledDimensionRef obj = OCTypeAlloc(
        struct __LabeledDimension,
        LabeledDimensionGetTypeID(),
        __LabeledDimensionFinalize,
        __LabeledDimensionEqual,
        __LabeledDimensionCopyFormattingDesc,
        __LabeledDimensionDeepCopy,
        __LabeledDimensionDeepCopy);
    if (!obj) return NULL;
    __InitBaseDimensionFields((DimensionRef)obj);
    obj->coordinateLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!obj->coordinateLabels) {
        OCRelease(obj);
        return NULL;
    }
    return obj;
}
LabeledDimensionRef
LabeledDimensionCreate(OCStringRef label, OCStringRef description, OCDictionaryRef metaData, OCArrayRef coordinateLabels) {
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr, "LabeledDimensionCreate: need ≥2 coordinate labels\n");
        return NULL;
    }
    LabeledDimensionRef dim = LabeledDimensionAllocate();
    if (!dim)
        return NULL;
    // Base fields
    if (label) {
        if (!DimensionSetLabel((DimensionRef)dim, label)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (description) {
        if (!DimensionSetDescription((DimensionRef)dim, description)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (metaData) {
        if (!DimensionSetMetaData((DimensionRef)dim, metaData)) {
            OCRelease(dim);
            return NULL;
        }
    }
    // Labels array
    if (dim->coordinateLabels) OCRelease(dim->coordinateLabels);
    dim->coordinateLabels = OCArrayCreateMutableCopy(coordinateLabels);
    if (!dim->coordinateLabels) {
        OCRelease(dim);
        return NULL;
    }
    return dim;
}
LabeledDimensionRef
LabeledDimensionCreateWithCoordinateLabels(OCArrayRef coordinateLabels) {
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr, "LabeledDimensionCreateWithCoordinateLabels: need ≥2 labels\n");
        return NULL;
    }
    // Delegate to the general constructor, using empty strings and NULL metadata
    return LabeledDimensionCreate(
        STR(""),
        STR(""),
        NULL,  // default metadata → creates an empty dictionary
        coordinateLabels);
}
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim) {
    return dim ? dim->coordinateLabels : NULL;
}
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim, OCArrayRef coordinateLabels) {
    if (!dim || !coordinateLabels)
        return false;
    if (dim->coordinateLabels == coordinateLabels)
        return true;
    OCRelease(dim->coordinateLabels);
    dim->coordinateLabels = OCArrayCreateMutableCopy(coordinateLabels);
    return true;
}
OCStringRef LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim, OCIndex index) {
    if (!dim || !dim->coordinateLabels || index < 0 || index >= OCArrayGetCount(dim->coordinateLabels))
        return NULL;
    return OCArrayGetValueAtIndex(dim->coordinateLabels, index);
}
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim, OCIndex index, OCStringRef label) {
    if (!dim || !dim->coordinateLabels || !label) return false;
    if (index < 0 || index >= OCArrayGetCount(dim->coordinateLabels)) return false;
    OCArraySetValueAtIndex(dim->coordinateLabels, index, label);
    return true;
}
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    // 1) extract & validate labels
    OCArrayRef coordinateLabels =
        OCTypeCast(OCArrayRef, OCDictionaryGetValue(dict, STR("labels")));
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr,
                "LabeledDimensionCreateFromDictionary: 'labels' missing or <2 entries\n");
        return NULL;
    }
    // 2) allocate the full subclass
    LabeledDimensionRef dim = LabeledDimensionAllocate();
    if (!dim) return NULL;
    // 3) fill base Dimension fields
    OCStringRef label = OCTypeCast(OCStringRef, OCDictionaryGetValue(dict, STR("label")));
    if (label && !DimensionSetLabel((DimensionRef)dim, label)) {
        OCRelease(dim);
        return NULL;
    }
    OCStringRef description =
        OCTypeCast(OCStringRef, OCDictionaryGetValue(dict, STR("description")));
    if (description && !DimensionSetDescription((DimensionRef)dim, description)) {
        OCRelease(dim);
        return NULL;
    }
    OCDictionaryRef metaData =
        OCTypeCast(OCDictionaryRef, OCDictionaryGetValue(dict, STR("metaData")));
    if (metaData && !DimensionSetMetaData((DimensionRef)dim, metaData)) {
        OCRelease(dim);
        return NULL;
    }
    // 4) deep‐copy the labels array
    OCRelease(dim->coordinateLabels);
    dim->coordinateLabels =
        OCArrayCreateMutableCopy(coordinateLabels);
    if (!dim->coordinateLabels) {
        OCRelease(dim);
        return NULL;
    }
    return dim;
}
OCDictionaryRef
LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim) {
    if (!dim) return NULL;
    // Start with the base–Dimension serialization
    OCDictionaryRef dict = DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // Grab our labels, deep‐copy them, stick them in under "labels"
    OCArrayRef labels = LabeledDimensionGetCoordinateLabels(dim);
    if (labels) {
        // OCTypeDeepCopy will deep‐copy the array structure *and* each element
        OCDictionarySetValue(dict,
                             STR("labels"),
                             (OCDictionaryRef)OCTypeDeepCopy((OCTypeRef)labels));
    }
    return dict;
}
#pragma endregion LabeledDimension
#pragma region SIDimension
    // ============================================================================
    // MARK: - (4) SIDimension
    // ============================================================================
    static OCTypeID kSIDimensionID = _kOCNotATypeID;
typedef struct __SIDimension {
    struct __Dimension  _super;          // inherit all base‐Dimension fields
    OCStringRef         quantityName;
    SIScalarRef         coordinatesOffset;
    SIScalarRef         originOffset;
    SIScalarRef         period;
    bool                periodic;
    dimensionScaling    scaling;
} *SIDimensionRef;
OCTypeID SIDimensionGetTypeID(void) {
    if (kSIDimensionID == _kOCNotATypeID)
        kSIDimensionID = OCRegisterType("SIDimension");
    return kSIDimensionID;
}
static void __SIDimensionFinalize(const void *vobj) {
    SIDimensionRef dim = (SIDimensionRef)vobj;
    __DimensionFinalize((DimensionRef)&dim->_super);
    OCRelease(dim->quantityName);
    OCRelease(dim->coordinatesOffset);
    OCRelease(dim->originOffset);
    OCRelease(dim->period);
}
static OCStringRef __SIDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<SIDimension>");
}
static void *__SIDimensionDeepCopy(const void *obj) {
    if (!obj)
        return NULL;
    SIDimensionRef src = (SIDimensionRef)obj;
    return SIDimensionCreate(
        DimensionGetLabel((DimensionRef)src),
        DimensionGetDescription((DimensionRef)src),
        DimensionGetMetaData((DimensionRef)src),
        DimensionGetQuantityName((DimensionRef)src),
        DimensionGetReferenceOffset((DimensionRef)src),
        DimensionGetOriginOffset((DimensionRef)src),
        DimensionGetPeriod((DimensionRef)src),
        DimensionIsPeriodic((DimensionRef)src),
        DimensionGetScaling((DimensionRef)src));
}
static void __InitSIDimensionFields(SIDimensionRef dim) {
    dim->quantityName = NULL;
    dim->coordinatesOffset = NULL;
    dim->originOffset = NULL;
    dim->period = NULL;
    dim->periodic = false;
    dim->scaling = kDimensionScalingNone;
}
static SIDimensionRef SIDimensionAllocate(void) {
    SIDimensionRef obj = OCTypeAlloc(
        struct __SIDimension,
        SIDimensionGetTypeID(),
        __SIDimensionFinalize,
        /*equal=*/NULL,
        __SIDimensionCopyFormattingDesc,
        __SIDimensionDeepCopy,
        __SIDimensionDeepCopy
    );
    __InitBaseDimensionFields((DimensionRef)&obj->_super);
    __InitSIDimensionFields(obj);
    return obj;
}
SIDimensionRef SIDimensionCreate(OCStringRef label, OCStringRef description, OCDictionaryRef metaData, OCStringRef quantityName, SIScalarRef coordinatesOffset, SIScalarRef originOffset, SIScalarRef period, bool periodic, dimensionScaling scaling) {
    if (!coordinatesOffset || !originOffset || !period) {
        fprintf(stderr, "SIDimensionCreate: coordinatesOffset, originOffset, and period must not be NULL\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)coordinatesOffset) ||
        SIQuantityIsComplexType((SIQuantityRef)originOffset) ||
        SIQuantityIsComplexType((SIQuantityRef)period)) {
        fprintf(stderr, "SIDimensionCreate: scalar inputs must be real-valued\n");
        return NULL;
    }
    // Retrieve dimensionalities
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coordinatesOffset);
    SIDimensionalityRef origDim = SIQuantityGetUnitDimensionality((SIQuantityRef)originOffset);
    SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)period);
    // Check dimensionality consistency among scalars
    if (!SIDimensionalityHasSameReducedDimensionality(refDim, origDim) ||
        !SIDimensionalityHasSameReducedDimensionality(refDim, perDim)) {
        fprintf(stderr, "SIDimensionCreate: coordinatesOffset, originOffset, and period must have the same dimensionality\n");
        return NULL;
    }
    // Validate quantityName against scalar dimensionality
    if (quantityName) {
        OCStringRef error = NULL;
        SIDimensionalityRef nameDim = SIDimensionalityForQuantity(quantityName, &error);
        if (!nameDim || !SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
            fprintf(stderr, "SIDimensionCreate: quantityName dimensionality mismatch\n");
            return NULL;
        }
    } else {
        // Attempt to infer quantityName from coordinatesOffset's unit
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)coordinatesOffset);
        quantityName = SIUnitGuessQuantityName(unit);
        if (!quantityName) {
            fprintf(stderr, "SIDimensionCreate: unable to infer quantityName from unit\n");
            return NULL;
        }
    }
    // Allocate the dimension object
    SIDimensionRef dim = SIDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "SIDimensionCreate: allocation failed\n");
        return NULL;
    }
    // Set label
    if (label) {
        dim->label = OCStringCreateCopy(label);
        if (!dim->label) {
            fprintf(stderr, "SIDimensionCreate: failed to copy label\n");
            OCRelease(dim);
            return NULL;
        }
    }
    // Set description
    if (description) {
        dim->description = OCStringCreateCopy(description);
        if (!dim->description) {
            fprintf(stderr, "SIDimensionCreate: failed to copy description\n");
            OCRelease(dim);
            return NULL;
        }
    }
    // Set metaData
    if (metaData) {
        dim->metaData = (OCDictionaryRef)OCTypeDeepCopy(metaData);
        if (!dim->metaData) {
            fprintf(stderr, "SIDimensionCreate: failed to copy metaData\n");
            OCRelease(dim);
            return NULL;
        }
    } else {
        dim->metaData = OCDictionaryCreateMutable(0);
        if (!dim->metaData) {
            fprintf(stderr, "SIDimensionCreate: failed to create metaData dictionary\n");
            OCRelease(dim);
            return NULL;
        }
    }
    // Set quantityName
    dim->quantityName = OCStringCreateCopy(quantityName);
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionCreate: failed to copy quantityName\n");
        OCRelease(dim);
        return NULL;
    }
    // Set scalar fields
    dim->coordinatesOffset = SIScalarCreateCopy(coordinatesOffset);
    if (!dim->coordinatesOffset) {
        fprintf(stderr, "SIDimensionCreate: failed to copy coordinatesOffset\n");
        OCRelease(dim);
        return NULL;
    }
    dim->originOffset = SIScalarCreateCopy(originOffset);
    if (!dim->originOffset) {
        fprintf(stderr, "SIDimensionCreate: failed to copy originOffset\n");
        OCRelease(dim);
        return NULL;
    }
    dim->period = SIScalarCreateCopy(period);
    if (!dim->period) {
        fprintf(stderr, "SIDimensionCreate: failed to copy period\n");
        OCRelease(dim);
        return NULL;
    }
    // Set boolean flags
    dim->periodic = periodic;
    dim->scaling = scaling;
    return dim;
}
static OCTypeID kSIDimensionID = _kOCNotATypeID;
struct __SIDimension {
    struct __Dimension super;  // ← inherits label, description, metaData
    // now only the SIDimension–specific fields:
    OCStringRef quantityName;
    SIScalarRef coordinatesOffset;
    SIScalarRef originOffset;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
};
OCTypeID SIDimensionGetTypeID(void) {
    if (kSIDimensionID == _kOCNotATypeID)
        kSIDimensionID = OCRegisterType("SIDimension");
    return kSIDimensionID;
}
static void __SIDimensionFinalize(const void *obj) {
    SIDimensionRef dim = (SIDimensionRef)obj;
    __DimensionFinalize((DimensionRef)dim);
    OCRelease(dim->quantityName);
    dim->quantityName = NULL;
    OCRelease(dim->coordinatesOffset);
    dim->coordinatesOffset = NULL;
    OCRelease(dim->originOffset);
    dim->originOffset = NULL;
    OCRelease(dim->period);
    dim->period = NULL;
}
static OCStringRef __SIDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<SIDimension>");
}
static void *static void *
__SIDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // 1) Deep-copy the shared Dimension fields
    DimensionRef baseCopy = DimensionDeepCopy((DimensionRef)obj);
    if (!baseCopy) return NULL;
    // 2) Reassign it to SIDimension
    SIDimensionRef copy = (SIDimensionRef)baseCopy;
    OCTypeSetTypeID(copy, SIDimensionGetTypeID());
    OCTypeSetFinalizeCallback(copy, __SIDimensionFinalize);
    OCTypeSetCopyFormattingDescCallback(copy, __SIDimensionCopyFormattingDesc);
    OCTypeSetDeepCopyCallback(copy, __SIDimensionDeepCopy);
    // 3) Deep-copy each SIDimension-only field
    SIDimensionRef src = (SIDimensionRef)obj;
    copy->quantityName = src->quantityName
                             ? OCStringCreateCopy(src->quantityName)
                             : NULL;
    copy->coordinatesOffset = src->coordinatesOffset
                                  ? SIScalarCreateCopy(src->coordinatesOffset)
                                  : NULL;
    copy->originOffset = src->originOffset
                             ? SIScalarCreateCopy(src->originOffset)
                             : NULL;
    copy->period = src->period
                       ? SIScalarCreateCopy(src->period)
                       : NULL;
    copy->periodic = src->periodic;
    copy->scaling = src->scaling;
    return copy;
}
static void __InitSIDimensionFields(SIDimensionRef dim) {
    dim->quantityName = STR("dimensionless");
    dim->coordinatesOffset = SIScalarCreateWithDouble(0.0, SIUnitDimensionlessAndUnderived());
    dim->originOffset = SIScalarCreateWithDouble(0.0, SIUnitDimensionlessAndUnderived());
    dim->period = NULL;
    dim->periodic = false;
    dim->scaling = kDimensionScalingNone;
}
static SIDimensionRef SIDimensionAllocate(void) {
    // 1) allocate a plain Dimension
    DimensionRef base = DimensionAllocate();
    if (!base) return NULL;
    // 2) “cast” it to SIDimension and fix up its type & callbacks
    SIDimensionRef dim = (SIDimensionRef)base;
    OCTypeSetTypeID(dim, SIDimensionGetTypeID());
    OCTypeSetFinalizeCallback(dim, __SIDimensionFinalize);
    OCTypeSetCopyFormattingDescCallback(dim, __SIDimensionCopyFormattingDesc);
    OCTypeSetDeepCopyCallback(dim, __SIDimensionDeepCopy);
    // 3) initialize the SIDimension–only fields
    __InitSIDimensionFields(dim);
    return dim;
}
SIDimensionRef
SIDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metaData,
    OCStringRef quantityName,
    SIScalarRef coordinatesOffset,
    SIScalarRef originOffset,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling) {
    // 1) Mandatory inputs: coordinatesOffset + quantityName
    if (!coordinatesOffset) {
        fprintf(stderr,
                "SIDimensionCreate: coordinatesOffset must be non-NULL\n");
        return NULL;
    }
    if (!quantityName) {
        fprintf(stderr,
                "SIDimensionCreate: quantityName must be non-NULL\n");
        return NULL;
    }
    // 2) coordinatesOffset must be real-valued
    if (SIQuantityIsComplexType((SIQuantityRef)coordinatesOffset)) {
        fprintf(stderr,
                "SIDimensionCreate: coordinatesOffset must be real-valued\n");
        return NULL;
    }
    // 3) Determine unit & dimensionality of coordinatesOffset
    SIUnitRef coordUnit =
        SIQuantityGetUnit((SIQuantityRef)coordinatesOffset);
    SIDimensionalityRef coordDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)coordinatesOffset);
    // 4) quantityName → dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim =
        SIDimensionalityForQuantity(quantityName, &err);
    if (!nameDim ||
        !SIDimensionalityHasSameReducedDimensionality(nameDim, coordDim)) {
        if (err) {
            fprintf(stderr,
                    "SIDimensionCreate: quantityName error: %s\n",
                    OCStringGetCStringPtr(err));
            OCRelease(err);
        } else {
            fprintf(stderr,
                    "SIDimensionCreate: quantityName \"%s\" incompatible with coordinatesOffset\n",
                    OCStringGetCStringPtr(quantityName));
        }
        return NULL;
    }
    // 5) Prepare originOffset: default to zero of same unit if NULL
    SIScalarRef finalOrigin;
    if (originOffset) {
        if (SIQuantityIsComplexType((SIQuantityRef)originOffset) ||
            !SIDimensionalityHasSameReducedDimensionality(
                coordDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)originOffset))) {
            fprintf(stderr,
                    "SIDimensionCreate: originOffset dimensionality mismatch\n");
            return NULL;
        }
        finalOrigin = SIScalarCreateCopy(originOffset);
    } else {
        finalOrigin = SIScalarCreateWithDouble(0.0, coordUnit);
    }
    // 6) Prepare period: required if periodic==true, else optional
    SIScalarRef finalPeriod = NULL;
    if (periodic) {
        if (!period) {
            fprintf(stderr,
                    "SIDimensionCreate: periodic==true requires non-NULL period\n");
            OCRelease(finalOrigin);
            return NULL;
        }
        if (SIQuantityIsComplexType((SIQuantityRef)period) ||
            !SIDimensionalityHasSameReducedDimensionality(
                coordDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)period))) {
            fprintf(stderr,
                    "SIDimensionCreate: period dimensionality mismatch\n");
            OCRelease(finalOrigin);
            return NULL;
        }
        finalPeriod = SIScalarCreateCopy(period);
    } else if (period) {
        // copy & validate non-periodic period
        if (SIQuantityIsComplexType((SIQuantityRef)period) ||
            !SIDimensionalityHasSameReducedDimensionality(
                coordDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)period))) {
            fprintf(stderr,
                    "SIDimensionCreate: period dimensionality mismatch\n");
            OCRelease(finalOrigin);
            return NULL;
        }
        finalPeriod = SIScalarCreateCopy(period);
    }
    // 7) Allocate & subclass
    SIDimensionRef dim = SIDimensionAllocate();
    if (!dim) {
        fprintf(stderr,
                "SIDimensionCreate: allocation failed\n");
        OCRelease(finalOrigin);
        if (finalPeriod) OCRelease(finalPeriod);
        return NULL;
    }
    // 8) Base‐class fields
    if (label &&
        !DimensionSetLabel((DimensionRef)dim, label)) goto fail;
    if (description &&
        !DimensionSetDescription((DimensionRef)dim, description)) goto fail;
    if (metaData &&
        !DimensionSetMetaData((DimensionRef)dim, metaData)) goto fail;
    // 9) SIDimension‐specific fields (all deep‐copied)
    dim->quantityName = OCStringCreateCopy(quantityName);
    dim->coordinatesOffset = SIScalarCreateCopy(coordinatesOffset);
    dim->originOffset = finalOrigin;
    dim->period = finalPeriod;
    dim->periodic = periodic;
    dim->scaling = scaling;
    return dim;
fail:
    OCRelease(dim);
    return NULL;
}
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
bool SIDimensionSetQuantityName(SIDimensionRef dim, OCStringRef name) {
    if (!dim || !name) {
        fprintf(stderr, "SIDimensionSetQuantityName: name must be non-NULL\n");
        return false;
    }
    // 1) Look up the dimensionality for the requested quantity name.
    OCStringRef error = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(name, &error);
    if (!nameDim) {
        // The name isn’t recognized as a valid quantity.
        if (error) {
            fprintf(stderr, "SIDimensionSetQuantityName: %s\n",
                    OCStringGetCStringPtr(error));
            OCRelease(error);
        } else {
            fprintf(stderr, "SIDimensionSetQuantityName: unknown quantity \"%s\"\n",
                    OCStringGetCStringPtr(name));
        }
        return false;
    }
    // 2) Get the dimensionality of our coordinatesOffset.
    SIScalarRef coords = dim->coordinatesOffset;
    if (!coords) {
        fprintf(stderr, "SIDimensionSetQuantityName: cannot validate without coordinatesOffset\n");
        return false;
    }
    SIDimensionalityRef refDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    // 3) Compare reduced dimensionalities.
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
        fprintf(stderr,
                "SIDimensionSetQuantityName: dimensionality mismatch between \"%s\" and existing unit\n",
                OCStringGetCStringPtr(name));
        return false;
    }
    // 4) All good — replace the old name.
    OCRelease(dim->quantityName);
    dim->quantityName = OCStringCreateCopy(name);
    return dim->quantityName != NULL;
}
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim) {
    return dim ? dim->coordinatesOffset : NULL;
}
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim, SIScalarRef val) {
    if (!dim) return false;
    OCRelease(dim->coordinatesOffset);
    dim->coordinatesOffset = val ? OCRetain(val) : NULL;
    return true;
}
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim) {
    if (!dim) return false;
    return dim ? dim->originOffset : NULL;
}
bool SIDimensionSetOriginOffset(SIDimensionRef dim, SIScalarRef val) {
    if (!dim) return false;
    OCRelease(dim->originOffset);
    dim->originOffset = val ? OCRetain(val) : NULL;
    return true;
}
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim) {
    return dim ? dim->period : NULL;
}
bool SIDimensionSetPeriod(SIDimensionRef dim, SIScalarRef newPeriod) {
    if (!dim) return false;
    // Reject null or complex-period values
    if (!newPeriod || SIQuantityIsComplexType((SIQuantityRef)newPeriod))
        return false;
    // If it’s literally the same object, nothing to do
    if (dim->period == newPeriod)
        return true;
    // Convert into the dimension’s “relative” unit
    SIScalarRef tmp = SIScalarCreateByConvertingToUnit(
        newPeriod,
        SIDimensionGetRelativeUnit((DimensionRef)dim),
        NULL);
    if (!tmp)
        return false;
    // Now that conversion succeeded, replace the old value
    OCRelease(dim->period);
    dim->period = OCRetain(tmp);
    // Ensure the element‐type is standard
    SIScalarSetElementType((SIMutableScalarRef)dim->period, kSINumberFloat64Type);
    // Balance the temporary’s retain
    OCRelease(tmp);
    return true;
}
bool SIDimensionIsPeriodic(SIDimensionRef dim) {
    if (!dim) return false;
    return dim ? dim->periodic : false;
}
bool SIDimensionSetPeriodic(SIDimensionRef dim, bool flag) {
    if (!dim) return false;
    dim->periodic = flag;
    return true;
}
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim) {
    if (!dim) return kDimensionScalingNone;
    return dim ? dim->scaling : kDimensionScalingNone;
}
bool SIDimensionSetScaling(SIDimensionRef dim, dimensionScaling scaling) {
    if (!dim) return false;
    dim->scaling = scaling;
    return true;
}
#pragma endregion SIDimension
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
