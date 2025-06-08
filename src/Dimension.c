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
    if (!dim) {
        return OCStringCreateWithCString("<Dimension: NULL>");
    }
    // Pull via getters so subclasses aren’t bypassed
    OCStringRef lbl = DimensionGetLabel(dim);
    OCStringRef desc = DimensionGetDescription(dim);
    // Fallbacks for empty strings
    if (!lbl || OCStringGetLength(lbl) == 0) {
        lbl = STR("(no label)");
    }
    if (!desc || OCStringGetLength(desc) == 0) {
        desc = STR("(no description)");
    }
    return OCStringCreateWithFormat(
        "<Dimension label=\"%@\" description=\"%@\">",
        lbl,
        desc);
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
    if (!dim) return false;
    // Always end up with a valid dictionary (never NULL)
    OCDictionaryRef dictCopy;
    if (dict) {
        dictCopy = (OCDictionaryRef)OCTypeDeepCopy(dict);
        if (!dictCopy) {
            fprintf(stderr, "DimensionSetMetaData: failed to copy metadata dictionary\n");
            return false;
        }
    } else {
        dictCopy = OCDictionaryCreateMutable(0);
        if (!dictCopy) {
            fprintf(stderr, "DimensionSetMetaData: failed to create empty metadata dictionary\n");
            return false;
        }
    }
    // Swap in the new dictionary
    OCRelease(dim->metaData);
    dim->metaData = dictCopy;
    return true;
}
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    // Pull values out of the dictionary and cast them to the expected types
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metaData = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metaData"));
    DimensionRef dim = DimensionAllocate();
    if (!dim) return NULL;
    // Apply them via the public setters
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
    if (!dict) return NULL;
    // Copy the label string
    OCStringRef lbl = DimensionGetLabel(dim);
    if (lbl) {
        OCStringRef lblCopy = OCStringCreateCopy(lbl);
        if (!lblCopy) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("label"), lblCopy);
        OCRelease(lblCopy);
    }
    // Copy the description string
    OCStringRef desc = DimensionGetDescription(dim);
    if (desc) {
        OCStringRef descCopy = OCStringCreateCopy(desc);
        if (!descCopy) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("description"), descCopy);
        OCRelease(descCopy);
    }
    // Deep-copy the metadata dictionary
    OCDictionaryRef meta = DimensionGetMetaData(dim);
    if (meta) {
        OCDictionaryRef metaCopy = (OCDictionaryRef)OCTypeDeepCopy((OCTypeRef)meta);
        if (!metaCopy) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metaData"), metaCopy);
        OCRelease(metaCopy);
    }
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
    // Base‐class fields via getters
    OCStringRef lbl = DimensionGetLabel((DimensionRef)dim);
    OCStringRef desc = DimensionGetDescription((DimensionRef)dim);
    if (!lbl || OCStringGetLength(lbl) == 0) lbl = STR("(no label)");
    if (!desc || OCStringGetLength(desc) == 0) desc = STR("(no description)");
    // LabeledDimension‐specific
    OCIndex count = dim->coordinateLabels
                        ? OCArrayGetCount(dim->coordinateLabels)
                        : 0;
    return OCStringCreateWithFormat(
        "<LabeledDimension label=\"%@\" description=\"%@\" coordinateLabelCount=%ld>",
        lbl, desc, (long)count);
}
static void *__LabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    const LabeledDimensionRef original = (const LabeledDimensionRef)obj;
    // 1) Grab the original’s array and ensure it’s valid
    OCArrayRef origLabels = LabeledDimensionGetCoordinateLabels(original);
    if (!origLabels || OCArrayGetCount(origLabels) < 2) {
        // can’t copy an invalid or too‐small label set
        return NULL;
    }
    // 2) Delegate to the public create API, which will deep‐copy label, description,
    //    metadata and coordinateLabels (and enforce ≥2 labels).
    return LabeledDimensionCreate(
        DimensionGetLabel((DimensionRef)original),
        DimensionGetDescription((DimensionRef)original),
        DimensionGetMetaData((DimensionRef)original),
        origLabels);
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
    // 1) Extract & validate labels
    OCArrayRef coordinateLabels =
        (OCArrayRef)OCDictionaryGetValue(dict, STR("labels"));
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr,
                "LabeledDimensionCreateFromDictionary: 'labels' missing or <2 entries\n");
        return NULL;
    }
    // 2) Allocate the full subclass
    LabeledDimensionRef dim = LabeledDimensionAllocate();
    if (!dim) return NULL;
    // 3) Fill base Dimension fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    if (label && !DimensionSetLabel((DimensionRef)dim, label)) goto Err;
    OCStringRef description =
        (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    if (description && !DimensionSetDescription((DimensionRef)dim, description)) goto Err;
    OCDictionaryRef metaData =
        (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metaData"));
    if (metaData && !DimensionSetMetaData((DimensionRef)dim, metaData)) goto Err;
    // 4) Deep-copy the labels array
    OCRelease(dim->coordinateLabels);
    dim->coordinateLabels = OCArrayCreateMutableCopy(coordinateLabels);
    if (!dim->coordinateLabels) goto Err;
    return dim;
Err:
    OCRelease(dim);
    return NULL;
}
OCDictionaryRef
LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim) {
    if (!dim) return NULL;
    // Start with the base–Dimension serialization
    OCDictionaryRef dict = DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // Grab our labels, deep-copy them, stick them in under "labels"
    OCArrayRef labels = LabeledDimensionGetCoordinateLabels(dim);
    if (labels) {
        OCArrayRef labelsCopy = (OCArrayRef)OCTypeDeepCopy((OCTypeRef)labels);
        if (labelsCopy) {
            OCDictionarySetValue(dict, STR("labels"), labelsCopy);
            // Note: the dictionary will retain labelsCopy for us
        }
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
    struct __Dimension _super;  // inherit all base‐Dimension fields
    OCStringRef quantityName;
    SIScalarRef coordinatesOffset;
    SIScalarRef originOffset;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
} *SIDimensionRef;
OCTypeID SIDimensionGetTypeID(void) {
    if (kSIDimensionID == _kOCNotATypeID)
        kSIDimensionID = OCRegisterType("SIDimension");
    return kSIDimensionID;
}
static bool __SIDimensionEqual(const void *a, const void *b) {
    const SIDimensionRef dimA = (const SIDimensionRef)a;
    const SIDimensionRef dimB = (const SIDimensionRef)b;
    if (!dimA || !dimB) return false;
    // 1) Base‐class fields
    if (!__DimensionEqual((const DimensionRef)&dimA->_super,
                          (const DimensionRef)&dimB->_super))
        return false;
    // 2) quantityName
    if (!OCEqual(dimA->quantityName, dimB->quantityName))
        return false;
    // 3) coordinate & origin offsets
    if (!OCEqual(dimA->coordinatesOffset, dimB->coordinatesOffset))
        return false;
    if (!OCEqual(dimA->originOffset, dimB->originOffset))
        return false;
    // 4) periodic flag
    if (dimA->periodic != dimB->periodic)
        return false;
    // 5) if periodic, both must have a non‐NULL period and be equal
    if (dimA->periodic) {
        if (!dimA->period || !dimB->period)
            return false;
        if (!OCEqual(dimA->period, dimB->period))
            return false;
    }
    // 6) scaling mode
    if (dimA->scaling != dimB->scaling)
        return false;
    return true;
}
static void __SIDimensionFinalize(const void *obj) {
    if (!obj) return;
    SIDimensionRef dim = (SIDimensionRef)obj;
    __DimensionFinalize((DimensionRef)&dim->_super);
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
    SIDimensionRef d = (SIDimensionRef)cf;
    if (!d) {
        return OCStringCreateWithCString("<SIDimension: NULL>");
    }
    // Base‐class fields
    OCStringRef lbl = DimensionGetLabel((DimensionRef)d);
    OCStringRef desc = DimensionGetDescription((DimensionRef)d);
    if (!lbl || OCStringGetLength(lbl) == 0) lbl = STR("(no label)");
    if (!desc || OCStringGetLength(desc) == 0) desc = STR("(no description)");
    // SIDimension‐specific: quantity name + periodic/scaling flags
    OCStringRef qty = SIDimensionGetQuantityName(d);
    const char *periodic_str = SIDimensionIsPeriodic(d) ? "true" : "false";
    int scaling_mode = (int)SIDimensionGetScaling(d);
    if (!qty || OCStringGetLength(qty) == 0) qty = STR("(no quantity)");
    // Scalars: reference offset, origin offset, and (optional) period
    SIScalarRef refOffset = SIDimensionGetCoordinatesOffset(d);
    SIScalarRef originOffset = SIDimensionGetOriginOffset(d);
    SIScalarRef period = SIDimensionGetPeriod(d);
    // Assume each SIScalar has a copy‐formatting routine
    OCStringRef refStr = refOffset
                             ? SIScalarCreateStringValue(refOffset)
                             : STR("(no refOffset)");
    OCStringRef origStr = originOffset
                              ? SIScalarCreateStringValue(originOffset)
                              : STR("(no originOffset)");
    OCStringRef periodStr = (periodic_str[0] == 't' && period)
                                ? SIScalarCreateStringValue(period)
                                : STR("(n/a)");
    OCStringRef fmt = OCStringCreateWithFormat(
        "<SIDimension label=\"%@\" desc=\"%@\" qty=\"%@\" refOffset=%@ originOffset=%@ period=%@ periodic=%s scaling=%d>",
        lbl, desc, qty, refStr, origStr, periodStr, periodic_str, scaling_mode);
    OCRelease(refStr);
    OCRelease(origStr);
    OCRelease(periodStr);
    return fmt;
}
static void *__SIDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    const SIDimensionRef src = (const SIDimensionRef)obj;

    // Delegate to the public constructor so it deep-copies everything
    return SIDimensionCreate(
        /* label:             */ DimensionGetLabel((DimensionRef)src),
        /* description:       */ DimensionGetDescription((DimensionRef)src),
        /* metaData:          */ DimensionGetMetaData((DimensionRef)src),
        /* quantityName:      */ SIDimensionGetQuantityName(src),
        /* coordinatesOffset: */ SIDimensionGetCoordinatesOffset(src),
        /* originOffset:      */ SIDimensionGetOriginOffset(src),
        /* period:            */ SIDimensionGetPeriod(src),
        /* periodic:          */ SIDimensionIsPeriodic(src),
        /* scaling:           */ SIDimensionGetScaling(src)
    );
}
static void __InitSIDimensionFields(SIDimensionRef dim) {
    // Default quantity: dimensionless
    dim->quantityName = STR("dimensionless");
    // Use the unit‐less SI unit for all default scalars
    SIUnitRef u = SIUnitDimensionlessAndUnderived();
    // Default coordinate offset & origin both zero in the same unit
    dim->coordinatesOffset = SIScalarCreateWithDouble(0.0, u);
    dim->originOffset = SIScalarCreateWithDouble(0.0, u);
    // No period until the user explicitly makes it periodic
    dim->period = NULL;
    dim->periodic = false;
    dim->scaling = kDimensionScalingNone;
}
static SIDimensionRef SIDimensionAllocate(void) {
    SIDimensionRef obj = OCTypeAlloc(
        struct __SIDimension,
        SIDimensionGetTypeID(),
        __SIDimensionFinalize,
        __SIDimensionEqual,
        __SIDimensionCopyFormattingDesc,
        __SIDimensionDeepCopy,
        __SIDimensionDeepCopy);
    __InitBaseDimensionFields((DimensionRef)&obj->_super);
    __InitSIDimensionFields(obj);
    return obj;
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
    // 1) Required: quantityName + coordinatesOffset
    if (!quantityName) {
        fprintf(stderr, "SIDimensionCreate: quantityName must be non-NULL\n");
        return NULL;
    }
    if (!coordinatesOffset) {
        fprintf(stderr, "SIDimensionCreate: coordinatesOffset must be non-NULL\n");
        return NULL;
    }
    // 2) coordinatesOffset must be real-valued, get its reduced dimensionality
    if (SIQuantityIsComplexType((SIQuantityRef)coordinatesOffset)) {
        fprintf(stderr, "SIDimensionCreate: coordinatesOffset must be real-valued\n");
        return NULL;
    }
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coordinatesOffset);
    // 3) quantityName must exist and match refDim
    {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim = SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim ||
            !SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
            fprintf(stderr, "SIDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                    OCStringGetCStringPtr(quantityName));
            if (err) OCRelease(err);
            return NULL;
        }
        OCRelease(err);
    }
    // 4) originOffset: default to 0×unit if NULL, else must be real & same dimensionality
    bool originWasDefaulted = false;
    SIScalarRef tmpZero = NULL;
    if (originOffset) {
        if (SIQuantityIsComplexType((SIQuantityRef)originOffset)) {
            fprintf(stderr, "SIDimensionCreate: originOffset must be real-valued\n");
            return NULL;
        }
        if (!SIDimensionalityHasSameReducedDimensionality(
                refDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)originOffset))) {
            fprintf(stderr, "SIDimensionCreate: originOffset dimensionality mismatch\n");
            return NULL;
        }
    } else {
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)coordinatesOffset);
        tmpZero = SIScalarCreateWithDouble(0.0, unit);
        if (!tmpZero) {
            fprintf(stderr, "SIDimensionCreate: failed to create zero origin\n");
            return NULL;
        }
        originOffset = tmpZero;
        originWasDefaulted = true;
    }
    // 5) period: only enforced if periodic==true
    if (periodic) {
        if (!period) {
            fprintf(stderr, "SIDimensionCreate: periodic==true requires non-NULL period\n");
            if (originWasDefaulted) OCRelease(tmpZero);
            return NULL;
        }
        if (SIQuantityIsComplexType((SIQuantityRef)period) ||
            !SIDimensionalityHasSameReducedDimensionality(
                refDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)period))) {
            fprintf(stderr, "SIDimensionCreate: period dimensionality mismatch or complex\n");
            if (originWasDefaulted) OCRelease(tmpZero);
            return NULL;
        }
    } else {
        // ignore any incoming period when not periodic
        period = NULL;
    }
    // 6) Allocate
    SIDimensionRef dim = SIDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "SIDimensionCreate: allocation failed\n");
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    // 7) Base fields (label/description/metaData)
    if (label && !DimensionSetLabel((DimensionRef)dim, label)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    if (description && !DimensionSetDescription((DimensionRef)dim, description)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    if (metaData && !DimensionSetMetaData((DimensionRef)dim, metaData)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    // 8) quantityName
    dim->quantityName = OCStringCreateCopy(quantityName);
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionCreate: failed to copy quantityName\n");
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    // 9) coordinatesOffset, originOffset, period
    dim->coordinatesOffset = SIScalarCreateCopy(coordinatesOffset);
    if (!dim->coordinatesOffset) {
        fprintf(stderr, "SIDimensionCreate: failed to copy coordinatesOffset\n");
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    dim->originOffset = SIScalarCreateCopy(originOffset);
    if (!dim->originOffset) {
        fprintf(stderr, "SIDimensionCreate: failed to copy originOffset\n");
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    if (period) {
        dim->period = SIScalarCreateCopy(period);
        if (!dim->period) {
            fprintf(stderr, "SIDimensionCreate: failed to copy period\n");
            OCRelease(dim);
            if (originWasDefaulted) OCRelease(tmpZero);
            return NULL;
        }
    }
    // release our temporary zero-origin if we made one
    if (originWasDefaulted) {
        OCRelease(tmpZero);
    }
    // 10) flags
    dim->periodic = periodic;
    dim->scaling = scaling;
    return dim;
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
    OCRelease(error);
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
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionSetQuantityName: failed to copy name\n");
        return false;
    }
    // 5) Ensure originOffset still matches; if not, reset to zero in coords’ unit
    if (dim->originOffset) {
        SIDimensionalityRef origDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->originOffset);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->originOffset);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)coords);
            dim->originOffset = SIScalarCreateWithDouble(0.0, u);
        }
    }
    // 6) If we were periodic, make sure period still matches; otherwise clear it
    if (dim->periodic && dim->period) {
        SIDimensionalityRef perDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->period);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
            OCRelease(dim->period);
            dim->period = NULL;
            dim->periodic = false;
        }
    }
    return true;
}
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim) {
    return dim ? dim->coordinatesOffset : NULL;
}
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim, SIScalarRef val) {
    if (!dim || !val) {
        fprintf(stderr, "SIDimensionSetCoordinatesOffset: dim and val must be non-NULL\n");
        return false;
    }
    // 1) No complex values allowed
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        fprintf(stderr, "SIDimensionSetCoordinatesOffset: val must be real-valued\n");
        return false;
    }
    // 2) Find dimensionality of our quantityName
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(dim->quantityName, &err);
    if (!nameDim) {
        fprintf(stderr,
                "SIDimensionSetCoordinatesOffset: invalid quantityName \"%s\"\n",
                err ? OCStringGetCStringPtr(err)
                    : OCStringGetCStringPtr(dim->quantityName));
        if (err) OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 3) Check that val’s dimensionality matches
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, valDim)) {
        fprintf(stderr,
                "SIDimensionSetCoordinatesOffset: dimensionality mismatch for \"%s\"\n",
                OCStringGetCStringPtr(dim->quantityName));
        return false;
    }
    // 4) Deep-copy & swap in the new coordinatesOffset
    SIScalarRef newCoords = SIScalarCreateCopy(val);
    if (!newCoords) {
        fprintf(stderr, "SIDimensionSetCoordinatesOffset: failed to copy scalar\n");
        return false;
    }
    OCRelease(dim->coordinatesOffset);
    dim->coordinatesOffset = newCoords;
    // 5) If we have an originOffset that no longer matches, reset it to zero
    if (dim->originOffset) {
        SIDimensionalityRef origDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->originOffset);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->originOffset);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)newCoords);
            dim->originOffset = SIScalarCreateWithDouble(0.0, u);
        }
    }
    // 6) If we were periodic but the old period no longer matches, clear it
    if (dim->periodic && dim->period) {
        SIDimensionalityRef perDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->period);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
            OCRelease(dim->period);
            dim->period = NULL;
            dim->periodic = false;
        }
    }
    return true;
}
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim) {
    // Return NULL if dim is invalid
    return dim ? dim->originOffset : NULL;
}
bool SIDimensionSetOriginOffset(SIDimensionRef dim, SIScalarRef val) {
    if (!dim || !val) {
        fprintf(stderr, "SIDimensionSetOriginOffset: dim and val must be non-NULL\n");
        return false;
    }
    // Reject complex‐valued scalars
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        fprintf(stderr, "SIDimensionSetOriginOffset: val must be real-valued\n");
        return false;
    }
    // Need a reference coordinatesOffset to validate against
    SIScalarRef coords = dim->coordinatesOffset;
    if (!coords) {
        fprintf(stderr, "SIDimensionSetOriginOffset: cannot validate without coordinatesOffset\n");
        return false;
    }
    // Both must share the same reduced dimensionality
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(refDim, valDim)) {
        fprintf(stderr,
                "SIDimensionSetOriginOffset: dimensionality mismatch with coordinatesOffset\n");
        return false;
    }
    // Deep‐copy & swap in the new originOffset
    SIScalarRef copy = SIScalarCreateCopy(val);
    if (!copy) {
        fprintf(stderr, "SIDimensionSetOriginOffset: failed to copy scalar\n");
        return false;
    }
    OCRelease(dim->originOffset);
    dim->originOffset = copy;
    return true;
}
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim) {
    // Always return the stored period (even if periodic == false),
    // so it can be re-enabled later without losing the old value.
    return dim ? dim->period : NULL;
}
bool SIDimensionSetPeriod(SIDimensionRef dim, SIScalarRef val) {
    if (!dim || !val) {
        fprintf(stderr, "SIDimensionSetPeriod: dim and val must be non-NULL\n");
        return false;
    }
    // 1) Reject complex-valued scalars
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        fprintf(stderr, "SIDimensionSetPeriod: val must be real-valued\n");
        return false;
    }
    // 2) Ensure it matches our quantityName dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(dim->quantityName, &err);
    if (!nameDim) {
        fprintf(stderr,
                "SIDimensionSetPeriod: invalid quantityName \"%s\"\n",
                err ? OCStringGetCStringPtr(err)
                    : OCStringGetCStringPtr(dim->quantityName));
        OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 3) Compare reduced dimensionalities
    SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
        fprintf(stderr,
                "SIDimensionSetPeriod: dimensionality mismatch for \"%s\"\n",
                OCStringGetCStringPtr(dim->quantityName));
        return false;
    }
    // 4) If it's the same object, just enable periodicity
    if (dim->period == val) {
        dim->periodic = true;  // ensure the flag tracks the value
        return true;
    }
    // 5) Convert & deep‐copy into our “relative” unit
    SIUnitRef relUnit = SIDimensionGetRelativeUnit((DimensionRef)dim);
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(val, relUnit, NULL);
    if (!copy) {
        fprintf(stderr, "SIDimensionSetPeriod: conversion to relative unit failed\n");
        return false;
    }
    // 6) Normalize element type
    SIScalarSetElementType((SIMutableScalarRef)copy, kSINumberFloat64Type);
    // 7) Swap in the new period value
    OCRelease(dim->period);
    dim->period = copy;
    // 8) Implicitly turn on periodicity whenever a valid period is set
    dim->periodic = true;
    return true;
}
bool SIDimensionIsPeriodic(SIDimensionRef dim) {
    // Simplify: just guard and return the flag
    return dim && dim->periodic;
}
bool SIDimensionSetPeriodic(SIDimensionRef dim, bool flag) {
    if (!dim) return false;
    if (flag) {
        // Enabling periodic requires a non-NULL period
        if (!dim->period) {
            fprintf(stderr,
                    "SIDimensionSetPeriodic: can't enable periodicity without a period\n");
            return false;
        }
        dim->periodic = true;
    } else {
        // Disabling periodic leaves the period value intact for future re-enablement
        dim->periodic = false;
    }
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
SIDimensionRef
SIDimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    // 1) Base Dimension fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef meta = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metaData"));
    // 2) SIDimension‐specific
    OCStringRef quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR("quantityName"));
    SIScalarRef coordinatesOffset = (SIScalarRef)OCDictionaryGetValue(dict, STR("coordinatesOffset"));
    SIScalarRef originOffset = (SIScalarRef)OCDictionaryGetValue(dict, STR("originOffset"));
    SIScalarRef period = (SIScalarRef)OCDictionaryGetValue(dict, STR("period"));
    // boolean “periodic” flag
    OCBooleanRef periodicObj = (OCBooleanRef)OCDictionaryGetValue(dict, STR("periodic"));
    bool periodic = periodicObj ? OCBooleanGetValue(periodicObj) : false;
    // numeric “scaling” enum
    OCNumberRef scalingNum = (OCNumberRef)OCDictionaryGetValue(dict, STR("scaling"));
    dimensionScaling scaling = scalingNum
                                   ? (dimensionScaling)OCNumberGetInt(scalingNum)
                                   : kDimensionScalingNone;
    // 3) Delegate to our validated constructor
    return SIDimensionCreate(
        label,
        description,
        meta,
        quantityName,
        coordinatesOffset,
        originOffset,
        period,
        periodic,
        scaling);
}
OCDictionaryRef
SIDimensionCopyAsDictionary(SIDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with base‐class serialization
    OCDictionaryRef dict = DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) quantityName
    OCStringRef qty = SIDimensionGetQuantityName(dim);
    if (qty) OCDictionarySetValue(dict, STR("quantityName"), qty);
    // 3) coordinatesOffset
    if (dim->coordinatesOffset) {
        SIScalarRef copy = SIScalarCreateCopy(dim->coordinatesOffset);
        if (copy) {
            OCDictionarySetValue(dict, STR("coordinatesOffset"), copy);
            OCRelease(copy);
        }
    }
    // 4) originOffset
    if (dim->originOffset) {
        SIScalarRef copy = SIScalarCreateCopy(dim->originOffset);
        if (copy) {
            OCDictionarySetValue(dict, STR("originOffset"), copy);
            OCRelease(copy);
        }
    }
    // 5) period (only if one exists)
    if (dim->period) {
        SIScalarRef copy = SIScalarCreateCopy(dim->period);
        if (copy) {
            OCDictionarySetValue(dict, STR("period"), copy);
            OCRelease(copy);
        }
    }
    // 6) periodic flag
    OCBooleanRef b = OCBooleanCreate(SIDimensionIsPeriodic(dim));
    OCDictionarySetValue(dict, STR("periodic"), b);
    OCRelease(b);
    // 7) scaling enum
    OCNumberRef num = OCNumberCreateInt((int)SIDimensionGetScaling(dim));
    OCDictionarySetValue(dict, STR("scaling"), num);
    OCRelease(num);
    return dict;
}
#pragma endregion SIDimension
