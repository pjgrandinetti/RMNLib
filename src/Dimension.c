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
static OCTypeID kDimensionID = kOCNotATypeID;
struct impl_Dimension {
    //  Dimension
    OCBase base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metadata;
};
static void impl_InitBaseDimensionFields(DimensionRef dim) {
    dim->label = STR("");
    dim->description = STR("");
    dim->metadata = OCDictionaryCreateMutable(0);
}
OCTypeID DimensionGetTypeID(void) {
    if (kDimensionID == kOCNotATypeID)
        kDimensionID = OCRegisterType("Dimension");
    return kDimensionID;
}
static bool impl_DimensionEqual(const void *a, const void *b) {
    const DimensionRef dimA = (const DimensionRef)a;
    const DimensionRef dimB = (const DimensionRef)b;
    if (!dimA || !dimB) return false;
    return OCTypeEqual(dimA->label, dimB->label) &&
           OCTypeEqual(dimA->description, dimB->description) &&
           OCTypeEqual(dimA->metadata, dimB->metadata);
}
static void impl_DimensionFinalize(const void *obj) {
    DimensionRef dim = (DimensionRef)obj;
    if (!dim) return;
    OCRelease(dim->label);
    OCRelease(dim->description);
    OCRelease(dim->metadata);
    dim->label = NULL;
    dim->description = NULL;
    dim->metadata = NULL;
}
static OCStringRef impl_DimensionCopyFormattingDesc(OCTypeRef cf) {
    DimensionRef dim = (DimensionRef)cf;
    if (!dim) {
        return OCStringCreateWithCString("<Dimension: NULL>");
    }
    // Pull via getters so subclasses aren’t bypassed
    OCStringRef lbl = DimensionGetLabel(dim);
    OCStringRef desc = DimensionGetDescription(dim);
    if (!lbl || OCStringGetLength(lbl) == 0) {
        lbl = STR("(no label)");
    }
    if (!desc || OCStringGetLength(desc) == 0) {
        desc = STR("(no description)");
    }
    OCStringRef out = OCStringCreateWithFormat(
        STR("<Dimension label=\"%@\" description=\"%@\">"),
        lbl, desc);
    return out;
}

static DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict);
static OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim);

static void *impl_DimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = impl_DimensionCopyAsDictionary((DimensionRef) obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    DimensionRef copy = impl_DimensionCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
static DimensionRef impl_DimensionAllocate(void) {
    DimensionRef dim = OCTypeAlloc(
        struct impl_Dimension,
        DimensionGetTypeID(),
        impl_DimensionFinalize,
        impl_DimensionEqual,
        impl_DimensionCopyFormattingDesc,
        impl_DimensionDeepCopy,
        impl_DimensionDeepCopy);
    if (!dim) return NULL;
    impl_InitBaseDimensionFields(dim);
    return dim;
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
OCDictionaryRef DimensionGetMetadata(DimensionRef dim) {
    return dim ? dim->metadata : NULL;
}
bool DimensionSetMetadata(DimensionRef dim, OCDictionaryRef dict) {
    if (!dim) return false;
    // Always end up with a valid dictionary (never NULL)
    OCDictionaryRef dictCopy;
    if (dict) {
        dictCopy = (OCDictionaryRef)OCTypeDeepCopy(dict);
        if (!dictCopy) {
            fprintf(stderr, "DimensionSetMetadata: failed to copy metadata dictionary\n");
            return false;
        }
    } else {
        dictCopy = OCDictionaryCreateMutable(0);
        if (!dictCopy) {
            fprintf(stderr, "DimensionSetMetadata: failed to create empty metadata dictionary\n");
            return false;
        }
    }
    // Swap in the new dictionary
    OCRelease(dim->metadata);
    dim->metadata = dictCopy;
    return true;
}
static DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    // Pull values out of the dictionary and cast them to the expected types
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    DimensionRef dim = impl_DimensionAllocate();
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
    if (metadata && !DimensionSetMetadata(dim, metadata)) {
        OCRelease(dim);
        return NULL;
    }
    return dim;
}
static OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim) {
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
        OCDictionarySetValue((OCMutableDictionaryRef) dict, STR("label"), lblCopy);
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
        OCDictionarySetValue((OCMutableDictionaryRef) dict, STR("description"), descCopy);
        OCRelease(descCopy);
    }
    // Deep-copy the metadata dictionary
    OCDictionaryRef meta = DimensionGetMetadata(dim);
    if (meta) {
        OCDictionaryRef metaCopy = (OCDictionaryRef)OCTypeDeepCopy((OCTypeRef)meta);
        if (!metaCopy) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue((OCMutableDictionaryRef) dict, STR("metadata"), metaCopy);
        OCRelease(metaCopy);
    }
    return dict;
}
#pragma endregion Dimension
#pragma region LabeledDimension
// ============================================================================
// MARK: - (3) LabeledDimension
// ============================================================================
static OCTypeID kLabeledDimensionID = kOCNotATypeID;
typedef struct impl_LabeledDimension {
    struct impl_Dimension _super;  // <-- inherit all base fields
    OCMutableArrayRef coordinateLabels;
} *LabeledDimensionRef;
OCTypeID LabeledDimensionGetTypeID(void) {
    if (kLabeledDimensionID == kOCNotATypeID)
        kLabeledDimensionID = OCRegisterType("LabeledDimension");
    return kLabeledDimensionID;
}
static bool impl_LabeledDimensionEqual(const void *a, const void *b) {
    const LabeledDimensionRef dimA = (const LabeledDimensionRef)a;
    const LabeledDimensionRef dimB = (const LabeledDimensionRef)b;
    if (!dimA || !dimB)
        return false;
    // Compare base Dimension fields via the embedded _super
    if (!impl_DimensionEqual((const DimensionRef)&dimA->_super,
                          (const DimensionRef)&dimB->_super))
        return false;
    // Compare LabeledDimension-specific field
    return OCTypeEqual(dimA->coordinateLabels, dimB->coordinateLabels);
}
static void impl_LabeledDimensionFinalize(const void *obj) {
    const LabeledDimensionRef dim = (const LabeledDimensionRef)obj;
    // Finalize only the base part:
    impl_DimensionFinalize((DimensionRef)&dim->_super);
    // Then clean up subclass fields:
    OCRelease(dim->coordinateLabels);
    /* dim->coordinateLabels = NULL;  // not strictly needed after finalize */
}
static OCStringRef impl_LabeledDimensionCopyFormattingDesc(OCTypeRef cf) {
    const LabeledDimensionRef dim = (const LabeledDimensionRef)cf;
    if (!dim) {
        return STR("<LabeledDimension: NULL>");
    }
    // Base‐class fields via getters
    OCStringRef lbl = DimensionGetLabel((DimensionRef)dim);
    OCStringRef desc = DimensionGetDescription((DimensionRef)dim);
    if (!lbl || OCStringGetLength(lbl) == 0) {
        lbl = STR("(no label)");
    }
    if (!desc || OCStringGetLength(desc) == 0) {
        desc = STR("(no description)");
    }
    // LabeledDimension‐specific
    OCIndex count = dim->coordinateLabels
                        ? OCArrayGetCount(dim->coordinateLabels)
                        : 0;
    OCStringRef fmt = OCStringCreateWithFormat(
        STR("<LabeledDimension label=\"%@\" description=\"%@\" coordinateLabelCount=%ld>"),
        lbl,
        desc,
        (long)count);
    return fmt;
}
static void *impl_LabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = LabeledDimensionCopyAsDictionary((LabeledDimensionRef) obj);
    if (!dict) return NULL;
    LabeledDimensionRef copy = LabeledDimensionCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
static LabeledDimensionRef LabeledDimensionAllocate(void) {
    LabeledDimensionRef obj = OCTypeAlloc(
        struct impl_LabeledDimension,
        LabeledDimensionGetTypeID(),
        impl_LabeledDimensionFinalize,
        impl_LabeledDimensionEqual,
        impl_LabeledDimensionCopyFormattingDesc,
        impl_LabeledDimensionDeepCopy,
        impl_LabeledDimensionDeepCopy);
    if (!obj) return NULL;

    impl_InitBaseDimensionFields((DimensionRef)obj);
    obj->coordinateLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!obj->coordinateLabels) {
        OCRelease(obj);
        return NULL;
    }
    return obj;
}
LabeledDimensionRef
LabeledDimensionCreate(OCStringRef label, OCStringRef description, OCDictionaryRef metadata, OCArrayRef coordinateLabels) {
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
    if (metadata) {
        if (!DimensionSetMetadata((DimensionRef)dim, metadata)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if(coordinateLabels) {
        if(!LabeledDimensionSetCoordinateLabels(dim, coordinateLabels)) {
            OCRelease(dim);
            return NULL;
        }

    }
    return dim;
}
LabeledDimensionRef
LabeledDimensionCreateWithCoordinateLabels(OCArrayRef coordinateLabels) {
    return LabeledDimensionCreate(NULL, 
            NULL, NULL, coordinateLabels);
}
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim) {
    return dim ? dim->coordinateLabels : NULL;
}
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim, OCArrayRef coordinateLabels) {
    if (!dim || !coordinateLabels)
        return false;
    if (dim->coordinateLabels == coordinateLabels)
        return true;
    if (OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr, "LabeledDimensionSetCoordinateLabels: need ≥2 coordinate labels\n");
        return false;
    }
    OCMutableArrayRef coordLabelsCopy = (OCMutableArrayRef)OCTypeDeepCopy((OCTypeRef)coordinateLabels);
    if (!coordLabelsCopy) {
        fprintf(stderr, "LabeledDimensionSetCoordinateLabels: failed to deep-copy coordinate labels\n");
        return false;
    }
    OCRelease(dim->coordinateLabels);
    dim->coordinateLabels = coordLabelsCopy;
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
    OCDictionaryRef metadata =
        (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    if (metadata && !DimensionSetMetadata((DimensionRef)dim, metadata)) goto Err;
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
    OCDictionaryRef dict = impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // Grab our labels, deep-copy them, stick them in under "labels"
    OCArrayRef labels = LabeledDimensionGetCoordinateLabels(dim);
    if (labels) {
        OCArrayRef labelsCopy = (OCArrayRef)OCTypeDeepCopy((OCTypeRef)labels);
        if (labelsCopy) {
            OCDictionarySetValue((OCMutableDictionaryRef) dict, STR("labels"), labelsCopy);
            OCRelease(labelsCopy);
        }
    }
    return dict;
}
#pragma endregion LabeledDimension
#pragma region SIDimension
// ============================================================================
// MARK: - (4) SIDimension
// ============================================================================
static OCTypeID kSIDimensionID = kOCNotATypeID;
typedef struct impl_SIDimension {
    struct impl_Dimension _super;  // inherit all base‐Dimension fields
    OCStringRef quantityName;
    SIScalarRef offset;
    SIScalarRef origin;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
} *SIDimensionRef;
OCTypeID SIDimensionGetTypeID(void) {
    if (kSIDimensionID == kOCNotATypeID)
        kSIDimensionID = OCRegisterType("SIDimension");
    return kSIDimensionID;
}
static bool impl_SIDimensionEqual(const void *a, const void *b) {
    const SIDimensionRef dimA = (const SIDimensionRef)a;
    const SIDimensionRef dimB = (const SIDimensionRef)b;
    if (!dimA || !dimB) return false;
    // 1) Base‐class fields
    if (!impl_DimensionEqual((const DimensionRef)&dimA->_super,
                          (const DimensionRef)&dimB->_super))
        return false;
    // 2) quantityName
    if (!OCTypeEqual(dimA->quantityName, dimB->quantityName))
        return false;
    // 3) coordinate & origin offsets
    if (!OCTypeEqual(dimA->offset, dimB->offset))
        return false;
    if (!OCTypeEqual(dimA->origin, dimB->origin))
        return false;
    // 4) periodic flag
    if (dimA->periodic != dimB->periodic)
        return false;
    // 5) if periodic, both must have a non‐NULL period and be equal
    if (dimA->periodic) {
        if (!dimA->period || !dimB->period)
            return false;
        if (!OCTypeEqual(dimA->period, dimB->period))
            return false;
    }
    // 6) scaling mode
    if (dimA->scaling != dimB->scaling)
        return false;
    return true;
}
static void impl_SIDimensionFinalize(const void *obj) {
    if (!obj) return;
    SIDimensionRef dim = (SIDimensionRef)obj;
    impl_DimensionFinalize((DimensionRef)&dim->_super);
    OCRelease(dim->quantityName);
    dim->quantityName = NULL;
    OCRelease(dim->offset);
    dim->offset = NULL;
    OCRelease(dim->origin);
    dim->origin = NULL;
    OCRelease(dim->period);
    dim->period = NULL;
}
static OCStringRef impl_SIDimensionCopyFormattingDesc(OCTypeRef cf) {
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
    OCStringRef qty = SIDimensionGetQuantity(d);
    const char *periodic_str = SIDimensionIsPeriodic(d) ? "true" : "false";
    int scaling_mode = (int)SIDimensionGetScaling(d);
    if (!qty || OCStringGetLength(qty) == 0) qty = STR("(no quantity name)");
    // Scalars: reference offset, origin offset, and (optional) period
    SIScalarRef offset = SIDimensionGetOffset(d);
    SIScalarRef origin = SIDimensionGetOrigin(d);
    SIScalarRef period = SIDimensionGetPeriod(d);
    // Assume each SIScalar has a copy‐formatting routine
    OCStringRef refStr = offset
                             ? SIScalarCreateStringValue(offset)
                             : STR("(no offset)");
    OCStringRef origStr = origin
                              ? SIScalarCreateStringValue(origin)
                              : STR("(no origin)");
    OCStringRef periodStr = (periodic_str[0] == 't' && period)
                                ? SIScalarCreateStringValue(period)
                                : STR("(n/a)");
    OCStringRef fmt = OCStringCreateWithFormat(
        STR("<SIDimension label=\"%@\" desc=\"%@\" qty=\"%@\" offset=%@ origin=%@ period=%@ periodic=%s scaling=%d>"),
        lbl, desc, qty, refStr, origStr, periodStr, periodic_str, scaling_mode);
    OCRelease(refStr);
    OCRelease(origStr);
    OCRelease(periodStr);
    return fmt;
}
static void *impl_SIDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = SIDimensionCopyAsDictionary((SIDimensionRef) obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    SIDimensionRef copy = SIDimensionCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
static void impl_InitSIDimensionFields(SIDimensionRef dim) {
    // Default quantity name: dimensionless
    dim->quantityName = STR("dimensionless");
    // Use the unit‐less SI unit for all default scalars
    SIUnitRef u = SIUnitDimensionlessAndUnderived();
    // Default coordinate offset & origin both zero in the same unit
    dim->offset = SIScalarCreateWithDouble(0.0, u);
    dim->origin = SIScalarCreateWithDouble(0.0, u);
    // No period until the user explicitly makes it periodic
    dim->period = NULL;
    dim->periodic = false;
    dim->scaling = kDimensionScalingNone;
}
static SIDimensionRef SIDimensionAllocate(void) {
    SIDimensionRef obj = OCTypeAlloc(
        struct impl_SIDimension,
        SIDimensionGetTypeID(),
        impl_SIDimensionFinalize,
        impl_SIDimensionEqual,
        impl_SIDimensionCopyFormattingDesc,
        impl_SIDimensionDeepCopy,
        impl_SIDimensionDeepCopy);
    impl_InitBaseDimensionFields((DimensionRef)&obj->_super);
    impl_InitSIDimensionFields(obj);
    return obj;
}
/// Returns true if `dim` passes *all* of the same checks that SIDimensionCreate performs.
/// On failure, returns false and (optionally) writes a human-readable error into *outErr.
bool SIDimensionValidate(SIDimensionRef dim, OCStringRef *outErr) {
    if (!dim) {
        if (outErr) *outErr = STR("dimension is NULL");
        return false;
    }
    // 1) quantityName must be non-NULL and known
    OCStringRef qty = SIDimensionGetQuantity(dim);
    if (!qty || OCStringGetLength(qty) == 0) {
        if (outErr) *outErr = STR("quantity name is empty");
        return false;
    }
    OCStringRef err = NULL;
    SIDimensionalityRef qDim = SIDimensionalityForQuantity(qty, &err);
    if (!qDim) {
        if (outErr)
            *outErr = err;
        else if (err)
            OCRelease(err);
        return false;
    }
    // 2) offset must be non-NULL, real, same dimensionality
    SIScalarRef off = SIDimensionGetOffset(dim);
    if (!off) {
        if (outErr) *outErr = STR("offset scalar is NULL");
        return false;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)off)) {
        if (outErr) *outErr = STR("offset is complex-valued");
        return false;
    }
    SIDimensionalityRef offDim = SIQuantityGetUnitDimensionality((SIQuantityRef)off);
    if (!SIDimensionalityHasSameReducedDimensionality(qDim, offDim)) {
        if (outErr) *outErr = STR("quantity name and offset dimensionality mismatch");
        return false;
    }
    // 3) origin must match offset dimensionality (or be defaulted to zero)
    SIScalarRef org = SIDimensionGetOrigin(dim);
    if (org) {
        if (SIQuantityIsComplexType((SIQuantityRef)org)) {
            if (outErr) *outErr = STR("origin is complex-valued");
            return false;
        }
        SIDimensionalityRef orgDim = SIQuantityGetUnitDimensionality((SIQuantityRef)org);
        if (!SIDimensionalityHasSameReducedDimensionality(offDim, orgDim)) {
            if (outErr) *outErr = STR("origin and offset dimensionality mismatch");
            return false;
        }
    }
    // 4) if periodic, period must be non-NULL, real, same dimensionality
    if (SIDimensionIsPeriodic(dim)) {
        SIScalarRef per = SIDimensionGetPeriod(dim);
        if (!per) {
            if (outErr) *outErr = STR("periodic but period is NULL");
            return false;
        }
        if (SIQuantityIsComplexType((SIQuantityRef)per)) {
            if (outErr) *outErr = STR("period is complex-valued");
            return false;
        }
        SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)per);
        if (!SIDimensionalityHasSameReducedDimensionality(offDim, perDim)) {
            if (outErr) *outErr = STR("period and offset dimensionality mismatch");
            return false;
        }
    }
    // 5) scaling is always valid (just an enum)
    return true;
}
static bool impl_InitSIDimensionFieldsFromArgs(
    SIDimensionRef dim,
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling) {
    DimensionRef baseDim = (DimensionRef) dim;
    return DimensionSetLabel(baseDim, label) && DimensionSetDescription(baseDim, description) && DimensionSetMetadata(baseDim, metadata) && SIDimensionSetQuantity(dim, quantityName) && SIDimensionSetOffset(dim, offset) && SIDimensionSetOrigin(dim, origin) && (!periodic || SIDimensionSetPeriod(dim, period)) && SIDimensionSetPeriodic(dim, periodic) && SIDimensionSetScaling(dim, scaling);
}
SIDimensionRef
SIDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling) {
    // 1) Required: quantityName + offset
    if (!quantityName) {
        fprintf(stderr, "SIDimensionCreate: quantity name must be non-NULL\n");
        return NULL;
    }
    if (!offset) {
        fprintf(stderr, "SIDimensionCreate: offset must be non-NULL\n");
        return NULL;
    }
    // 2) offset must be real-valued, get its reduced dimensionality
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr, "SIDimensionCreate: offset must be real-valued\n");
        return NULL;
    }
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    // 3) quantityName must exist and match refDim
    {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim = SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim ||
            !SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
            fprintf(stderr, "SIDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                    OCStringGetCString(quantityName));
            if (err) OCRelease(err);
            return NULL;
        }
        OCRelease(err);
    }
    // 4) origin: default to 0×unit if NULL, else must be real & same dimensionality
    bool originWasDefaulted = false;
    SIScalarRef tmpZero = NULL;
    if (origin) {
        if (SIQuantityIsComplexType((SIQuantityRef)origin)) {
            fprintf(stderr, "SIDimensionCreate: origin must be real-valued\n");
            return NULL;
        }
        if (!SIDimensionalityHasSameReducedDimensionality(
                refDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)origin))) {
            fprintf(stderr, "SIDimensionCreate: origin dimensionality mismatch\n");
            return NULL;
        }
    } else {
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)offset);
        tmpZero = SIScalarCreateWithDouble(0.0, unit);
        if (!tmpZero) {
            fprintf(stderr, "SIDimensionCreate: failed to create zero origin\n");
            return NULL;
        }
        origin = tmpZero;
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
    // 7) Base fields (label/description/metadata)
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
    if (metadata && !DimensionSetMetadata((DimensionRef)dim, metadata)) {
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
    // 9) offset, origin, period
    dim->offset = SIScalarCreateCopy(offset);
    if (!dim->offset) {
        fprintf(stderr, "SIDimensionCreate: failed to copy offset\n");
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    dim->origin = SIScalarCreateCopy(origin);
    if (!dim->origin) {
        fprintf(stderr, "SIDimensionCreate: failed to copy origin\n");
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
OCStringRef SIDimensionGetQuantity(SIDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
bool SIDimensionSetQuantity(SIDimensionRef dim, OCStringRef name) {
    if (!dim || !name) {
        fprintf(stderr, "SIDimensionSetQuantity: name must be non-NULL\n");
        return false;
    }
    // 1) Look up the dimensionality for the requested quantityName name.
    OCStringRef error = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(name, &error);
    if (!nameDim) {
        if (error) {
            fprintf(stderr, "SIDimensionSetQuantity: %s\n",
                    OCStringGetCString(error));
            OCRelease(error);
        } else {
            fprintf(stderr, "SIDimensionSetQuantity: unknown quantityName \"%s\"\n",
                    OCStringGetCString(name));
        }
        return false;
    }
    OCRelease(error);
    // 2) Get the dimensionality of our offset.
    SIScalarRef coords = dim->offset;
    if (!coords) {
        fprintf(stderr, "SIDimensionSetQuantity: cannot validate without offset\n");
        return false;
    }
    SIDimensionalityRef refDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    // 3) Compare reduced dimensionalities.
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
        fprintf(stderr,
                "SIDimensionSetQuantity: dimensionality mismatch between \"%s\" and existing unit\n",
                OCStringGetCString(name));
        return false;
    }
    // 4) All good — replace the old name.
    OCRelease(dim->quantityName);
    dim->quantityName = OCStringCreateCopy(name);
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionSetQuantity: failed to copy name\n");
        return false;
    }
    // 5) Ensure origin still matches; if not, reset to zero in coords’ unit
    if (dim->origin) {
        SIDimensionalityRef origDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->origin);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->origin);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)coords);
            dim->origin = SIScalarCreateWithDouble(0.0, u);
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
SIScalarRef SIDimensionGetOffset(SIDimensionRef dim) {
    return dim ? dim->offset : NULL;
}
bool SIDimensionSetOffset(SIDimensionRef dim, SIScalarRef val) {
    if (!dim || !val) {
        fprintf(stderr, "SIDimensionSetOffset: dim and val must be non-NULL\n");
        return false;
    }
    // 1) No complex values allowed
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        fprintf(stderr, "SIDimensionSetOffset: val must be real-valued\n");
        return false;
    }
    // 2) Find dimensionality of our quantityName
    OCStringRef err = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(dim->quantityName, &err);
    if (!nameDim) {
        fprintf(stderr,
                "SIDimensionSetOffset: invalid quantityName \"%s\"\n",
                err ? OCStringGetCString(err)
                    : OCStringGetCString(dim->quantityName));
        if (err) OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 3) Check that val’s dimensionality matches
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, valDim)) {
        fprintf(stderr,
                "SIDimensionSetOffset: dimensionality mismatch for \"%s\"\n",
                OCStringGetCString(dim->quantityName));
        return false;
    }
    // 4) Deep-copy & swap in the new offset
    SIScalarRef newCoords = SIScalarCreateCopy(val);
    if (!newCoords) {
        fprintf(stderr, "SIDimensionSetOffset: failed to copy scalar\n");
        return false;
    }
    OCRelease(dim->offset);
    dim->offset = newCoords;
    // 5) If we have an origin that no longer matches, reset it to zero
    if (dim->origin) {
        SIDimensionalityRef origDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)dim->origin);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, origDim)) {
            OCRelease(dim->origin);
            SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)newCoords);
            dim->origin = SIScalarCreateWithDouble(0.0, u);
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
SIScalarRef SIDimensionGetOrigin(SIDimensionRef dim) {
    // Return NULL if dim is invalid
    return dim ? dim->origin : NULL;
}
bool SIDimensionSetOrigin(SIDimensionRef dim, SIScalarRef val) {
    if (!dim || !val) {
        fprintf(stderr, "SIDimensionSetOrigin: dim and val must be non-NULL\n");
        return false;
    }
    // Reject complex‐valued scalars
    if (SIQuantityIsComplexType((SIQuantityRef)val)) {
        fprintf(stderr, "SIDimensionSetOrigin: val must be real-valued\n");
        return false;
    }
    // Need a reference offset to validate against
    SIScalarRef coords = dim->offset;
    if (!coords) {
        fprintf(stderr, "SIDimensionSetOrigin: cannot validate without offset\n");
        return false;
    }
    // Both must share the same reduced dimensionality
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    SIDimensionalityRef valDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(refDim, valDim)) {
        fprintf(stderr,
                "SIDimensionSetOrigin: dimensionality mismatch with offset\n");
        return false;
    }
    // Deep‐copy & swap in the new origin
    SIScalarRef copy = SIScalarCreateCopy(val);
    if (!copy) {
        fprintf(stderr, "SIDimensionSetOrigin: failed to copy scalar\n");
        return false;
    }
    OCRelease(dim->origin);
    dim->origin = copy;
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
                err ? OCStringGetCString(err)
                    : OCStringGetCString(dim->quantityName));
        OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 3) Compare reduced dimensionalities
    SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)val);
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
        fprintf(stderr,
                "SIDimensionSetPeriod: dimensionality mismatch for \"%s\"\n",
                OCStringGetCString(dim->quantityName));
        return false;
    }
    // 4) If it's the same object, just enable periodicity
    if (dim->period == val) {
        dim->periodic = true;  // ensure the flag tracks the value
        return true;
    }
    // 5) Convert & deep‐copy into our “relative” unit
    SIUnitRef relUnit = SIQuantityGetUnit((SIQuantityRef) dim->offset);
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
    OCDictionaryRef meta = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    // 2) SIDimension‐specific
    OCStringRef quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR("quantity_name"));
    SIScalarRef offset  = (SIScalarRef)OCDictionaryGetValue(dict, STR("offset"));
    SIScalarRef origin  = (SIScalarRef)OCDictionaryGetValue(dict, STR("origin"));
    SIScalarRef period  = (SIScalarRef)OCDictionaryGetValue(dict, STR("period"));
    OCBooleanRef periodicObj = (OCBooleanRef)OCDictionaryGetValue(dict, STR("periodic"));
    bool periodic = periodicObj ? OCBooleanGetValue(periodicObj) : false;
    OCNumberRef scalingNum = (OCNumberRef)OCDictionaryGetValue(dict, STR("scaling"));
    int scalingNumValue = 0;
    (void)OCNumberGetValue(scalingNum, kOCNumberIntType, &scalingNumValue);
    dimensionScaling scaling = scalingNum
                                   ? (dimensionScaling)scalingNumValue
                                   : kDimensionScalingNone;
    // 3) Delegate to our validated constructor
    return SIDimensionCreate(
        label,
        description,
        meta,
        quantityName,
        offset,
        origin,
        period,
        periodic,
        scaling);
}
OCDictionaryRef
SIDimensionCopyAsDictionary(SIDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with base‐class serialization
    OCMutableDictionaryRef dict = (OCMutableDictionaryRef) impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) quantityName
    OCStringRef qty = SIDimensionGetQuantity(dim);
    if (qty) OCDictionarySetValue(dict, STR("quantity_name"), qty);
    // 3) offset
    if (dim->offset) {
        SIScalarRef copy = SIScalarCreateCopy(dim->offset);
        if (copy) {
            OCDictionarySetValue(dict, STR("offset"), copy);
            OCRelease(copy);
        }
    }
    // 4) origin
    if (dim->origin) {
        SIScalarRef copy = SIScalarCreateCopy(dim->origin);
        if (copy) {
            OCDictionarySetValue(dict, STR("origin"), copy);
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
    OCBooleanRef b = OCBooleanGetWithBool(SIDimensionIsPeriodic(dim));
    OCDictionarySetValue(dict, STR("periodic"), b);
    // 7) scaling enum
    OCNumberRef num = OCNumberCreateWithInt((int)SIDimensionGetScaling(dim));
    OCDictionarySetValue(dict, STR("scaling"), num);
    OCRelease(num);
    return (OCDictionaryRef) dict;
}
#pragma endregion SIDimension
#pragma region SIMonotonicDimension
// ============================================================================
// MARK: - (5) SIMonotonicDimension
// ============================================================================
static OCTypeID kSIMonotonicDimensionID = kOCNotATypeID;
typedef struct impl_SIMonotonicDimension {
    struct impl_SIDimension _super;  // ← inherit all Dimension + SI fields
    // SIMonotonicDimension‐specific:
    SIDimensionRef reciprocal;
    OCMutableArrayRef coordinates;  // array of SIScalarRef (≥2 entries)
} *SIMonotonicDimensionRef;
OCTypeID SIMonotonicDimensionGetTypeID(void) {
    if (kSIMonotonicDimensionID == kOCNotATypeID)
        kSIMonotonicDimensionID = OCRegisterType("SIMonotonicDimension");
    return kSIMonotonicDimensionID;
}
static bool impl_SIMonotonicDimensionEqual(const void *a, const void *b) {
    const SIMonotonicDimensionRef A = (const SIMonotonicDimensionRef)a;
    const SIMonotonicDimensionRef B = (const SIMonotonicDimensionRef)b;
    if (!A || !B) return false;
    // 1) compare all SIDimension fields
    if (!impl_SIDimensionEqual((const void *)&A->_super, (const void *)&B->_super))
        return false;
    // 2) reciprocal
    if (!OCTypeEqual(A->reciprocal, B->reciprocal))
        return false;
    // 3) coordinates array
    if (!OCTypeEqual(A->coordinates, B->coordinates))
        return false;
    return true;
}
static void impl_SIMonotonicDimensionFinalize(const void *obj) {
    if (!obj) return;
    SIMonotonicDimensionRef dim = (SIMonotonicDimensionRef)obj;
    // finalize SIDimension‐super
    impl_SIDimensionFinalize((const void *)&dim->_super);
    // then our own
    OCRelease(dim->reciprocal);
    OCRelease(dim->coordinates);
}
static OCStringRef
impl_SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf)
{
    SIMonotonicDimensionRef d = (SIMonotonicDimensionRef)cf;
    if (!d) {
        return STR("<SIMonotonicDimension: NULL>");
    }

    // 1) Grab the base SIDimension description
    OCStringRef base = impl_SIDimensionCopyFormattingDesc(cf);

    // 2) Build a small “yes”/“no” string for reciprocal
    
    OCStringRef recStr = d->reciprocal ? STR("yes") : STR("no");

    // 3) Count the coordinates
    OCIndex count = d->coordinates ? OCArrayGetCount(d->coordinates) : 0;

    // 4) Stitch them together, making sure to close with '>'
    OCStringRef fmt = OCStringCreateWithFormat(
        STR("%@ coordinatesCount=%lu reciprocal=%@>"),
        base,
        (unsigned long)count,
        recStr);

    // 5) Clean up temporaries
    OCRelease(base);
    OCRelease(recStr);

    return fmt;
}
static void *impl_SIMonotonicDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // 1) Serialize to a dictionary
    OCDictionaryRef dict = SIMonotonicDimensionCopyAsDictionary((SIMonotonicDimensionRef) obj);
    if (!dict) return NULL;
    // 2) Rehydrate a new instance
    SIMonotonicDimensionRef copy =
        SIMonotonicDimensionCreateFromDictionary(dict);
    // 3) Clean up
    OCRelease(dict);
    return copy;
}
static SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void) {
    SIMonotonicDimensionRef obj = OCTypeAlloc(
        struct impl_SIMonotonicDimension,
        SIMonotonicDimensionGetTypeID(),
        impl_SIMonotonicDimensionFinalize,
        impl_SIMonotonicDimensionEqual,
        impl_SIMonotonicDimensionCopyFormattingDesc,
        impl_SIMonotonicDimensionDeepCopy,
        impl_SIMonotonicDimensionDeepCopy);
    if (!obj) return NULL;
    // Initialize the Dimension base fields
    impl_InitBaseDimensionFields((DimensionRef)&obj->_super._super);
    // Initialize the SI‐dimension fields
    impl_InitSIDimensionFields((SIDimensionRef)&obj->_super);
    // Set up our subclass fields
    obj->coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!obj->coordinates) {
        OCRelease(obj);
        return NULL;
    }
    obj->reciprocal = NULL;
    return obj;
}
static bool _SIDimensionIsReciprocalOf(SIDimensionRef src, SIDimensionRef rec) {
    if (!src || !rec) return false;
    // 1) Grab the reduced dimensionality of the source’s offset unit
    SIScalarRef srcOffset = SIDimensionGetOffset(src);
    SIDimensionalityRef srcDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)srcOffset);
    // 2) Invert it via "power -1"
    OCStringRef err = NULL;
    SIDimensionalityRef invDim = SIDimensionalityByRaisingToPower(srcDim, -1.0, &err);
    if (!invDim) {
        if (err) {
            fprintf(stderr,
                    "_SIDimensionIsReciprocalOf: failed to invert dimensionality: %s\n",
                    OCStringGetCString(err));
            OCRelease(err);
        }
        return false;
    }
    // 3) Grab the dimensionality of rec’s offset unit
    SIScalarRef recOffset = SIDimensionGetOffset(rec);
    SIDimensionalityRef recDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)recOffset);
    // 4) Compare
    bool match = SIDimensionalityHasSameReducedDimensionality(invDim, recDim);
    OCRelease(invDim);
    return match;
}
SIMonotonicDimensionRef
SIMonotonicDimensionCreate(
    OCStringRef      label,
    OCStringRef      description,
    OCDictionaryRef  metadata,
    OCStringRef      quantityName,
    SIScalarRef      offset,
    SIScalarRef      origin,
    SIScalarRef      period,
    bool             periodic,
    dimensionScaling scaling,
    OCArrayRef       coordinates,
    SIDimensionRef   reciprocal)
{
    // 1) Monotonic-specific: need ≥2 points
    if (!coordinates || OCArrayGetCount(coordinates) < 2) {
        fprintf(stderr,
                "SIMonotonicDimensionCreate: need ≥2 coordinates\n");
        return NULL;
    }

    // 2) Allocate subclass and zero-init
    SIMonotonicDimensionRef dim = SIMonotonicDimensionAllocate();
    if (!dim) return NULL;

    // 2.5) Validate quantityName + offset (lifted from SIDimensionCreate)
    if (!quantityName) {
        fprintf(stderr,
                "SIMonotonicDimensionCreate: quantityName must be non-NULL\n");
        OCRelease(dim);
        return NULL;
    }
    if (!offset) {
        fprintf(stderr,
                "SIMonotonicDimensionCreate: offset must be non-NULL\n");
        OCRelease(dim);
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr,
                "SIMonotonicDimensionCreate: offset must be real-valued\n");
        OCRelease(dim);
        return NULL;
    }
    SIDimensionalityRef refDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    {
        OCStringRef err = NULL;
        SIDimensionalityRef qDim =
            SIDimensionalityForQuantity(quantityName, &err);
        if (!qDim ||
            !SIDimensionalityHasSameReducedDimensionality(qDim, refDim)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                    OCStringGetCString(quantityName));
            OCRelease(err);
            OCRelease(dim);
            return NULL;
        }
        OCRelease(err);
    }

    // 3) Provide a default zero-origin if none was passed in
    SIScalarRef originArg   = origin;
    SIScalarRef tmpZero     = NULL;
    bool        originIsTmp = false;
    if (!origin) {
        SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)offset);
        tmpZero     = SIScalarCreateWithDouble(0.0, u);
        if (!tmpZero) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: failed to create zero-origin\n");
            OCRelease(dim);
            return NULL;
        }
        originArg   = tmpZero;
        originIsTmp = true;
    }

    // 4) Initialize embedded SI fields via our helper
    if (!impl_InitSIDimensionFieldsFromArgs(
            (SIDimensionRef)dim,
            label, description, metadata,
            quantityName, offset, originArg,
            period, periodic, scaling))
    {
        OCRelease(dim);
        if (originIsTmp) OCRelease(tmpZero);
        return NULL;
    }
    if (originIsTmp) {
        OCRelease(tmpZero);
    }

    // 5) Monotonic-specific: clone the coordinates array
    OCRelease(dim->coordinates);  // drop the empty array from allocate
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    if (!dim->coordinates) {
        OCRelease(dim);
        return NULL;
    }

    // 6) Reciprocal-dimensionality check (optional)
    if (reciprocal) {
        OCStringRef verr = NULL;
        if (!SIDimensionValidate(reciprocal, &verr)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: invalid reciprocal dimension: %s\n",
                    OCStringGetCString(verr));
            OCRelease(verr);
            OCRelease(dim);
            return NULL;
        }
        if (!_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: reciprocal dimensionality mismatch\n");
            OCRelease(dim);
            return NULL;
        }
        OCRetain(reciprocal);
        dim->reciprocal = reciprocal;
    }

    return dim;
}
// getters & setters
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim) {
    return dim ? dim->coordinates : NULL;
}
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim, OCArrayRef coords) {
    if (!dim || !coords || OCArrayGetCount(coords) < 2) return false;
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coords);
    return dim->coordinates != NULL;
}
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}
bool SIMonotonicDimensionSetReciprocal(SIMonotonicDimensionRef dim, SIDimensionRef r) {
    if (!dim) return false;
    if (dim->reciprocal == r) return true;
    OCRelease(dim->reciprocal);
    dim->reciprocal = r;
    if (r) OCRetain(r);
    return true;
}
// dictionary serialization
SIMonotonicDimensionRef
SIMonotonicDimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;

    // 1) Base Dimension fields
    OCStringRef label       = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));

    // 2) SIDimension fields
    OCStringRef   quantityName = (OCStringRef) OCDictionaryGetValue(dict, STR("quantity_name"));
    SIScalarRef   offset      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("offset"));
    SIScalarRef   origin      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("origin"));
    SIScalarRef   period      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("period"));
    OCBooleanRef  periodicObj = (OCBooleanRef)  OCDictionaryGetValue(dict, STR("periodic"));
    bool          periodic    = periodicObj ? OCBooleanGetValue(periodicObj) : false;
    OCNumberRef   scalingNum  = (OCNumberRef)   OCDictionaryGetValue(dict, STR("scaling"));
    int           scalingInt  = 0;
    if (scalingNum) {
        OCNumberGetValue(scalingNum, kOCNumberIntType, &scalingInt);
    }
    dimensionScaling scaling = (dimensionScaling)scalingInt;

    // 3) Monotonic-specific fields
    OCArrayRef     coordinates = (OCArrayRef)    OCDictionaryGetValue(dict, STR("coordinates"));
    SIDimensionRef reciprocal  = (SIDimensionRef)OCDictionaryGetValue(dict, STR("reciprocal"));

    // 4) Validate coordinates count
    if (!coordinates || OCArrayGetCount(coordinates) < 2) {
        fprintf(stderr,
                "SIMonotonicDimensionCreateFromDictionary: need ≥2 coordinates\n");
        return NULL;
    }

    // 5) Delegate to the one-stop constructor
    return SIMonotonicDimensionCreate(
        label,
        description,
        metadata,
        quantityName,
        offset,
        origin,
        period,
        periodic,
        scaling,
        coordinates,
        reciprocal
    );
}
OCDictionaryRef
SIMonotonicDimensionCopyAsDictionary(SIMonotonicDimensionRef dim) {
    if (!dim) return NULL;

    // 1) base + SIDimension
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) return NULL;

    // 2) record concrete subclass name
    OCDictionarySetValue(dict, STR("type"), STR("SIMonotonicDimension"));

    // 3) coordinates
    OCArrayRef coords = dim->coordinates;
    OCArrayRef copyC =
        (OCArrayRef)OCTypeDeepCopy((OCTypeRef)coords);
    if (copyC) {
        OCDictionarySetValue(dict, STR("coordinates"), copyC);
        OCRelease(copyC);
    }

    // 4) reciprocal (optional)
    if (dim->reciprocal) {
        SIDimensionRef copy =
            (SIDimensionRef)OCTypeDeepCopy((OCTypeRef)dim->reciprocal);
        OCDictionarySetValue(dict, STR("reciprocal"), copy);
        OCRelease(copy);
    }

    return (OCDictionaryRef)dict;
}
#pragma endregion SIMonotonicDimension
#pragma region SILinearDimension
// ============================================================================
// MARK: - (6) SILinearDimension
// ============================================================================
static OCTypeID kSILinearDimensionID = kOCNotATypeID;
typedef struct impl_SILinearDimension {
    struct impl_SIDimension _super;  // inherit Dimension + SI fields
    SIDimensionRef reciprocal;    // optional reciprocal dimension
    OCIndex count;                // number of points (>=2)
    SIScalarRef increment;        // spacing between points
    SIScalarRef reciprocalIncrement;
    bool fft;  // FFT flag
} *SILinearDimensionRef;
OCTypeID SILinearDimensionGetTypeID(void) {
    if (kSILinearDimensionID == kOCNotATypeID)
        kSILinearDimensionID = OCRegisterType("SILinearDimension");
    return kSILinearDimensionID;
}
static bool impl_SILinearDimensionEqual(const void *a, const void *b) {
    const SILinearDimensionRef A = (const SILinearDimensionRef)a;
    const SILinearDimensionRef B = (const SILinearDimensionRef)b;
    if (!A || !B) return false;
    // compare base SI fields
    if (!impl_SIDimensionEqual((const void *)&A->_super, (const void *)&B->_super))
        return false;
    // compare subclass fields
    if (A->count != B->count || A->fft != B->fft)
        return false;
    if (!OCTypeEqual(A->increment, B->increment) ||
        !OCTypeEqual(A->reciprocalIncrement, B->reciprocalIncrement) ||
        !OCTypeEqual(A->reciprocal, B->reciprocal))
        return false;
    return true;
}
static void impl_SILinearDimensionFinalize(const void *obj) {
    if (!obj) return;
    SILinearDimensionRef dim = (SILinearDimensionRef)obj;
    // finalize SI superclass
    impl_SIDimensionFinalize((const void *)&dim->_super);
    // clean up subclass fields
    OCRelease(dim->increment);
    OCRelease(dim->reciprocalIncrement);
    OCRelease(dim->reciprocal);
}
static OCStringRef impl_SILinearDimensionCopyFormattingDesc(OCTypeRef cf) {
    SILinearDimensionRef d = (SILinearDimensionRef)cf;
    if (!d) {
        return STR("<SILinearDimension: NULL>");
    }
    // 1) Base SI description
    OCStringRef base = impl_SIDimensionCopyFormattingDesc(cf);
    // 2) Subclass values
    OCStringRef incStr = SIScalarCreateStringValue(d->increment);
    OCStringRef invStr = SIScalarCreateStringValue(d->reciprocalIncrement);
    OCStringRef fftStr = d->fft ? STR("true") : STR("false");    // 3) Stitch together
    OCStringRef fmt = OCStringCreateWithFormat(
        STR("%@ count=%lu increment=%@ reciprocalIncrement=%@ fft=%@>"),
        base,
        (unsigned long)d->count,
        incStr,
        invStr,
        fftStr);
    // 4) Clean up
    OCRelease(base);
    OCRelease(incStr);
    OCRelease(invStr);
    OCRelease(fftStr);
    return fmt;
}
static void *impl_SILinearDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = SILinearDimensionCopyAsDictionary((SILinearDimensionRef) obj);
    if (!dict) return NULL;
    SILinearDimensionRef copy = SILinearDimensionCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
static SILinearDimensionRef SILinearDimensionAllocate(void) {
    SILinearDimensionRef obj = OCTypeAlloc(
        struct impl_SILinearDimension,
        SILinearDimensionGetTypeID(),
        impl_SILinearDimensionFinalize,
        impl_SILinearDimensionEqual,
        impl_SILinearDimensionCopyFormattingDesc,
        impl_SILinearDimensionDeepCopy,
        impl_SILinearDimensionDeepCopy);
    impl_InitBaseDimensionFields((DimensionRef)&obj->_super._super);
    impl_InitSIDimensionFields((SIDimensionRef)&obj->_super);
    obj->count = 0;
    obj->increment = NULL;
    obj->reciprocalIncrement = NULL;
    obj->fft = false;
    obj->reciprocal = NULL;
    return obj;
}
SILinearDimensionRef
SILinearDimensionCreate(
    OCStringRef       label,
    OCStringRef       description,
    OCDictionaryRef   metadata,
    OCStringRef       quantityName,
    SIScalarRef       offset,
    SIScalarRef       origin,
    SIScalarRef       period,
    bool              periodic,
    dimensionScaling  scaling,
    OCIndex           count,
    SIScalarRef       increment,
    bool              fft,
    SIDimensionRef    reciprocal)
{
    // 1) Linear‐specific: need ≥2 points + a valid increment
    if (count < 2 || !increment) {
        fprintf(stderr,
                "SILinearDimensionCreate: need ≥2 points and a valid increment\n");
        return NULL;
    }

    // 2) Validate quantityName + offset (lifted from SIDimensionCreate)
    if (!quantityName) {
        fprintf(stderr,
                "SILinearDimensionCreate: quantityName must be non-NULL\n");
        return NULL;
    }
    if (!offset) {
        fprintf(stderr,
                "SILinearDimensionCreate: offset must be non-NULL\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr,
                "SILinearDimensionCreate: offset must be real-valued\n");
        return NULL;
    }
    SIDimensionalityRef refDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    {
        OCStringRef err = NULL;
        SIDimensionalityRef qDim =
            SIDimensionalityForQuantity(quantityName, &err);
        if (!qDim ||
            !SIDimensionalityHasSameReducedDimensionality(qDim, refDim)) {
            fprintf(stderr,
                    "SILinearDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                    OCStringGetCString(quantityName));
            OCRelease(err);
            return NULL;
        }
        OCRelease(err);
    }

    // 3) Allocate + zero‐init
    SILinearDimensionRef dim = SILinearDimensionAllocate();
    if (!dim) return NULL;

    // 4) supply a default origin if none provided
    SIScalarRef originArg   = origin;
    SIScalarRef tmpZero     = NULL;
    bool        originIsTmp = false;
    if (!origin) {
        SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)offset);
        tmpZero     = SIScalarCreateWithDouble(0.0, u);
        if (!tmpZero) {
            fprintf(stderr,
                    "SILinearDimensionCreate: failed to create zero-origin\n");
            OCRelease(dim);
            return NULL;
        }
        originArg   = tmpZero;
        originIsTmp = true;
    }

    // 5) Initialize all the shared SI fields in one go
    if (!impl_InitSIDimensionFieldsFromArgs(
            (SIDimensionRef)dim,
            label, description, metadata,
            quantityName, offset, originArg,
            period, periodic, scaling))
    {
        OCRelease(dim);
        if (originIsTmp) OCRelease(tmpZero);
        return NULL;
    }
    if (originIsTmp) OCRelease(tmpZero);

    // 6) Linear‐specific: set count + increment
    dim->count = count;
    dim->increment = SIScalarCreateCopy(increment);
    if (!dim->increment) {
        OCRelease(dim);
        return NULL;
    }

    // 7) Compute reciprocalIncrement = (count·increment)⁻¹
    {
        SIMutableScalarRef recInc = SIScalarCreateMutableCopy(increment);
        if (!recInc) {
            OCRelease(dim);
            return NULL;
        }
        // ensure recInc is in same unit as offset:
        SIScalarConvertToUnit(recInc,
                              SIQuantityGetUnit((SIQuantityRef)offset),
                              NULL);
        if (!SIScalarMultiplyByDimensionlessRealConstant(recInc, (double)count) ||
        !SIScalarRaiseToAPowerWithoutReducingUnit(recInc, -1.0, NULL))
        {
            OCRelease(recInc);
            OCRelease(dim);
            return NULL;
        }
        SIScalarSetElementType(recInc, kSINumberFloat64Type);
        dim->reciprocalIncrement = recInc;
    }

    // 8) FFT flag + optional reciprocal
    dim->fft = fft;
    if (reciprocal) {
        OCStringRef verr = NULL;
        if (!SIDimensionValidate(reciprocal, &verr) ||
            !_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal))
        {
            fprintf(stderr,
                    "SILinearDimensionCreate: invalid or mismatched reciprocal\n");
            OCRelease(verr);
            OCRelease(dim);
            return NULL;
        }
        OCRetain(reciprocal);
        dim->reciprocal = reciprocal;
    }

    return dim;
}
SILinearDimensionRef
SILinearDimensionCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;

    // 1) Base Dimension fields
    OCStringRef     label       = (OCStringRef)   OCDictionaryGetValue(dict, STR("label"));
    OCStringRef     description = (OCStringRef)   OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata    = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));

    // 2) SIDimension fields
    OCStringRef   quantityName = (OCStringRef)   OCDictionaryGetValue(dict, STR("quantity_name"));
    SIScalarRef   offset      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("offset"));
    SIScalarRef   origin      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("origin"));
    SIScalarRef   period      = (SIScalarRef)   OCDictionaryGetValue(dict, STR("period"));
    OCBooleanRef  periodicObj = (OCBooleanRef)  OCDictionaryGetValue(dict, STR("periodic"));
    bool          periodic    = periodicObj ? OCBooleanGetValue(periodicObj) : false;
    OCNumberRef   scalingNum  = (OCNumberRef)   OCDictionaryGetValue(dict, STR("scaling"));
    int           scalingInt  = 0;
    if (scalingNum) {
        OCNumberGetValue(scalingNum, kOCNumberIntType, &scalingInt);
    }
    dimensionScaling scaling = (dimensionScaling)scalingInt;

    // 3) Linear-specific fields
    OCNumberRef    countNum            = (OCNumberRef)  OCDictionaryGetValue(dict, STR("count"));
    OCIndex        count               = 0;
    if (countNum) {
        int tmp = 0;
        OCNumberGetValue(countNum, kOCNumberIntType, &tmp);
        count = (OCIndex)tmp;
    }
    SIScalarRef    increment           = (SIScalarRef)  OCDictionaryGetValue(dict, STR("increment"));
    OCBooleanRef   fftObj              = (OCBooleanRef) OCDictionaryGetValue(dict, STR("fft"));
    bool           fft                 = fftObj ? OCBooleanGetValue(fftObj) : false;
    SIScalarRef    reciprocalDimension = (SIScalarRef)  OCDictionaryGetValue(dict, STR("reciprocal"));

    // 4) Validate required linear fields
    if (count < 2 || !increment) {
        fprintf(stderr,
                "SILinearDimensionCreateFromDictionary: need ≥2 points and a valid increment\n");
        return NULL;
    }

    // 5) Delegate to public constructor (allocates and fully initializes)
    return SILinearDimensionCreate(
        label,
        description,
        metadata,
        quantityName,
        offset,
        origin,
        period,
        periodic,
        scaling,
        count,
        increment,
        fft,
        (SIDimensionRef)reciprocalDimension
    );
}
OCDictionaryRef
SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim) {
    if (!dim) return NULL;

    // 1) Base + SIDimension
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) return NULL;

    // 2) Type discriminator
    OCDictionarySetValue(dict, STR("type"), STR("SILinearDimension"));

    // 3) count
    OCNumberRef c = OCNumberCreateWithInt((int)dim->count);
    OCDictionarySetValue(dict, STR("count"), c);
    OCRelease(c);

    // 4) increment
    if (dim->increment) {
        SIScalarRef incCopy = SIScalarCreateCopy(dim->increment);
        if (incCopy) {
            OCDictionarySetValue(dict, STR("increment"), incCopy);
            OCRelease(incCopy);
        }
    }

    // 5) reciprocalIncrement
    if (dim->reciprocalIncrement) {
        SIScalarRef invCopy = SIScalarCreateCopy(dim->reciprocalIncrement);
        if (invCopy) {
            OCDictionarySetValue(dict, STR("reciprocalIncrement"), invCopy);
            OCRelease(invCopy);
        }
    }

    // 6) fft flag
    OCBooleanRef b = OCBooleanGetWithBool(dim->fft);
    OCDictionarySetValue(dict, STR("fft"), b);
    OCRelease(b);

    // 7) reciprocal dimension
    if (dim->reciprocal) {
        SIDimensionRef copy = (SIDimensionRef) OCTypeDeepCopy((OCTypeRef) dim->reciprocal);
        OCDictionarySetValue(dict, STR("reciprocal"), copy);
        OCRelease(copy);
    }

    return (OCDictionaryRef)dict;
}


#pragma mark — LinearDimension accessors

OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim) {
    return dim ? dim->count : 0;
}

bool SILinearDimensionSetCount(SILinearDimensionRef dim, OCIndex count) {
    if (!dim || count < 2) return false;
    dim->count = count;
    // if you want to keep reciprocalIncrement in sync, you could recompute it here…
    return true;
}

SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim) {
    return dim ? dim->increment : NULL;
}

bool SILinearDimensionSetIncrement(SILinearDimensionRef dim, SIScalarRef inc) {
    if (!dim || !inc) return false;
    // ensure real & same dimensionality as offset/origin
    SIUnitRef offsetUnit =
        SIQuantityGetUnit((SIQuantityRef)SIDimensionGetOffset((SIDimensionRef)dim));
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(inc, offsetUnit, NULL);
    if (!copy) return false;

    OCRelease(dim->increment);
    dim->increment = copy;

    // and recompute reciprocalIncrement
    OCRelease(dim->reciprocalIncrement);
    SIMutableScalarRef rec = SIScalarCreateMutableCopy(copy);
    SIScalarMultiplyByDimensionlessRealConstant(rec, (double)dim->count);
    SIScalarRaiseToAPowerWithoutReducingUnit(rec, -1.0, NULL);
    SIScalarSetElementType(rec, kSINumberFloat64Type);
    dim->reciprocalIncrement = rec;

    return true;
}

SIScalarRef SILinearDimensionGetReciprocalIncrement(SILinearDimensionRef dim) {
    return dim ? dim->reciprocalIncrement : NULL;
}

bool SILinearDimensionIsFFT(SILinearDimensionRef dim) {
    return dim ? dim->fft : false;
}

bool SILinearDimensionSetFFT(SILinearDimensionRef dim, bool fft) {
    if (!dim) return false;
    dim->fft = fft;
    return true;
}

SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}

bool SILinearDimensionSetReciprocal(SILinearDimensionRef dim, SIDimensionRef rec) {
    if (!dim) return false;
    if (dim->reciprocal == rec) return true;
    // optional: validate rec’s dimensionality against (count·increment)⁻¹
    OCRelease(dim->reciprocal);
    dim->reciprocal = rec;
    if (rec) OCRetain(rec);
    return true;
}

#pragma endregion SILinearDimension

