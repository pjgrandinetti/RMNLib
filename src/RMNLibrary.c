#include "RMNLibrary.h"
cJSON *OCMetadataCopyJSON(OCDictionaryRef dict) {
    if (!dict) return cJSON_CreateNull();
    cJSON *json = cJSON_CreateObject();
    OCArrayRef keys = OCDictionaryCreateArrayWithAllKeys(dict);
    if (!keys) return json;
    for (OCIndex i = 0; i < OCArrayGetCount(keys); ++i) {
        OCStringRef key = (OCStringRef)OCArrayGetValueAtIndex(keys, i);
        OCTypeRef val = OCDictionaryGetValue(dict, key);
        if (!key || !val) continue;
        cJSON *entry = cJSON_CreateObject();
        OCTypeID tid = OCGetTypeID(val);
        if (tid == OCStringGetTypeID()) {
            cJSON_AddStringToObject(entry, "type", "string");
            cJSON_AddStringToObject(entry, "value", OCStringGetCString((OCStringRef)val));
        } else if (tid == OCNumberGetTypeID()) {
            double f = 0.0;
            OCNumberTryGetDouble((OCNumberRef)val, &f);
            cJSON_AddStringToObject(entry, "type", "float64");
            cJSON_AddNumberToObject(entry, "value", f);
        } else if (tid == OCBooleanGetTypeID()) {
            cJSON_AddStringToObject(entry, "type", "bool");
            cJSON_AddBoolToObject(entry, "value", OCBooleanGetValue((OCBooleanRef)val));
        } else if (tid == OCDataGetTypeID()) {
            OCStringRef b64 = OCDataCreateBase64EncodedString((OCDataRef)val, 0);
            cJSON_AddStringToObject(entry, "type", "binary");
            cJSON_AddStringToObject(entry, "value", OCStringGetCString(b64));
            OCRelease(b64);
        } else if (tid == SIScalarGetTypeID()) {
            OCStringRef s = SIScalarCreateStringValue((SIScalarRef)val);
            cJSON_AddStringToObject(entry, "type", "scalar");
            cJSON_AddStringToObject(entry, "value", OCStringGetCString(s));
            OCRelease(s);
        } else if (tid == SIUnitGetTypeID()) {
            OCStringRef s = SIUnitCopySymbol((SIUnitRef)val);
            cJSON_AddStringToObject(entry, "type", "unit");
            cJSON_AddStringToObject(entry, "value", OCStringGetCString(s));
            OCRelease(s);
        } else if (tid == OCDictionaryGetTypeID()) {
            cJSON *nested = OCMetadataCopyJSON((OCDictionaryRef)val);
            cJSON_AddStringToObject(entry, "type", "dictionary");
            cJSON_AddItemToObject(entry, "value", nested);
        } else if (tid == OCArrayGetTypeID()) {
            cJSON *arrJSON = cJSON_CreateArray();
            OCArrayRef array = (OCArrayRef)val;
            for (OCIndex j = 0; j < OCArrayGetCount(array); ++j) {
                OCTypeRef elem = OCArrayGetValueAtIndex(array, j);
                OCMutableDictionaryRef singleton = OCDictionaryCreateMutable(0);
                OCDictionarySetValue(singleton, STR("elem"), elem);
                cJSON *wrapped = OCMetadataCopyJSON(singleton);
                OCRelease(singleton);
                if (wrapped) {
                    cJSON *e = cJSON_GetObjectItem(wrapped, "elem");
                    if (e) cJSON_AddItemToArray(arrJSON, cJSON_Duplicate(e, 1));
                    cJSON_Delete(wrapped);
                }
            }
            cJSON_AddStringToObject(entry, "type", "array");
            cJSON_AddItemToObject(entry, "value", arrJSON);
        }
        cJSON_AddItemToObject(json, OCStringGetCString(key), entry);
    }
    OCRelease(keys);
    return json;
}
OCDictionaryRef OCMetadataCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected JSON object for metadata");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, json) {
        const char *key = entry->string;
        if (!cJSON_IsObject(entry)) continue;
        cJSON *type = cJSON_GetObjectItem(entry, "type");
        cJSON *value = cJSON_GetObjectItem(entry, "value");
        if (!cJSON_IsString(type) || !value) continue;
        OCStringRef k = OCStringCreateWithCString(key);
        OCTypeRef v = NULL;
        if (strcmp(type->valuestring, "string") == 0 && cJSON_IsString(value)) {
            v = (OCTypeRef)OCStringCreateWithCString(value->valuestring);
        } else if (strcmp(type->valuestring, "float64") == 0 && cJSON_IsNumber(value)) {
            v = (OCTypeRef)OCNumberCreateWithDouble(value->valuedouble);
        } else if (strcmp(type->valuestring, "bool") == 0 && cJSON_IsBool(value)) {
            v = (OCTypeRef)OCBooleanGetWithBool(cJSON_IsTrue(value));
        } else if (strcmp(type->valuestring, "binary") == 0 && cJSON_IsString(value)) {
            OCStringRef b64 = OCStringCreateWithCString(value->valuestring);
            v = (OCTypeRef)OCDataCreateFromBase64EncodedString(b64);
            OCRelease(b64);
        } else if (strcmp(type->valuestring, "scalar") == 0 && cJSON_IsString(value)) {
            OCStringRef s = OCStringCreateWithCString(value->valuestring);
            v = (OCTypeRef)SIScalarCreateFromExpression(s, outError);
            OCRelease(s);
            if (!v) {
                OCRelease(k);
                OCRelease(dict);
                return NULL;
            }
        } else if (strcmp(type->valuestring, "unit") == 0 && cJSON_IsString(value)) {
            OCStringRef s = OCStringCreateWithCString(value->valuestring);
            SIScalarRef tmpScalar = SIScalarCreateFromExpression(s, outError);
            OCRelease(s);
            if (!tmpScalar) {
                OCRelease(k);
                OCRelease(dict);
                return NULL;
            }
            v = (OCTypeRef)SIQuantityGetUnit((SIQuantityRef)tmpScalar);
            v = (OCTypeRef)OCRetain(v);
            OCRelease(tmpScalar);
        } else if (strcmp(type->valuestring, "dictionary") == 0 && cJSON_IsObject(value)) {
            v = (OCTypeRef)OCMetadataCreateFromJSON(value, outError);
            if (!v) {
                OCRelease(k);
                OCRelease(dict);
                return NULL;
            }
        } else if (strcmp(type->valuestring, "array") == 0 && cJSON_IsArray(value)) {
            OCMutableArrayRef arr = OCArrayCreateMutable(cJSON_GetArraySize(value), &kOCTypeArrayCallBacks);
            cJSON *elem = NULL;
            cJSON_ArrayForEach(elem, value) {
                cJSON *wrap = cJSON_CreateObject();
                cJSON_AddItemToObject(wrap, "elem", cJSON_Duplicate(elem, 1));
                OCDictionaryRef tmp = OCMetadataCreateFromJSON(wrap, outError);
                cJSON_Delete(wrap);
                if (!tmp) {
                    OCRelease(arr);
                    OCRelease(k);
                    OCRelease(dict);
                    return NULL;
                }
                OCTypeRef item = OCDictionaryGetValue(tmp, STR("elem"));
                if (item) OCArrayAppendValue(arr, item);
                OCRelease(tmp);
            }
            v = (OCTypeRef)arr;
        }
        if (v) {
            OCDictionarySetValue(dict, k, v);
            OCRelease(v);
            OCRelease(k);
        } else {
            // no valid branch matched
            OCRelease(k);
            OCRelease(dict);
            return NULL;
        }
    }
    return dict;
}
void RMNLibTypesShutdown(void) {
    fprintf(stderr, "Cleaning up SITypes...\n");
    SITypesShutdown();
    fprintf(stderr, "Cleaning up OCTypes...\n");
    OCTypesShutdown();
}
// If you want automatic teardown when the library is unloaded:
// __attribute__((destructor(100)))
// static void __RMNLibCleanup(void) {
//     RMNLibTypesShutdown();
// }
