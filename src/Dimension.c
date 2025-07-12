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
#define kDimensionLabelKey "label"
#define kDimensionDescriptionKey "description"
#define kDimensionMetadataKey "metadata"
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
    if (dimA == dimB) return true;
    if (dimA->label != dimB->label &&
        !OCTypeEqual(dimA->label, dimB->label)) return false;
    if (dimA->description != dimB->description &&
        !OCTypeEqual(dimA->description, dimB->description)) return false;
    if (dimA->metadata != dimB->metadata &&
        !OCTypeEqual(dimA->metadata, dimB->metadata)) return false;
    return true;
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
static DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
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
    DimensionRef copy = impl_DimensionCreateFromDictionary(dict, NULL);
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
                                         OCDictionaryRef metadata, OCStringRef *outError) {
    // 1) Raw allocation
    DimensionRef dim = impl_DimensionAllocate();
    if (!dim) return NULL;
    // 2) One-time default initialization
    impl_InitBaseDimensionFields(dim);
    // 3) Override with user values via setters (they handle copying & erroring)
    if (label) {
        if (!DimensionSetLabel(dim, label, outError)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (description) {
        if (!DimensionSetDescription(dim, description, outError)) {
            OCRelease(dim);
            return NULL;
        }
    }
    if (metadata) {
        if (!DimensionSetMetadata(dim, metadata, outError)) {
            OCRelease(dim);
            return NULL;
        }
    }
    return dim;
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
    OCStringRef labelCopy = label ? OCStringCreateCopy(label) : NULL;
    if (label && !labelCopy) {
        if (outError)
            *outError = STR("DimensionSetLabel: failed to copy label string");
        return false;
    }
    OCRelease(dim->label);
    dim->label = labelCopy;
    return true;
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
    OCStringRef descCopy = desc ? OCStringCreateCopy(desc) : NULL;
    if (desc && !descCopy) {
        if (outError)
            *outError = STR("DimensionSetDescription: failed to copy description string");
        return false;
    }
    OCRelease(dim->description);
    dim->description = descCopy;
    return true;
}
OCDictionaryRef DimensionGetMetadata(DimensionRef dim) {
    return dim ? dim->metadata : NULL;
}
bool DimensionSetMetadata(DimensionRef dim,
                          OCDictionaryRef dict,
                          OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dim) {
        if (outError)
            *outError = STR("DimensionSetMetadata: dim is NULL");
        return false;
    }
    OCDictionaryRef dictCopy = NULL;
    if (dict) {
        dictCopy = (OCDictionaryRef)OCTypeDeepCopy(dict);
        if (!dictCopy) {
            if (outError)
                *outError = STR("DimensionSetMetadata: failed to copy metadata dictionary");
            return false;
        }
    } else {
        dictCopy = OCDictionaryCreateMutable(0);
        if (!dictCopy) {
            if (outError)
                *outError = STR("DimensionSetMetadata: failed to create empty metadata dictionary");
            return false;
        }
    }
    OCRelease(dim->metadata);
    dim->metadata = dictCopy;
    return true;
}
static DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict,
                                                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError)
            *outError = STR("impl_DimensionCreateFromDictionary: input dictionary is NULL");
        return NULL;
    }
    // 1) Pull values out of the dictionary
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionMetadataKey));
    // 2) Delegate to the single “true” constructor, passing through outError
    return impl_DimensionCreate(label, description, metadata, outError);
}
// ----------------------------------------------------------------------------
// Helpers for impl_*CopyAsDictionary  — now take an OCStringRef key
// ----------------------------------------------------------------------------
static inline bool CopyStringField(OCMutableDictionaryRef dict,
                                   OCStringRef key,
                                   OCStringRef value) {
    if (!value) return true;
    OCStringRef copy = OCStringCreateCopy(value);
    if (!copy) return false;
    OCDictionarySetValue(dict, key, copy);
    OCRelease(copy);
    return true;
}
static inline bool CopyDictField(OCMutableDictionaryRef dict,
                                 OCStringRef key,
                                 OCDictionaryRef value) {
    if (!value) return true;
    OCDictionaryRef copy = (OCDictionaryRef)OCTypeDeepCopy((OCTypeRef)value);
    if (!copy) return false;
    OCDictionarySetValue(dict, key, copy);
    OCRelease(copy);
    return true;
}
static inline bool CopyArrayField(OCMutableDictionaryRef dict,
                                  OCStringRef key,
                                  OCArrayRef value) {
    if (!value) return true;
    OCMutableArrayRef copy = OCArrayCreateMutableCopy(value);
    if (!copy) return false;
    OCDictionarySetValue(dict, key, copy);
    OCRelease(copy);
    return true;
}
static inline bool CopyBoolField(OCMutableDictionaryRef dict,
                                 OCStringRef key,
                                 bool value) {
    OCBooleanRef b = OCBooleanGetWithBool(value);
    if (!b) return false;
    OCDictionarySetValue(dict, key, b);
    OCRelease(b);
    return true;
}
static inline bool CopyNumField(OCMutableDictionaryRef dict,
                                OCStringRef key,
                                int value) {
    OCNumberRef n = OCNumberCreateWithInt(value);
    if (!n) return false;
    OCDictionarySetValue(dict, key, n);
    OCRelease(n);
    return true;
}
static OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim) {
    if (!dim) {
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        return NULL;
    }
    // Copy label
    if (!CopyStringField(dict, STR(kDimensionLabelKey), DimensionGetLabel(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // Copy description
    if (!CopyStringField(dict, STR(kDimensionDescriptionKey), DimensionGetDescription(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // Copy metadata
    if (!CopyDictField(dict, STR(kDimensionMetadataKey), DimensionGetMetadata(dim))) {
        OCRelease(dict);
        return NULL;
    }
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef impl_DimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for Dimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionLabelKey);
    if (cJSON_IsString(item)) {
        OCStringRef label = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionLabelKey), label);
        OCRelease(label);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef desc = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionDescriptionKey), desc);
        OCRelease(desc);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionMetadataKey);
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionMetadataKey), metadata);
        OCRelease(metadata);
    }
    return dict;
}
static DimensionRef impl_DimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for Dimension");
        return NULL;
    }
    // Schema-bound: interpret known fields only
    OCDictionaryRef dict = impl_DimensionDictionaryCreateFromJSON(json, outError);
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
#define kLabeledDimensionCoordinateLabelsKey "labels"
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
    if (dimA == dimB)
        return true;
    // Compare base fields
    if (!impl_DimensionEqual((const DimensionRef)&dimA->_super,
                             (const DimensionRef)&dimB->_super))
        return false;
    // Compare LabeledDimension-specific field, null-safe
    if (dimA->coordinateLabels != dimB->coordinateLabels &&
        !OCTypeEqual(dimA->coordinateLabels, dimB->coordinateLabels))
        return false;
    return true;
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
            cJSON_AddItemToObject(json, kLabeledDimensionCoordinateLabelsKey, labels_json);
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
LabeledDimensionRef LabeledDimensionCreate(OCStringRef label,
                                           OCStringRef description,
                                           OCDictionaryRef metadata,
                                           OCArrayRef coordinateLabels,
                                           OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!coordinateLabels || OCArrayGetCount(coordinateLabels) < 2) {
        if (outError) *outError = STR("need ≥2 coordinate labels");
        return NULL;
    }
    LabeledDimensionRef dim = LabeledDimensionAllocate();
    if (!dim) {
        if (outError) *outError = STR("allocation failed");
        return NULL;
    }
    // zero‐init subclass pointers and base fields
    impl_InitBaseDimensionFields((DimensionRef)dim);
    dim->coordinateLabels = NULL;
    // apply common setters
    if (label &&
        !DimensionSetLabel((DimensionRef)dim, label, outError)) {
        OCRelease(dim);
        return NULL;
    }
    if (description &&
        !DimensionSetDescription((DimensionRef)dim, description, outError)) {
        OCRelease(dim);
        return NULL;
    }
    if (metadata &&
        !DimensionSetMetadata((DimensionRef)dim, metadata, outError)) {
        OCRelease(dim);
        return NULL;
    }
    // deep‐copy the coordinates via your setter
    if (!LabeledDimensionSetCoordinateLabels(dim, coordinateLabels, outError)) {
        OCRelease(dim);
        return NULL;
    }
    return dim;
}
LabeledDimensionRef LabeledDimensionCreateWithCoordinateLabels(OCArrayRef coordinateLabels) {
    return LabeledDimensionCreate(NULL,
                                  NULL, NULL, coordinateLabels, NULL);
}
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim) {
    return dim ? dim->coordinateLabels : NULL;
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
    // no-op if it’s already the same array
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
OCStringRef LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim, OCIndex index) {
    if (!dim || !dim->coordinateLabels || index < 0 || index >= OCArrayGetCount(dim->coordinateLabels))
        return NULL;
    return OCArrayGetValueAtIndex(dim->coordinateLabels, index);
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
// ============================================================================
// (3) LabeledDimensionCreateFromDictionary
// ============================================================================
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
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
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionMetadataKey));
    OCArrayRef coordinateLabels = (OCArrayRef)OCDictionaryGetValue(dict, STR(kLabeledDimensionCoordinateLabelsKey));
    // Construct
    LabeledDimensionRef dim = LabeledDimensionCreate(
        label,
        description,
        metadata,
        coordinateLabels, outError);
    if (!dim && outError) {
        *outError = OCStringCreateWithCString("LabeledDimensionCreate() failed");
    }
    return dim;
}
OCDictionaryRef LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Start with base-class fields (label/description/metadata)
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) Concrete type discriminator
    if (!CopyStringField(dict, STR("type"), STR("labeled"))) {
        OCRelease(dict);
        return NULL;
    }
    // 3) Deep-copy coordinateLabels → "labels"
    if (!CopyArrayField(dict, STR(kLabeledDimensionCoordinateLabelsKey), LabeledDimensionGetCoordinateLabels(dim))) {
        OCRelease(dict);
        return NULL;
    }
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef LabeledDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("LabeledDimension: expected JSON object");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Required: type == "labeled"
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item) || strcmp(item->valuestring, "labeled") != 0) {
        if (outError) *outError = STR("LabeledDimension: missing or invalid \"type\":\"labeled\"");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef type = OCStringCreateWithCString(item->valuestring);
    OCDictionarySetValue(dict, STR("type"), type);
    OCRelease(type);
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionLabelKey);
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionLabelKey), s);
        OCRelease(s);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionDescriptionKey), s);
        OCRelease(s);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionMetadataKey);
    if (cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionMetadataKey), metadata);
        OCRelease(metadata);
    }
    // Required: labels array
    item = cJSON_GetObjectItemCaseSensitive(json, kLabeledDimensionCoordinateLabelsKey);
    if (!cJSON_IsArray(item)) {
        if (outError) *outError = STR("LabeledDimension: missing or invalid \"labels\" array");
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
    OCDictionarySetValue(dict, STR(kLabeledDimensionCoordinateLabelsKey), labelArr);
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
#define kSIDimensionQuantityNameKey "quantity_name"
#define kSIDimensionOffsetKey "coordinates_offset"
#define kSIDimensionOriginKey "origin_offset"
#define kSIDimensionPeriodKey "period"
#define kSIDimensionPeriodicKey "periodic"
#define kSIDimensionScalingKey "scaling"
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
    if (dimA == dimB) return true;
    // 1) Base‐class fields
    if (!impl_DimensionEqual((const DimensionRef)&dimA->_super,
                             (const DimensionRef)&dimB->_super))
        return false;
    // 2) quantityName
    if (dimA->quantityName != dimB->quantityName &&
        !OCTypeEqual(dimA->quantityName, dimB->quantityName))
        return false;
    // 3) coordinate & origin offsets
    if (dimA->offset != dimB->offset &&
        !OCTypeEqual(dimA->offset, dimB->offset))
        return false;
    if (dimA->origin != dimB->origin &&
        !OCTypeEqual(dimA->origin, dimB->origin))
        return false;
    // 4) periodic flag
    if (dimA->periodic != dimB->periodic)
        return false;
    // 5) if periodic, both must have a non‐NULL period and be equal
    if (dimA->periodic) {
        if (dimA->period != dimB->period &&
            !OCTypeEqual(dimA->period, dimB->period))
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
    SIScalarRef offset = SIDimensionGetCoordinatesOffset(d);
    SIScalarRef origin = SIDimensionGetOriginOffset(d);
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
    // 1) Serialize base fields
    cJSON *json = impl_DimensionCreateJSON(&sidim->_super);
    if (!json) return cJSON_CreateNull();
// 2) Helper to add optional items
#define ADD_JSON_ITEM(key, cond, make_item)                 \
    do {                                                    \
        if (cond) {                                         \
            cJSON *itm = (make_item);                       \
            if (itm) cJSON_AddItemToObject(json, key, itm); \
        }                                                   \
    } while (0)
    ADD_JSON_ITEM(kSIDimensionQuantityNameKey,
                  sidim->quantityName != NULL,
                  OCTypeCopyJSON((OCTypeRef)sidim->quantityName));
    ADD_JSON_ITEM(kSIDimensionOffsetKey,
                  sidim->offset != NULL,
                  OCTypeCopyJSON((OCTypeRef)sidim->offset));
    ADD_JSON_ITEM(kSIDimensionOriginKey,
                  sidim->origin != NULL,
                  OCTypeCopyJSON((OCTypeRef)sidim->origin));
    ADD_JSON_ITEM(kSIDimensionPeriodKey,
                  sidim->period != NULL,
                  OCTypeCopyJSON((OCTypeRef)sidim->period));
#undef ADD_JSON_ITEM
    // 3) Always include these primitives
    cJSON_AddBoolToObject(json, kSIDimensionPeriodicKey, sidim->periodic);
    cJSON_AddNumberToObject(json, kSIDimensionScalingKey, (int)sidim->scaling);
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
    dim->quantityName = kSIQuantityDimensionless;
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
    SIScalarRef off = SIDimensionGetCoordinatesOffset(dim);
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
    SIScalarRef org = SIDimensionGetOriginOffset(dim);
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
    success *= DimensionSetLabel(baseDim, label, NULL);
    success *= DimensionSetDescription(baseDim, description, NULL);
    success *= DimensionSetMetadata(baseDim, metadata, NULL);
    success *= SIDimensionSetCoordinatesOffset(dim, offset, NULL);
    success *= SIDimensionSetQuantityName(dim, quantityName, NULL);
    success *= SIDimensionSetOriginOffset(dim, origin, NULL);
    success *= (!periodic || SIDimensionSetPeriod(dim, period, NULL));
    success *= SIDimensionSetPeriodic(dim, periodic, NULL);
    success *= SIDimensionSetScaling(dim, scaling);
    return success;
}
// Helper: ensure *scalarPtr points to a real SIScalar compatible with dimensionality.
// If *scalarPtr is NULL, creates a zero scalar in unit.
// Returns true on success, false + *outError on failure.
static bool impl_validateOrDefaultScalar(
    const char *paramName,               // e.g. "offset", "origin", "period"
    SIScalarRef *scalarPtr,              // address of the scalar to validate or default
    SIUnitRef unit,                      // unit to use when defaulting
    SIDimensionalityRef dimensionality,  // dimensionality to match against
    OCStringRef *outError) {
    OCStringRef err = NULL;
    if (*scalarPtr) {
        // must be a SIScalar
        if (OCGetTypeID((OCTypeRef)*scalarPtr) != SIScalarGetTypeID()) {
            err = OCStringCreateWithFormat(
                STR("%s must be a SIScalar"), paramName);
            goto fail;
        }
        // must be real-valued
        if (SIQuantityIsComplexType((SIQuantityRef)*scalarPtr)) {
            err = OCStringCreateWithFormat(
                STR("%s must be real-valued"), paramName);
            goto fail;
        }
        // dimensionality must match
        SIDimensionalityRef dim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)*scalarPtr);
        if (!SIDimensionalityHasSameReducedDimensionality(dim, dimensionality)) {
            OCStringRef gotSym =
                SIUnitCopySymbol(SIQuantityGetUnit((SIQuantityRef)*scalarPtr));
            OCStringRef wantSym =
                SIUnitCopySymbol(unit);
            err = OCStringCreateWithFormat(
                STR("%s unit \"%@\" does not match required unit \"%@\""),
                paramName, gotSym, wantSym);
            OCRelease(gotSym);
            OCRelease(wantSym);
            goto fail;
        }
    } else {
        // default to zero in unit
        SIScalarRef zero =
            SIScalarCreateWithDouble(0.0, unit);
        if (!zero) {
            err = OCStringCreateWithFormat(
                STR("failed to create default zero %s"), paramName);
            goto fail;
        }
        *scalarPtr = zero;  // take ownership
    }
    return true;
fail:
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return false;
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
    dimensionScaling scaling,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    OCStringRef err = NULL;
    // In this function all parameters are optional.
    // 1) Determine baseUnit & baseDim (priority: offset → origin → period → quantityName → dimensionless)
    SIUnitRef baseUnit = NULL;
    SIDimensionalityRef baseDim = NULL;
    if (offset) {
        baseUnit = SIQuantityGetUnit((SIQuantityRef)offset);
        baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)offset);
    } else if (origin) {
        baseUnit = SIQuantityGetUnit((SIQuantityRef)origin);
        baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)origin);
    } else if (period) {
        baseUnit = SIQuantityGetUnit((SIQuantityRef)period);
        baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)period);
    } else if (quantityName) {
        SIDimensionalityRef qDim = SIDimensionalityForQuantity(quantityName, &err);
        if (!qDim) goto Fail;
        OCArrayRef units = SIUnitCreateArrayOfUnitsForDimensionality(qDim);
        baseUnit = (SIUnitRef)OCArrayGetValueAtIndex(units, 0);
        OCRelease(units);
        baseDim = qDim;
    } else {
        baseUnit = SIUnitDimensionlessAndUnderived();
        baseDim = SIDimensionalityForQuantity(kSIQuantityDimensionless, NULL);
        quantityName = kSIQuantityDimensionless;
    }
    // 2) Default quantityName if missing
    if (!quantityName) {
        OCArrayRef qList = SIDimensionalityCreateArrayOfQuantities(baseDim);
        quantityName = (OCStringRef)OCArrayGetValueAtIndex(qList, 0);
        OCRelease(qList);
    }
    // 3) Validate or default each scalar
    if (!impl_validateOrDefaultScalar("offset", &offset, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("origin", &origin, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("period", &period, baseUnit, baseDim, &err)) {
        goto Fail;
    }
    // 4) Allocate and initialize
    SIDimensionRef dim = SIDimensionAllocate();
    if (!dim) {
        err = STR("SIDimensionCreate: allocation failed");
        goto Fail;
    }
    impl_InitBaseDimensionFields((DimensionRef)&dim->_super);
    impl_InitSIDimensionFields(dim);
    // 8) Apply user values (label, description, metadata)
    if (label &&
        !DimensionSetLabel((DimensionRef)dim, label, outError)) {
        OCRelease(dim);
        return NULL;
    }
    if (description &&
        !DimensionSetDescription((DimensionRef)dim, description, outError)) {
        OCRelease(dim);
        return NULL;
    }
    if (metadata &&
        !DimensionSetMetadata((DimensionRef)dim, metadata, outError)) {
        OCRelease(dim);
        return NULL;
    }
    // 9) Copy SI‐specific fields: release defaults and copy new values
    OCRelease(dim->quantityName);
    OCRelease(dim->offset);
    OCRelease(dim->origin);
    dim->quantityName = OCStringCreateCopy(quantityName);
    if (!dim->quantityName) {
        if (outError) *outError = STR("SIDimensionCreate: failed to copy quantityName");
        OCRelease(dim);
        return NULL;
    }
    dim->offset = SIScalarCreateCopy(offset);
    if (!dim->offset) {
        if (outError) *outError = STR("SIDimensionCreate: failed to copy offset");
        OCRelease(dim);
        return NULL;
    }
    dim->origin = SIScalarCreateCopy(origin);
    if (!dim->origin) {
        if (outError) *outError = STR("SIDimensionCreate: failed to copy origin");
        OCRelease(dim);
        return NULL;
    }
    if (period) {
        dim->period = SIScalarCreateCopy(period);
        if (!dim->period) {
            if (outError) *outError = STR("SIDimensionCreate: failed to copy period");
            OCRelease(dim);
            return NULL;
        }
    }
    // 11) Flags
    dim->periodic = periodic;
    dim->scaling = scaling;
    return dim;
Fail:
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return NULL;
}
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim) {
    return dim ? dim->quantityName : NULL;
}
bool SIDimensionSetQuantityName(SIDimensionRef dim,
                                OCStringRef name,
                                OCStringRef *outError) {
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
    // 7) If we were periodic but the period no longer matches, clear it
    if (dim->periodic && dim->period) {
        SIDimensionalityRef perDim = SIQuantityGetUnitDimensionality((SIQuantityRef)dim->period);
        if (!SIDimensionalityHasSameReducedDimensionality(nameDim, perDim)) {
            OCRelease(dim->period);
            dim->period = NULL;
            dim->periodic = false;
        }
    }
    return true;
}
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim) {
    return dim ? dim->offset : NULL;
}
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim,
                                     SIScalarRef val,
                                     OCStringRef *outError) {
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
    // 4) Check that val’s dimensionality matches
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
    // 7) If periodic but period no longer matches, clear it
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
    return dim ? dim->origin : NULL;
}
bool SIDimensionSetOriginOffset(SIDimensionRef dim,
                                SIScalarRef val,
                                OCStringRef *outError) {
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
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim) {
    // Always return the stored period (even if periodic == false),
    // so it can be re-enabled later without losing the old value.
    return dim ? dim->period : NULL;
}
bool SIDimensionSetPeriod(SIDimensionRef dim,
                          SIScalarRef val,
                          OCStringRef *outError) {
    if (outError) *outError = NULL;
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
    // 5) If it's the same object, just enable periodicity
    if (dim->period == val) {
        dim->periodic = true;
        return true;
    }
    // 6) Convert & deep‐copy into our “relative” unit
    SIUnitRef relUnit = SIQuantityGetUnit((SIQuantityRef)dim->offset);
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(val, relUnit, NULL);
    if (!copy) {
        if (outError) *outError = STR("SIDimensionSetPeriod: conversion to relative unit failed");
        return false;
    }
    // 7) Normalize element type
    SIScalarSetElementType((SIMutableScalarRef)copy, (SINumberType)kOCNumberFloat64Type);
    // 8) Swap in the new period value and enable periodicity
    OCRelease(dim->period);
    dim->period = copy;
    dim->periodic = true;
    return true;
}
bool SIDimensionIsPeriodic(SIDimensionRef dim) {
    // Simplify: just guard and return the flag
    return dim && dim->periodic;
}
bool SIDimensionSetPeriodic(SIDimensionRef dim,
                            bool flag,
                            OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Must have a valid dimension
    if (!dim) {
        if (outError) *outError = STR("SIDimensionSetPeriodic: dim is NULL");
        return false;
    }
    if (flag) {
        // Enabling periodic requires a non-NULL period
        if (!dim->period) {
            if (outError)
                *outError = STR("SIDimensionSetPeriodic: can't enable periodicity without a period");
            return false;
        }
        dim->periodic = true;
    } else {
        // Disabling periodic leaves the stored period intact
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
    OCStringRef label = NULL;
    OCStringRef description = NULL;
    OCDictionaryRef metadata = NULL;
    OCStringRef quantityName = NULL;
    SIScalarRef offset = NULL;
    SIScalarRef origin = NULL;
    SIScalarRef period = NULL;
    OCBooleanRef boolObj = NULL;
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
    label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    description = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionMetadataKey));
    /* 2) quantity_name (required) */
    quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey));
    /* 3) offset (string → SIScalar) */
    OCStringRef offsetStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOffsetKey));
    if(offsetStr) {
        offset = SIScalarCreateFromExpression(offsetStr, &parseErr);
        if (!offset) {
            if (outError)
                *outError = parseErr;
            else if (parseErr)
                OCRelease(parseErr);
            return NULL;
        }
    }
    /* 4) origin (optional) */
    OCStringRef originStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOriginKey));
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
    OCStringRef periodStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodKey));
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
    boolObj = (OCBooleanRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodicKey));
    periodic = boolObj ? OCBooleanGetValue(boolObj) : false;
    /* 7) scaling enum */
    numObj = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSIDimensionScalingKey));
    if (numObj) {
        int tmp = 0;
        OCNumberTryGetInt(numObj, &tmp);
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
        scaling, outError);
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
    // 1) Start with base class fields
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)impl_DimensionCopyAsDictionary((DimensionRef)dim);
    if (!dict) return NULL;
    // 2) quantity_name → string
    if (!CopyStringField(dict, STR(kSIDimensionQuantityNameKey), SIDimensionGetQuantityName(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // 3) offset → string
    {
        OCStringRef tmp = SIScalarCreateStringValue(SIDimensionGetCoordinatesOffset(dim));
        if (!tmp || !CopyStringField(dict, STR(kSIDimensionOffsetKey), tmp)) {
            if (tmp) OCRelease(tmp);
            OCRelease(dict);
            return NULL;
        }
        OCRelease(tmp);
    }
    // 4) origin → string
    {
        OCStringRef tmp = SIScalarCreateStringValue(SIDimensionGetOriginOffset(dim));
        if (!tmp || !CopyStringField(dict, STR(kSIDimensionOriginKey), tmp)) {
            if (tmp) OCRelease(tmp);
            OCRelease(dict);
            return NULL;
        }
        OCRelease(tmp);
    }
    // 5) period → string
    {
        SIScalarRef periodScalar = SIDimensionGetPeriod(dim);
        if (periodScalar) {
            OCStringRef tmp = SIScalarCreateStringValue(periodScalar);
            if (!tmp || !CopyStringField(dict, STR(kSIDimensionPeriodKey), tmp)) {
                if (tmp) OCRelease(tmp);
                OCRelease(dict);
                return NULL;
            }
            OCRelease(tmp);
        }
    }
    // 6) periodic flag
    if (!CopyBoolField(dict, STR(kSIDimensionPeriodicKey), SIDimensionIsPeriodic(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // 7) scaling enum
    if (!CopyNumField(dict, STR(kSIDimensionScalingKey), SIDimensionGetScaling(dim))) {
        OCRelease(dict);
        return NULL;
    }
    return (OCDictionaryRef)dict;
}
static OCDictionaryRef SIDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("SIDimension: expected JSON object");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item;
    // Optional: label
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionLabelKey);
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionLabelKey), s);
        OCRelease(s);
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionDescriptionKey), s);
        OCRelease(s);
    }
    // Optional: metadata
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionMetadataKey);
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionMetadataKey), metadata);
        OCRelease(metadata);
    }
    // quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionQuantityNameKey);
    OCStringRef qname;
    if (cJSON_IsString(item) && item->valuestring[0] != '\0') {
        qname = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionQuantityNameKey), qname);
        OCRelease(qname);

    }
    // offset
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionOffsetKey);
    if (cJSON_IsString(item)) {
        OCStringRef offset = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionOffsetKey), offset);
        OCRelease(offset);
    }
    // Optional: period
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionPeriodKey);
    if (cJSON_IsString(item)) {
        OCStringRef period = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionPeriodKey), period);
        OCRelease(period);
    }
    // Optional: periodic
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionPeriodicKey);
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR(kSIDimensionPeriodicKey), b);
        OCRelease(b);
    }
    // Optional: scaling
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionScalingKey);
    if (cJSON_IsNumber(item)) {
        OCNumberRef n = OCNumberCreateWithInt(item->valueint);
        OCDictionarySetValue(dict, STR(kSIDimensionScalingKey), n);
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
#define kSIDimensionReciprocalKey "reciprocal"
#define kSIMonotonicDimensionCoordinatesKey "coordinates"
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
    if (A == B) return true;
    // 1) compare all SIDimension fields
    if (!impl_SIDimensionEqual((const void *)&A->_super, (const void *)&B->_super))
        return false;
    // 2) reciprocal (null-safe)
    if (A->reciprocal != B->reciprocal &&
        !OCTypeEqual(A->reciprocal, B->reciprocal))
        return false;
    // 3) coordinates array (null-safe)
    if (A->coordinates != B->coordinates &&
        !OCTypeEqual(A->coordinates, B->coordinates))
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
            cJSON_AddItemToObject(json, kSIDimensionReciprocalKey, recip_json);
    }
    // coordinates (OCMutableArrayRef)
    if (mono->coordinates) {
        cJSON *coords_json = OCTypeCopyJSON((OCTypeRef)mono->coordinates);
        if (coords_json)
            cJSON_AddItemToObject(json, kSIMonotonicDimensionCoordinatesKey, coords_json);
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
static bool _SIDimensionIsReciprocalOf(SIDimensionRef src,
                                       SIDimensionRef rec,
                                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Both inputs must be non-NULL
    if (!src || !rec) {
        if (outError)
            *outError = STR("_SIDimensionIsReciprocalOf: src and rec must be non-NULL");
        return false;
    }
    // 2) Get source offset
    SIScalarRef srcOffset = SIDimensionGetCoordinatesOffset(src);
    if (!srcOffset) {
        if (outError)
            *outError = STR("_SIDimensionIsReciprocalOf: src offset is NULL");
        return false;
    }
    SIDimensionalityRef srcDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)srcOffset);
    // 3) Invert dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef invDim =
        SIDimensionalityByRaisingToPower(srcDim, -1.0, &err);
    if (!invDim) {
        if (outError) {
            // take ownership of err if provided, else use generic message
            *outError = err
                            ? err
                            : STR("_SIDimensionIsReciprocalOf: failed to invert dimensionality");
        }
        return false;
    }
    // 4) Get reciprocal’s offset
    SIScalarRef recOffset = SIDimensionGetCoordinatesOffset(rec);
    if (!recOffset) {
        if (outError)
            *outError = STR("_SIDimensionIsReciprocalOf: reciprocal offset is NULL");
        OCRelease(invDim);
        return false;
    }
    SIDimensionalityRef recDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)recOffset);
    // 5) Compare
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
    SIDimensionRef reciprocal,
    OCStringRef *outError)
{
    if (outError) *outError = NULL;
    OCStringRef err = NULL;

    // 1) Validate coordinates (≥2)
    OCIndex count = coordinates ? OCArrayGetCount(coordinates) : 0;
    if (count < 2) {
        err = STR("SIMonotonicDimensionCreate: need ≥2 coordinates");
        goto Fail;
    }

    // 2) Derive baseUnit/baseDim from first coordinate
    SIScalarRef first = (SIScalarRef)OCArrayGetValueAtIndex(coordinates, 0);
    SIUnitRef baseUnit = SIQuantityGetUnit((SIQuantityRef)first);
    SIDimensionalityRef baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)first);

    // 3) Validate or default other params
    if (!quantityName) {
        OCArrayRef qnList = SIDimensionalityCreateArrayOfQuantities(baseDim);
        quantityName = (OCStringRef)OCArrayGetValueAtIndex(qnList, 0);
        OCRelease(qnList);
    }

    if (!impl_validateOrDefaultScalar("offset", &offset, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("origin", &origin, baseUnit, baseDim, &err) ||
        (periodic && !impl_validateOrDefaultScalar("period", &period, baseUnit, baseDim, &err))) {
        goto Fail;
    }

    // Ensure required fields are not NULL after validation
    if (!quantityName || !offset || !origin || !coordinates) {
        err = STR("SIMonotonicDimensionCreate: internal error — required field is NULL after validation");
        goto Fail;
    }

    // 4) Allocate and initialize
    SIMonotonicDimensionRef dim = SIMonotonicDimensionAllocate();
    if (!dim) {
        err = STR("SIMonotonicDimensionCreate: allocation failed");
        goto Fail;
    }

    impl_InitBaseDimensionFields((DimensionRef)&dim->_super._super);
    impl_InitSIDimensionFields((SIDimensionRef)dim);

    SIDimensionRef si = (SIDimensionRef)dim;

    // 5) Apply base fields (deep copies)
    OCRelease(si->_super.label);
    si->_super.label = label ? OCStringCreateCopy(label) : NULL;

    OCRelease(si->_super.description);
    si->_super.description = description ? OCStringCreateCopy(description) : NULL;

    OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCTypeDeepCopy(metadata) : NULL;

    // 6) SI-specific fields (deep copies)
    OCRelease(si->quantityName);
    si->quantityName = OCStringCreateCopy(quantityName);
    if (!si->quantityName) {
        err = STR("SIMonotonicDimensionCreate: failed to copy quantityName");
        goto FailWithDim;
    }

    OCRelease(si->offset);
    si->offset = SIScalarCreateCopy(offset);
    if (!si->offset) {
        err = STR("SIMonotonicDimensionCreate: failed to copy offset");
        goto FailWithDim;
    }

    OCRelease(si->origin);
    si->origin = SIScalarCreateCopy(origin);
    if (!si->origin) {
        err = STR("SIMonotonicDimensionCreate: failed to copy origin");
        goto FailWithDim;
    }

    OCRelease(si->period);
    si->period = period ? SIScalarCreateCopy(period) : NULL;
    if (period && !si->period) {
        err = STR("SIMonotonicDimensionCreate: failed to copy period");
        goto FailWithDim;
    }

    si->periodic = periodic;
    si->scaling = scaling;

    // 7) Coordinates array (deep copy)
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(coordinates);
    if (!dim->coordinates) {
        err = STR("SIMonotonicDimensionCreate: failed to copy coordinates array");
        goto FailWithDim;
    }

    // 8) Reciprocal
    if (reciprocal) {
        if (!_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal, &err)) {
            goto FailWithDim;
        }
        dim->reciprocal = OCTypeDeepCopy(reciprocal);
        if (!dim->reciprocal) {
            err = STR("SIMonotonicDimensionCreate: failed to copy reciprocal");
            goto FailWithDim;
        }
    } else {
        // build default reciprocal here (if needed)
        dim->reciprocal = NULL;
    }

    return dim;

FailWithDim:
    OCRelease(dim);
Fail:
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return NULL;
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
        if (!_SIDimensionIsReciprocalOf((SIDimensionRef)dim, r, &err)) {
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
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionMetadataKey));
    // 3) SIDimension fields
    OCStringRef quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey));
    if (!quantityName) {
        if (outError) *outError = STR("SIMonotonicDimensionFromDict: missing \"quantity_name\"");
        return NULL;
    }
    OCStringRef err = NULL;
    SIScalarRef offset = NULL;
    SIScalarRef origin = NULL;
    SIScalarRef period = NULL;
    // 3a) offset (required)
    OCStringRef s = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOffsetKey));
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
    s = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOriginKey));
    if (s) {
        origin = SIScalarCreateFromExpression(s, &err);
        if (!origin) {
            OCRelease(offset);
            if (outError) *outError = err;
            return NULL;
        }
    }
    // 3c) period (optional)
    s = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodKey));
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
    OCBooleanRef pb = (OCBooleanRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodicKey));
    bool periodic = pb ? OCBooleanGetValue(pb) : false;
    OCNumberRef scn = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSIDimensionScalingKey));
    int tmp = 0;
    if (scn) OCNumberTryGetInt(scn, &tmp);
    dimensionScaling scaling = (dimensionScaling)tmp;
    // 5) Monotonic‐specific: coordinates
    OCArrayRef coordStrs = (OCArrayRef)OCDictionaryGetValue(dict, STR(kSIMonotonicDimensionCoordinatesKey));
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
    OCDictionaryRef recDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kSIDimensionReciprocalKey));
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
        reciprocal, outError);
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
    if (!dim) {
        return NULL;
    }

    // 1) Base + SI fields
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) {
        return NULL;
    }

    // 2) Type discriminator
    if (!CopyStringField(dict, STR("type"), STR("monotonic"))) {
        OCRelease(dict);
        return NULL;
    }

    // 3) Coordinates → array of strings
    OCArrayRef coords = SIMonotonicDimensionGetCoordinates(dim);
    if (coords) {
        OCIndex count = OCArrayGetCount(coords);
        OCMutableArrayRef coordsArr =
            OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
        if (!coordsArr) {
            OCRelease(dict);
            return NULL;
        }
        for (OCIndex i = 0; i < count; ++i) {
            SIScalarRef s = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
            OCStringRef sStr = s ? SIScalarCreateStringValue(s) : NULL;
            if (!sStr) {
                OCRelease(coordsArr);
                OCRelease(dict);
                return NULL;
            }
            OCArrayAppendValue(coordsArr, sStr);
            OCRelease(sStr);
        }
        OCDictionarySetValue(dict,
                             STR(kSIMonotonicDimensionCoordinatesKey),
                             coordsArr);
        OCRelease(coordsArr);
    }

    // 4) Reciprocal dimension (optional)
    SIDimensionRef rec = SIMonotonicDimensionGetReciprocal(dim);
    if (rec) {
        // use the generic dispatcher so any subclass gets its own fields
        OCDictionaryRef recDict = SIDimensionCopyAsDictionary(rec);
        if (!recDict) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict,
                             STR(kSIDimensionReciprocalKey),
                             recDict);
        OCRelease(recDict);
    }

    return (OCDictionaryRef)dict;
}
static OCDictionaryRef SIMonotonicDimensionDictionaryCreateFromJSON(cJSON *json,
                                                                    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for MonotonicDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item;
    // --- type discriminator ---
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item) || strcmp(item->valuestring, "monotonic") != 0) {
        if (outError) *outError = STR("Expected \"type\": \"monotonic\"");
        OCRelease(dict);
        return NULL;
    }
    OCDictionarySetValue(dict, STR("type"), OCStringCreateWithCString(item->valuestring));
    // --- label / description / metadata (optional) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionLabelKey);
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR(kDimensionLabelKey),
                             OCStringCreateWithCString(item->valuestring));
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionDescriptionKey);
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR(kDimensionDescriptionKey),
                             OCStringCreateWithCString(item->valuestring));
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionMetadataKey);
    if (cJSON_IsObject(item)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(item, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionMetadataKey), md);
        OCRelease(md);
    }
    // --- quantity_name (optional, default to "dimensionless") ---
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionQuantityNameKey);
    OCStringRef qn;
    if (cJSON_IsString(item) && item->valuestring[0] != '\0') {
        qn = OCStringCreateWithCString(item->valuestring);
    } else {
        qn = kSIQuantityDimensionless;
    }
    OCDictionarySetValue(dict, STR(kSIDimensionQuantityNameKey), qn);
    // Validate quantity_name dimensionality
    OCStringRef derr = NULL;
    SIDimensionalityRef nameDim = SIDimensionalityForQuantity(qn, &derr);
    // only release our copy if we created it
    if (!cJSON_IsString(item) || item->valuestring[0] == '\0') {
        OCRelease(qn);
    }
    if (!nameDim) {
        if (outError) {
            *outError = derr
                            ? OCStringCreateWithFormat(
                                  STR("MonotonicDimension: invalid quantity_name dimensionality: %@"),
                                  derr)
                            : STR("MonotonicDimension: invalid quantity_name dimensionality");
        }
        OCRelease(derr);
        OCRelease(dict);
        return NULL;
    }
    OCRelease(derr);
    // --- offset / coordinates_offset (required or defaulted) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionOffsetKey);
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR(kSIDimensionOffsetKey),
                             OCStringCreateWithCString(item->valuestring));
    } else {
        // fallback → zero scalar in first available unit
        OCStringRef berr = NULL;
        SIDimensionalityRef nameDim2 = SIDimensionalityForQuantity(
            (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey)), &berr);
        OCArrayRef units = SIUnitCreateArrayOfUnitsForDimensionality(nameDim2);
        OCRelease(berr);
        if (!units || OCArrayGetCount(units) == 0) {
            if (outError) *outError = STR("MonotonicDimension: no SI units available for fallback offset");
            if (units) OCRelease(units);
            OCRelease(dict);
            return NULL;
        }
        SIUnitRef u0 = OCArrayGetValueAtIndex(units, 0);
        OCRelease(units);
        SIScalarRef zero = SIScalarCreateWithDouble(0.0, u0);
        if (!zero) {
            if (outError) *outError = STR("MonotonicDimension: failed to create fallback zero scalar");
            OCRelease(dict);
            return NULL;
        }
        OCStringRef zstr = SIScalarCreateStringValue(zero);
        OCRelease(zero);
        if (!zstr) {
            if (outError) *outError = STR("MonotonicDimension: failed to stringify fallback zero scalar");
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kSIDimensionOffsetKey), zstr);
        OCRelease(zstr);
    }
    // --- origin / period / periodic / scaling (optional) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionOriginKey);
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR(kSIDimensionOriginKey),
                             OCStringCreateWithCString(item->valuestring));
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionPeriodKey);
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR(kSIDimensionPeriodKey),
                             OCStringCreateWithCString(item->valuestring));
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionPeriodicKey);
    if (cJSON_IsBool(item)) {
        OCBooleanRef b = OCBooleanGetWithBool(cJSON_IsTrue(item));
        OCDictionarySetValue(dict, STR(kSIDimensionPeriodicKey), b);
        OCRelease(b);
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionScalingKey);
    if (cJSON_IsNumber(item)) {
        OCNumberRef n = OCNumberCreateWithInt(item->valueint);
        OCDictionarySetValue(dict, STR(kSIDimensionScalingKey), n);
        OCRelease(n);
    }
    // --- coordinates (required) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kSIMonotonicDimensionCoordinatesKey);
    if (!cJSON_IsArray(item)) {
        if (outError) *outError = STR("MonotonicDimension: missing or invalid \"coordinates\"");
        OCRelease(dict);
        return NULL;
    }
    OCMutableArrayRef coords = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
    cJSON *coord;
    cJSON_ArrayForEach(coord, item) {
        if (cJSON_IsString(coord)) {
            OCStringRef s = OCStringCreateWithCString(coord->valuestring);
            OCArrayAppendValue(coords, s);
            OCRelease(s);
        }
    }
    OCDictionarySetValue(dict, STR(kSIMonotonicDimensionCoordinatesKey), coords);
    OCRelease(coords);
    // --- reciprocal (optional) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionReciprocalKey);
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef rec = SIDimensionDictionaryCreateFromJSON(item, outError);
        if (!rec) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kSIDimensionReciprocalKey), rec);
        OCRelease(rec);
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
#define kSILinearDimensionCountKey "count"
#define kSILinearDimensionIncrementKey "increment"
#define kSILinearDimensionFFTKey "complex_fft"
static OCTypeID kSILinearDimensionID = kOCNotATypeID;
typedef struct impl_SILinearDimension {
    struct impl_SIDimension _super;  // inherit Dimension + SI fields
    SIDimensionRef reciprocal;       // optional reciprocal dimension
    OCIndex count;                   // number of points (>=2)
    SIScalarRef increment;           // spacing between points
    bool fft;                        // FFT flag
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
    if (A == B) return true;
    // compare base SI fields
    if (!impl_SIDimensionEqual((const void *)&A->_super, (const void *)&B->_super))
        return false;
    // compare subclass fields
    if (A->count != B->count || A->fft != B->fft)
        return false;
    if (A->increment != B->increment &&
        !OCTypeEqual(A->increment, B->increment))
        return false;
    if (A->reciprocal != B->reciprocal &&
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
    OCStringRef incStr = d->increment
                             ? SIScalarCreateStringValue(d->increment)
                             : STR("(no increment)");
    OCStringRef fftStr = d->fft ? STR("true") : STR("false");  // 3) Stitch together
    OCStringRef fmt = OCStringCreateWithFormat(
        STR("%@ count=%lu increment=%@ fft=%@>"),
        base,
        (unsigned long)d->count,
        incStr,
        fftStr);
    // 4) Clean up
    OCRelease(base);
    OCRelease(incStr);
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
            cJSON_AddItemToObject(json, kSIDimensionReciprocalKey, recip_json);
    }
    // count (OCIndex, presumably integer)
    cJSON_AddNumberToObject(json, kSILinearDimensionCountKey, (int)lin->count);
    // increment (SIScalarRef)
    if (lin->increment) {
        cJSON *inc_json = OCTypeCopyJSON((OCTypeRef)lin->increment);
        if (inc_json)
            cJSON_AddItemToObject(json, kSILinearDimensionIncrementKey, inc_json);
    }
    // fft (bool)
    cJSON_AddBoolToObject(json, kSILinearDimensionFFTKey, lin->fft);
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
    SIDimensionRef reciprocal,
    OCStringRef *outError)
{
    if (outError) *outError = NULL;
    OCStringRef err = NULL;

    // 1) Validate count & increment
    if (count < 2) {
        err = STR("SILinearDimensionCreate: need ≥2 points");
        goto Fail;
    }
    if (!increment || OCGetTypeID((OCTypeRef)increment) != SIScalarGetTypeID() ||
        SIQuantityIsComplexType((SIQuantityRef)increment)) {
        err = STR("SILinearDimensionCreate: increment must be a real SIScalar");
        goto Fail;
    }

    // 2) Derive baseUnit/baseDim from increment
    SIUnitRef baseUnit = SIQuantityGetUnit((SIQuantityRef)increment);
    SIDimensionalityRef baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)increment);

    // 3) Default quantityName if missing
    if (!quantityName) {
        OCArrayRef qnList = SIDimensionalityCreateArrayOfQuantities(baseDim);
        quantityName = (OCStringRef)OCArrayGetValueAtIndex(qnList, 0);
        OCRelease(qnList);
    }

    // 4) Validate or default offset/origin/period
    if (!impl_validateOrDefaultScalar("offset", &offset, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("origin", &origin, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("period", &period, baseUnit, baseDim, &err)) {
        goto Fail;
    }

    // Ensure required fields are now non-NULL
    if (!quantityName || !offset || !origin || !increment) {
        err = STR("SILinearDimensionCreate: internal error — required field is NULL after validation");
        goto Fail;
    }

    // 5) Allocate & init
    SILinearDimensionRef dim = SILinearDimensionAllocate();
    if (!dim) {
        err = STR("SILinearDimensionCreate: allocation failed");
        goto Fail;
    }

    impl_InitBaseDimensionFields((DimensionRef)&dim->_super._super);
    impl_InitSIDimensionFields((SIDimensionRef)dim);

    // 6) Apply base + SI setters (with deep copies)
    SIDimensionRef si = (SIDimensionRef)dim;

    OCRelease(si->_super.label);
    si->_super.label = label ? OCStringCreateCopy(label) : NULL;

    OCRelease(si->_super.description);
    si->_super.description = description ? OCStringCreateCopy(description) : NULL;

    OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCTypeDeepCopy(metadata) : NULL;

    OCRelease(si->quantityName);
    si->quantityName = OCStringCreateCopy(quantityName);
    if (!si->quantityName) {
        err = STR("SILinearDimensionCreate: failed to copy quantityName");
        goto FailWithDim;
    }

    OCRelease(si->offset);
    si->offset = SIScalarCreateCopy(offset);
    if (!si->offset) {
        err = STR("SILinearDimensionCreate: failed to copy offset");
        goto FailWithDim;
    }

    OCRelease(si->origin);
    si->origin = SIScalarCreateCopy(origin);
    if (!si->origin) {
        err = STR("SILinearDimensionCreate: failed to copy origin");
        goto FailWithDim;
    }

    OCRelease(si->period);
    si->period = period ? SIScalarCreateCopy(period) : NULL;
    if (period && !si->period) {
        err = STR("SILinearDimensionCreate: failed to copy period");
        goto FailWithDim;
    }

    si->periodic = periodic;
    si->scaling = scaling;

    // 7) Linear-specific fields
    dim->count = count;

    OCRelease(dim->increment);
    dim->increment = SIScalarCreateCopy(increment);
    if (!dim->increment) {
        err = STR("SILinearDimensionCreate: failed to copy increment");
        goto FailWithDim;
    }

    dim->fft = fft;

    // 8) Reciprocal: validate or build default
    if (reciprocal) {
        if (!_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal, &err)) {
            goto FailWithDim;
        }
        dim->reciprocal = (SIDimensionRef)OCTypeDeepCopy(reciprocal);
        if (!dim->reciprocal) {
            err = STR("SILinearDimensionCreate: failed to copy reciprocal dimension");
            goto FailWithDim;
        }
    } else {
        // build default reciprocal here if applicable
        dim->reciprocal = NULL; // placeholder
    }

    // 9) Compute reciprocalIncrement
    // [Reciprocal increment logic not shown]

    return dim;

FailWithDim:
    OCRelease(dim);
Fail:
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return NULL;
}
SILinearDimensionRef SILinearDimensionCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError)
{
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("SILinearDimensionFromDict: dict is NULL");
        return NULL;
    }

    // make sure 'dim' always has a defined value
    SILinearDimensionRef dim = NULL;

    // 1) type discriminator
    OCStringRef type = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (!type || !OCStringEqual(type, STR("linear"))) {
        if (outError)
            *outError = STR("SILinearDimensionFromDict: wrong or missing \"type\"");
        return NULL;
    }

    // 2) pull raw values out of dict
    OCStringRef label      = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef desc       = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionMetadataKey));
    OCStringRef qtyName    = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey));
    OCStringRef offStr     = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOffsetKey));
    OCStringRef origStr    = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOriginKey));
    OCStringRef periodStr  = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodKey));
    OCBooleanRef pb        = (OCBooleanRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodicKey));
    OCNumberRef num        = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSIDimensionScalingKey));
    OCNumberRef cntNum     = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionCountKey));
    OCStringRef incStr     = (OCStringRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionIncrementKey));
    OCBooleanRef fftB      = (OCBooleanRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionFFTKey));
    OCDictionaryRef recDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kSIDimensionReciprocalKey));

    // 3) parse & validate primitives

    // Scalars
    SIScalarRef offset    = offStr    ? SIScalarCreateFromExpression(offStr,    outError) : NULL;
    SIScalarRef origin    = origStr   ? SIScalarCreateFromExpression(origStr,   outError) : NULL;
    SIScalarRef period    = periodStr ? SIScalarCreateFromExpression(periodStr, outError) : NULL;
    if (outError && *outError) goto Cleanup;

    // periodic flag
    bool periodic = pb ? OCBooleanGetValue(pb) : false;

    // scaling enum: must use OCNumberTryGetInt
    int scalingInt = kDimensionScalingNone;
    if (num) {
        if (!OCNumberTryGetInt(num, &scalingInt)) {
            if (outError) *outError = STR("SILinearDimensionFromDict: invalid \"scaling\" value");
            goto Cleanup;
        }
    }
    dimensionScaling scaling = (dimensionScaling)scalingInt;

    // count: must use OCNumberTryGetOCIndex
    OCIndex count = 0;
    if (cntNum) {
        if (!OCNumberTryGetOCIndex(cntNum, &count)) {
            if (outError) *outError = STR("SILinearDimensionFromDict: invalid \"count\" value");
            goto Cleanup;
        }
    }

    // increment scalar
    SIScalarRef increment = incStr ? SIScalarCreateFromExpression(incStr, outError) : NULL;
    if (outError && *outError) goto Cleanup;

    // fft flag
    bool fft = fftB ? OCBooleanGetValue(fftB) : false;

    // reciprocal dimension (nested)
    SIDimensionRef reciprocal = NULL;
    if (recDict) {
        reciprocal = (SIDimensionRef)SIDimensionCreateFromDictionary(recDict, outError);
        if (outError && *outError) goto Cleanup;
    }

    // 4) delegate to the core constructor
    dim = SILinearDimensionCreate(
        label, desc, metadata,
        qtyName,
        offset, origin, period,
        periodic, scaling,
        count, increment,
        fft, reciprocal,
        outError);

Cleanup:
    // 5) cleanup temporaries
    OCRelease(offset);
    OCRelease(origin);
    OCRelease(period);
    OCRelease(increment);
    OCRelease(reciprocal);

    return dim;
}

OCDictionaryRef SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim) {
    if (!dim) return NULL;
    // 1) Base + SI fields (offset/origin/period now strings)
    OCMutableDictionaryRef dict =
        (OCMutableDictionaryRef)SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    if (!dict) return NULL;
    // 2) Type discriminator
    if (!CopyStringField(dict, STR("type"), STR("linear"))) {
        OCRelease(dict);
        return NULL;
    }
    // 3) Count
    if (!CopyNumField(dict, STR(kSILinearDimensionCountKey), (int)SILinearDimensionGetCount(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // 4) Increment as string
    SIScalarRef inc = SILinearDimensionGetIncrement(dim);
    if (inc) {
        OCStringRef incStr = SIScalarCreateStringValue(inc);
        if (!incStr ||
            !CopyStringField(dict, STR(kSILinearDimensionIncrementKey), incStr)) {
            if (incStr) OCRelease(incStr);
            OCRelease(dict);
            return NULL;
        }
        OCRelease(incStr);
    }
    // 5) FFT flag
    if (!CopyBoolField(dict, STR(kSILinearDimensionFFTKey), SILinearDimensionGetComplexFFT(dim))) {
        OCRelease(dict);
        return NULL;
    }
    // 6) Reciprocal dimension (optional)
    SIDimensionRef rec = SILinearDimensionGetReciprocal(dim);
    if (rec) {
        OCDictionaryRef recDict = SIDimensionCopyAsDictionary(rec);
        if (!recDict) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kSIDimensionReciprocalKey), recDict);
        OCRelease(recDict);
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
        SIQuantityGetUnit((SIQuantityRef)SIDimensionGetCoordinatesOffset((SIDimensionRef)dim));
    SIScalarRef copy = SIScalarCreateByConvertingToUnit(inc, offsetUnit, NULL);
    if (!copy) return false;
    OCRelease(dim->increment);
    dim->increment = copy;
    return true;
}
SIScalarRef SILinearDimensionGetReciprocalIncrement(SILinearDimensionRef dim) {
    if (!dim || !dim->increment || dim->count < 2) {
        return NULL;
    }
    // Make a mutable copy of the spacing
    SIMutableScalarRef rec = SIScalarCreateMutableCopy(dim->increment);
    if (!rec) return NULL;
    // rec *= count
    SIScalarMultiplyByDimensionlessRealConstant(rec, (double)dim->count);
    // rec = 1 / rec
    SIScalarRaiseToAPowerWithoutReducingUnit(rec, -1.0, NULL);
    // normalize to double
    SIScalarSetElementType(rec, (SINumberType)kOCNumberFloat64Type);
    return rec;
}
bool SILinearDimensionGetComplexFFT(SILinearDimensionRef dim) {
    return dim ? dim->fft : false;
}
bool SILinearDimensionSetComplexFFT(SILinearDimensionRef dim, bool fft) {
    if (!dim) return false;
    dim->fft = fft;
    return true;
}
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim) {
    return dim ? dim->reciprocal : NULL;
}
bool SILinearDimensionSetReciprocal(
    SILinearDimensionRef dim,
    SIDimensionRef rec,
    OCStringRef *outError) {
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
static OCDictionaryRef SILinearDimensionDictionaryCreateFromJSON(
    cJSON *json,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for SILinearDimension");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item;
// Helper macros for one-liner copies
#define COPY_STR_FIELD(key)                                                                 \
    do {                                                                                    \
        if ((item = cJSON_GetObjectItemCaseSensitive(json, key)) && cJSON_IsString(item)) { \
            OCDictionarySetValue(dict,                                                      \
                                 STR(key),                                                  \
                                 OCStringCreateWithCString(item->valuestring));             \
        }                                                                                   \
    } while (0)
#define COPY_NUM_FIELD(key)                                                                 \
    do {                                                                                    \
        if ((item = cJSON_GetObjectItemCaseSensitive(json, key)) && cJSON_IsNumber(item)) { \
            OCDictionarySetValue(dict,                                                      \
                                 STR(key),                                                  \
                                 OCNumberCreateWithInt(item->valueint));                    \
        }                                                                                   \
    } while (0)
#define COPY_BOOL_FIELD(key)                                                              \
    do {                                                                                  \
        if ((item = cJSON_GetObjectItemCaseSensitive(json, key)) && cJSON_IsBool(item)) { \
            OCDictionarySetValue(dict,                                                    \
                                 STR(key),                                                \
                                 OCBooleanGetWithBool(cJSON_IsTrue(item)));               \
        }                                                                                 \
    } while (0)
    // --- Pass through all your simple fields ---
    COPY_STR_FIELD("type");
    COPY_STR_FIELD(kDimensionLabelKey);
    COPY_STR_FIELD(kDimensionDescriptionKey);
    COPY_STR_FIELD(kSIDimensionQuantityNameKey);
    COPY_STR_FIELD(kSIDimensionOffsetKey);
    COPY_STR_FIELD(kSIDimensionOriginKey);
    COPY_STR_FIELD(kSIDimensionPeriodKey);
    COPY_STR_FIELD(kSILinearDimensionIncrementKey);
    COPY_BOOL_FIELD(kSIDimensionPeriodicKey);
    COPY_NUM_FIELD(kSIDimensionScalingKey);
    COPY_NUM_FIELD(kSILinearDimensionCountKey);
    COPY_BOOL_FIELD(kSILinearDimensionFFTKey);
    // --- Metadata sub-object via your OCMetadataCreateFromJSON ---
    if ((item = cJSON_GetObjectItemCaseSensitive(json, kDimensionMetadataKey)) && cJSON_IsObject(item)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(item, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionMetadataKey), md);
        OCRelease(md);
    }
    // --- Optional reciprocal dimension (nested JSON → dict) ---
    if ((item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionReciprocalKey)) && cJSON_IsObject(item)) {
        OCDictionaryRef rec = SIDimensionDictionaryCreateFromJSON(item, outError);
        if (!rec) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kSIDimensionReciprocalKey), rec);
        OCRelease(rec);
    }
#undef COPY_STR_FIELD
#undef COPY_NUM_FIELD
#undef COPY_BOOL_FIELD
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
        SIScalarRef offset = SIDimensionGetCoordinatesOffset((SIDimensionRef)dim);
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
    if (tid == LabeledDimensionGetTypeID())
        return LabeledDimensionCopyAsDictionary((LabeledDimensionRef)dim);
    else if (tid == SIMonotonicDimensionGetTypeID())
        return SIMonotonicDimensionCopyAsDictionary((SIMonotonicDimensionRef)dim);
    else if (tid == SILinearDimensionGetTypeID())
        return SILinearDimensionCopyAsDictionary((SILinearDimensionRef)dim);
    else if (tid == SIDimensionGetTypeID())
        return SIDimensionCopyAsDictionary((SIDimensionRef)dim);
    else
        return impl_DimensionCopyAsDictionary(dim);  // fallback
}
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError)
            *outError = OCStringCreateWithCString("DimensionCreateFromDictionary: input dictionary is NULL");
        return NULL;
    }
    OCStringRef type = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
    if (type) {
        if (OCStringEqual(type, STR("labeled"))) {
            return (DimensionRef)LabeledDimensionCreateFromDictionary(dict, outError);
        } else if (OCStringEqual(type, STR("linear"))) {
            return (DimensionRef)SILinearDimensionCreateFromDictionary(dict, outError);
        } else if (OCStringEqual(type, STR("monotonic"))) {
            return (DimensionRef)SIMonotonicDimensionCreateFromDictionary(dict, outError);
        } else if (OCStringEqual(type, STR("si_dimension"))) {
            return (DimensionRef)SIDimensionCreateFromDictionary(dict, outError);
        } else {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("DimensionCreateFromDictionary: unknown type \"%@\""), type);
            }
            return NULL;
        }
    }
    // fallback to base
    return impl_DimensionCreateFromDictionary(dict, NULL);
}
DimensionRef DimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError)
            *outError = STR("DimensionCreateFromJSON: expected JSON object");
        return NULL;
    }
    cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (typeItem && cJSON_IsString(typeItem)) {
        const char *typeStr = typeItem->valuestring;
        if (strcmp(typeStr, "labeled") == 0)
            return (DimensionRef)LabeledDimensionCreateFromJSON(json, outError);
        else if (strcmp(typeStr, "linear") == 0)
            return (DimensionRef)SILinearDimensionCreateFromJSON(json, outError);
        else if (strcmp(typeStr, "monotonic") == 0)
            return (DimensionRef)SIMonotonicDimensionCreateFromJSON(json, outError);
        else if (strcmp(typeStr, "si_dimension") == 0)
            return (DimensionRef)SIDimensionCreateFromJSON(json, outError);
        else {
            if (outError)
                *outError = OCStringCreateWithFormat(STR("DimensionCreateFromJSON: unknown type \"%s\""), typeStr);
            return NULL;
        }
    }
    return impl_DimensionCreateFromJSON(json, outError);
}
#pragma endregion Dimension Utilities
