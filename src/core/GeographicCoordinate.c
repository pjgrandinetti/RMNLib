#include "../RMNLibrary.h"
// Dictionary keys
#define kGeoCoordLatitudeKey "latitude"
#define kGeoCoordLongitudeKey "longitude"
#define kGeoCoordAltitudeKey "altitude"
#define kGeoCoordApplicationKey "application"
// Type registration
static OCTypeID kGeographicCoordinateID = kOCNotATypeID;
OCTypeID GeographicCoordinateGetTypeID(void) {
    if (kGeographicCoordinateID == kOCNotATypeID) {
        kGeographicCoordinateID = OCRegisterType("GeographicCoordinate", (OCTypeRef (*)(cJSON *, OCStringRef *))GeographicCoordinateCreateFromJSON);
    }
    return kGeographicCoordinateID;
}
// ivar struct
struct impl_GeographicCoordinate {
    OCBase base;
    SIScalarRef latitude;
    SIScalarRef longitude;
    SIScalarRef altitude;  // optional
    OCMutableDictionaryRef application;
};
// Finalize & Equal
static void impl_GeographicCoordinateFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_GeographicCoordinate *gc = (struct impl_GeographicCoordinate *)ptr;
    OCRelease(gc->latitude);
    OCRelease(gc->longitude);
    OCRelease(gc->altitude);
    OCRelease(gc->application);
}
static bool impl_GeographicCoordinateEqual(const void *a, const void *b) {
    const struct impl_GeographicCoordinate *A = a, *B = b;
    if (!A || !B) return false;
    if (A == B) return true;
    if (!OCTypeEqual(A->latitude, B->latitude)) return false;
    if (!OCTypeEqual(A->longitude, B->longitude)) return false;
    if (A->altitude || B->altitude) {
        if (!(A->altitude && B->altitude && OCTypeEqual(A->altitude, B->altitude)))
            return false;
    }
    if (!OCTypeEqual(A->application, B->application)) return false;
    return true;
}
// Copy-formatting description
static OCStringRef impl_GeographicCoordinateCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_GeographicCoordinate *gc = (const void *)cf;
    return OCStringCreateWithFormat(
        STR("<GeographicCoordinate lat=%@, lon=%@, alt=%@>"),
        gc->latitude,
        gc->longitude,
        gc->altitude ? gc->altitude : (SIScalarRef)STR("null"));
}
// JSON serialization
static cJSON *impl_GeographicCoordinateCopyJSON(const void *obj, bool typed, OCStringRef *outError) {
    return GeographicCoordinateCopyAsJSON((GeographicCoordinateRef)obj, typed, outError);
}
// Deep-copy
static void *impl_GeographicCoordinateDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    GeographicCoordinateRef src = (GeographicCoordinateRef)ptr;
    OCDictionaryRef dict = GeographicCoordinateCopyAsDictionary(src);
    if (!dict) return NULL;
    OCStringRef err = NULL;
    GeographicCoordinateRef copy = GeographicCoordinateCreateFromDictionary(dict, &err);
    OCRelease(dict);
    return copy;
}
// Allocator + init
static struct impl_GeographicCoordinate *GeographicCoordinateAllocate(void) {
    return OCTypeAlloc(
        struct impl_GeographicCoordinate,
        GeographicCoordinateGetTypeID(),
        impl_GeographicCoordinateFinalize,
        impl_GeographicCoordinateEqual,
        impl_GeographicCoordinateCopyFormattingDesc,
        impl_GeographicCoordinateCopyJSON,
        impl_GeographicCoordinateDeepCopy,
        impl_GeographicCoordinateDeepCopy);
}
static void impl_InitGeographicCoordinateFields(GeographicCoordinateRef gc) {
    gc->latitude = NULL;
    gc->longitude = NULL;
    gc->altitude = NULL;
    gc->application = OCDictionaryCreateMutable(0);
}
// Validation
static bool validateGeographicCoordinate(GeographicCoordinateRef gc, OCStringRef *outError) {
    if (!gc) return true;
    if (!gc->latitude || !gc->longitude) {
        if (outError) *outError = STR("GeographicCoordinate validation error: latitude and longitude are required");
        return false;
    }
    // Optionally: range-check lat/lon
    return true;
}
// Public creators
GeographicCoordinateRef GeographicCoordinateCreate(
    SIScalarRef latitude,
    SIScalarRef longitude,
    SIScalarRef altitude,
    OCDictionaryRef metadata,
    OCStringRef *outError) {
    if (outError) *outError = NULL;

    if (!latitude || !longitude) {
        if (outError) *outError = STR("GeographicCoordinateCreate: latitude and longitude are required");
        return NULL;
    }

    struct impl_GeographicCoordinate *gc = GeographicCoordinateAllocate();
    if (!gc) {
        if (outError) *outError = STR("GeographicCoordinateCreate: allocation failed");
        return NULL;
    }

    impl_InitGeographicCoordinateFields((GeographicCoordinateRef)gc);

    // assign
    gc->latitude = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)latitude);
    if (!gc->latitude) {
        if (outError) *outError = STR("GeographicCoordinateCreate: failed to copy latitude");
        OCRelease(gc);
        return NULL;
    }

    gc->longitude = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)longitude);
    if (!gc->longitude) {
        if (outError) *outError = STR("GeographicCoordinateCreate: failed to copy longitude");
        OCRelease(gc);
        return NULL;
    }

    if (altitude) {
        gc->altitude = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)altitude);
        if (!gc->altitude) {
            if (outError) *outError = STR("GeographicCoordinateCreate: failed to copy altitude");
            OCRelease(gc);
            return NULL;
        }
    }

    OCRelease(gc->application);
    gc->application = metadata
                       ? (OCMutableDictionaryRef)OCTypeDeepCopyMutable(metadata)
                       : OCDictionaryCreateMutable(0);
    if (!gc->application) {
        if (outError) *outError = STR("GeographicCoordinateCreate: failed to create application metadata");
        OCRelease(gc);
        return NULL;
    }

    if (!validateGeographicCoordinate((GeographicCoordinateRef)gc, outError)) {
        OCRelease(gc);
        return NULL;
    }

    return (GeographicCoordinateRef)gc;
}
GeographicCoordinateRef GeographicCoordinateCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("GeographicCoordinateCreateFromDictionary: input dictionary is NULL");
        return NULL;
    }
    SIScalarRef lat = NULL;
    SIScalarRef lon = NULL;
    SIScalarRef alt = NULL;
    // latitude
    OCStringRef latStr = OCDictionaryGetValue(dict, STR(kGeoCoordLatitudeKey));
    if (latStr && OCGetTypeID(latStr) == OCStringGetTypeID()) {
        lat = SIScalarCreateFromExpression(latStr, outError);
        if (!lat) goto fail;
    }
    // longitude
    OCStringRef lonStr = OCDictionaryGetValue(dict, STR(kGeoCoordLongitudeKey));
    if (lonStr && OCGetTypeID(lonStr) == OCStringGetTypeID()) {
        lon = SIScalarCreateFromExpression(lonStr, outError);
        if (!lon) goto fail;
    }
    // altitude (optional)
    OCStringRef altStr = OCDictionaryGetValue(dict, STR(kGeoCoordAltitudeKey));
    if (altStr && OCGetTypeID(altStr) == OCStringGetTypeID()) {
        alt = SIScalarCreateFromExpression(altStr, outError);
        if (!alt) goto fail;
    }
    // metadata (optional)
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kGeoCoordApplicationKey));
    // required: lat/lon
    if (!lat || !lon) {
        if (outError && !*outError) {
            *outError = STR("GeographicCoordinateCreateFromDictionary: missing latitude or longitude");
        }
        goto fail;
    }
    GeographicCoordinateRef gc = GeographicCoordinateCreate(lat, lon, alt, md, outError);
    OCRelease(lat);
    OCRelease(lon);
    OCRelease(alt);
    return gc;
fail:
    OCRelease(lat);
    OCRelease(lon);
    OCRelease(alt);
    return NULL;
}
OCDictionaryRef GeographicCoordinateCopyAsDictionary(GeographicCoordinateRef gc) {
    if (!gc) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    // latitude
    if (gc->latitude) {
        OCStringRef s = SIScalarCreateStringValue(gc->latitude);
        OCDictionarySetValue(dict, STR(kGeoCoordLatitudeKey), s);
        OCRelease(s);
    }
    // longitude
    if (gc->longitude) {
        OCStringRef s = SIScalarCreateStringValue(gc->longitude);
        OCDictionarySetValue(dict, STR(kGeoCoordLongitudeKey), s);
        OCRelease(s);
    }
    // altitude (optional)
    if (gc->altitude) {
        OCStringRef s = SIScalarCreateStringValue(gc->altitude);
        OCDictionarySetValue(dict, STR(kGeoCoordAltitudeKey), s);
        OCRelease(s);
    }
    // metadata (optional)
    if (gc->application) {
        OCDictionaryRef copyMd = (OCDictionaryRef)OCTypeDeepCopyMutable(gc->application);
        OCDictionarySetValue(dict, STR(kGeoCoordApplicationKey), copyMd);
        OCRelease(copyMd);
    }
    return dict;
}

cJSON *GeographicCoordinateCopyAsJSON(GeographicCoordinateRef gc, bool typed, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!gc) return cJSON_CreateNull();

    cJSON *json = cJSON_CreateObject();
    if (!json) return cJSON_CreateNull();

    // Add latitude
    if (gc->latitude) {
        cJSON *lat = SIScalarCopyAsJSON(gc->latitude, typed, NULL);
        if (lat) cJSON_AddItemToObject(json, kGeoCoordLatitudeKey, lat);
    }

    // Add longitude
    if (gc->longitude) {
        cJSON *lon = SIScalarCopyAsJSON(gc->longitude, typed, NULL);
        if (lon) cJSON_AddItemToObject(json, kGeoCoordLongitudeKey, lon);
    }

    // Add altitude (optional)
    if (gc->altitude) {
        cJSON *alt = SIScalarCopyAsJSON(gc->altitude, typed, NULL);
        if (alt) cJSON_AddItemToObject(json, kGeoCoordAltitudeKey, alt);
    }

    // Add application metadata (optional)
    if (gc->application) {
        // CRITICAL REQUIREMENT: application ivar in ALL RMNLib types MUST ALWAYS be encoded
        // into JSON as typed=true, NO EXCEPTIONS. Even if the rest of the JSON is untyped,
        // application must always remain typed to preserve complex nested type information.
        cJSON *app = OCTypeCopyJSON((OCTypeRef)gc->application, true, NULL);
        if (app) cJSON_AddItemToObject(json, kGeoCoordApplicationKey, app);
    }

    if (typed) {
        // Wrap in typed object format
        cJSON *wrapper = cJSON_CreateObject();
        if (wrapper) {
            cJSON_AddStringToObject(wrapper, "type", "GeographicCoordinate");
            cJSON_AddItemToObject(wrapper, "value", json);
            return wrapper;
        } else {
            cJSON_Delete(json);
            return cJSON_CreateNull();
        }
    }

    return json;
}

GeographicCoordinateRef GeographicCoordinateCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json) {
        if (outError) *outError = STR("Expected a JSON object for GeographicCoordinate");
        return NULL;
    }

    cJSON *actualJson = json;

    // Check if this is a typed JSON object (has "type" and "value" fields)
    if (cJSON_IsObject(json)) {
        cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(json, "type");
        cJSON *valueItem = cJSON_GetObjectItemCaseSensitive(json, "value");

        if (typeItem && cJSON_IsString(typeItem) &&
            strcmp(typeItem->valuestring, "GeographicCoordinate") == 0 &&
            valueItem && cJSON_IsObject(valueItem)) {
            // This is a typed JSON, use the value part
            actualJson = valueItem;
        }
    }

    if (!cJSON_IsObject(actualJson)) {
        if (outError) *outError = STR("Expected a JSON object for GeographicCoordinate");
        return NULL;
    }

    SIScalarRef lat = NULL;
    SIScalarRef lon = NULL;
    SIScalarRef alt = NULL;
    OCDictionaryRef metadata = NULL;

    cJSON *entry;

    // Parse latitude
    entry = cJSON_GetObjectItemCaseSensitive(actualJson, kGeoCoordLatitudeKey);
    if (entry) {
        lat = SIScalarCreateFromJSON(entry, outError);
        if (!lat) {
            if (outError && !*outError) *outError = STR("Failed to parse latitude");
            goto fail;
        }
    }

    // Parse longitude
    entry = cJSON_GetObjectItemCaseSensitive(actualJson, kGeoCoordLongitudeKey);
    if (entry) {
        lon = SIScalarCreateFromJSON(entry, outError);
        if (!lon) {
            if (outError && !*outError) *outError = STR("Failed to parse longitude");
            goto fail;
        }
    }

    // Parse altitude (optional)
    entry = cJSON_GetObjectItemCaseSensitive(actualJson, kGeoCoordAltitudeKey);
    if (entry) {
        alt = SIScalarCreateFromJSON(entry, outError);
        // Note: altitude parsing failure is not fatal, just skip it
    }

    // Parse application metadata (optional)
    entry = cJSON_GetObjectItemCaseSensitive(actualJson, kGeoCoordApplicationKey);
    if (entry && cJSON_IsObject(entry)) {
        // Use OCDictionary's native JSON deserialization to handle typed data properly
        // Since we always serialize application metadata with typed=true, always use OCDictionaryCreateFromJSONTyped
        metadata = OCDictionaryCreateFromJSONTyped(entry, outError);
    }

    // Create the coordinate
    GeographicCoordinateRef gc = GeographicCoordinateCreate(lat, lon, alt, metadata, outError);

    // Clean up
    OCRelease(lat);
    OCRelease(lon);
    OCRelease(alt);
    OCRelease(metadata);

    if (!gc && outError && !*outError) {
        *outError = STR("Failed to create GeographicCoordinate");
    }

    return gc;

fail:
    OCRelease(lat);
    OCRelease(lon);
    OCRelease(alt);
    OCRelease(metadata);
    return NULL;
}
// Getters
SIScalarRef GeographicCoordinateGetLatitude(GeographicCoordinateRef gc) {
    return gc ? gc->latitude : NULL;
}
SIScalarRef GeographicCoordinateGetLongitude(GeographicCoordinateRef gc) {
    return gc ? gc->longitude : NULL;
}
SIScalarRef GeographicCoordinateGetAltitude(GeographicCoordinateRef gc) {
    return gc ? gc->altitude : NULL;
}
OCDictionaryRef GeographicCoordinateGetApplicationMetaData(GeographicCoordinateRef gc) {
    return gc ? gc->application : NULL;
}
// Setters
bool GeographicCoordinateSetLatitude(GeographicCoordinateRef gc, SIScalarRef latitude) {
    if (!gc || !latitude) return false;
    SIScalarRef copy = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)latitude);
    if (!copy) return false;
    OCRelease(gc->latitude);
    gc->latitude = copy;
    return true;
}
bool GeographicCoordinateSetLongitude(GeographicCoordinateRef gc, SIScalarRef longitude) {
    if (!gc || !longitude) return false;
    SIScalarRef copy = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)longitude);
    if (!copy) return false;
    OCRelease(gc->longitude);
    gc->longitude = copy;
    return true;
}
bool GeographicCoordinateSetAltitude(GeographicCoordinateRef gc, SIScalarRef altitude) {
    if (!gc) return false;
    if (gc->altitude) OCRelease(gc->altitude);
    gc->altitude = altitude ? (SIScalarRef)OCTypeDeepCopy((OCTypeRef)altitude) : NULL;
    return true;
}
bool GeographicCoordinateSetApplicationMetaData(GeographicCoordinateRef gc, OCDictionaryRef metadata) {
    if (!gc) return false;
    OCRelease(gc->application);
    gc->application = metadata
                       ? (OCMutableDictionaryRef)OCTypeDeepCopyMutable(metadata)
                       : OCDictionaryCreateMutable(0);
    return true;
}
// Deep copy
GeographicCoordinateRef GeographicCoordinateCreateCopy(GeographicCoordinateRef gc) {
    return (GeographicCoordinateRef)OCTypeDeepCopy((OCTypeRef)gc);
}
#include <curl/curl.h>
// simple growable buffer for libcurl
typedef struct {
    char *data;
    size_t len;
} curl_buffer_t;
static size_t _curl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    curl_buffer_t *buf = (curl_buffer_t *)userdata;
    char *newdata = realloc(buf->data, buf->len + total + 1);
    if (!newdata) return 0;
    buf->data = newdata;
    memcpy(buf->data + buf->len, ptr, total);
    buf->len += total;
    buf->data[buf->len] = '\0';
    return total;
}
/**
 * @brief Returns a GeographicCoordinateRef filled via IP-lookup.
 * @param metadata arbitrary metadata dictionary (may be NULL)
 * @param outError on error, a short OCStringRef
 * @return new GeographicCoordinateRef or NULL on failure
 */
GeographicCoordinateRef GeographicCoordinateGetCurrent(OCDictionaryRef metadata,
                                                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    CURL *curl = curl_easy_init();
    if (!curl) {
        if (outError) *outError = STR("Failed to init HTTP client");
        return NULL;
    }
    curl_buffer_t buf = {.data = NULL, .len = 0};
    curl_easy_setopt(curl, CURLOPT_URL, "https://ipapi.co/json/");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        if (outError) *outError = STR("HTTP request failed");
        free(buf.data);
        return NULL;
    }
    cJSON *root = cJSON_Parse(buf.data);
    free(buf.data);
    if (!root) {
        if (outError) *outError = STR("Failed to parse JSON");
        return NULL;
    }
    cJSON *jlat = cJSON_GetObjectItemCaseSensitive(root, "latitude");
    cJSON *jlon = cJSON_GetObjectItemCaseSensitive(root, "longitude");
    cJSON *jalt = cJSON_GetObjectItemCaseSensitive(root, "altitude");
    if (!cJSON_IsNumber(jlat) || !cJSON_IsNumber(jlon)) {
        cJSON_Delete(root);
        if (outError) *outError = STR("Geolocation JSON missing lat/lon");
        return NULL;
    }
    SIScalarRef lat = SIScalarCreateWithDouble(jlat->valuedouble,
                                               SIUnitWithSymbol(STR("°")));
    SIScalarRef lon = SIScalarCreateWithDouble(jlon->valuedouble,
                                               SIUnitWithSymbol(STR("°")));
    SIScalarRef alt = NULL;
    if (cJSON_IsNumber(jalt)) {
        alt = SIScalarCreateWithDouble(jalt->valuedouble,
                                       SIUnitWithSymbol(STR("m")));
    }
    GeographicCoordinateRef gc = GeographicCoordinateCreate(lat, lon, alt, metadata, outError);
    OCRelease(lat);
    OCRelease(lon);
    if (alt) OCRelease(alt);
    cJSON_Delete(root);
    return gc;
}
