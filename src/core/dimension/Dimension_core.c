/**
 * @file Dimension_core.c
 * @brief Core infrastructure for Dimension types
 *
 * This module handles type registration, lifecycle management, and core
 * infrastructure functions for all dimension types.
 *
 * Functions include:
 * - Type registration (GetTypeID functions)
 * - Object lifecycle (finalize, equal, copy)
 * - Object allocation and creation
 * - JSON serialization and deserialization
 * - Base field initialization
 * - Parameter validation
 */
#include "../RMNLibrary.h"
#include "Dimension_private.h"
#ifdef __cplusplus
extern "C" {
#endif
// ============================================================================
#pragma region Type Registration
// ============================================================================
// Static type ID variables
static OCTypeID _kDimensionTypeID = kOCNotATypeID;
static OCTypeID _kLabeledDimensionTypeID = kOCNotATypeID;
static OCTypeID _kSIDimensionTypeID = kOCNotATypeID;
static OCTypeID _kSIMonotonicDimensionTypeID = kOCNotATypeID;
static OCTypeID _kSILinearDimensionTypeID = kOCNotATypeID;
// Type registration functions
OCTypeID DimensionGetTypeID(void) {
    if (_kDimensionTypeID == kOCNotATypeID) {
        _kDimensionTypeID = OCRegisterType("Dimension");
    }
    return _kDimensionTypeID;
}
OCTypeID LabeledDimensionGetTypeID(void) {
    if (_kLabeledDimensionTypeID == kOCNotATypeID) {
        _kLabeledDimensionTypeID = OCRegisterType("LabeledDimension");
    }
    return _kLabeledDimensionTypeID;
}
OCTypeID SIDimensionGetTypeID(void) {
    if (_kSIDimensionTypeID == kOCNotATypeID) {
        _kSIDimensionTypeID = OCRegisterType("SIDimension");
    }
    return _kSIDimensionTypeID;
}
OCTypeID SIMonotonicDimensionGetTypeID(void) {
    if (_kSIMonotonicDimensionTypeID == kOCNotATypeID) {
        _kSIMonotonicDimensionTypeID = OCRegisterType("SIMonotonicDimension");
    }
    return _kSIMonotonicDimensionTypeID;
}
OCTypeID SILinearDimensionGetTypeID(void) {
    if (_kSILinearDimensionTypeID == kOCNotATypeID) {
        _kSILinearDimensionTypeID = OCRegisterType("SILinearDimension");
    }
    return _kSILinearDimensionTypeID;
}
#pragma endregion
// ============================================================================
#pragma region Helper functions for CopyAsDictionary operations
// ============================================================================
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
#pragma endregion
// ============================================================================
#pragma region Dimension (Base Class)
// ============================================================================
bool impl_DimensionEqual(const void *a, const void *b) {
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
void impl_DimensionFinalize(const void *obj) {
    DimensionRef dim = (DimensionRef)obj;
    if (!dim) return;
    OCRelease(dim->label);
    OCRelease(dim->description);
    OCRelease(dim->metadata);
    dim->label = NULL;
    dim->description = NULL;
    dim->metadata = NULL;
}
OCStringRef impl_DimensionCopyFormattingDesc(OCTypeRef cf) {
    DimensionRef dim = (DimensionRef)cf;
    if (!dim) {
        return OCStringCreateWithCString("<Dimension: NULL>");
    }
    // Pull via getters so subclasses aren't bypassed
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
// Forward declarations for functions that have circular dependencies
DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim);
cJSON *impl_DimensionCreateJSON(const void *obj) {
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
void *impl_DimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = impl_DimensionCopyAsDictionary((DimensionRef)obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    DimensionRef copy = impl_DimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
DimensionRef impl_DimensionAllocate(void) {
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
DimensionRef impl_DimensionCreate(OCStringRef label,
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
        if (!DimensionSetApplicationMetaData(dim, metadata, outError)) {
            OCRelease(dim);
            return NULL;
        }
    }
    return dim;
}
DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict,
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
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionApplicationKey));
    // 2) Delegate to the single "true" constructor, passing through outError
    return impl_DimensionCreate(label, description, metadata, outError);
}
OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim) {
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
    if (!CopyDictField(dict, STR(kDimensionApplicationKey), DimensionGetApplicationMetaData(dim))) {
        OCRelease(dict);
        return NULL;
    }
    return (OCDictionaryRef)dict;
}
// Implementation of core infrastructure functions
void impl_InitBaseDimensionFields(DimensionRef dim) {
    dim->label = STR("");
    dim->description = STR("");
    dim->metadata = OCDictionaryCreateMutable(0);
}
OCDictionaryRef impl_DimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
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
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionApplicationKey);
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionApplicationKey), metadata);
        OCRelease(metadata);
    }
    return dict;
}
DimensionRef impl_DimensionCreateFromJSON(cJSON *json, OCStringRef *outError) {
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
#pragma endregion
// ============================================================================
#pragma region LabeledDimension
// ============================================================================
#define kLabeledDimensionCoordinateLabelsKey "labels"
bool impl_LabeledDimensionEqual(const void *a, const void *b) {
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
void impl_LabeledDimensionFinalize(const void *obj) {
    const LabeledDimensionRef dim = (const LabeledDimensionRef)obj;
    // Finalize only the base part:
    impl_DimensionFinalize((DimensionRef)&dim->_super);
    // Then clean up subclass fields:
    OCRelease(dim->coordinateLabels);
    /* dim->coordinateLabels = NULL;  // not strictly needed after finalize */
}
OCStringRef impl_LabeledDimensionCopyFormattingDesc(OCTypeRef cf) {
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
    
    // Add type discriminator
    cJSON_AddStringToObject(json, "type", "labeled");
    
    // Now, add the subclass field: coordinateLabels (OCMutableArrayRef)
    if (ldim->coordinateLabels) {
        cJSON *labels_json = OCTypeCopyJSON((OCTypeRef)ldim->coordinateLabels);
        if (labels_json)
            cJSON_AddItemToObject(json, kLabeledDimensionCoordinateLabelsKey, labels_json);
    }
    return json;
}
void *impl_LabeledDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = LabeledDimensionCopyAsDictionary((LabeledDimensionRef)obj);
    if (!dict) return NULL;
    LabeledDimensionRef copy = LabeledDimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
LabeledDimensionRef LabeledDimensionAllocate(void) {
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
        !DimensionSetApplicationMetaData((DimensionRef)dim, metadata, outError)) {
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
                STR("LabeledDimensionFromDict: expected type \"labeled\", got \"%@\""),
                type ? type : STR("<none>"));
        }
        return NULL;
    }
    // Pull out fields
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef description = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionApplicationKey));
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
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionApplicationKey);
    if (cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionApplicationKey), metadata);
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
#pragma endregion
// ============================================================================
#pragma region SIDimension
// ============================================================================
#define kSIDimensionQuantityNameKey "quantity_name"
#define kSIDimensionOffsetKey "coordinates_offset"
#define kSIDimensionOriginKey "origin_offset"
#define kSIDimensionPeriodKey "period"
#define kSIDimensionPeriodicKey "periodic"
#define kSIDimensionScalingKey "scaling"
bool impl_SIDimensionEqual(const void *a, const void *b) {
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
    // 4) periods must be equal (both NULL or both equal)
    if (dimA->period != dimB->period &&
        !OCTypeEqual(dimA->period, dimB->period))
        return false;
    // 5) scaling mode
    if (dimA->scaling != dimB->scaling)
        return false;
    return true;
}
void impl_SIDimensionFinalize(const void *obj) {
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
OCStringRef impl_SIDimensionCopyFormattingDesc(OCTypeRef cf) {
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
    cJSON_AddBoolToObject(json, kSIDimensionPeriodicKey, sidim->period != NULL);
    cJSON_AddNumberToObject(json, kSIDimensionScalingKey, (int)sidim->scaling);
    return json;
}
void *impl_SIDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    // Serialize to a dictionary
    OCDictionaryRef dict = SIDimensionCopyAsDictionary((SIDimensionRef)obj);
    if (!dict) return NULL;
    // Rehydrate a new instance from that dictionary
    SIDimensionRef copy = SIDimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
void impl_InitSIDimensionFields(SIDimensionRef dim) {
    // Default quantity name: dimensionless
    dim->quantityName = kSIQuantityDimensionless;
    // Use the unit‐less SI unit for all default scalars
    SIUnitRef u = SIUnitDimensionlessAndUnderived();
    // Default coordinate offset & origin both zero in the same unit
    dim->offset = SIScalarCreateWithDouble(0.0, u);
    dim->origin = SIScalarCreateWithDouble(0.0, u);
    // No period until the user explicitly sets one
    dim->period = SIScalarCreateWithDouble(INFINITY, u);
    dim->scaling = kDimensionScalingNone;
}
bool impl_InitSIDimensionFieldsFromArgs(
    SIDimensionRef dim,
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    dimensionScaling scaling) {
    DimensionRef baseDim = (DimensionRef)dim;
    bool success = true;
    success &= DimensionSetLabel(baseDim, label, NULL);
    success &= DimensionSetDescription(baseDim, description, NULL);
    success &= DimensionSetApplicationMetaData(baseDim, metadata, NULL);
    success &= SIDimensionSetCoordinatesOffset(dim, offset, NULL);
    success &= SIDimensionSetQuantityName(dim, quantityName, NULL);
    success &= SIDimensionSetOriginOffset(dim, origin, NULL);
    success &= (!period || SIDimensionSetPeriod(dim, period, NULL));
    success &= SIDimensionSetScaling(dim, scaling);
    return success;
}
// Helper: ensure *scalarPtr points to a real SIScalar compatible with dimensionality.
// If *scalarPtr is NULL, creates a zero scalar in unit.
// Returns true on success, false + *outError on failure.
bool impl_validateOrDefaultScalar(
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
SIDimensionRef SIDimensionAllocate(void) {
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
SIDimensionRef SIDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    dimensionScaling scaling,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    OCStringRef err = NULL;
    // Track which parameters were originally NULL for cleanup
    bool offset_was_null = (offset == NULL);
    bool origin_was_null = (origin == NULL);
    bool period_was_null = (period == NULL);
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
    if (period_was_null) SIScalarSetDoubleValue((SIMutableScalarRef) period, INFINITY);
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
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
        return NULL;
    }
    if (description &&
        !DimensionSetDescription((DimensionRef)dim, description, outError)) {
        OCRelease(dim);
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
        return NULL;
    }
    if (metadata &&
        !DimensionSetApplicationMetaData((DimensionRef)dim, metadata, outError)) {
        OCRelease(dim);
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
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
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
        return NULL;
    }
    dim->offset = SIScalarCreateCopy(offset);
    if (!dim->offset) {
        if (outError) *outError = STR("SIDimensionCreate: failed to copy offset");
        OCRelease(dim);
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
        return NULL;
    }
    dim->origin = SIScalarCreateCopy(origin);
    if (!dim->origin) {
        if (outError) *outError = STR("SIDimensionCreate: failed to copy origin");
        OCRelease(dim);
        // Release temporary SIScalar objects created by validation if they were NULL inputs
        if (offset_was_null && offset) OCRelease(offset);
        if (origin_was_null && origin) OCRelease(origin);
        if (period_was_null && period) OCRelease(period);
        return NULL;
    }
    if (period) {
        OCRelease(dim->period);  // Release the default period first!
        dim->period = SIScalarCreateCopy(period);
        if (!dim->period) {
            if (outError) *outError = STR("SIDimensionCreate: failed to copy period");
            OCRelease(dim);
            // Release temporary SIScalar objects created by validation if they were NULL inputs
            if (offset_was_null && offset) OCRelease(offset);
            if (origin_was_null && origin) OCRelease(origin);
            if (period_was_null && period) OCRelease(period);
            return NULL;
        }
    }
    // 11) Flags
    dim->scaling = scaling;
    // 12) Release temporary SIScalar objects created by validation if they were NULL inputs
    if (offset_was_null && offset) {
        OCRelease(offset);
    }
    if (origin_was_null && origin) {
        OCRelease(origin);
    }
    if (period_was_null && period) {
        OCRelease(period);
    }
    return dim;
Fail:
    // Release temporary SIScalar objects created by validation if they were NULL inputs
    if (offset_was_null && offset) {
        OCRelease(offset);
    }
    if (origin_was_null && origin) {
        OCRelease(origin);
    }
    if (period_was_null && period) {
        OCRelease(period);
    }
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return NULL;
}
OCDictionaryRef SIDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
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
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionApplicationKey);
    if (item && cJSON_IsObject(item)) {
        OCDictionaryRef metadata = OCMetadataCreateFromJSON(item, outError);
        if (!metadata) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionApplicationKey), metadata);
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
// Helper function for reciprocal validation
bool impl_SIDimensionIsReciprocalOf(SIDimensionRef src,
                                    SIDimensionRef rec,
                                    OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Both inputs must be non-NULL
    if (!src || !rec) {
        if (outError)
            *outError = STR("impl_SIDimensionIsReciprocalOf: src and rec must be non-NULL");
        return false;
    }
    // 2) Get source offset
    SIScalarRef srcOffset = SIDimensionGetCoordinatesOffset(src);
    if (!srcOffset) {
        if (outError)
            *outError = STR("impl_SIDimensionIsReciprocalOf: src offset is NULL");
        return false;
    }
    SIDimensionalityRef srcDim =
        SIQuantityGetUnitDimensionality((SIQuantityRef)srcOffset);
    // 3) Invert dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef invDim =
        SIDimensionalityByRaisingToPower(srcDim, -1.0);
    if (!invDim) {
        if (outError) {
            // take ownership of err if provided, else use generic message
            *outError = err
                            ? err
                            : STR("impl_SIDimensionIsReciprocalOf: failed to invert dimensionality");
        }
        return false;
    }
    // 4) Get reciprocal's offset
    SIScalarRef recOffset = SIDimensionGetCoordinatesOffset(rec);
    if (!recOffset) {
        if (outError)
            *outError = STR("impl_SIDimensionIsReciprocalOf: reciprocal offset is NULL");
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
    metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionApplicationKey));
    /* 2) quantity_name (required) */
    quantityName = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey));
    /* 3) offset (string → SIScalar) */
    OCStringRef offsetStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOffsetKey));
    if (offsetStr) {
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
    /* 6) scaling enum */
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
        scaling, outError);
    /* 9) clean up temporaries */
    OCRelease(offset);
    if (origin) OCRelease(origin);
    if (period) OCRelease(period);
    if (!dim && outError)
        *outError = STR("SIDimensionCreateFromDictionary: SIDimensionCreate failed");
    return dim;
}
SIDimensionRef SIDimensionCreateCopy(SIDimensionRef dim) {
    return impl_SIDimensionDeepCopy(dim);
}
#pragma endregion
// ============================================================================
#pragma region SIMonotonicDimension
// ============================================================================
#define kSIDimensionReciprocalKey "reciprocal"
#define kSIMonotonicDimensionCoordinatesKey "coordinates"
bool impl_SIMonotonicDimensionEqual(const void *a, const void *b) {
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
void impl_SIMonotonicDimensionFinalize(const void *obj) {
    if (!obj) return;
    SIMonotonicDimensionRef dim = (SIMonotonicDimensionRef)obj;
    // finalize SIDimension‐super
    impl_SIDimensionFinalize((const void *)&dim->_super);
    // then our own
    OCRelease(dim->reciprocal);
    OCRelease(dim->coordinates);
}
OCStringRef impl_SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf) {
    SIMonotonicDimensionRef d = (SIMonotonicDimensionRef)cf;
    if (!d) {
        return STR("<SIMonotonicDimension: NULL>");
    }
    // 1) Grab the base SIDimension description
    OCStringRef base = impl_SIDimensionCopyFormattingDesc(cf);
    // 2) Build a small "yes"/"no" string for reciprocal
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
    
    // Add type discriminator
    cJSON_AddStringToObject(json, "type", "monotonic");
    
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
void *impl_SIMonotonicDimensionDeepCopy(const void *obj) {
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
SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void) {
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
SIMonotonicDimensionRef SIMonotonicDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    dimensionScaling scaling,
    OCArrayRef coordinates,
    SIDimensionRef reciprocal,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    OCStringRef err = NULL;
    // Track which parameters were originally NULL for cleanup
    bool offset_was_null = (offset == NULL);
    bool origin_was_null = (origin == NULL);
    bool period_was_null = (period == NULL);
    // 1) Validate coordinates (≥2)
    OCIndex count = coordinates ? OCArrayGetCount(coordinates) : 0;
    if (count < 2) {
        err = STR("SIMonotonicDimensionCreate: need ≥2 coordinates");
        goto Fail;
    }
    // All coordinates must be SIScalarRef
    // We allow OCNumbers to be passed in for convenience, but convert them to SIScalar with dimensionless unit
    OCArrayRef scalarCoords = SIScalarCreateArrayFromMixedTypeArray(coordinates, outError);
    if (!scalarCoords) {
        err = STR("SIMonotonicDimensionCreate: failed to convert coordinates to SIScalar array");
        goto Fail;
    }

    // 2) Derive baseUnit/baseDim from first coordinate
    SIScalarRef first = (SIScalarRef)OCArrayGetValueAtIndex(scalarCoords, 0);
    SIUnitRef baseUnit = SIQuantityGetUnit((SIQuantityRef)first);
    SIDimensionalityRef baseDim = SIQuantityGetUnitDimensionality((SIQuantityRef)first);

    // Validate that all coordinates have the same dimensionality
    if (!SIQuantityValidateMixedArrayForDimensionality(scalarCoords, baseDim, &err)) {
        goto FailWithCoords;
    }

    // 3) Validate or default other params
    if (!quantityName) {
        OCArrayRef qnList = SIDimensionalityCreateArrayOfQuantities(baseDim);
        quantityName = (OCStringRef)OCArrayGetValueAtIndex(qnList, 0);
        OCRelease(qnList);
    } else if (SIDimensionalityForQuantity(quantityName, &err) == NULL) {
        err = STR("SIMonotonicDimensionCreate: invalid quantityName");
        goto FailWithCoords;
    }
    if (!impl_validateOrDefaultScalar("offset", &offset, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("origin", &origin, baseUnit, baseDim, &err) ||
        (!impl_validateOrDefaultScalar("period", &period, baseUnit, baseDim, &err))) {
        goto FailWithCoords;
    }
    if (period_was_null) SIScalarSetDoubleValue((SIMutableScalarRef)period, INFINITY);
    // 4) Validate that required fields are not NULL after validation
    // Ensure required fields are not NULL after validation
    if (!quantityName || !offset || !origin || !coordinates) {
        err = STR("SIMonotonicDimensionCreate: internal error — required field is NULL after validation");
        goto FailWithCoords;
    }
    // 5) Allocate and initialize
    SIMonotonicDimensionRef dim = SIMonotonicDimensionAllocate();
    if (!dim) {
        err = STR("SIMonotonicDimensionCreate: allocation failed");
        goto FailWithCoords;
    }
    impl_InitBaseDimensionFields((DimensionRef)&dim->_super._super);
    impl_InitSIDimensionFields((SIDimensionRef)dim);
    SIDimensionRef si = (SIDimensionRef)dim;
    // 6) Apply base fields (deep copies)
    OCRelease(si->_super.label);
    si->_super.label = label ? OCStringCreateCopy(label) : STR("");
    OCRelease(si->_super.description);
    si->_super.description = description ? OCStringCreateCopy(description) : STR("");
    OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCTypeDeepCopy(metadata) : OCDictionaryCreateMutable(0);
    // 7) SI-specific fields (deep copies)
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
    si->scaling = scaling;
    // 8) Coordinates array (deep copy)
    OCRelease(dim->coordinates);
    dim->coordinates = OCArrayCreateMutableCopy(scalarCoords);
    if (!dim->coordinates) {
        err = STR("SIMonotonicDimensionCreate: failed to copy coordinates array");
        goto FailWithDim;
    }
    // 9) Reciprocal
    if (reciprocal) {
        if (!impl_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal, &err)) {
            goto FailWithDim;
        }
        dim->reciprocal = OCTypeDeepCopy(reciprocal);
        if (!dim->reciprocal) {
            err = STR("SIMonotonicDimensionCreate: failed to copy reciprocal");
            goto FailWithDim;
        }
    } else {
        // build default reciprocal dimension
        SIDimensionalityRef inverseDim = SIDimensionalityByRaisingToPowerWithoutReducing(baseDim, -1);
        if (!inverseDim) goto FailWithDim;
        OCArrayRef qnList = SIDimensionalityCreateArrayOfQuantities(inverseDim);
        OCStringRef inverseQuantityName = (OCStringRef)OCArrayGetValueAtIndex(qnList, 0);
        dim->reciprocal = SIDimensionCreate(
            NULL,                   // label
            NULL,                   // description
            NULL,                   // metadata
            inverseQuantityName,    // quantityName
            NULL,                   // offset
            NULL,                   // origin
            NULL,                   // period
            kDimensionScalingNone,  // scaling
            &err);
        OCRelease(qnList);
        OCRelease(inverseDim);
        if (!dim->reciprocal) goto FailWithDim;
    }
    OCRelease(scalarCoords);
    // Release temporary SIScalar objects created by validation if they were NULL inputs
    if (offset_was_null && offset) {
        OCRelease(offset);
    }
    if (origin_was_null && origin) {
        OCRelease(origin);
    }
    if (period_was_null && period) {
        OCRelease(period);
    }
    return dim;
FailWithDim:
    OCRelease(dim);
FailWithCoords:
    OCRelease(scalarCoords);
    // Release temporary SIScalar objects created by validation if they were NULL inputs
    if (offset_was_null && offset) {
        OCRelease(offset);
    }
    if (origin_was_null && origin) {
        OCRelease(origin);
    }
    if (period_was_null && period) {
        OCRelease(period);
    }
    goto Fail;
Fail:
    if (outError)
        *outError = err;
    else
        OCRelease(err);
    return NULL;
}
SIMonotonicDimensionRef SIMonotonicDimensionCreateMinimal(
    OCStringRef quantityName,
    OCArrayRef coordinates,
    SIDimensionRef reciprocal,
    OCStringRef *outError) {
    return SIMonotonicDimensionCreate(
        NULL,                   // label
        NULL,                   // description
        NULL,                   // metadata
        quantityName,           // quantityName
        NULL,                   // offset (will be defaulted)
        NULL,                   // origin (will be defaulted)
        NULL,                   // period (will be defaulted)
        kDimensionScalingNone,  // scaling
        coordinates,            // coordinates
        reciprocal,             // reciprocal
        outError);              // outError
}
OCDictionaryRef SIMonotonicDimensionDictionaryCreateFromJSON(cJSON *json,
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
    {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR("type"), tmp);
        OCRelease(tmp);
    }
    // --- label / description / metadata (optional) ---
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionLabelKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionLabelKey), tmp);
        OCRelease(tmp);
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDimensionDescriptionKey), tmp);
        OCRelease(tmp);
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kDimensionApplicationKey);
    if (cJSON_IsObject(item)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(item, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionApplicationKey), md);
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
    if (cJSON_IsString(item) && item->valuestring[0] != '\0') {
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
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionOffsetKey), tmp);
        OCRelease(tmp);
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
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionOriginKey), tmp);
        OCRelease(tmp);
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kSIDimensionPeriodKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSIDimensionPeriodKey), tmp);
        OCRelease(tmp);
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
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionApplicationKey));
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
#pragma endregion
// ============================================================================
#pragma region SILinearDimension
// ============================================================================
#define kSILinearDimensionCountKey "count"
#define kSILinearDimensionIncrementKey "increment"
#define kSILinearDimensionFFTKey "complex_fft"
bool impl_SILinearDimensionEqual(const void *a, const void *b) {
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
void impl_SILinearDimensionFinalize(const void *obj) {
    if (!obj) return;
    SILinearDimensionRef dim = (SILinearDimensionRef)obj;
    // finalize SI superclass
    impl_SIDimensionFinalize((const void *)&dim->_super);
    // clean up subclass fields
    OCRelease(dim->increment);
    OCRelease(dim->reciprocal);
}
OCStringRef impl_SILinearDimensionCopyFormattingDesc(OCTypeRef cf) {
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
    
    // Add type discriminator
    cJSON_AddStringToObject(json, "type", "linear");
    
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
void *impl_SILinearDimensionDeepCopy(const void *obj) {
    if (!obj) return NULL;
    OCDictionaryRef dict = SILinearDimensionCopyAsDictionary((SILinearDimensionRef)obj);
    if (!dict) return NULL;
    SILinearDimensionRef copy = SILinearDimensionCreateFromDictionary(dict, NULL);
    OCRelease(dict);
    return copy;
}
SILinearDimensionRef SILinearDimensionAllocate(void) {
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
    dimensionScaling scaling,
    OCIndex count,
    SIScalarRef increment,
    bool fft,
    SIDimensionRef reciprocal,
    OCStringRef *outError) {
    /*
     * NULL INPUT HANDLING:
     *
     * REQUIRED (must not be NULL):
     * - increment: Must be a real-valued SIScalar. Used to derive base unit and dimensionality.
     * - count: Primitive type, cannot be NULL (must be ≥2).
     *
     * OPTIONAL (can be NULL, function provides defaults):
     * - label: NULL → no label set
     * - description: NULL → no description set
     * - metadata: NULL → no metadata set
     * - quantityName: NULL → derived from increment's dimensionality
     * - offset: NULL → creates zero scalar in increment's unit via impl_validateOrDefaultScalar
     * - origin: NULL → creates zero scalar in increment's unit via impl_validateOrDefaultScalar
     * - period: NULL → creates zero scalar in increment's unit via impl_validateOrDefaultScalar
     * - reciprocal: NULL → no reciprocal dimension set
     * - outError: NULL → errors are silently released instead of returned
     *
     * PRIMITIVE TYPES (cannot be NULL):
     * - periodic: boolean primitive
     * - scaling: enum primitive
     * - fft: boolean primitive
     *
     * KEY BEHAVIOR:
     * The function tracks which scalar parameters were originally NULL so it can properly
     * clean up temporary scalars created by impl_validateOrDefaultScalar. After validation,
     * offset, origin, and period are guaranteed to be non-NULL and are deep-copied into
     * the dimension structure. The temporary scalars are then released if they were
     * created by this function (not provided by the caller).
     */
    if (outError) *outError = NULL;
    OCStringRef err = NULL;
    // Track which parameters were originally NULL for cleanup
    bool offset_was_null = (offset == NULL);
    bool origin_was_null = (origin == NULL);
    bool period_was_null = (period == NULL);
    // 1) Validate count & increment
    if (count < 2) {
        err = STR("SILinearDimensionCreate: need ≥2 points");
        goto Fail;
    }
    if (!increment) {
        err = STR("SILinearDimensionCreate: increment is NULL");
        goto Fail;
    }
    OCTypeID incrementTypeID = OCGetTypeID((OCTypeRef)increment);
    OCTypeID expectedTypeID = SIScalarGetTypeID();
    if (incrementTypeID != expectedTypeID) {
        err = OCStringCreateWithFormat(STR("SILinearDimensionCreate: increment has wrong type ID (got %lu, expected %lu)"),
                                       (unsigned long)incrementTypeID, (unsigned long)expectedTypeID);
        goto Fail;
    }
    if (SIQuantityIsComplexType((SIQuantityRef)increment)) {
        err = STR("SILinearDimensionCreate: increment must be real-valued, not complex");
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
    } else if (SIDimensionalityForQuantity(quantityName, &err) == NULL) {
        err = STR("SIMonotonicDimensionCreate: invalid quantityName");
        goto Fail;
    }
    // 4) Validate or default offset/origin/period
    if (!impl_validateOrDefaultScalar("offset", &offset, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("origin", &origin, baseUnit, baseDim, &err) ||
        !impl_validateOrDefaultScalar("period", &period, baseUnit, baseDim, &err)) {
        goto Fail;
    }
    if (period_was_null) SIScalarSetDoubleValue((SIMutableScalarRef)period, INFINITY);
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
    si->_super.label = label ? OCStringCreateCopy(label) : STR("");
    OCRelease(si->_super.description);
    si->_super.description = description ? OCStringCreateCopy(description) : STR("");
    OCRelease(si->_super.metadata);
    si->_super.metadata = metadata ? OCTypeDeepCopy(metadata) : OCDictionaryCreateMutable(0);
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
        if (!impl_SIDimensionIsReciprocalOf((SIDimensionRef)dim, reciprocal, &err)) {
            goto FailWithDim;
        }
        dim->reciprocal = (SIDimensionRef)OCTypeDeepCopy(reciprocal);
        if (!dim->reciprocal) {
            err = STR("SILinearDimensionCreate: failed to copy reciprocal dimension");
            goto FailWithDim;
        }
    } else {
        // build default reciprocal dimension
        SIDimensionalityRef inverseDim = SIDimensionalityByRaisingToPowerWithoutReducing(baseDim, -1);
        if (!inverseDim) goto FailWithDim;
        OCArrayRef qnList = SIDimensionalityCreateArrayOfQuantities(inverseDim);
        OCStringRef inverseQuantityName = (OCStringRef)OCArrayGetValueAtIndex(qnList, 0);
        dim->reciprocal = SIDimensionCreate(
            NULL,                   // label
            NULL,                   // description
            NULL,                   // metadata
            inverseQuantityName,    // quantityName
            NULL,                   // offset
            NULL,                   // origin
            NULL,                   // period
            kDimensionScalingNone,  // scaling
            &err);
        OCRelease(qnList);
        OCRelease(inverseDim);
        if (!dim->reciprocal) goto FailWithDim;
    }
    // 9) Compute reciprocalIncrement
    // [Reciprocal increment logic not shown]
    // 10) Release temporary SIScalar objects created by validation if they were NULL inputs
    // These were created by impl_validateOrDefaultScalar and need to be released
    // since we copied them into the dimension structure
    if (offset_was_null && offset) {
        OCRelease(offset);  // Release the temporary one created by validation
    }
    if (origin_was_null && origin) {
        OCRelease(origin);  // Release the temporary one created by validation
    }
    if (period && period_was_null) {
        OCRelease(period);  // Release the temporary one created by validation
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
SILinearDimensionRef SILinearDimensionCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError) {
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
    OCStringRef label = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionLabelKey));
    OCStringRef desc = (OCStringRef)OCDictionaryGetValue(dict, STR(kDimensionDescriptionKey));
    OCDictionaryRef metadata = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDimensionApplicationKey));
    OCStringRef qtyName = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionQuantityNameKey));
    OCStringRef offStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOffsetKey));
    OCStringRef origStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionOriginKey));
    OCStringRef periodStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSIDimensionPeriodKey));
    OCNumberRef num = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSIDimensionScalingKey));
    OCNumberRef cntNum = (OCNumberRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionCountKey));
    OCStringRef incStr = (OCStringRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionIncrementKey));
    OCBooleanRef fftB = (OCBooleanRef)OCDictionaryGetValue(dict, STR(kSILinearDimensionFFTKey));
    OCDictionaryRef recDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kSIDimensionReciprocalKey));
    // 3) parse & validate primitives
    // Scalars
    SIScalarRef offset = offStr ? SIScalarCreateFromExpression(offStr, outError) : NULL;
    SIScalarRef origin = origStr ? SIScalarCreateFromExpression(origStr, outError) : NULL;
    SIScalarRef period = periodStr ? SIScalarCreateFromExpression(periodStr, outError) : NULL;
    if (outError && *outError) goto Cleanup;
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
        scaling,
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
#pragma endregion
// ============================================================================
#pragma region Helper macros for one-liner copies
    // ============================================================================
#define COPY_STR_FIELD(key)                                                                 \
    do {                                                                                    \
        if ((item = cJSON_GetObjectItemCaseSensitive(json, key)) && cJSON_IsString(item)) { \
            OCStringRef tmp = OCStringCreateWithCString(item->valuestring);                 \
            OCDictionarySetValue(dict, STR(key), tmp);                                      \
            OCRelease(tmp);                                                                 \
        }                                                                                   \
    } while (0)
#define COPY_NUM_FIELD(key)                                                                 \
    do {                                                                                    \
        if ((item = cJSON_GetObjectItemCaseSensitive(json, key)) && cJSON_IsNumber(item)) { \
            OCNumberRef tmp = OCNumberCreateWithInt(item->valueint);                        \
            OCDictionarySetValue(dict, STR(key), tmp);                                      \
            OCRelease(tmp);                                                                 \
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
    if ((item = cJSON_GetObjectItemCaseSensitive(json, kDimensionApplicationKey)) && cJSON_IsObject(item)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(item, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDimensionApplicationKey), md);
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

// ============================================================================
#pragma region Convenience Functions (converted from static inline)
// ============================================================================

SIDimensionRef SIDimensionCreateWithQuantity(OCStringRef quantityName, OCStringRef *outError) {
    return SIDimensionCreate(
        NULL,                   // label
        NULL,                   // description
        NULL,                   // metadata
        quantityName,           // quantityName
        NULL,                   // offset
        NULL,                   // origin
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        outError                // outError
    );
}

SILinearDimensionRef SILinearDimensionCreateMinimal(
    OCStringRef quantityName,
    OCIndex count,
    SIScalarRef increment,
    SIDimensionRef reciprocal,
    OCStringRef *outError) {
    return SILinearDimensionCreate(
        NULL,                   // label
        NULL,                   // description
        NULL,                   // metadata
        quantityName,           // quantityName
        NULL,                   // offset
        NULL,                   // origin
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        count,                  // count
        increment,              // increment
        false,                  // fft
        reciprocal,             // reciprocal
        outError                // outError
    );
}

#pragma endregion
#pragma endregion
#ifdef __cplusplus
}
#endif
