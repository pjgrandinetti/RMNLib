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
static cJSON *impl_DimensionCreateJSON(const void *obj) {
    DimensionRef dim = (DimensionRef)obj;
    if (!dim) return cJSON_CreateNull();
    // 1) Serialize to an OC-dictionary
    OCDictionaryRef dict = impl_DimensionCopyAsDictionary(dim);
    if (!dict) return cJSON_CreateNull();
    // 2) Convert that entire dictionary to JSON
    cJSON *json = OCDictionaryCreateJSON(dict);
    // 3) Clean up
    OCRelease(dict);
    return json;
}
static void *impl_DimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = impl_DimensionCopyAsDictionary((DimensionRef)obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    DimensionRef copy = impl_DimensionCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
static DimensionRef impl_DimensionAllocate(void) {
    return OCTypeAlloc(
        struct impl_Dimension,
        DimensionGetTypeID(),
        impl_DimensionFinalize,
        impl_DimensionEqual,
        impl_DimensionCopyFormattingDesc,
        impl_DimensionCreateJSON,
        impl_DimensionDeepCopy,
        impl_DimensionDeepCopy);
}
static DimensionRef impl_DimensionCreate(OCStringRef label,
                                         OCStringRef description,
                                         OCDictionaryRef metadata) {
    // 1) Raw allocation
    DimensionRef dim = impl_DimensionAllocate();
    if (!dim) return NULL;
    // 2) One-time default initialization
    impl_InitBaseDimensionFields(dim);
    // 3) Override with user values via setters (they handle copying & erroring)
    if (label) {
        if (!DimensionSetLabel(dim, label)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (description) {
        if (!DimensionSetDescription(dim, description)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (metadata) {
        if (!DimensionSetMetadata(dim, metadata)) {
            OCRelease(dim);
            return NULL;
        }
    }
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
    // 1) Pull values out of the dictionary
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    // 2) Just call your one true initializer
    //    (it will allocate + init exactly once, then return NULL on any failure)
    return impl_DimensionCreate(label, description, metadata);
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
        OCDictionarySetValue((OCMutableDictionaryRef)dict, STR("label"), lblCopy);
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
        OCDictionarySetValue((OCMutableDictionaryRef)dict, STR("description"), descCopy);
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
        OCDictionarySetValue((OCMutableDictionaryRef)dict, STR("metadata"), metaCopy);
        OCRelease(metaCopy);
    }
    return dict;
}
static OCDictionaryRef DimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for Dimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, "label");
    if (cJSON_IsString(item)) {
        OCStringRef label = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("label"), label);
        OCRelease(label);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCStringRef desc = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("description"), desc);
        OCRelease(desc);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, "metadata");
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), metadata);
        OCRelease(metadata);
    }
    return dict;
}
DimensionRef DimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for Dimension");
        return NULL;
    }
    // Schema-bound: interpret known fields only
    OCDictionaryRef dict = DimensionDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    // Delegate to the type-dispatching dictionary constructor
    DimensionRef dim = DimensionCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dim;
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
cJSON *impl_LabeledDimensionCreateJSON(const void *obj) {
    const LabeledDimensionRef ldim = (const LabeledDimensionRef)obj;
    if (!ldim) return cJSON_CreateNull();
    // First, serialize the base fields using impl_DimensionCreateJSON
    cJSON *json = impl_DimensionCreateJSON(&ldim->_super);
    if (!json) return NULL;
    // Now, add the subclass field: coordinateLabels (OCMutableArrayRef)
    if (ldim->coordinateLabels) {
        cJSON *labels_json = OCTypeCopyJSON((OCTypeRef)ldim->coordinateLabels);
        if (labels_json)
            cJSON_AddItemToObject(json, "coordinateLabels", labels_json);
    }
    return json;
}
static void *impl_LabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = LabeledDimensionCopyAsDictionary((LabeledDimensionRef)obj);
    if (!dict) return NULL;
    LabeledDimensionRef copy = LabeledDimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
static LabeledDimensionRef LabeledDimensionAllocate(void) {
    return OCTypeAlloc(
        struct impl_LabeledDimension,
        LabeledDimensionGetTypeID(),
        impl_LabeledDimensionFinalize,
        impl_LabeledDimensionEqual,
        impl_LabeledDimensionCopyFormattingDesc,
        impl_LabeledDimensionCreateJSON,
        impl_LabeledDimensionDeepCopy,
        impl_LabeledDimensionDeepCopy);
}
LabeledDimensionRef
LabeledDimensionCreate(OCStringRef label,
                       OCStringRef description,
                       OCDictionaryRef metadata,
                       OCArrayRef coordinateLabels) {
    // 1) Preconditions
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        fprintf(stderr, "LabeledDimensionCreate: need ≥2 coordinate labels\n");
        return NULL;
    }
    // 2) Allocate the raw object
    LabeledDimensionRef dim = LabeledDimensionAllocate();
    if (!dim) return NULL;
    // 3) Initialize the base Dimension fields
    impl_InitBaseDimensionFields((DimensionRef)dim);
    if (label) {
        if (!DimensionSetLabel((DimensionRef)dim, label)) goto Err;
    }
    if (description) {
        if (!DimensionSetDescription((DimensionRef)dim, description)) goto Err;
    }
    if (metadata) {
        if (!DimensionSetMetadata((DimensionRef)dim, metadata)) goto Err;
    }
    // 4) Initialize the subclass field exactly once
    dim->coordinateLabels = OCArrayCreateMutableCopy(coordinateLabels);
    if (!dim->coordinateLabels) goto Err;
    return dim;
Err:
    OCRelease(dim);
    return NULL;
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
// ============================================================================
// (3) LabeledDimensionCreateFromDictionary
// ============================================================================
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict,
                                     OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError)
            *outError = OCStringCreateWithCString("LabeledDimensionFromDict: dict is NULL");
        return NULL;
    }
    // Type discriminator
    OCStringRef type = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (!type || !OCStringEqual(type, STR("labeled"))) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("LabeledDimensionFromDict: expected type “labeled”, got “%@”"),
                type ? type : STR("<none>"));
        }
        return NULL;
    }
    // Pull out fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    OCArrayRef coordinateLabels = (OCArrayRef)OCDictionaryGetValue(dict, STR("labels"));
    // Construct
    LabeledDimensionRef dim = LabeledDimensionCreate(
        label,
        description,
        metadata,
        coordinateLabels);
    if (!dim && outError) {
        *outError = OCStringCreateWithCString("LabeledDimensionCreate() failed");
    }
    return dim;
}
OCDictionaryRef LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Begin with the base‐class serialization (immutable) and cast to mutable
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) Record the concrete type so CreateFromDictionary knows which constructor to use
    OCDictionarySetValue(dict, STR("type"), STR("labeled"));
    // 3) Deep‐copy coordinate labels into "labels"
    OCArrayRef labels = LabeledDimensionGetCoordinateLabels(dim);
    if (labels) {
        OCArrayRef labelsCopy = (OCArrayRef)OCTypeDeepCopy((OCTypeRef)labels);
        if (labelsCopy) {
            OCDictionarySetValue(dict, STR("labels"), labelsCopy);
            OCRelease(labelsCopy);
        }
    }
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef LabeledDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for LabeledDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    // type (must be "labeled")
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item) || strcmp(item->valuestring, "labeled") != 0) {
        if (outError) *outError = STR("Expected \"type\": \"labeled\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef type = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("type"), type);
    OCRelease(type);
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, "label");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("label"), s);
        OCRelease(s);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("description"), s);
        OCRelease(s);
    }
    // Optional: metadata (if available)
    item = cJSON_GetObjectItemCaseSensitive(json, "metadata");
    if (cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), metadata);
        OCRelease(metadata);
    }
    // Required: labels array
    item = cJSON_GetObjectItemCaseSensitive(json, "labels");
    if (!cJSON_IsArray(item)) {
        if (outError) *outError = STR("Missing or invalid \"labels\" array");
        OCRelease(dict);
        return NULL;
    }
    OCMutableArrayRef labelArr = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
    cJSON *entry;
    cJSON_ArrayForEach(entry, item) {
        if (cJSON_IsString(entry)) {
            OCStringRef lbl = OCStringCreateWithCString(entry->valuestring);
            OCArrayAppendValue(labelArr, lbl);
            OCRelease(lbl);
        }
    }
    OCDictionarySetValue(dict, STR("labels"), labelArr);
    OCRelease(labelArr);
    return dict;
}
LabeledDimensionRef LabeledDimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for LabeledDimension");
        return NULL;
    }
    OCDictionaryRef dict = LabeledDimensionDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    LabeledDimensionRef dim = LabeledDimensionCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dim;
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
    OCStringRef qty = SIDimensionGetQuantityName(d);
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
#include "cJSON.h"
// Assuming dimensionScaling is an enum or integer type.
cJSON *impl_SIDimensionCreateJSON(const void *obj) {
    const SIDimensionRef sidim = (const SIDimensionRef)obj;
    if (!sidim) return cJSON_CreateNull();
    // Serialize base fields using impl_DimensionCreateJSON
    cJSON *json = impl_DimensionCreateJSON(&sidim->_super);
    if (!json) return NULL;
    // quantityName (OCStringRef)
    if (sidim->quantityName) {
        cJSON *qname_json = OCTypeCopyJSON((OCTypeRef)sidim->quantityName);
        if (qname_json)
            cJSON_AddItemToObject(json, "quantityName", qname_json);
    }
    // offset (SIScalarRef)
    if (sidim->offset) {
        cJSON *offset_json = OCTypeCopyJSON((OCTypeRef)sidim->offset);
        if (offset_json)
            cJSON_AddItemToObject(json, "offset", offset_json);
    }
    // origin (SIScalarRef)
    if (sidim->origin) {
        cJSON *origin_json = OCTypeCopyJSON((OCTypeRef)sidim->origin);
        if (origin_json)
            cJSON_AddItemToObject(json, "origin", origin_json);
    }
    // period (SIScalarRef)
    if (sidim->period) {
        cJSON *period_json = OCTypeCopyJSON((OCTypeRef)sidim->period);
        if (period_json)
            cJSON_AddItemToObject(json, "period", period_json);
    }
    // periodic (bool)
    cJSON_AddBoolToObject(json, "periodic", sidim->periodic);
    // scaling (dimensionScaling, assumed integer/enum)
    cJSON_AddNumberToObject(json, "scaling", (int)sidim->scaling);
    return json;
}
static void *impl_SIDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = SIDimensionCopyAsDictionary((SIDimensionRef)obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    SIDimensionRef copy = SIDimensionCreateFromDictionary(dict, NULL);
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
    return OCTypeAlloc(
        struct impl_SIDimension,
        SIDimensionGetTypeID(),
        impl_SIDimensionFinalize,
        impl_SIDimensionEqual,
        impl_SIDimensionCopyFormattingDesc,
        impl_SIDimensionCreateJSON,
        impl_SIDimensionDeepCopy,
        impl_SIDimensionDeepCopy);
}
/// Returns true if `dim` passes *all* of the same checks that SIDimensionCreate performs.
/// On failure, returns false and (optionally) writes a human-readable error into *outErr.
bool SIDimensionValidate(SIDimensionRef dim, OCStringRef *outErr) {
    if (!dim) {
        if (outErr) *outErr = STR("dimension is NULL");
        return false;
    }
    // 1) quantityName must be non-NULL and known
    OCStringRef qty = SIDimensionGetQuantityName(dim);
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
    DimensionRef baseDim = (DimensionRef)dim;
    bool success = true;
    success *= DimensionSetLabel(baseDim, label);
    success *= DimensionSetDescription(baseDim, description);
    success *= DimensionSetMetadata(baseDim, metadata);
    success *= SIDimensionSetOffset(dim, offset);
    success *= SIDimensionSetQuantityName(dim, quantityName);
    success *= SIDimensionSetOrigin(dim, origin);
    success *= (!periodic || SIDimensionSetPeriod(dim, period));
    success *= SIDimensionSetPeriodic(dim, periodic);
    success *= SIDimensionSetScaling(dim, scaling);
    return success;
}
SIDimensionRef SIDimensionCreate(
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
    // 2) offset must be real-valued
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr, "SIDimensionCreate: offset must be real-valued\n");
        return NULL;
    }
    SIDimensionalityRef refDim = SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    // 3) quantityName must match offset dimensionality
    {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim = SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim ||
            !SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
            fprintf(stderr,
                    "SIDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                    OCStringGetCString(quantityName));
            if (err) OCRelease(err);
            return NULL;
        }
        OCRelease(err);
    }
    // 4) origin: default to zero if NULL, else validate
    bool originWasDefaulted = false;
    SIScalarRef tmpZero = NULL;
    if (origin) {
        if (SIQuantityIsComplexType((SIQuantityRef)origin) ||
            !SIDimensionalityHasSameReducedDimensionality(
                refDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)origin))) {
            fprintf(stderr, "SIDimensionCreate: origin invalid or mismatched\n");
            return NULL;
        }
    } else {
        SIUnitRef u = SIQuantityGetUnit((SIQuantityRef)offset);
        tmpZero = SIScalarCreateWithDouble(0.0, u);
        if (!tmpZero) {
            fprintf(stderr, "SIDimensionCreate: failed to create zero origin\n");
            return NULL;
        }
        origin = tmpZero;
        originWasDefaulted = true;
    }
    // 5) period: only enforce if periodic==true
    if (periodic) {
        if (!period ||
            SIQuantityIsComplexType((SIQuantityRef)period) ||
            !SIDimensionalityHasSameReducedDimensionality(
                refDim,
                SIQuantityGetUnitDimensionality((SIQuantityRef)period))) {
            fprintf(stderr, "SIDimensionCreate: invalid or mismatched period\n");
            if (originWasDefaulted) OCRelease(tmpZero);
            return NULL;
        }
    } else {
        period = NULL;
    }
    // 6) Allocate
    SIDimensionRef dim = SIDimensionAllocate();
    if (!dim) {
        fprintf(stderr, "SIDimensionCreate: allocation failed\n");
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    // 7) Initialize default fields
    impl_InitBaseDimensionFields((DimensionRef)&dim->_super);
    impl_InitSIDimensionFields(dim);
    // 8) Apply user values
    if (label &&
        !DimensionSetLabel((DimensionRef)dim, label)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    if (description &&
        !DimensionSetDescription((DimensionRef)dim, description)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    if (metadata &&
        !DimensionSetMetadata((DimensionRef)dim, metadata)) {
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
    // 9) Copy SI‐specific fields
    dim->quantityName = OCStringCreateCopy(quantityName);
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionCreate: failed to copy quantityName\n");
        OCRelease(dim);
        if (originWasDefaulted) OCRelease(tmpZero);
        return NULL;
    }
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
    // 10) Clean up temp zero‐origin
    if (originWasDefaulted) {
        OCRelease(tmpZero);
    }
    // 11) Flags
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
    // 1) Look up the dimensionality for the requested quantityName name.
    OCStringRef error = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(name, &error);
    if (!nameDim) {
        if (error) {
            fprintf(stderr, "SIDimensionSetQuantityName: %s\n",
                    OCStringGetCString(error));
            OCRelease(error);
        } else {
            fprintf(stderr, "SIDimensionSetQuantityName: unknown quantityName \"%s\"\n",
                    OCStringGetCString(name));
        }
        return false;
    }
    OCRelease(error);
    // 2) Get the dimensionality of our offset.
    SIScalarRef coords = dim->offset;
    if (!coords) {
        fprintf(stderr, "SIDimensionSetQuantityName: cannot validate without offset\n");
        return false;
    }
    SIDimensionalityRef refDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)coords);
    // 3) Compare reduced dimensionalities.
    if (!SIDimensionalityHasSameReducedDimensionality(nameDim, refDim)) {
        fprintf(stderr,
                "SIDimensionSetQuantityName: dimensionality mismatch between \"%s\" and existing unit\n",
                OCStringGetCString(name));
        return false;
    }
    // 4) All good — replace the old name.
    OCRelease(dim->quantityName);
    dim->quantityName = OCStringCreateCopy(name);
    if (!dim->quantityName) {
        fprintf(stderr, "SIDimensionSetQuantityName: failed to copy name\n");
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
    SIUnitRef relUnit = SIQuantityGetUnit((SIQuantityRef)dim->offset);
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
SIDimensionRef SIDimensionCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError) {
    OCStringRef parseErr = NULL;
    OCStringRef label;
    OCStringRef description;
    OCDictionaryRef metadata;
    OCStringRef quantityName;
    OCStringRef offsetStr;
    SIScalarRef offset;
    OCStringRef originStr;
    SIScalarRef origin;
    OCStringRef periodStr;
    SIScalarRef period;
    OCBooleanRef boolObj;
    bool periodic;
    OCNumberRef numObj;
    dimensionScaling scaling;
    SIDimensionRef dim;
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError)
            *outError = STR("SIDimensionCreateFromDictionary: dictionary is NULL");
        return NULL;
    }
    /* 1) Base fields */
    label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    /* 2) quantity_name (required) */
    quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR("quantity_name"));
    if (!quantityName) {
        if (outError)
            *outError = STR("SIDimensionCreateFromDictionary: missing \"quantity_name\"");
        return NULL;
    }
    /* 3) offset (string → SIScalar) */
    offsetStr = (OCStringRef)OCDictionaryGetValue(dict, STR("offset"));
    if (!offsetStr) {
        if (outError)
            *outError = STR("SIDimensionCreateFromDictionary: missing \"offset\"");
        return NULL;
    }
    offset = SIScalarCreateFromExpression(offsetStr, &parseErr);
    if (!offset) {
        if (outError)
            *outError = parseErr;
        else if (parseErr)
            OCRelease(parseErr);
        return NULL;
    }
    /* 4) origin (optional) */
    origin = NULL;
    originStr = (OCStringRef)OCDictionaryGetValue(dict, STR("origin"));
    if (originStr) {
        origin = SIScalarCreateFromExpression(originStr, &parseErr);
        if (!origin) {
            if (outError)
                *outError = parseErr;
            else if (parseErr)
                OCRelease(parseErr);
            OCRelease(offset);
            return NULL;
        }
    }
    /* 5) period (optional) */
    period = NULL;
    periodStr = (OCStringRef)OCDictionaryGetValue(dict, STR("period"));
    if (periodStr) {
        period = SIScalarCreateFromExpression(periodStr, &parseErr);
        if (!period) {
            if (outError)
                *outError = parseErr;
            else if (parseErr)
                OCRelease(parseErr);
            OCRelease(offset);
            if (origin) OCRelease(origin);
            return NULL;
        }
    }
    /* 6) periodic flag */
    boolObj = (OCBooleanRef)OCDictionaryGetValue(dict, STR("periodic"));
    periodic = boolObj ? OCBooleanGetValue(boolObj) : false;
    /* 7) scaling enum */
    numObj = (OCNumberRef)OCDictionaryGetValue(dict, STR("scaling"));
    if (numObj) {
        int tmp = 0;
        OCNumberGetValue(numObj, kOCNumberIntType, &tmp);
        scaling = (dimensionScaling)tmp;
    } else {
        scaling = kDimensionScalingNone;
    }
    /* 8) call the real constructor (it deep-copies offset/origin/period internally) */
    dim = SIDimensionCreate(
        label,
        description,
        metadata,
        quantityName,
        offset,
        origin,
        period,
        periodic,
        scaling);
    /* 9) clean up temporaries */
    OCRelease(offset);
    if (origin) OCRelease(origin);
    if (period) OCRelease(period);
    if (!dim && outError)
        *outError = STR("SIDimensionCreateFromDictionary: SIDimensionCreate failed");
    return dim;
}
OCDictionaryRef SIDimensionCopyAsDictionary(SIDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with base–class serialization (label/description/metadata)
    OCMutableDictionaryRef dict = (OCMutableDictionaryRef)impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) Deep-copy the metadata dictionary
    OCDictionaryRef origMD = DimensionGetMetadata((DimensionRef)dim);
    if (origMD) {
        OCMutableDictionaryRef mdCopy = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(origMD);
        OCDictionarySetValue(dict, STR("metadata"), mdCopy);
        OCRelease(mdCopy);
    }
    // 3) quantity_name → string
    OCStringRef qty = SIDimensionGetQuantityName(dim);
    if (qty) {
        OCStringRef qtyCopy = OCStringCreateCopy(qty);
        OCDictionarySetValue(dict, STR("quantity_name"), qtyCopy);
        OCRelease(qtyCopy);
    }
    // 4) offset → string
    SIScalarRef off = SIDimensionGetOffset(dim);
    if (off) {
        OCStringRef offStr = SIScalarCreateStringValue(off);
        OCDictionarySetValue(dict, STR("offset"), offStr);
        OCRelease(offStr);
    }
    // 5) origin → string
    SIScalarRef orig = SIDimensionGetOrigin(dim);
    if (orig) {
        OCStringRef origStr = SIScalarCreateStringValue(orig);
        OCDictionarySetValue(dict, STR("origin"), origStr);
        OCRelease(origStr);
    }
    // 6) period → string
    SIScalarRef per = SIDimensionGetPeriod(dim);
    if (per) {
        OCStringRef perStr = SIScalarCreateStringValue(per);
        OCDictionarySetValue(dict, STR("period"), perStr);
        OCRelease(perStr);
    }
    // 7) periodic flag (bool)
    OCBooleanRef b = OCBooleanGetWithBool(SIDimensionIsPeriodic(dim));
    OCDictionarySetValue(dict, STR("periodic"), b);
    OCRelease(b);
    // 8) scaling enum (int)
    OCNumberRef n = OCNumberCreateWithInt((int)SIDimensionGetScaling(dim));
    OCDictionarySetValue(dict, STR("scaling"), n);
    OCRelease(n);
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef SIDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for SIDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item;
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, "label");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("label"), s);
        OCRelease(s);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("description"), s);
        OCRelease(s);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, "metadata");
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), metadata);
        OCRelease(metadata);
    }
    // Required: quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, "quantity_name");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"quantity_name\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef qname = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("quantity_name"), qname);
    OCRelease(qname);
    // Required: offset
    item = cJSON_GetObjectItemCaseSensitive(json, "offset");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"offset\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef offset = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("offset"), offset);
    OCRelease(offset);
    // Optional: origin
    item = cJSON_GetObjectItemCaseSensitive(json, "origin");
    if (cJSON_IsString(item)) {
        OCStringRef origin = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("origin"), origin);
        OCRelease(origin);
    }
    // Optional: period
    item = cJSON_GetObjectItemCaseSensitive(json, "period");
    if (cJSON_IsString(item)) {
        OCStringRef period = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("period"), period);
        OCRelease(period);
    }
    // Optional: periodic (bool)
    item = cJSON_GetObjectItemCaseSensitive(json, "periodic");
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR("periodic"), b);
        OCRelease(b);
    }
    // Optional: scaling (int enum)
    item = cJSON_GetObjectItemCaseSensitive(json, "scaling");
    if (cJSON_IsNumber(item)) {
        OCNumberRef n = OCNumberCreateWithInt(item->valueint);
        OCDictionarySetValue(dict, STR("scaling"), n);
        OCRelease(n);
    }
    // Optional: type discriminator
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (cJSON_IsString(item)) {
        OCStringRef t = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("type"), t);
        OCRelease(t);
    }
    return dict;
}
SIDimensionRef SIDimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for SIDimension");
        return NULL;
    }
    OCDictionaryRef dict = SIDimensionDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    SIDimensionRef dim = SIDimensionCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dim;
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
impl_SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf) {
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
cJSON *impl_SIMonotonicDimensionCreateJSON(const void *obj) {
    const SIMonotonicDimensionRef mono = (const SIMonotonicDimensionRef)obj;
    if (!mono) return cJSON_CreateNull();
    // Serialize base fields using impl_SIDimensionCreateJSON
    cJSON *json = impl_SIDimensionCreateJSON(&mono->_super);
    if (!json) return NULL;
    // reciprocal (SIDimensionRef, i.e., struct impl_SIDimension *)
    if (mono->reciprocal) {
        cJSON *recip_json = impl_SIDimensionCreateJSON(mono->reciprocal);
        if (recip_json)
            cJSON_AddItemToObject(json, "reciprocal", recip_json);
    }
    // coordinates (OCMutableArrayRef)
    if (mono->coordinates) {
        cJSON *coords_json = OCTypeCopyJSON((OCTypeRef)mono->coordinates);
        if (coords_json)
            cJSON_AddItemToObject(json, "coordinates", coords_json);
    }
    return json;
}
static void *impl_SIMonotonicDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // 1) Serialize to a dictionary
    OCDictionaryRef dict = SIMonotonicDimensionCopyAsDictionary((SIMonotonicDimensionRef)obj);
    if (!dict) return NULL;
    // 2) Rehydrate a new instance
    SIMonotonicDimensionRef copy =
        SIMonotonicDimensionCreateFromDictionary(dict, NULL);
    // 3) Clean up
    OCRelease(dict);
    return copy;
}
// internal, doesn’t touch any fields
static SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void) {
    return OCTypeAlloc(
        struct impl_SIMonotonicDimension,
        SIMonotonicDimensionGetTypeID(),
        impl_SIMonotonicDimensionFinalize,
        impl_SIMonotonicDimensionEqual,
        impl_SIMonotonicDimensionCopyFormattingDesc,
        impl_SIMonotonicDimensionCreateJSON,
        impl_SIMonotonicDimensionDeepCopy,
        impl_SIMonotonicDimensionDeepCopy);
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
SIMonotonicDimensionRef SIMonotonicDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling,
    OCArrayRef coordinates,
    SIDimensionRef reciprocal) {
    // 1) Basic prechecks
    if (!coordinates || OCArrayGetCount(coordinates) < 2) {
        fprintf(stderr, "SIMonotonicDimensionCreate: need ≥2 coordinates\n");
        return NULL;
    }
    if (!quantityName) {
        fprintf(stderr, "SIMonotonicDimensionCreate: quantityName must be non-NULL\n");
        return NULL;
    }
    if (!offset) {
        fprintf(stderr, "SIMonotonicDimensionCreate: offset must be non-NULL\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr, "SIMonotonicDimensionCreate: offset must be real-valued\n");
        return NULL;
    }
    // 2) Dimensionality for quantity
    OCStringRef err = NULL;
    SIDimensionalityRef qDim = SIDimensionalityForQuantity(quantityName, &err);
    if (!qDim) {
        fprintf(stderr, "SIMonotonicDimensionCreate: quantityName not recognized: %s\n", OCStringGetCString(quantityName));
        if (err) OCRelease(err);
        return NULL;
    }
    SIDimensionalityRef unitDim = SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    if (!SIDimensionalityHasSameReducedDimensionality(qDim, unitDim)) {
        fprintf(stderr,
                "SIMonotonicDimensionCreate: quantityName dimensionality mismatch \"%s\"\n",
                OCStringGetCString(quantityName));
        if (err) OCRelease(err);
        return NULL;
    }
    if (err) OCRelease(err);
    // 3) Check all coordinates: must be SIScalarRef, same dimensionality
    OCIndex nCoords = OCArrayGetCount(coordinates);
    for (OCIndex i = 0; i < nCoords; ++i) {
        SIScalarRef s = (SIScalarRef)OCArrayGetValueAtIndex(coordinates, i);
        if (!s || !SIQuantityGetUnitDimensionality((SIQuantityRef)s) ||
            !SIDimensionalityHasSameReducedDimensionality(
                SIQuantityGetUnitDimensionality((SIQuantityRef)s), qDim)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: coordinate %lld is not compatible with quantityName\n", (long long)i);
            return NULL;
        }
    }
    // 4) origin/period, if provided, must also match
    if (origin) {
        if (!SIDimensionalityHasSameReducedDimensionality(
                SIQuantityGetUnitDimensionality((SIQuantityRef)origin), qDim)) {
            fprintf(stderr, "SIMonotonicDimensionCreate: origin dimensionality mismatch\n");
            return NULL;
        }
    }
    if (period) {
        if (!SIDimensionalityHasSameReducedDimensionality(
                SIQuantityGetUnitDimensionality((SIQuantityRef)period), qDim)) {
            fprintf(stderr, "SIMonotonicDimensionCreate: period dimensionality mismatch\n");
            return NULL;
        }
    }
    // 5) Reciprocal (optional) - validate before allocating
    if (reciprocal) {
        OCStringRef verr = NULL;
        if (!SIDimensionValidate(reciprocal, &verr)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: invalid reciprocal: %s\n",
                    OCStringGetCString(verr));
            if (verr) OCRelease(verr);
            return NULL;
        }
        if (verr) OCRelease(verr);
        // _SIDimensionIsReciprocalOf checked below
    }
    // --- All validation complete! Now allocate and set fields ---
    SIMonotonicDimensionRef dim = SIMonotonicDimensionAllocate();
    if (!dim) return NULL;
    impl_InitBaseDimensionFields((DimensionRef)&dim->_super._super);
    SIDimensionRef si = (SIDimensionRef)dim;
    // --- Defensive: Release previous references (if not NULL) before assigning ---
    if (si->_super.label) OCRelease(si->_super.label);
    si->_super.label = label ? OCRetain(label) : NULL;
    if (si->_super.description) OCRelease(si->_super.description);
    si->_super.description = description ? OCRetain(description) : NULL;
    if (si->_super.metadata) OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCRetain(metadata) : NULL;
    if (si->quantityName) OCRelease(si->quantityName);
    si->quantityName = OCRetain(quantityName);
    if (si->offset) OCRelease(si->offset);
    si->offset = OCRetain(offset);
    if (si->origin) OCRelease(si->origin);
    si->origin = origin ? OCRetain(origin) : NULL;
    if (si->period) OCRelease(si->period);
    si->period = period ? OCRetain(period) : NULL;
    si->periodic = periodic;
    si->scaling = scaling;
    if (dim->coordinates) OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    // Assign reciprocal (optional)
    if (dim->reciprocal) OCRelease(dim->reciprocal);
    if (reciprocal) {
        if (!_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal)) {
            fprintf(stderr,
                    "SIMonotonicDimensionCreate: reciprocal dimensionality mismatch\n");
            OCRelease(dim);
            return NULL;
        }
        dim->reciprocal = (SIDimensionRef)OCRetain(reciprocal);
    } else {
        dim->reciprocal = NULL;
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
SIMonotonicDimensionRef SIMonotonicDimensionCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: dict is NULL");
        return NULL;
    }
    // 1) Type discriminator
    OCStringRef t = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (!t || !OCStringEqual(t, STR("monotonic"))) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: wrong or missing \"type\"");
        return NULL;
    }
    // 2) Base fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    // 3) SIDimension fields
    OCStringRef quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR("quantity_name"));
    if (!quantityName) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: missing \"quantity_name\"");
        return NULL;
    }
    OCStringRef err = NULL;
    SIScalarRef offset = NULL;
    SIScalarRef origin = NULL;
    SIScalarRef period = NULL;
    // 3a) offset (required)
    OCStringRef s = (OCStringRef)OCDictionaryGetValue(dict, STR("offset"));
    if (!s) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: missing \"offset\"");
        return NULL;
    }
    offset = SIScalarCreateFromExpression(s, &err);
    if (!offset) {
        if (outError) *outError = err;
        return NULL;
    }
    // 3b) origin (optional)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR("origin"));
    if (s) {
        origin = SIScalarCreateFromExpression(s, &err);
        if (!origin) {
            OCRelease(offset);
            if (outError) *outError = err;
            return NULL;
        }
    }
    // 3c) period (optional)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR("period"));
    if (s) {
        period = SIScalarCreateFromExpression(s, &err);
        if (!period) {
            OCRelease(offset);
            OCRelease(origin);
            if (outError) *outError = err;
            return NULL;
        }
    }
    // 4) Flags & enums
    OCBooleanRef pb = (OCBooleanRef)OCDictionaryGetValue(dict, STR("periodic"));
    bool periodic = pb ? OCBooleanGetValue(pb) : false;
    OCNumberRef scn = (OCNumberRef)OCDictionaryGetValue(dict, STR("scaling"));
    int tmp = 0;
    if (scn) OCNumberGetValue(scn, kOCNumberIntType, &tmp);
    dimensionScaling scaling = (dimensionScaling)tmp;
    // 5) Monotonic‐specific: coordinates
    OCArrayRef coordStrs = (OCArrayRef)OCDictionaryGetValue(dict, STR("coordinates"));
    if (!coordStrs || OCArrayGetCount(coordStrs) < 2) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: need ≥2 coordinates");
        OCRelease(offset);
        OCRelease(origin);
        OCRelease(period);
        return NULL;
    }
    OCIndex n = OCArrayGetCount(coordStrs);
    OCMutableArrayRef coords = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < n; ++i) {
        OCStringRef str = (OCStringRef)OCArrayGetValueAtIndex(coordStrs, i);
        SIScalarRef sc = SIScalarCreateFromExpression(str, &err);
        if (!sc) {
            if (outError) *outError = err;
            OCRelease(offset);
            OCRelease(origin);
            OCRelease(period);
            OCRelease(coords);
            return NULL;
        }
        OCArrayAppendValue(coords, sc);
        OCRelease(sc);
    }
    // 6) Reciprocal dimension (optional)
    SIDimensionRef reciprocal = NULL;
    OCDictionaryRef recDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("reciprocal"));
    if (recDict) {
        reciprocal = SIDimensionCreateFromDictionary(recDict, outError);
        if (!reciprocal) {
            OCRelease(offset);
            OCRelease(origin);
            OCRelease(period);
            OCRelease(coords);
            return NULL;
        }
    }
    // 7) Construct
    SIMonotonicDimensionRef dim = SIMonotonicDimensionCreate(
        label,
        description,
        metadata,
        quantityName,
        offset,
        origin,
        period,
        periodic,
        scaling,
        (OCArrayRef)coords,
        reciprocal);
    // cleanup temporaries
    OCRelease(offset);
    OCRelease(origin);
    OCRelease(period);
    OCRelease(coords);
    OCRelease(reciprocal);
    if (!dim && outError) {
        *outError = STR("SIMonotonicDimensionCreateFromDict: create failed");
    }
    return dim;
}
OCDictionaryRef SIMonotonicDimensionCopyAsDictionary(SIMonotonicDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with base + SI‐dimension fields
    OCMutableDictionaryRef dict = (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) return NULL;
    // 2) Type discriminator
    OCDictionarySetValue(dict, STR("type"), STR("monotonic"));
    // 3) Serialize coordinates as strings
    OCArrayRef coords = SIMonotonicDimensionGetCoordinates(dim);
    if (coords) {
        OCIndex n = OCArrayGetCount(coords);
        OCMutableArrayRef coordsArr = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < n; ++i) {
            SIScalarRef s = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
            if (s) {
                OCStringRef sStr = SIScalarCreateStringValue(s);
                OCArrayAppendValue(coordsArr, sStr);
                OCRelease(sStr);
            }
        }
        OCDictionarySetValue(dict, STR("coordinates"), coordsArr);
        OCRelease(coordsArr);
    }
    // 4) Optional reciprocal dimension (deep-copied)
    SIDimensionRef rec = SIMonotonicDimensionGetReciprocal(dim);
    if (rec) {
        OCDictionaryRef recCopy = SIDimensionCopyAsDictionary(rec);
        OCDictionarySetValue(dict, STR("reciprocal"), recCopy);
        OCRelease(recCopy);
    }
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef SIMonotonicDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for MonotonicDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Required: type
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item) || strcmp(item->valuestring, "monotonic") != 0) {
        if (outError) *outError = STR("Expected \"type\": \"monotonic\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef type = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("type"), type);
    OCRelease(type);
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, "label");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("label"), s);
        OCRelease(s);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("description"), s);
        OCRelease(s);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, "metadata");
    if (cJSON_IsObject(item)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(item, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), md);
        OCRelease(md);
    }
    // Required: quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, "quantity_name");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"quantity_name\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef qn = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("quantity_name"), qn);
    OCRelease(qn);
    // Required: offset
    item = cJSON_GetObjectItemCaseSensitive(json, "offset");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"offset\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef off = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("offset"), off);
    OCRelease(off);
    // Optional: origin
    item = cJSON_GetObjectItemCaseSensitive(json, "origin");
    if (cJSON_IsString(item)) {
        OCStringRef val = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("origin"), val);
        OCRelease(val);
    }
    // Optional: period
    item = cJSON_GetObjectItemCaseSensitive(json, "period");
    if (cJSON_IsString(item)) {
        OCStringRef val = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("period"), val);
        OCRelease(val);
    }
    // Optional: periodic
    item = cJSON_GetObjectItemCaseSensitive(json, "periodic");
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR("periodic"), b);
        OCRelease(b);
    }
    // Optional: scaling
    item = cJSON_GetObjectItemCaseSensitive(json, "scaling");
    if (cJSON_IsNumber(item)) {
        OCNumberRef n = OCNumberCreateWithInt(item->valueint);
        OCDictionarySetValue(dict, STR("scaling"), n);
        OCRelease(n);
    }
    // Required: coordinates
    item = cJSON_GetObjectItemCaseSensitive(json, "coordinates");
    if (!cJSON_IsArray(item)) {
        if (outError) *outError = STR("Missing or invalid \"coordinates\"");
        OCRelease(dict);
        return NULL;
    }
    OCMutableArrayRef coords = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
    cJSON *coord = NULL;
    cJSON_ArrayForEach(coord, item) {
        if (cJSON_IsString(coord)) {
            OCStringRef str = OCStringCreateWithCString(coord->valuestring);
            OCArrayAppendValue(coords, str);
            OCRelease(str);
        }
    }
    OCDictionarySetValue(dict, STR("coordinates"), coords);
    OCRelease(coords);
    // Optional: reciprocal (SIDimension)
    item = cJSON_GetObjectItemCaseSensitive(json, "reciprocal");
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef recDict = SIDimensionDictionaryCreateFromJSON(item, outError);
        if (!recDict) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("reciprocal"), recDict);
        OCRelease(recDict);
    }
    return dict;
}
SIMonotonicDimensionRef SIMonotonicDimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for MonotonicDimension");
        return NULL;
    }
    OCDictionaryRef dict = SIMonotonicDimensionDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    SIMonotonicDimensionRef dim = SIMonotonicDimensionCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dim;
}
#pragma endregion SIMonotonicDimension
#pragma region SILinearDimension
// ============================================================================
// MARK: - (6) SILinearDimension
// ============================================================================
static OCTypeID kSILinearDimensionID = kOCNotATypeID;
typedef struct impl_SILinearDimension {
    struct impl_SIDimension _super;  // inherit Dimension + SI fields
    SIDimensionRef reciprocal;       // optional reciprocal dimension
    OCIndex count;                   // number of points (>=2)
    SIScalarRef increment;           // spacing between points
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
    OCStringRef fftStr = d->fft ? STR("true") : STR("false");  // 3) Stitch together
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
cJSON *impl_SILinearDimensionCreateJSON(const void *obj) {
    const SILinearDimensionRef lin = (const SILinearDimensionRef)obj;
    if (!lin) return cJSON_CreateNull();
    // Serialize base fields using impl_SIDimensionCreateJSON
    cJSON *json = impl_SIDimensionCreateJSON(&lin->_super);
    if (!json) return NULL;
    // reciprocal (SIDimensionRef)
    if (lin->reciprocal) {
        cJSON *recip_json = impl_SIDimensionCreateJSON(lin->reciprocal);
        if (recip_json)
            cJSON_AddItemToObject(json, "reciprocal", recip_json);
    }
    // count (OCIndex, presumably integer)
    cJSON_AddNumberToObject(json, "count", (int)lin->count);
    // increment (SIScalarRef)
    if (lin->increment) {
        cJSON *inc_json = OCTypeCopyJSON((OCTypeRef)lin->increment);
        if (inc_json)
            cJSON_AddItemToObject(json, "increment", inc_json);
    }
    // reciprocalIncrement (SIScalarRef)
    if (lin->reciprocalIncrement) {
        cJSON *recip_inc_json = OCTypeCopyJSON((OCTypeRef)lin->reciprocalIncrement);
        if (recip_inc_json)
            cJSON_AddItemToObject(json, "reciprocalIncrement", recip_inc_json);
    }
    // fft (bool)
    cJSON_AddBoolToObject(json, "fft", lin->fft);
    return json;
}
static void *impl_SILinearDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = SILinearDimensionCopyAsDictionary((SILinearDimensionRef)obj);
    if (!dict) return NULL;
    SILinearDimensionRef copy = SILinearDimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
static SILinearDimensionRef SILinearDimensionAllocate(void) {
    return (SILinearDimensionRef)OCTypeAlloc(
        struct impl_SILinearDimension,
        SILinearDimensionGetTypeID(),
        impl_SILinearDimensionFinalize,
        impl_SILinearDimensionEqual,
        impl_SILinearDimensionCopyFormattingDesc,
        impl_SILinearDimensionCreateJSON,
        impl_SILinearDimensionDeepCopy,
        impl_SILinearDimensionDeepCopy);
}
SILinearDimensionRef SILinearDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling,
    OCIndex count,
    SIScalarRef increment,
    bool fft,
    SIDimensionRef reciprocal) {
    // 1) Linear‐specific: need ≥2 points + a valid increment
    if (count < 2) {
        fprintf(stderr, "SILinearDimensionCreate: need ≥2 points (got %lld)\n", (long long)count);
        return NULL;
    }
    if (!increment) {
        fprintf(stderr, "SILinearDimensionCreate: increment must be non-NULL\n");
        return NULL;
    }
    if (!quantityName) {
        fprintf(stderr, "SILinearDimensionCreate: quantityName must be non-NULL\n");
        return NULL;
    }
    if (!offset) {
        fprintf(stderr, "SILinearDimensionCreate: offset must be non-NULL\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)offset)) {
        fprintf(stderr, "SILinearDimensionCreate: offset must be real-valued\n");
        return NULL;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)increment)) {
        fprintf(stderr, "SILinearDimensionCreate: increment must be real-valued\n");
        return NULL;
    }
    // 2) Validate dimensionality for quantityName, offset, increment
    OCStringRef err = NULL;
    SIDimensionalityRef qDim = SIDimensionalityForQuantity(quantityName, &err);
    if (!qDim) {
        fprintf(stderr, "SILinearDimensionCreate: quantityName not recognized: %s\n",
                OCStringGetCString(quantityName));
        if (err) OCRelease(err);
        return NULL;
    }
    SIDimensionalityRef offsetDim = SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    if (!SIDimensionalityHasSameReducedDimensionality(qDim, offsetDim)) {
        OCStringRef offSym = SIUnitCopySymbol(SIQuantityGetUnit((SIQuantityRef)offset));
        fprintf(stderr,
                "SILinearDimensionCreate: quantityName dimensionality mismatch \"%s\" vs offset unit \"%s\"\n",
                OCStringGetCString(quantityName), OCStringGetCString(offSym));
        if (offSym) OCRelease(offSym);
        if (err) OCRelease(err);
        return NULL;
    }
    SIDimensionalityRef incDim = SIQuantityGetUnitDimensionality((SIQuantityRef)increment);
    if (!SIDimensionalityHasSameReducedDimensionality(qDim, incDim)) {
        OCStringRef incSym = SIUnitCopySymbol(SIQuantityGetUnit((SIQuantityRef)increment));
        OCStringRef offSym = SIUnitCopySymbol(SIQuantityGetUnit((SIQuantityRef)offset));
        fprintf(stderr,
                "SILinearDimensionCreate: increment dimensionality mismatch (increment unit = \"%s\", offset unit = \"%s\")\n",
                OCStringGetCString(incSym), OCStringGetCString(offSym));
        if (incSym) OCRelease(incSym);
        if (offSym) OCRelease(offSym);
        if (err) OCRelease(err);
        return NULL;
    }
    if (err) OCRelease(err);
    // 3) Validate optional origin/period
    if (origin) {
        SIDimensionalityRef originDim = SIQuantityGetUnitDimensionality((SIQuantityRef)origin);
        if (!SIDimensionalityHasSameReducedDimensionality(qDim, originDim)) {
            fprintf(stderr, "SILinearDimensionCreate: origin dimensionality mismatch\n");
            return NULL;
        }
    }
    if (period) {
        SIDimensionalityRef periodDim = SIQuantityGetUnitDimensionality((SIQuantityRef)period);
        if (!SIDimensionalityHasSameReducedDimensionality(qDim, periodDim)) {
            fprintf(stderr, "SILinearDimensionCreate: period dimensionality mismatch\n");
            return NULL;
        }
    }
    // 4) Validate reciprocal (if provided)
    if (reciprocal) {
        OCStringRef verr = NULL;
        if (!SIDimensionValidate(reciprocal, &verr)) {
            fprintf(stderr, "SILinearDimensionCreate: invalid reciprocal: %s\n",
                    OCStringGetCString(verr));
            if (verr) OCRelease(verr);
            return NULL;
        }
        // Optionally: check _SIDimensionIsReciprocalOf(...)
        if (verr) OCRelease(verr);
    }
    // --- All validation complete! Now allocate and set fields ---
    SILinearDimensionRef dim = SILinearDimensionAllocate();
    if (!dim) return NULL;
    impl_InitBaseDimensionFields((DimensionRef)&dim->_super._super);
    SIDimensionRef si = (SIDimensionRef)dim;
    // Defensive: release any old references before assigning
    if (si->_super.label) OCRelease(si->_super.label);
    si->_super.label = label ? OCRetain(label) : NULL;
    if (si->_super.description) OCRelease(si->_super.description);
    si->_super.description = description ? OCRetain(description) : NULL;
    if (si->_super.metadata) OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCRetain(metadata) : NULL;
    if (si->quantityName) OCRelease(si->quantityName);
    si->quantityName = OCRetain(quantityName);
    if (si->offset) OCRelease(si->offset);
    si->offset = OCRetain(offset);
    if (si->origin) OCRelease(si->origin);
    si->origin = origin ? OCRetain(origin) : NULL;
    if (si->period) OCRelease(si->period);
    si->period = period ? OCRetain(period) : NULL;
    si->periodic = periodic;
    si->scaling = scaling;
    // Linear-specific fields
    dim->count = count;
    if (dim->increment) OCRelease(dim->increment);
    dim->increment = OCRetain(increment);
    // Compute reciprocalIncrement = (count * increment)^-1
    if (dim->reciprocalIncrement) OCRelease(dim->reciprocalIncrement);
    dim->reciprocalIncrement = NULL;
    {
        SIMutableScalarRef recInc = SIScalarCreateMutableCopy(increment);
        if (!recInc) {
            OCRelease(dim);
            return NULL;
        }
        SIScalarConvertToUnit(recInc, SIQuantityGetUnit((SIQuantityRef)offset), NULL);
        if (!SIScalarMultiplyByDimensionlessRealConstant(recInc, (double)count) ||
            !SIScalarRaiseToAPowerWithoutReducingUnit(recInc, -1.0, NULL)) {
            OCRelease(recInc);
            OCRelease(dim);
            return NULL;
        }
        SIScalarSetElementType(recInc, kSINumberFloat64Type);
        dim->reciprocalIncrement = recInc;
    }
    dim->fft = fft;
    if (dim->reciprocal) OCRelease(dim->reciprocal);
    dim->reciprocal = reciprocal ? (SIDimensionRef)OCRetain(reciprocal) : NULL;
    return dim;
}
SILinearDimensionRef SILinearDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError)
            *outError = OCStringCreateWithCString("SILinearDimensionFromDict: dict is NULL");
        return NULL;
    }
    // Discriminator
    OCStringRef t = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (!t || !OCStringEqual(t, STR("linear"))) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("SILinearDimensionFromDict: wrong or missing “type” (got “%@”)"),
                t ? t : STR("<none>"));
        }
        return NULL;
    }
    // Base + SI fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR("label"));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata"));
    OCStringRef quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR("quantity_name"));
    // Parse scalars
    OCStringRef errStr = NULL;
    SIScalarRef offset = NULL, origin = NULL, period = NULL;
    OCStringRef s;
    // offset (required)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR("offset"));
    if (!s) {
        if (outError) *outError = STR("missing “offset”");
        return NULL;
    }
    offset = SIScalarCreateFromExpression(s, &errStr);
    if (!offset) {
        if (outError) *outError = errStr;
        return NULL;
    }
    // origin (optional)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR("origin"));
    if (s) {
        origin = SIScalarCreateFromExpression(s, &errStr);
        if (!origin) {
            OCRelease(offset);
            if (outError) *outError = errStr;
            return NULL;
        }
    }
    // period (optional)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR("period"));
    if (s) {
        period = SIScalarCreateFromExpression(s, &errStr);
        if (!period) {
            OCRelease(offset);
            OCRelease(origin);
            if (outError) *outError = errStr;
            return NULL;
        }
    }
    // Flags & enums
    OCBooleanRef pb = (OCBooleanRef)OCDictionaryGetValue(dict, STR("periodic"));
    bool periodic = pb ? OCBooleanGetValue(pb) : false;
    OCNumberRef num = (OCNumberRef)OCDictionaryGetValue(dict, STR("scaling"));
    int tmp = 0;
    dimensionScaling scaling = num && OCNumberGetValue(num, kOCNumberIntType, &tmp)
                                   ? (dimensionScaling)tmp
                                   : kDimensionScalingNone;
    // Linear-specific
    OCIndex count = 0;
    SIScalarRef increment = NULL;
    bool fft = false;
    // count
    OCNumberRef cn = (OCNumberRef)OCDictionaryGetValue(dict, STR("count"));
    if (cn) {
        int ctmp = 0;
        OCNumberGetValue(cn, kOCNumberIntType, &ctmp);
        count = (OCIndex)ctmp;
    }
    // increment (required)
    OCStringRef incStr = (OCStringRef)OCDictionaryGetValue(dict, STR("increment"));
    if (!incStr) {
        if (outError) *outError = STR("missing “increment”");
        goto Fail;
    }
    increment = SIScalarCreateFromExpression(incStr, &errStr);
    if (!increment) {
        if (outError) *outError = errStr;
        goto Fail;
    }
    // fft
    OCBooleanRef fb = (OCBooleanRef)OCDictionaryGetValue(dict, STR("fft"));
    if (fb) fft = OCBooleanGetValue(fb);
    // Optional reciprocal dimension
    SIDimensionRef reciprocal = NULL;
    {
        OCDictionaryRef recDict =
            (OCDictionaryRef)OCDictionaryGetValue(dict, STR("reciprocal"));
        if (recDict) {
            reciprocal = SIDimensionCreateFromDictionary(recDict, &errStr);
            if (!reciprocal) {
                if (outError) *outError = errStr;
                goto Fail;
            }
        }
    }
    // Validate
    if (count < 2 || !increment) {
        if (outError)
            *outError = STR("SILinearDimensionFromDict: need ≥2 points and a valid increment");
        goto Fail;
    }
    // Construct
    SILinearDimensionRef dim = SILinearDimensionCreate(
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
        reciprocal);
    if (!dim && outError) {
        *outError = STR("SILinearDimensionCreateFromDictionary: create failed");
    }
    // Cleanup and return
    OCRelease(offset);
    OCRelease(origin);
    OCRelease(period);
    OCRelease(increment);
    OCRelease(reciprocal);
    return dim;
Fail:
    OCRelease(offset);
    OCRelease(origin);
    OCRelease(period);
    OCRelease(increment);
    OCRelease(reciprocal);
    return NULL;
}
OCDictionaryRef SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with the base + SI‐dimension fields (offset/origin/period now strings)
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) return NULL;
    // 2) Type discriminator
    OCDictionarySetValue(dict, STR("type"), STR("linear"));
    // 3) Count
    {
        OCNumberRef cnt = OCNumberCreateWithInt((int)SILinearDimensionGetCount(dim));
        OCDictionarySetValue(dict, STR("count"), cnt);
        OCRelease(cnt);
    }
    // 4) Increment as string
    {
        SIScalarRef inc = SILinearDimensionGetIncrement(dim);
        if (inc) {
            OCStringRef incStr = SIScalarCreateStringValue(inc);
            OCDictionarySetValue(dict, STR("increment"), incStr);
            OCRelease(incStr);
        }
    }
    // 5) Reciprocal increment as string
    {
        SIScalarRef recInc = SILinearDimensionGetReciprocalIncrement(dim);
        if (recInc) {
            OCStringRef recIncStr = SIScalarCreateStringValue(recInc);
            OCDictionarySetValue(dict, STR("reciprocal_increment"), recIncStr);
            OCRelease(recIncStr);
        }
    }
    // 6) FFT flag
    {
        OCBooleanRef b = OCBooleanGetWithBool(SILinearDimensionIsFFT(dim));
        OCDictionarySetValue(dict, STR("fft"), b);
        OCRelease(b);
    }
    // 7) Reciprocal dimension (via the generic helper)
    {
        SIDimensionRef rec = SILinearDimensionGetReciprocal(dim);
        if (rec) {
            OCDictionaryRef recDict = DimensionCopyAsDictionary((DimensionRef)rec);
            if (recDict) {
                OCDictionarySetValue(dict, STR("reciprocal"), recDict);
                OCRelease(recDict);
            }
        }
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
static OCDictionaryRef SILinearDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for SILinearDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Required: type = "linear"
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item) || strcmp(item->valuestring, "linear") != 0) {
        if (outError) *outError = STR("Expected \"type\": \"linear\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef type = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("type"), type);
    OCRelease(type);
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, "label");
    if (cJSON_IsString(item)) {
        OCStringRef label = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("label"), label);
        OCRelease(label);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCStringRef desc = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("description"), desc);
        OCRelease(desc);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, "metadata");
    if (cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), metadata);
        OCRelease(metadata);
    }
    // Required: quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, "quantity_name");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"quantity_name\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef qn = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("quantity_name"), qn);
    OCRelease(qn);
    // Required: offset
    item = cJSON_GetObjectItemCaseSensitive(json, "offset");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"offset\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef offset = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("offset"), offset);
    OCRelease(offset);
    // Optional: origin
    item = cJSON_GetObjectItemCaseSensitive(json, "origin");
    if (cJSON_IsString(item)) {
        OCStringRef val = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("origin"), val);
        OCRelease(val);
    }
    // Optional: period
    item = cJSON_GetObjectItemCaseSensitive(json, "period");
    if (cJSON_IsString(item)) {
        OCStringRef val = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("period"), val);
        OCRelease(val);
    }
    // Optional: periodic
    item = cJSON_GetObjectItemCaseSensitive(json, "periodic");
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR("periodic"), b);
        OCRelease(b);
    }
    // Optional: scaling
    item = cJSON_GetObjectItemCaseSensitive(json, "scaling");
    if (cJSON_IsNumber(item)) {
        OCNumberRef n = OCNumberCreateWithInt(item->valueint);
        OCDictionarySetValue(dict, STR("scaling"), n);
        OCRelease(n);
    }
    // Required: count
    item = cJSON_GetObjectItemCaseSensitive(json, "count");
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"count\"");
        OCRelease(dict);
        return NULL;
    }
    OCNumberRef cnt = OCNumberCreateWithInt(item->valueint);
    OCDictionarySetValue(dict, STR("count"), cnt);
    OCRelease(cnt);
    // Required: increment
    item = cJSON_GetObjectItemCaseSensitive(json, "increment");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"increment\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef inc = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("increment"), inc);
    OCRelease(inc);
    // Optional: reciprocal_increment (ignored in constructor, derived internally)
    // Optional: fft
    item = cJSON_GetObjectItemCaseSensitive(json, "fft");
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR("fft"), b);
        OCRelease(b);
    }
    // Optional: reciprocal (SIDimension)
    item = cJSON_GetObjectItemCaseSensitive(json, "reciprocal");
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef recDict = SIDimensionDictionaryCreateFromJSON(item, outError);
        if (!recDict) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("reciprocal"), recDict);
        OCRelease(recDict);
    }
    return dict;
}
SILinearDimensionRef SILinearDimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for SILinearDimension");
        return NULL;
    }
    OCDictionaryRef dict = SILinearDimensionDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    SILinearDimensionRef dim = SILinearDimensionCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dim;
}
#pragma endregion SILinearDimension
#pragma region Dimension Utilities
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
OCStringRef CreateLongDimensionLabel(DimensionRef dim, OCIndex index) {
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
        SIScalarRef offset = SIDimensionGetOffset((SIDimensionRef)dim);
        if (offset)
            unitStr = SIScalarCreateUnitString(offset);
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
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim) {
    if (!dim) return NULL;
    OCTypeID tid = OCGetTypeID(dim);
    OCDictionaryRef dict = NULL;
    if (tid == LabeledDimensionGetTypeID()) {
        dict = LabeledDimensionCopyAsDictionary((LabeledDimensionRef)dim);
    } else if (tid == SIMonotonicDimensionGetTypeID()) {
        dict = SIMonotonicDimensionCopyAsDictionary((SIMonotonicDimensionRef)dim);
    } else if (tid == SILinearDimensionGetTypeID()) {
        dict = SILinearDimensionCopyAsDictionary((SILinearDimensionRef)dim);
    } else {
        // fallback to the abstract‐base serialization
        dict = impl_DimensionCopyAsDictionary(dim);
    }
    return dict;  // caller takes ownership of the returned dictionary
}
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = OCStringCreateWithCString(
                          "DimensionCreateFromDictionary: input dictionary is NULL");
        return NULL;
    }
    OCStringRef type = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (type) {
        // subclass dispatch
        if (OCStringEqual(type, STR("labeled"))) {
            return (DimensionRef)LabeledDimensionCreateFromDictionary(dict, outError);
        } else if (OCStringEqual(type, STR("linear"))) {
            return (DimensionRef)SILinearDimensionCreateFromDictionary(dict, outError);
        } else if (OCStringEqual(type, STR("monotonic"))) {
            return (DimensionRef)SIMonotonicDimensionCreateFromDictionary(dict, outError);
        } else {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("DimensionCreateFromDictionary: unknown type \"%@\""),
                    type);
            }
            return NULL;
        }
    }
    // no type discriminator → treat as abstract‐base Dimension
    return impl_DimensionCreateFromDictionary(dict);
}
