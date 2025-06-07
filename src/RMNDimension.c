// ============================================================================
// RMNDimension.c
//
// All RMNDimension types (base + Labeled, Quantitative, Monotonic, Linear)
// are defined and implemented in one file, grouped by type.
//
// (1)  Shared utilities & macros
// (2)  RMNDimension (abstract base)
// (3)  RMNLabeledDimension
// (4)  RMNQuantitativeDimension
// (5)  RMNMonotonicDimension
// (6)  RMNLinearDimension
// ============================================================================
#include "RMNLibrary.h"
#pragma region RMNDimension Helpers
static RMNMonotonicDimensionRef RMNMonotonicDimensionCreateCopy(RMNMonotonicDimensionRef src);
RMNDimensionRef RMNDimensionCreateDeepCopy(RMNDimensionRef original) {
    if (!original) return NULL;

    OCTypeID typeID = OCGetTypeID(original);

    if (typeID == RMNLinearDimensionGetTypeID()) {
        // use OCRetain from OCTypes
        return (RMNDimensionRef)OCRetain((OCTypeRef)original);
    }
    if (typeID == RMNMonotonicDimensionGetTypeID()) {
        return (RMNDimensionRef)RMNMonotonicDimensionCreateCopy((RMNMonotonicDimensionRef)original);
    }
    if (typeID == RMNLabeledDimensionGetTypeID()) {
        OCArrayRef labels = RMNLabeledDimensionGetLabels((RMNLabeledDimensionRef)original);
        if (!labels) return NULL;
        return (RMNDimensionRef)RMNLabeledDimensionCreateWithLabels(labels);
    }
    if (typeID == RMNQuantitativeDimensionGetTypeID()) {
        RMNQuantitativeDimensionRef src = (RMNQuantitativeDimensionRef)original;
        return (RMNDimensionRef)RMNQuantitativeDimensionCreate(
            OCDimensionGetLabel((RMNDimensionRef)src),
            OCDimensionGetDescription((RMNDimensionRef)src),
            OCDimensionGetMetaData((RMNDimensionRef)src),
            RMNQuantitativeDimensionGetQuantityName(src),
            RMNQuantitativeDimensionGetReferenceOffset(src),
            RMNQuantitativeDimensionGetOriginOffset(src),
            RMNQuantitativeDimensionGetPeriod(src),
            RMNQuantitativeDimensionIsPeriodic(src),
            RMNQuantitativeDimensionGetScaling(src)
        );
    }

    fprintf(stderr, "RMNDimensionCreateDeepCopy: Unsupported typeID %u\n", typeID);
    return NULL;
}
#pragma endregion RMNDimension Helpers
#pragma region RMNDimension
// ============================================================================
// MARK: - (1) RMNDimension (Abstract Base)
// ============================================================================
static OCTypeID kRMNDimensionID = _kOCNotATypeID;
struct __RMNDimension {
 //  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;
};
static void __RMNInitBaseFields(RMNDimensionRef dim) {
    dim->label      = STR("");
    dim->description = STR("");
    dim->metaData    = OCDictionaryCreateMutable(0);
}
OCTypeID RMNDimensionGetTypeID(void) {
    if (kRMNDimensionID == _kOCNotATypeID)
        kRMNDimensionID = OCRegisterType("RMNDimension");
    return kRMNDimensionID;
}
static void __RMNDimensionFinalize(const void *obj) {
    RMNDimensionRef dim = (RMNDimensionRef)obj;
    if (!dim) return;
    OCRelease(dim->label); dim->label = NULL;
    OCRelease(dim->description); dim->description = NULL;
    OCRelease(dim->metaData); dim->metaData = NULL;
}
static OCStringRef __RMNDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNDimension>");
}
OCStringRef OCDimensionGetLabel(RMNDimensionRef dim) {
    return dim ? dim->label : NULL;
}
bool OCDimensionSetLabel(RMNDimensionRef dim, OCStringRef label) {
    if (!dim) return false;

    OCStringRef labelCopy = label ? OCStringCreateCopy(label) : NULL;
    if (label && !labelCopy) {
        fprintf(stderr, "OCDimensionSetLabel: failed to copy label string\n");
        return false;
    }

    OCRelease(dim->label);
    dim->label = labelCopy;
    return true;
}
OCStringRef OCDimensionGetDescription(RMNDimensionRef dim) {
    return dim ? dim->description : NULL;
}
bool OCDimensionSetDescription(RMNDimensionRef dim, OCStringRef desc) {
    if (!dim) return false;

    OCStringRef descCopy = desc ? OCStringCreateCopy(desc) : NULL;
    if (desc && !descCopy) {
        fprintf(stderr, "OCDimensionSetDescription: failed to copy description string\n");
        return false;
    }

    OCRelease(dim->description);
    dim->description = descCopy;
    return true;
}
OCDictionaryRef OCDimensionGetMetaData(RMNDimensionRef dim) {
    return dim ? dim->metaData : NULL;
}
bool OCDimensionSetMetaData(RMNDimensionRef dim, OCDictionaryRef dict) {
    if (!dim) return false;

    OCDictionaryRef dictCopy = dict ? OCDictionaryCreateCopy(dict) : NULL;
    if (dict && !dictCopy) {
        fprintf(stderr, "OCDimensionSetMetaData: failed to copy metadata dictionary\n");
        return false;
    }

    OCRelease(dim->metaData);
    dim->metaData = dictCopy;
    return true;
}
#pragma endregion RMNDimension
#pragma region RMNLabeledDimension
// ============================================================================
// MARK: - (3) RMNLabeledDimension
// ============================================================================
static OCTypeID kRMNLabeledDimensionID = _kOCNotATypeID;
struct __RMNLabeledDimension {
 //  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;

 //  RMNLabeledDimension
    OCMutableArrayRef labels;
};
OCTypeID RMNLabeledDimensionGetTypeID(void) {
    if (kRMNLabeledDimensionID == _kOCNotATypeID)
        kRMNLabeledDimensionID = OCRegisterType("RMNLabeledDimension");
    return kRMNLabeledDimensionID;
}
static void __RMNLabeledDimensionFinalize(const void *obj) {
    RMNLabeledDimensionRef dim = (RMNLabeledDimensionRef)obj;
    __RMNDimensionFinalize((RMNDimensionRef)dim);
    OCRelease(dim->labels);
    dim->labels = NULL;
}
static OCStringRef __RMNLabeledDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNLabeledDimension>");
}
static void *__RMNLabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    return RMNLabeledDimensionCreateWithLabels(RMNLabeledDimensionGetLabels((RMNLabeledDimensionRef)obj));
}
static RMNLabeledDimensionRef RMNLabeledDimensionAllocate(void) {
    RMNLabeledDimensionRef obj = OCTypeAlloc(
        struct __RMNLabeledDimension,
        RMNLabeledDimensionGetTypeID(),
        __RMNLabeledDimensionFinalize,
        NULL,
        __RMNLabeledDimensionCopyFormattingDesc,
        __RMNLabeledDimensionDeepCopy,
        __RMNLabeledDimensionDeepCopy
    );
    __RMNInitBaseFields((RMNDimensionRef)obj);
    obj->labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return obj;
}
RMNLabeledDimensionRef
RMNLabeledDimensionCreate(
    OCStringRef       label,
    OCStringRef       description,
    OCDictionaryRef   metaData,
    OCArrayRef        inputLabels
) {
    if (!inputLabels || OCArrayGetCount(inputLabels) < 2) {
        fprintf(stderr, "RMNLabeledDimensionCreate: need ≥2 labels\n");
        return NULL;
    }

    RMNLabeledDimensionRef dim = RMNLabeledDimensionAllocate();
    if (!dim) return NULL;

    // Base fields
    if (label) {
        OCRelease(dim->label);
        dim->label = OCStringCreateCopy(label);
        if (!dim->label) { OCRelease(dim); return NULL; }
    }
    if (description) {
        OCRelease(dim->description);
        dim->description = OCStringCreateCopy(description);
        if (!dim->description) { OCRelease(dim); return NULL; }
    }
    if (metaData) {
        OCRelease(dim->metaData);
        dim->metaData = OCDictionaryCreateCopy(metaData);
        if (!dim->metaData) { OCRelease(dim); return NULL; }
    }

    // Labels array
    OCRelease(dim->labels);
    dim->labels = OCArrayCreateMutableCopy(inputLabels);
    if (!dim->labels) {
        OCRelease(dim);
        return NULL;
    }

    return dim;
}
RMNLabeledDimensionRef
RMNLabeledDimensionCreateWithLabels(OCArrayRef inputLabels)
{
    if (!inputLabels || OCArrayGetCount(inputLabels) < 2) {
        fprintf(stderr, "RMNLabeledDimensionCreateWithLabels: need ≥2 labels\n");
        return NULL;
    }
    // Delegate to the general constructor, using empty strings and NULL metadata
    return RMNLabeledDimensionCreate(
        STR(""), 
        STR(""),
        NULL,   // default metadata → creates an empty dictionary
        inputLabels     
    );
}
OCArrayRef RMNLabeledDimensionGetLabels(RMNLabeledDimensionRef dim) {
    return dim ? dim->labels : NULL;
}
bool RMNLabeledDimensionSetLabels(RMNLabeledDimensionRef dim, OCArrayRef labels) {
    if (!dim || !labels) return false;
    if (dim->labels == labels) return true;
    OCRelease(dim->labels);
    dim->labels = OCArrayCreateMutableCopy(labels);
    return true;
}
OCStringRef RMNLabeledDimensionGetLabelAtIndex(RMNLabeledDimensionRef dim, OCIndex index) {
    if (!dim || !dim->labels || index < 0 || index >= OCArrayGetCount(dim->labels))
        return NULL;
    return OCArrayGetValueAtIndex(dim->labels, index);
}
bool RMNLabeledDimensionSetLabelAtIndex(RMNLabeledDimensionRef dim, OCIndex index, OCStringRef label) {
    if (!dim || !dim->labels || !label) return false;
    if (index < 0 || index >= OCArrayGetCount(dim->labels)) return false;
    OCArraySetValueAtIndex(dim->labels, index, label);
    return true;
}
OCIndex RMNLabeledDimensionGetCount(RMNLabeledDimensionRef theDimension)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDimension,0);
    IF_NO_OBJECT_EXISTS_RETURN(theDimension->labels,0);
    return OCArrayGetCount(theDimension->labels);
}
#pragma endregion RMNLabeledDimension
#pragma region RMNQuantitativeDimension
// ============================================================================
// MARK: - (4) RMNQuantitativeDimension
// ============================================================================
static OCTypeID kRMNQuantitativeDimensionID = _kOCNotATypeID;
struct __RMNQuantitativeDimension {
 //  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;

//  RMNQuantitativeDimension
    OCStringRef quantityName;
    SIScalarRef referenceOffset;
    SIScalarRef originOffset;
    SIScalarRef period;

    bool periodic;
    dimensionScaling scaling;
};
OCTypeID RMNQuantitativeDimensionGetTypeID(void) {
    if (kRMNQuantitativeDimensionID == _kOCNotATypeID)
        kRMNQuantitativeDimensionID = OCRegisterType("RMNQuantitativeDimension");
    return kRMNQuantitativeDimensionID;
}
static void __RMNQuantitativeDimensionFinalize(const void *obj) {
    RMNQuantitativeDimensionRef dim = (RMNQuantitativeDimensionRef)obj;
    __RMNDimensionFinalize((RMNDimensionRef)dim);
    OCRelease(dim->quantityName);        dim->quantityName = NULL;
    OCRelease(dim->referenceOffset);     dim->referenceOffset = NULL;
    OCRelease(dim->originOffset);        dim->originOffset = NULL;
    OCRelease(dim->period);              dim->period = NULL;
}
static OCStringRef __RMNQuantitativeDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNQuantitativeDimension>");
}
static void *__RMNQuantitativeDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    RMNQuantitativeDimensionRef src = (RMNQuantitativeDimensionRef)obj;
    return RMNQuantitativeDimensionCreate(
        OCDimensionGetLabel((RMNDimensionRef)src),
        OCDimensionGetDescription((RMNDimensionRef)src),
        OCDimensionGetMetaData((RMNDimensionRef)src),
        RMNQuantitativeDimensionGetQuantityName(src),
        RMNQuantitativeDimensionGetReferenceOffset(src),
        RMNQuantitativeDimensionGetOriginOffset(src),
        RMNQuantitativeDimensionGetPeriod(src),
        RMNQuantitativeDimensionIsPeriodic(src),
        RMNQuantitativeDimensionGetScaling(src)
    );
}
static void __RMNInitQuantitativeFields(RMNQuantitativeDimensionRef dim) {
    dim->quantityName    = NULL;
    dim->referenceOffset = NULL;
    dim->originOffset    = NULL;
    dim->period          = NULL;
    dim->periodic        = false;
    dim->scaling         = kDimensionScalingNone;
}
static RMNQuantitativeDimensionRef RMNQuantitativeDimensionAllocate(void) {
    RMNQuantitativeDimensionRef obj = OCTypeAlloc(
        struct __RMNQuantitativeDimension,
        RMNQuantitativeDimensionGetTypeID(),
        __RMNQuantitativeDimensionFinalize,
        NULL,
        __RMNQuantitativeDimensionCopyFormattingDesc,
        __RMNQuantitativeDimensionDeepCopy,
        __RMNQuantitativeDimensionDeepCopy
    );
    __RMNInitBaseFields((RMNDimensionRef)obj);
    __RMNInitQuantitativeFields(obj);
    return obj;
}
RMNQuantitativeDimensionRef RMNQuantitativeDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metaData,
    OCStringRef quantityName,
    SIScalarRef referenceOffset,
    SIScalarRef originOffset,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling)
{
    if (!referenceOffset || !originOffset || !period) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: referenceOffset, originOffset, and period must not be NULL\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)referenceOffset) ||
        SIQuantityIsComplexType((SIQuantityRef)originOffset) ||
        SIQuantityIsComplexType((SIQuantityRef)period)) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: scalar inputs must be real-valued\n");
        return NULL;
    }

    // Retrieve dimensionalities
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)referenceOffset);
    SIDimensionalityRef origDim = SIQuantityGetUnitDimensionality((SIQuantityRef)originOffset);
    SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)period);

    // Check dimensionality consistency among scalars
    if (!SIDimensionalityHasSameReducedDimensionality(refDim, origDim) ||
        !SIDimensionalityHasSameReducedDimensionality(refDim, perDim)) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: referenceOffset, originOffset, and period must have the same dimensionality\n");
        return NULL;
    }

    // Validate quantityName against scalar dimensionality
    if (quantityName) {
        OCStringRef error = NULL;
        SIDimensionalityRef nameDim = SIDimensionalityForQuantity(quantityName, &error);
        if (!nameDim || !SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: quantityName dimensionality mismatch\n");
            return NULL;
        }
    } else {
        // Attempt to infer quantityName from referenceOffset's unit
        SIUnitRef unit = SIQuantityGetUnit((SIQuantityRef)referenceOffset);
        quantityName = SIUnitGuessQuantityName(unit);
        if (!quantityName) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: unable to infer quantityName from unit\n");
            return NULL;
        }
    }

    // Allocate the dimension object
    RMNQuantitativeDimensionRef dim = RMNQuantitativeDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: allocation failed\n");
        return NULL;
    }

    // Set label
    if (label) {
        dim->label = OCStringCreateCopy(label);
        if (!dim->label) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy label\n");
            OCRelease(dim);
            return NULL;
        }
    }

    // Set description
    if (description) {
        dim->description = OCStringCreateCopy(description);
        if (!dim->description) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy description\n");
            OCRelease(dim);
            return NULL;
        }
    }

    // Set metaData
    if (metaData) {
        dim->metaData = OCDictionaryCreateCopy(metaData);
        if (!dim->metaData) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy metaData\n");
            OCRelease(dim);
            return NULL;
        }
    } else {
        dim->metaData = OCDictionaryCreateMutable(0);
        if (!dim->metaData) {
            fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to create metaData dictionary\n");
            OCRelease(dim);
            return NULL;
        }
    }

    // Set quantityName
    dim->quantityName = OCStringCreateCopy(quantityName);
    if (!dim->quantityName) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy quantityName\n");
        OCRelease(dim);
        return NULL;
    }

    // Set scalar fields
    dim->referenceOffset = SIScalarCreateCopy(referenceOffset);
    if (!dim->referenceOffset) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy referenceOffset\n");
        OCRelease(dim);
        return NULL;
    }

    dim->originOffset = SIScalarCreateCopy(originOffset);
    if (!dim->originOffset) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy originOffset\n");
        OCRelease(dim);
        return NULL;
    }

    dim->period = SIScalarCreateCopy(period);
    if (!dim->period) {
        fprintf(stderr, "RMNQuantitativeDimensionCreate: failed to copy period\n");
        OCRelease(dim);
        return NULL;
    }

    // Set boolean flags
    dim->periodic = periodic;
    dim->scaling = scaling;

    return dim;
}
OCStringRef RMNQuantitativeDimensionGetQuantityName(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
bool RMNQuantitativeDimensionSetQuantityName(RMNQuantitativeDimensionRef dim, OCStringRef name) {
    if (!dim) return false;
    OCRelease(dim->quantityName);
    dim->quantityName = name ? OCRetain(name) : NULL;
    return true;
}
SIScalarRef RMNQuantitativeDimensionGetReferenceOffset(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->referenceOffset : NULL;
}
bool RMNQuantitativeDimensionSetReferenceOffset(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return false;
    OCRelease(dim->referenceOffset);
    dim->referenceOffset = val ? OCRetain(val) : NULL;
    return true;
}
SIScalarRef RMNQuantitativeDimensionGetOriginOffset(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->originOffset : NULL;
}
bool RMNQuantitativeDimensionSetOriginOffset(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return false;
    OCRelease(dim->originOffset);
    dim->originOffset = val ? OCRetain(val) : NULL;
    return true;
}
SIScalarRef RMNQuantitativeDimensionGetPeriod(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->period : NULL;
}
bool RMNQuantitativeDimensionSetPeriod(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return false;
    OCRelease(dim->period);
    dim->period = val ? OCRetain(val) : NULL;
    return true;
}
bool RMNQuantitativeDimensionIsPeriodic(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->periodic : false;
}
bool RMNQuantitativeDimensionSetPeriodic(RMNQuantitativeDimensionRef dim, bool flag) {
    if (!dim) return false;
    dim->periodic = flag;
    return true;
}
dimensionScaling RMNQuantitativeDimensionGetScaling(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->scaling : kDimensionScalingNone;
}
bool RMNQuantitativeDimensionSetScaling(RMNQuantitativeDimensionRef dim, dimensionScaling scaling) {
    if (!dim) return false;
    dim->scaling = scaling;
    return true;
}
bool RMNQuantitativeDimensionMultiplyByScalar(RMNQuantitativeDimensionRef theDimension,
                                             SIScalarRef theScalar,
                                             OCStringRef *error)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDimension,false);
    IF_NO_OBJECT_EXISTS_RETURN(theScalar,false);
    if(SIScalarDoubleValue(theScalar) == 0.0) return false;
    if(SIQuantityHasDimensionality((SIQuantityRef) theScalar,SIDimensionalityDimensionless())&& SIScalarIsReal(theScalar) && SIScalarDoubleValue(theScalar)==1) return true;
    
    SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef) theDimension->originOffset, theScalar, error);
    SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef) theDimension->referenceOffset, theScalar, error);
    if(theDimension->period) SIScalarMultiplyWithoutReducingUnit((SIMutableScalarRef) theDimension->period, theScalar, error);
    
    if(theDimension->quantityName) OCRelease(theDimension->quantityName);
    OCStringRef quantityName = SIUnitGuessQuantityName(SIQuantityGetUnit((SIQuantityRef)theDimension->originOffset));
    theDimension->quantityName = OCStringCreateCopy(quantityName);
    
    return true;
}
#pragma endregion RMNQuantitativeDimension
#pragma region RMNMonotonicDimension
// ============================================================================
// MARK: - (5) RMNMonotonicDimension
// ============================================================================
static OCTypeID kRMNMonotonicDimensionID = _kOCNotATypeID;
struct __RMNMonotonicDimension {
//  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;

//  RMNQuantitativeDimension
    OCStringRef quantityName;
    SIScalarRef referenceOffset;
    SIScalarRef originOffset;
    SIScalarRef period;

    bool periodic;
    dimensionScaling scaling;

//  RMNMonotonicDimension
    RMNQuantitativeDimensionRef reciprocal;
    OCMutableArrayRef coordinates;
};
OCTypeID RMNMonotonicDimensionGetTypeID(void) {
    if (kRMNMonotonicDimensionID == _kOCNotATypeID)
        kRMNMonotonicDimensionID = OCRegisterType("RMNMonotonicDimension");
    return kRMNMonotonicDimensionID;
}
static void __RMNMonotonicDimensionFinalize(const void *obj) {
    RMNMonotonicDimensionRef dim = (RMNMonotonicDimensionRef)obj;
    __RMNQuantitativeDimensionFinalize((struct __RMNDimension*)dim);
    OCRelease(dim->coordinates);
    dim->coordinates = NULL;
}
static OCStringRef __RMNMonotonicDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNMonotonicDimension>");
}
static void *__RMNMonotonicDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    return RMNMonotonicDimensionCreateCopy((RMNMonotonicDimensionRef)obj);
}
static RMNMonotonicDimensionRef RMNMonotonicDimensionAllocate(void) {
    RMNMonotonicDimensionRef obj = OCTypeAlloc(
        struct __RMNMonotonicDimension,
        RMNMonotonicDimensionGetTypeID(),
        __RMNMonotonicDimensionFinalize,
        NULL,
        __RMNMonotonicDimensionCopyFormattingDesc,
        __RMNMonotonicDimensionDeepCopy,
        __RMNMonotonicDimensionDeepCopy
    );
    __RMNInitBaseFields((RMNDimensionRef)obj);
    __RMNInitQuantitativeFields((RMNQuantitativeDimensionRef)obj);
    obj->coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return obj;
}
/// Fully-customizable constructor for RMNMonotonicDimension
RMNMonotonicDimensionRef
RMNMonotonicDimensionCreate(
    OCStringRef     label,
    OCStringRef     description,
    OCDictionaryRef metaData,
    OCArrayRef      coordinates,
    OCStringRef     quantityName,
    SIScalarRef     originOffset,
    SIScalarRef     referenceOffset,
    bool            periodic,
    dimensionScaling scaling
) {
    // 1) Validate coordinates array
    if (!coordinates) {
        fprintf(stderr, "RMNMonotonicDimensionCreate: coordinates array is NULL\n");
        return NULL;
    }
    OCIndex count = OCArrayGetCount(coordinates);
    if (count < 2) {
        fprintf(stderr, "RMNMonotonicDimensionCreate: need ≥2 coordinates\n");
        return NULL;
    }

    // 2) Check first coord is real, grab its unit & dimensionality
    SIScalarRef first = OCArrayGetValueAtIndex(coordinates, 0);
    if (SIQuantityIsComplexType((SIQuantityRef)first)) {
        fprintf(stderr, "RMNMonotonicDimensionCreate: first coordinate is complex\n");
        return NULL;
    }
    SIDimensionalityRef dim0 = SIQuantityGetUnitDimensionality((SIQuantityRef)first);
    SIUnitRef             unit = SIQuantityGetUnit((SIQuantityRef)first);

    // 3) Ensure all coords share the same reduced dimensionality & are real
    for (OCIndex i = 1; i < count; ++i) {
        SIScalarRef c = OCArrayGetValueAtIndex(coordinates, i);
        if (SIQuantityIsComplexType((SIQuantityRef)c)) {
            fprintf(stderr, "RMNMonotonicDimensionCreate: coordinate %u is complex\n", (unsigned)i);
            return NULL;
        }
        if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)first, (SIQuantityRef)c)) {
            fprintf(stderr, "RMNMonotonicDimensionCreate: coordinate %u mismatched dimensionality\n", (unsigned)i);
            return NULL;
        }
    }

    // 4) Validate or infer quantityName
    if (quantityName) {
        OCStringRef err = NULL;
        SIDimensionalityRef qdim = SIDimensionalityForQuantity(quantityName, &err);
        if (!qdim || !SIDimensionalityHasSameReducedDimensionality(qdim, dim0)) {
            fprintf(stderr, "RMNMonotonicDimensionCreate: quantityName dimensionality mismatch\n");
            return NULL;
        }
    } else {
        quantityName = SIUnitGuessQuantityName(unit);
        if (!quantityName) {
            fprintf(stderr, "RMNMonotonicDimensionCreate: cannot infer quantityName from unit\n");
            return NULL;
        }
    }

    // 5) Allocate
    RMNMonotonicDimensionRef dim = RMNMonotonicDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "RMNMonotonicDimensionCreate: allocation failed\n");
        return NULL;
    }

    // 6) Set base fields
    if (label)       OCDimensionSetLabel   ((RMNDimensionRef)dim, label);
    if (description) OCDimensionSetDescription((RMNDimensionRef)dim, description);
    if (metaData)    OCDimensionSetMetaData((RMNDimensionRef)dim, metaData);

    // 7) Set quantityName
    {
        OCStringRef nameCopy = OCStringCreateCopy(quantityName);
        RMNQuantitativeDimensionSetQuantityName((RMNQuantitativeDimensionRef)dim, nameCopy);
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
    if (!originOffset)     originOffset     = zero;
    if (!referenceOffset)  referenceOffset  = zero;
    RMNQuantitativeDimensionSetOriginOffset     ((RMNQuantitativeDimensionRef)dim, originOffset);
    RMNQuantitativeDimensionSetReferenceOffset  ((RMNQuantitativeDimensionRef)dim, referenceOffset);
    if (zero != originOffset)    OCRelease(zero);
    if (zero != referenceOffset) OCRelease(zero);

    // 10) Compute & set period = last – first
    SIScalarRef last   = OCArrayGetValueAtIndex(coordinates, count - 1);
    SIScalarRef period = SIScalarCreateBySubtracting(last, first, NULL);
    RMNQuantitativeDimensionSetPeriod((RMNQuantitativeDimensionRef)dim, period);
    OCRelease(period);

    // 11) Flags
    RMNQuantitativeDimensionSetPeriodic((RMNQuantitativeDimensionRef)dim, periodic);
    RMNQuantitativeDimensionSetScaling ((RMNQuantitativeDimensionRef)dim, scaling);

    return dim;
}
RMNMonotonicDimensionRef
RMNMonotonicDimensionCreateWithCoordinatesAndQuantity(OCArrayRef coordinates,
                                  OCStringRef quantityName)
{
    return RMNMonotonicDimensionCreate(
        /* label: */        NULL,
        /* description: */  NULL,
        /* metaData: */     NULL,
        /* coordinates: */  coordinates,
        /* quantityName: */ quantityName,
        /* originOffset: */ NULL,
        /* referenceOffset: */ NULL,
        /* periodic: */     false,
        /* scaling: */      kDimensionScalingNone
    );
}
RMNMonotonicDimensionRef RMNMonotonicDimensionCreateCopy(RMNMonotonicDimensionRef src) {
    if (!src) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: source is NULL\n");
        return NULL;
    }

    OCArrayRef originalCoords = RMNMonotonicDimensionGetCoordinates(src);
    if (!originalCoords || OCArrayGetCount(originalCoords) < 2) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: invalid or insufficient coordinates\n");
        return NULL;
    }

    const OCIndex count = OCArrayGetCount(originalCoords);
    OCMutableArrayRef copiedCoords = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    if (!copiedCoords) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to allocate coordinate array copy\n");
        return NULL;
    }

    for (OCIndex i = 0; i < count; ++i) {
        SIScalarRef coord = OCArrayGetValueAtIndex(originalCoords, i);
        SIScalarRef copy = SIScalarCreateCopy(coord);
        if (!copy) {
            fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to copy scalar at index %ld\n", i);
            OCRelease(copiedCoords);
            return NULL;
        }
        OCArrayAppendValue(copiedCoords, copy);
        OCRelease(copy);
    }

    RMNMonotonicDimensionRef copy = RMNMonotonicDimensionCreateWithCoordinates(copiedCoords);
    OCRelease(copiedCoords);
    if (!copy) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to create copy with coordinates\n");
        return NULL;
    }

    RMNQuantitativeDimensionRef dst = (RMNQuantitativeDimensionRef)copy;
    RMNQuantitativeDimensionRef srcBase = (RMNQuantitativeDimensionRef)src;

    SIScalarRef origin = SIScalarCreateCopy(RMNQuantitativeDimensionGetOriginOffset(srcBase));
    SIScalarRef reference = SIScalarCreateCopy(RMNQuantitativeDimensionGetReferenceOffset(srcBase));
    if (!origin || !reference) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to copy origin or reference offset\n");
        OCRelease(origin);
        OCRelease(reference);
        OCRelease(copy);
        return NULL;
    }

    if (!RMNQuantitativeDimensionSetOriginOffset(dst, origin) ||
        !RMNQuantitativeDimensionSetReferenceOffset(dst, reference)) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to set origin or reference offset\n");
        OCRelease(origin);
        OCRelease(reference);
        OCRelease(copy);
        return NULL;
    }
    OCRelease(origin);
    OCRelease(reference);

    SIScalarRef srcPeriod = RMNQuantitativeDimensionGetPeriod(srcBase);
    if (srcPeriod) {
        SIScalarRef period = SIScalarCreateCopy(srcPeriod);
        if (!period || !RMNQuantitativeDimensionSetPeriod(dst, period)) {
            fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to copy or set period\n");
            OCRelease(period);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(period);
    }

    if (!RMNQuantitativeDimensionSetPeriodic(dst, RMNQuantitativeDimensionIsPeriodic(srcBase)) ||
        !RMNQuantitativeDimensionSetScaling(dst, RMNQuantitativeDimensionGetScaling(srcBase))) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to set periodic flag or scaling\n");
        OCRelease(copy);
        return NULL;
    }

    OCStringRef label = OCStringCreateCopy(OCDimensionGetLabel((RMNDimensionRef)src));
    OCStringRef desc  = OCStringCreateCopy(OCDimensionGetDescription((RMNDimensionRef)src));
    if (!OCDimensionSetLabel((RMNDimensionRef)copy, label) ||
        !OCDimensionSetDescription((RMNDimensionRef)copy, desc)) {
        fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to set label or description\n");
        OCRelease(label);
        OCRelease(desc);
        OCRelease(copy);
        return NULL;
    }
    OCRelease(label);
    OCRelease(desc);

    OCDictionaryRef meta = OCDimensionGetMetaData((RMNDimensionRef)src);
    if (meta) {
        OCDictionaryRef metaCopy = OCDictionaryCreateCopy(meta);
        if (!metaCopy || !OCDimensionSetMetaData((RMNDimensionRef)copy, metaCopy)) {
            fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to copy metadata\n");
            OCRelease(metaCopy);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(metaCopy);
    }

    OCStringRef quantityName = RMNQuantitativeDimensionGetQuantityName(srcBase);
    if (quantityName) {
        OCStringRef nameCopy = OCStringCreateCopy(quantityName);
        if (!nameCopy || !RMNQuantitativeDimensionSetQuantityName(dst, nameCopy)) {
            fprintf(stderr, "RMNMonotonicDimensionCreateCopy: failed to copy quantity name\n");
            OCRelease(nameCopy);
            OCRelease(copy);
            return NULL;
        }
        OCRelease(nameCopy);
    }

    return copy;
}
OCArrayRef RMNMonotonicDimensionGetCoordinates(RMNMonotonicDimensionRef dim) {
    return dim ? dim->coordinates : NULL;
}
bool RMNMonotonicDimensionSetCoordinates(RMNMonotonicDimensionRef dim, OCArrayRef coordinates) {
    if (!dim || !coordinates) return false;
    if (dim->coordinates == coordinates) return true;
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    return true;
}
#pragma endregion RMNMonotonicDimension
#pragma region RMNLinearDimension
// ============================================================================
// MARK: - (6) RMNLinearDimension
// ============================================================================
static OCTypeID kRMNLinearDimensionID = _kOCNotATypeID;
struct __RMNLinearDimension {
//  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;

//  RMNQuantitativeDimension
    OCStringRef quantityName;
    SIScalarRef referenceOffset;
    SIScalarRef originOffset;
    SIScalarRef period;

    bool periodic;
    dimensionScaling scaling;

//  RMNLinearDimension
    RMNQuantitativeDimensionRef reciprocal;
    OCIndex count;
    SIScalarRef increment;
    SIScalarRef inverseIncrement;
    bool fft;
};
OCTypeID RMNLinearDimensionGetTypeID(void) {
    if (kRMNLinearDimensionID == _kOCNotATypeID)
        kRMNLinearDimensionID = OCRegisterType("RMNLinearDimension");
    return kRMNLinearDimensionID;
}
static void __RMNLinearDimensionFinalize(const void *obj) {
    RMNLinearDimensionRef dim = (RMNLinearDimensionRef)obj;
    __RMNQuantitativeDimensionFinalize((struct __RMNDimension*)dim);
    OCRelease(dim->increment);           dim->increment = NULL;
    OCRelease(dim->inverseIncrement);    dim->inverseIncrement = NULL;
    dim->count    = 0;
    dim->periodic = false;
    dim->fft      = false;
}
static OCStringRef __RMNLinearDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNLinearDimension>");
}
static void *__RMNLinearDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    RMNLinearDimensionRef src = (RMNLinearDimensionRef)obj;

    return RMNLinearDimensionCreate(
        RMNLinearDimensionGetCount(src),
        RMNLinearDimensionGetIncrement(src),
        RMNLinearDimensionGetOrigin(src),
        RMNLinearDimensionGetReferenceOffset(src),
        RMNQuantitativeDimensionGetPeriod((RMNQuantitativeDimensionRef)src),
        RMNQuantitativeDimensionGetQuantityName((RMNQuantitativeDimensionRef)src)
    );
}
static RMNLinearDimensionRef RMNLinearDimensionAllocate(void) {
    RMNLinearDimensionRef obj = OCTypeAlloc(struct __RMNLinearDimension,
            RMNLinearDimensionGetTypeID(),
            __RMNLinearDimensionFinalize,
            NULL,
            __RMNLinearDimensionCopyFormattingDesc,
            __RMNLinearDimensionDeepCopy,
            __RMNLinearDimensionDeepCopy);

    __RMNInitBaseFields((RMNDimensionRef)obj);
    __RMNInitQuantitativeFields((RMNQuantitativeDimensionRef)obj);
    obj->count            = 0;
    obj->increment        = NULL;
    obj->inverseIncrement = NULL;
    obj->fft              = false;
    return obj;
}
RMNLinearDimensionRef RMNLinearDimensionCreate(OCIndex count,
                                              SIScalarRef increment,
                                              SIScalarRef origin,
                                              SIScalarRef referenceOffset,
                                              SIScalarRef period,
                                              OCStringRef quantityName)
{
    if (!increment || count <= 1) return NULL;
    RMNLinearDimensionRef dim = RMNLinearDimensionAllocate();
    if (!dim) return NULL;
    dim->count          = count;
    dim->increment      = OCRetain(increment);
    dim->originOffset   = origin ? OCRetain(origin) : NULL;
    dim->referenceOffset = referenceOffset ? OCRetain(referenceOffset) : NULL;
    dim->period         = period ? OCRetain(period) : NULL;
    dim->quantityName   = quantityName ? OCRetain(quantityName) : NULL;
    return dim;
}
OCIndex RMNLinearDimensionGetCount(RMNLinearDimensionRef dim) {
    return dim ? dim->count : 0;
}
SIScalarRef RMNLinearDimensionGetIncrement(RMNLinearDimensionRef dim) {
    return dim ? dim->increment : NULL;
}
SIScalarRef RMNLinearDimensionGetOrigin(RMNLinearDimensionRef dim) {
    return dim ? dim->originOffset : NULL;
}
SIScalarRef RMNLinearDimensionGetReferenceOffset(RMNLinearDimensionRef dim) {
    return dim ? dim->referenceOffset : NULL;
}
bool RMNIsLinearDimension(OCTypeRef obj) {
    return OCGetTypeID(obj) == RMNLinearDimensionGetTypeID();
}
#pragma endregion RMNLinearDimension
