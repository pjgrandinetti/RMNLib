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

// ============================================================================
// MARK: - (1) Shared Utility Functions & Macros
// ============================================================================

// Initialize the three “base” fields.
// Used by every allocator after OCTypeAlloc(…) has constructed the object.
static void __RMNInitBaseFields(RMNDimensionRef dim) {
    dim->label      = OCStringCreateWithCString("");
    dim->description = OCStringCreateWithCString("");
    dim->metaData    = OCDictionaryCreateMutable(0);
}

// A little macro to reduce boilerplate.  It allocates an object of type
// “struct __<TypeName>”, registers it with its TypeID, installs the Finalizer & CopyFormatting,
// then calls __RMNInitBaseFields() to set the label/description/metaData to default values.
// Finally returns the newly allocated object.
#define RMN_ALLOC_RETURN(TypeName) ({ \
    TypeName##Ref obj = (TypeName##Ref)OCTypeAlloc(sizeof(struct __##TypeName), \
        TypeName##GetTypeID(), \
        __##TypeName##Finalize, \
        NULL, \
        __##TypeName##CopyFormattingDesc); \
    __RMNInitBaseFields((struct __RMNDimension*)obj); \
    obj; \
})

// ============================================================================
// MARK: - (2) RMNDimension (Abstract Base)
// ============================================================================

static OCTypeID kRMNDimensionID = _kOCNotATypeID;

struct __RMNDimension {
 //  RMNDimension
    OCBase _base;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metaData;
};

OCTypeID RMNDimensionGetTypeID(void) {
    if (kRMNDimensionID == _kOCNotATypeID)
        kRMNDimensionID = OCRegisterType("RMNDimension");
    return kRMNDimensionID;
}

// Finalizer: release label/description/metaData.
static void __RMNDimensionFinalize(const void *obj) {
    if (!dim) return;
    OCRelease(dim->label); dim->label = NULL;
    OCRelease(dim->description); dim->description = NULL;
    OCRelease(dim->metaData); dim->metaData = NULL;
}

static OCStringRef __RMNDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNDimension>");
}

// Accessors (shared by all RMNDimensionRef instances)
OCStringRef OCDimensionGetLabel(RMNDimensionRef dim) {
    return dim ? dim->label : NULL;
}

void OCDimensionSetLabel(RMNDimensionRef dim, OCStringRef label) {
    if (!dim) return;
    OCRelease(dim->label);
    dim->label = label ? OCRetain(label) : NULL;
}

OCStringRef OCDimensionGetDescription(RMNDimensionRef dim) {
    return dim ? dim->description : NULL;
}

void OCDimensionSetDescription(RMNDimensionRef dim, OCStringRef desc) {
    if (!dim) return;
    OCRelease(dim->description);
    dim->description = desc ? OCRetain(desc) : NULL;
}

OCDictionaryRef OCDimensionGetMetaData(RMNDimensionRef dim) {
    return dim ? dim->metaData : NULL;
}

void OCDimensionSetMetaData(RMNDimensionRef dim, OCDictionaryRef dict) {
    if (!dim) return;
    if (dim->metaData == dict) return;
    OCRelease(dim->metaData);
    dim->metaData = dict ? OCRetain(dict) : NULL;
}

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

// Finalizer: first release base fields, then release the “labels” array.
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

// Allocator: use RMN_ALLOC_RETURN macro, then initialize “labels” to an empty mutable array.
static RMNLabeledDimensionRef RMNLabeledDimensionAllocate(void) {
    RMNLabeledDimensionRef obj = RMN_ALLOC_RETURN(RMNLabeledDimension);
    obj->labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return obj;
}

// Factory: must supply at least 2 labels or fail
RMNLabeledDimensionRef RMNLabeledDimensionCreateWithLabels(OCArrayRef inputLabels) {
    if (!inputLabels || OCArrayGetCount(inputLabels) < 2)
        return NULL;
    RMNLabeledDimensionRef dim = RMNLabeledDimensionAllocate();
    if (!dim) return NULL;
    OCRelease(dim->labels);
    dim->labels = OCArrayCreateMutableCopy(inputLabels);
    return dim;
}

// Accessors:
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

    RMNQuantitativeDimensionRef reciprocal;
};

OCTypeID RMNQuantitativeDimensionGetTypeID(void) {
    if (kRMNQuantitativeDimensionID == _kOCNotATypeID)
        kRMNQuantitativeDimensionID = OCRegisterType("RMNQuantitativeDimension");
    return kRMNQuantitativeDimensionID;
}

// Finalizer: release base fields + quantityName/offsets/period/reciprocal
static void __RMNQuantitativeDimensionFinalize(const void *obj) {
    RMNQuantitativeDimensionRef dim = (RMNQuantitativeDimensionRef)obj;
    __RMNDimensionFinalize((RMNDimensionRef)dim);
    OCRelease(dim->quantityName);        dim->quantityName = NULL;
    OCRelease(dim->referenceOffset);     dim->referenceOffset = NULL;
    OCRelease(dim->originOffset);        dim->originOffset = NULL;
    OCRelease(dim->period);              dim->period = NULL;
    OCRelease(dim->reciprocal);          dim->reciprocal = NULL;
}

static OCStringRef __RMNQuantitativeDimensionCopyFormattingDesc(OCTypeRef cf) {
    (void)cf;
    return OCStringCreateWithCString("<RMNQuantitativeDimension>");
}

// Helper to initialize numeric dimension fields (assumes base fields already set)
static void __RMNInitQuantitativeFields(RMNQuantitativeDimensionRef dim) {
    dim->quantityName    = NULL;
    dim->referenceOffset = NULL;
    dim->originOffset    = NULL;
    dim->period          = NULL;
    dim->periodic        = false;
    dim->scaling         = kDimensionScalingNone;
    dim->reciprocal      = NULL;
}

// Allocator for RMNQuantitativeDimension: use RMN_ALLOC_RETURN, then init the quantitative fields
static RMNQuantitativeDimensionRef RMNQuantitativeDimensionAllocate(void) {
    RMNQuantitativeDimensionRef obj = RMN_ALLOC_RETURN(RMNQuantitativeDimension);
    __RMNInitQuantitativeFields(obj);
    return obj;
}

// Factory: a “Create” function that returns an object with default numeric fields
RMNQuantitativeDimensionRef RMNQuantitativeDimensionCreate(void) {
    RMNQuantitativeDimensionRef dim = RMNQuantitativeDimensionAllocate();
    return dim;
}

// Accessors (quantityName, referenceOffset, originOffset, period, periodic, scaling, reciprocal):
OCStringRef RMNQuantitativeDimensionGetQuantityName(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
void RMNQuantitativeDimensionSetQuantityName(RMNQuantitativeDimensionRef dim, OCStringRef name) {
    if (!dim) return;
    OCRelease(dim->quantityName);
    dim->quantityName = name ? OCRetain(name) : NULL;
}

SIScalarRef RMNQuantitativeDimensionGetReferenceOffset(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->referenceOffset : NULL;
}
void RMNQuantitativeDimensionSetReferenceOffset(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return;
    OCRelease(dim->referenceOffset);
    dim->referenceOffset = val ? OCRetain(val) : NULL;
}

SIScalarRef RMNQuantitativeDimensionGetOriginOffset(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->originOffset : NULL;
}
void RMNQuantitativeDimensionSetOriginOffset(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return;
    OCRelease(dim->originOffset);
    dim->originOffset = val ? OCRetain(val) : NULL;
}

SIScalarRef RMNQuantitativeDimensionGetPeriod(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->period : NULL;
}
void RMNQuantitativeDimensionSetPeriod(RMNQuantitativeDimensionRef dim, SIScalarRef val) {
    if (!dim) return;
    OCRelease(dim->period);
    dim->period = val ? OCRetain(val) : NULL;
}

bool RMNQuantitativeDimensionIsPeriodic(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->periodic : false;
}
void RMNQuantitativeDimensionSetPeriodic(RMNQuantitativeDimensionRef dim, bool flag) {
    if (dim) dim->periodic = flag;
}

dimensionScaling RMNQuantitativeDimensionGetScaling(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->scaling : kDimensionScalingNone;
}
void RMNQuantitativeDimensionSetScaling(RMNQuantitativeDimensionRef dim, dimensionScaling scaling) {
    if (dim) dim->scaling = scaling;
}

RMNQuantitativeDimensionRef RMNQuantitativeDimensionGetReciprocal(RMNQuantitativeDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}
void RMNQuantitativeDimensionSetReciprocal(RMNQuantitativeDimensionRef dim, RMNQuantitativeDimensionRef r) {
    if (!dim) return;
    OCRelease(dim->reciprocal);
    dim->reciprocal = r ? OCRetain(r) : NULL;
}

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

    RMNQuantitativeDimensionRef reciprocal;

//  RMNMonotonicDimension
    OCMutableArrayRef coordinates;
};

OCTypeID RMNMonotonicDimensionGetTypeID(void) {
    if (kRMNMonotonicDimensionID == _kOCNotATypeID)
        kRMNMonotonicDimensionID = OCRegisterType("RMNMonotonicDimension");
    return kRMNMonotonicDimensionID;
}

// Finalizer: first release quantitative fields, then release “coordinates”
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

static RMNMonotonicDimensionRef RMNMonotonicDimensionAllocate(void) {
    RMNMonotonicDimensionRef obj = RMN_ALLOC_RETURN(RMNMonotonicDimension);
    __RMNInitQuantitativeFields((RMNQuantitativeDimensionRef)obj);
    obj->coordinates = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return obj;
}

// Factory: requires at least two coordinates
RMNMonotonicDimensionRef RMNMonotonicDimensionCreateWithCoordinates(OCArrayRef coordinates) {
    if (!coordinates || OCArrayGetCount(coordinates) < 2)
        return NULL;
    RMNMonotonicDimensionRef dim = RMNMonotonicDimensionAllocate();
    if (!dim) return NULL;
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    return dim;
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

    RMNQuantitativeDimensionRef reciprocal;

//  RMNLinearDimension
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

// Finalizer: first release quantitative fields, then release the RMNLinear fields
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

static RMNLinearDimensionRef RMNLinearDimensionAllocate(void) {
    RMNLinearDimensionRef obj = RMN_ALLOC_RETURN(RMNLinearDimension);
    __RMNInitQuantitativeFields((RMNQuantitativeDimensionRef)obj);
    obj->count     = 0;
    obj->increment = NULL;
    obj->inverseIncrement = NULL;
    obj->fft       = false;
    return obj;
}

// Factory: must supply a valid increment and count > 1
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
    SIScalarRef inv     = SIScalarCreateInverse(increment);
    dim->inverseIncrement = inv ? inv : NULL;
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