#include "RMNLibrary.h"

// Dictionary keys
#define kGeoCoordLatitudeKey        "latitude"
#define kGeoCoordLongitudeKey       "longitude"
#define kGeoCoordAltitudeKey        "altitude"
#define kGeoCoordMetaDataKey        "application"

// Type registration
static OCTypeID kGeographicCoordinateID = kOCNotATypeID;
OCTypeID GeographicCoordinateGetTypeID(void) {
    if (kGeographicCoordinateID == kOCNotATypeID) {
        kGeographicCoordinateID = OCRegisterType("GeographicCoordinate");
    }
    return kGeographicCoordinateID;
}

// ivar struct
struct impl_GeographicCoordinate {
    OCBase base;
    SIScalarRef latitude;
    SIScalarRef longitude;
    SIScalarRef altitude;    // optional
    OCDictionaryRef metadata;
};

// Finalize & Equal
static void impl_GeographicCoordinateFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_GeographicCoordinate *gc = (struct impl_GeographicCoordinate*)ptr;
    OCRelease(gc->latitude);
    OCRelease(gc->longitude);
    OCRelease(gc->altitude);
    OCRelease(gc->metadata);
}

static bool impl_GeographicCoordinateEqual(const void *a, const void *b) {
    const struct impl_GeographicCoordinate *A = a, *B = b;
    if (!A || !B) return false;
    if (A == B) return true;
    if (!OCTypeEqual(A->latitude,  B->latitude))  return false;
    if (!OCTypeEqual(A->longitude, B->longitude)) return false;
    if (A->altitude || B->altitude) {
        if (!(A->altitude && B->altitude && OCTypeEqual(A->altitude, B->altitude)))
            return false;
    }
    if (!OCTypeEqual(A->metadata, B->metadata)) return false;
    return true;
}

// Copy-formatting description
static OCStringRef impl_GeographicCoordinateCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_GeographicCoordinate *gc = (const void*)cf;
    return OCStringCreateWithFormat(
        STR("<GeographicCoordinate lat=%@, lon=%@, alt=%@>"),
        gc->latitude,
        gc->longitude,
        gc->altitude ? gc->altitude : (SIScalarRef)STR("null")
    );
}

// JSON serialization
static cJSON *impl_GeographicCoordinateCreateJSON(const void *obj) {
    if (!obj) return cJSON_CreateNull();
    GeographicCoordinateRef gc = (GeographicCoordinateRef)obj;
    OCDictionaryRef dict = GeographicCoordinateCopyAsDictionary(gc);
    if (!dict) return cJSON_CreateNull();
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    OCRelease(dict);
    return json;
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
        impl_GeographicCoordinateCreateJSON,
        impl_GeographicCoordinateDeepCopy,
        impl_GeographicCoordinateDeepCopy
    );
}

static void impl_InitGeographicCoordinateFields(GeographicCoordinateRef gc) {
    gc->latitude  = NULL;
    gc->longitude = NULL;
    gc->altitude  = NULL;
    gc->metadata  = OCDictionaryCreateMutable(0);
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
    OCDictionaryRef metadata
) {
    if (!latitude || !longitude) return NULL;
    struct impl_GeographicCoordinate *gc = GeographicCoordinateAllocate();
    if (!gc) return NULL;
    impl_InitGeographicCoordinateFields((GeographicCoordinateRef)gc);

    // assign
    gc->latitude  = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)latitude);
    gc->longitude = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)longitude);
    if (altitude) {
        gc->altitude = (SIScalarRef)OCTypeDeepCopy((OCTypeRef)altitude);
    }
    OCRelease(gc->metadata);
    gc->metadata = metadata
        ? (OCDictionaryRef)OCTypeDeepCopyMutable(metadata)
        : OCDictionaryCreateMutable(0);

    if (!validateGeographicCoordinate((GeographicCoordinateRef)gc, NULL)) {
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

    // metadata
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kGeoCoordMetaDataKey));

    // required: lat/lon
    if (!lat || !lon) {
        if (outError && !*outError) {
            *outError = STR("GeographicCoordinateCreateFromDictionary: missing latitude or longitude");
        }
        goto fail;
    }

    GeographicCoordinateRef gc = GeographicCoordinateCreate(lat, lon, alt, md);
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
    if (gc->metadata) {
        OCDictionaryRef copyMd = (OCDictionaryRef)OCTypeDeepCopyMutable(gc->metadata);
        OCDictionarySetValue(dict, STR(kGeoCoordMetaDataKey), copyMd);
        OCRelease(copyMd);
    }

    return dict;
}

static OCDictionaryRef GeographicCoordinateDictionaryCreateFromJSON(cJSON *json,
                                                                    OCStringRef *outError)
{
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected a JSON object for GeographicCoordinate");
        return NULL;
    }

    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        if (outError) *outError = STR("Failed to create dictionary");
        return NULL;
    }

    cJSON *entry;

    // latitude
    entry = cJSON_GetObjectItemCaseSensitive(json, kGeoCoordLatitudeKey);
    if (cJSON_IsString(entry)) {
        OCStringRef s = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kGeoCoordLatitudeKey), s);
        OCRelease(s);
    } else if (cJSON_IsNumber(entry)) {
        // fallback if JSON has numbers
        OCStringRef s = OCStringCreateWithFormat(STR("%g °"), entry->valuedouble);
        OCDictionarySetValue(dict, STR(kGeoCoordLatitudeKey), s);
        OCRelease(s);
    }

    // longitude
    entry = cJSON_GetObjectItemCaseSensitive(json, kGeoCoordLongitudeKey);
    if (cJSON_IsString(entry)) {
        OCStringRef s = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kGeoCoordLongitudeKey), s);
        OCRelease(s);
    } else if (cJSON_IsNumber(entry)) {
        OCStringRef s = OCStringCreateWithFormat(STR("%g °"), entry->valuedouble);
        OCDictionarySetValue(dict, STR(kGeoCoordLongitudeKey), s);
        OCRelease(s);
    }

    // altitude (optional)
    entry = cJSON_GetObjectItemCaseSensitive(json, kGeoCoordAltitudeKey);
    if (cJSON_IsString(entry)) {
        OCStringRef s = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kGeoCoordAltitudeKey), s);
        OCRelease(s);
    } else if (cJSON_IsNumber(entry)) {
        OCStringRef s = OCStringCreateWithFormat(STR("%g m"), entry->valuedouble);
        OCDictionarySetValue(dict, STR(kGeoCoordAltitudeKey), s);
        OCRelease(s);
    }

    // nested metadata (optional)
    entry = cJSON_GetObjectItemCaseSensitive(json, kGeoCoordMetaDataKey);
    if (entry && cJSON_IsObject(entry)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(entry, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kGeoCoordMetaDataKey), md);
        OCRelease(md);
    }

    return dict;
}

GeographicCoordinateRef GeographicCoordinateCreateFromJSON(cJSON *json, OCStringRef *outError)
{
    OCDictionaryRef dict = GeographicCoordinateDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;

    GeographicCoordinateRef gc =
        GeographicCoordinateCreateFromDictionary(dict, outError);

    OCRelease(dict);
    return gc;
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
OCDictionaryRef GeographicCoordinateGetMetaData(GeographicCoordinateRef gc) {
    return gc ? gc->metadata : NULL;
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
bool GeographicCoordinateSetMetaData(GeographicCoordinateRef gc, OCDictionaryRef metadata) {
    if (!gc) return false;
    OCRelease(gc->metadata);
    gc->metadata = metadata
        ? (OCDictionaryRef)OCTypeDeepCopyMutable(metadata)
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
    char   *data;
    size_t  len;
} curl_buffer_t;

static size_t _curl_write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;
    curl_buffer_t *buf = (curl_buffer_t*)userdata;
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
                                                      OCStringRef   *outError)
{
    if (outError) *outError = NULL;

    CURL *curl = curl_easy_init();
    if (!curl) {
        if (outError) *outError = STR("Failed to init HTTP client");
        return NULL;
    }

    curl_buffer_t buf = { .data = NULL, .len = 0 };
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
                                              SIUnitFindWithUnderivedSymbol(STR("deg")));
    SIScalarRef lon = SIScalarCreateWithDouble(jlon->valuedouble,
                                              SIUnitFindWithUnderivedSymbol(STR("deg")));
    SIScalarRef alt = NULL;
    if (cJSON_IsNumber(jalt)) {
        alt = SIScalarCreateWithDouble(jalt->valuedouble,
                                   SIUnitFindWithUnderivedSymbol(STR("m")));
    }

    GeographicCoordinateRef gc = GeographicCoordinateCreate(lat, lon, alt, metadata);

    OCRelease(lat);
    OCRelease(lon);
    if (alt) OCRelease(alt);
    cJSON_Delete(root);
    return gc;
}