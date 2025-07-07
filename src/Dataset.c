// ============================================================================
// Dataset.c
// ============================================================================
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "RMNLibrary.h"
#if defined(_WIN32)
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define PATH_SEPARATOR '\\'
#else
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#define PATH_SEPARATOR '/'
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define kDatasetCsdmEnvelopeKey "csdm"
#define kDatasetVersionKey "version"
#define kDatasetTimestampKey "timestamp"
#define kDatasetReadOnlyKey "read_only"
#define kDatasetGeoCoordinateKey "geographic_coordinate"
#define kDatasetTagsKey "tags"
#define kDatasetDescriptionKey "description"
#define kDatasetTitleKey "title"
#define kDatasetDimensionsKey "dimensions"
#define kDatasetDimensionPrecedenceKey "dimension_precedence"
#define kDatasetDependentVariablesKey "dependent_variables"
#define kDatasetFocusKey "focus"
#define kDatasetPreviousFocusKey "previous_focus"
#define kDatasetMetadataKey "metadata"
#pragma region Type Registration
static OCTypeID kDatasetID = kOCNotATypeID;
struct impl_Dataset {
    OCBase base;
    OCStringRef version;                           // e.g. "1.0"
    OCStringRef timestamp;                         // ISO-8601 UTC
    GeographicCoordinateRef geographicCoordinate;  // or NULL if not set
    bool readOnly;                                 // default false
    OCMutableArrayRef dimensions;
    OCMutableArrayRef dependentVariables;
    OCMutableArrayRef tags;
    OCStringRef description;
    // RMN extras
    OCStringRef title;
    DatumRef focus;
    DatumRef previousFocus;
    OCMutableIndexArrayRef dimensionPrecedence;
    OCDictionaryRef metaData;
};
OCTypeID DatasetGetTypeID(void) {
    if (kDatasetID == kOCNotATypeID)
        kDatasetID = OCRegisterType("Dataset");
    return kDatasetID;
}
static void impl_DatasetFinalize(const void *ptr) {
    if (!ptr) return;
    DatasetRef ds = (DatasetRef)ptr;
    // release all owned fields
    OCRelease(ds->dimensions);
    OCRelease(ds->dependentVariables);
    OCRelease(ds->tags);
    OCRelease(ds->description);
    OCRelease(ds->title);
    OCRelease(ds->focus);
    OCRelease(ds->previousFocus);
    OCRelease(ds->dimensionPrecedence);
    OCRelease(ds->version);
    OCRelease(ds->timestamp);
    OCRelease(ds->geographicCoordinate);
    OCRelease(ds->metaData);
}
static bool impl_DatasetEqual(const void *a, const void *b) {
    const DatasetRef A = (const DatasetRef)a;
    const DatasetRef B = (const DatasetRef)b;
    if (!A || !B) return false;
    if (A == B) return true;
    if (A->dimensions != B->dimensions &&
        !OCTypeEqual(A->dimensions, B->dimensions)) return false;
    if (A->dependentVariables != B->dependentVariables &&
        !OCTypeEqual(A->dependentVariables, B->dependentVariables)) return false;
    if (A->tags != B->tags &&
        !OCTypeEqual(A->tags, B->tags)) return false;
    if (A->description != B->description &&
        !OCTypeEqual(A->description, B->description)) return false;
    if (A->title != B->title &&
        !OCTypeEqual(A->title, B->title)) return false;
    if (A->focus != B->focus &&
        !OCTypeEqual(A->focus, B->focus)) return false;
    if (A->previousFocus != B->previousFocus &&
        !OCTypeEqual(A->previousFocus, B->previousFocus)) return false;
    if (A->dimensionPrecedence != B->dimensionPrecedence &&
        !OCTypeEqual(A->dimensionPrecedence, B->dimensionPrecedence)) return false;
    if (!OCStringEqual(A->version, B->version)) return false;
    if (!OCStringEqual(A->timestamp, B->timestamp)) return false;
    if ((A->geographicCoordinate != B->geographicCoordinate) &&
        !OCTypeEqual(A->geographicCoordinate, B->geographicCoordinate)) return false;
    if (A->readOnly != B->readOnly) return false;
    if (A->metaData != B->metaData &&
        !OCTypeEqual(A->metaData, B->metaData)) return false;
    return true;
}
static OCStringRef impl_DatasetCopyFormattingDesc(OCTypeRef cf) {
    DatasetRef ds = (DatasetRef)cf;
    if (!ds) return STR("<Dataset: NULL>");
    return OCStringCreateWithFormat(
        STR("<Dataset dims=%lu vars=%lu tags=%lu title=%@>"),
        (unsigned long)OCArrayGetCount(ds->dimensions),
        (unsigned long)OCArrayGetCount(ds->dependentVariables),
        (unsigned long)OCArrayGetCount(ds->tags),
        ds->title);
}
static cJSON *impl_DatasetCreateJSON(const void *obj) {
    if (!obj) return cJSON_CreateNull();
    DatasetRef ds = (DatasetRef)obj;
    // Build the “core” dictionary
    OCDictionaryRef core = DatasetCopyAsDictionary(ds);
    if (!core) return cJSON_CreateNull();
    // Wrap under the “csdm” envelope
    cJSON *root = cJSON_CreateObject();
    cJSON *inner = OCTypeCopyJSON((OCTypeRef)core);
    cJSON_AddItemToObject(root,
                          kDatasetCsdmEnvelopeKey,
                          inner);
    OCRelease(core);
    return root;
}
static void *impl_DatasetDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    DatasetRef src = (DatasetRef)ptr;
    OCDictionaryRef dict = DatasetCopyAsDictionary(src);
    if (!dict) return NULL;
    OCStringRef err = NULL;
    DatasetRef copy = DatasetCreateFromDictionary(dict, &err);
    OCRelease(dict);
    OCRelease(err);  // Discard error, since deep copy failure isn't recoverable here
    return copy;
}
static struct impl_Dataset *DatasetAllocate(void) {
    return OCTypeAlloc(
        struct impl_Dataset,
        DatasetGetTypeID(),
        impl_DatasetFinalize,
        impl_DatasetEqual,
        impl_DatasetCopyFormattingDesc,
        impl_DatasetCreateJSON,
        impl_DatasetDeepCopy,
        impl_DatasetDeepCopy);
}
static void impl_InitDatasetFields(DatasetRef ds) {
    ds->dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->dependentVariables = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->description = STR("");
    ds->title = STR("");
    ds->focus = NULL;
    ds->previousFocus = NULL;
    ds->dimensionPrecedence = OCIndexArrayCreateMutable(0);
    ds->version = STR("1.0");
    ds->timestamp = OCCreateISO8601Timestamp();
    ds->geographicCoordinate = NULL;
    ds->readOnly = false;
    ds->metaData = OCDictionaryCreateMutable(0);
}
static bool impl_ValidateDatasetParameters(OCArrayRef dimensions, OCArrayRef dependentVariables) {
    // must have at least one dependent variable
    if (!dependentVariables || OCArrayGetCount(dependentVariables) == 0)
        return false;
    // compute expected length from dimensions (defaults to 1 if dimensions==NULL)
    OCIndex expectedSize = RMNCalculateSizeFromDimensions(dimensions);
    // if dimensions provided, check that each is a DimensionRef
    if (dimensions) {
        OCIndex nDims = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < nDims; i++) {
            const void *obj = OCArrayGetValueAtIndex(dimensions, i);
            OCTypeID tid = OCGetTypeID(obj);
            if (tid != DimensionGetTypeID() && tid != LabeledDimensionGetTypeID() && tid != SIMonotonicDimensionGetTypeID() && tid != SILinearDimensionGetTypeID()) {
                return false;
            }
        }
    }
    // now validate each dependent variable
    OCIndex dvCount = OCArrayGetCount(dependentVariables);
    for (OCIndex i = 0; i < dvCount; i++) {
        const void *obj = OCArrayGetValueAtIndex(dependentVariables, i);
        if (OCGetTypeID(obj) != DependentVariableGetTypeID())
            return false;
        DependentVariableRef dv = (DependentVariableRef)obj;
        OCIndex dvSize = DependentVariableGetSize(dv);
        if (dvSize != expectedSize) {
            fprintf(stderr,
                    "RMNValidateDatasetParameters: size mismatch for DV at index %ld: got %ld, expected %ld\n",
                    (long)i, (long)dvSize, (long)expectedSize);
            return false;
        }
    }
    return true;
}
#pragma endregion Type Registration
#pragma region Creators
DatasetRef DatasetCreate(
    OCArrayRef dimensions,
    OCIndexArrayRef dimensionPrecedence,
    OCArrayRef dependentVariables,
    OCArrayRef tags,
    OCStringRef description,
    OCStringRef title,
    DatumRef focus,
    DatumRef previousFocus,
    OCDictionaryRef metaData) {
    if (!impl_ValidateDatasetParameters(dimensions, dependentVariables))
        return NULL;
    DatasetRef ds = DatasetAllocate();
    if (!ds) return NULL;
    impl_InitDatasetFields(ds);
    if (dimensions) {
        OCIndex n = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < n; i++) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
            DimensionRef dc = (DimensionRef)OCTypeDeepCopyMutable(d);
            OCArrayAppendValue(ds->dimensions, dc);
            OCRelease(dc);
        }
    }
    if (dependentVariables) {
        OCIndex n = OCArrayGetCount(dependentVariables);
        for (OCIndex i = 0; i < n; i++) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, i);
            DependentVariableRef copy = DependentVariableCreateCopy(dv);
            OCArrayAppendValue(ds->dependentVariables, copy);
            OCRelease(copy);
        }
    }
    if (tags) {
        OCIndex n = OCArrayGetCount(tags);
        for (OCIndex i = 0; i < n; i++) {
            OCStringRef s = (OCStringRef)OCArrayGetValueAtIndex(tags, i);
            OCArrayAppendValue(ds->tags, s);
        }
    }
    OCIndex dimCount = OCArrayGetCount(ds->dimensions);
    if (dimensionPrecedence && OCIndexArrayGetCount(dimensionPrecedence) == dimCount) {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCIndex idx = OCIndexArrayGetValueAtIndex(dimensionPrecedence, i);
            OCIndexArrayAppendValue(ds->dimensionPrecedence, idx);
        }
    } else {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCIndexArrayAppendValue(ds->dimensionPrecedence, i);
        }
    }
    ds->description = description ? OCStringCreateCopy(description) : STR("");
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    if (metaData) {
        OCRelease(ds->metaData);
        ds->metaData = OCTypeDeepCopyMutable(metaData);
    }
    return ds;
}
OCDictionaryRef DatasetCopyAsDictionary(DatasetRef ds) {
    if (!ds) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) return NULL;
    // — CSDM‐1.0 fields —
    OCDictionarySetValue(dict,
                         STR(kDatasetVersionKey),
                         ds->version);
    OCDictionarySetValue(dict,
                         STR(kDatasetTimestampKey),
                         ds->timestamp);
    if (ds->readOnly) {
        OCDictionarySetValue(dict,
                             STR(kDatasetReadOnlyKey),
                             OCBooleanGetWithBool(true));
    }
    if (ds->geographicCoordinate) {
        OCDictionaryRef geo = GeographicCoordinateCopyAsDictionary(ds->geographicCoordinate);
        OCDictionarySetValue(dict,
                             STR(kDatasetGeoCoordinateKey),
                             geo);
        OCRelease(geo);
    }
    // — RMN extras —
    // tags
    if (ds->tags) {
        OCMutableArrayRef tags_copy = OCArrayCreateMutableCopy(ds->tags);
        OCDictionarySetValue(dict,
                             STR(kDatasetTagsKey),
                             tags_copy);
        OCRelease(tags_copy);
    }
    // description & title
    if (ds->description) {
        OCStringRef dcopy = OCStringCreateCopy(ds->description);
        OCDictionarySetValue(dict,
                             STR(kDatasetDescriptionKey),
                             dcopy);
        OCRelease(dcopy);
    }
    if (ds->title) {
        OCStringRef tcopy = OCStringCreateCopy(ds->title);
        OCDictionarySetValue(dict,
                             STR(kDatasetTitleKey),
                             tcopy);
        OCRelease(tcopy);
    }
    // dimensions
    if (ds->dimensions) {
        OCIndex n = OCArrayGetCount(ds->dimensions);
        OCMutableArrayRef dims_arr = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < n; ++i) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(ds->dimensions, i);
            OCDictionaryRef dd = DimensionCopyAsDictionary(d);
            OCArrayAppendValue(dims_arr, dd);
            OCRelease(dd);
        }
        OCDictionarySetValue(dict,
                             STR(kDatasetDimensionsKey),
                             dims_arr);
        OCRelease(dims_arr);
    }
    // dimension_precedence
    if (ds->dimensionPrecedence) {
        OCIndexArrayRef prec_copy = OCIndexArrayCreateMutableCopy(ds->dimensionPrecedence);
        OCDictionarySetValue(dict,
                             STR(kDatasetDimensionPrecedenceKey),
                             prec_copy);
        OCRelease(prec_copy);
    }
    // dependent_variables
    if (ds->dependentVariables) {
        OCIndex m = OCArrayGetCount(ds->dependentVariables);
        OCMutableArrayRef dvs_arr = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < m; ++i) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(ds->dependentVariables, i);
            DependentVariableRef copy = DependentVariableCreateCopy(dv);
            DependentVariableSetType(copy, STR("internal"));
            OCDictionaryRef ddv = DependentVariableCopyAsDictionary(copy);
            OCArrayAppendValue(dvs_arr, ddv);
            OCRelease(ddv);
            OCRelease(copy);
        }
        OCDictionarySetValue(dict,
                             STR(kDatasetDependentVariablesKey),
                             dvs_arr);
        OCRelease(dvs_arr);
    }
    // focus & previous_focus
    if (ds->focus) {
        OCDictionaryRef fdict = DatumCopyAsDictionary(ds->focus);
        OCDictionarySetValue(dict,
                             STR(kDatasetFocusKey),
                             fdict);
        OCRelease(fdict);
    }
    if (ds->previousFocus) {
        OCDictionaryRef pf = DatumCopyAsDictionary(ds->previousFocus);
        OCDictionarySetValue(dict,
                             STR(kDatasetPreviousFocusKey),
                             pf);
        OCRelease(pf);
    }
    // metadata
    if (ds->metaData) {
        OCDictionaryRef meta_copy = (OCDictionaryRef)OCTypeDeepCopyMutable(ds->metaData);
        OCDictionarySetValue(dict,
                             STR(kDatasetMetadataKey),
                             meta_copy);
        OCRelease(meta_copy);
    }
    return (OCDictionaryRef)dict;
}
DatasetRef DatasetCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("Input dictionary is NULL");
        return NULL;
    }
    OCArrayRef dims = NULL;
    OCIndexArrayRef dimPrec = NULL;
    OCArrayRef dvs = NULL;
    OCArrayRef tags = NULL;
    OCStringRef desc = NULL;
    OCStringRef title = NULL;
    DatumRef focus = NULL;
    DatumRef prevFocus = NULL;
    OCDictionaryRef metadata = NULL;
    DatasetRef ds = NULL;

    // — dimensions —
    OCArrayRef rawDims = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDimensionsKey));
    if (rawDims) {
        OCIndex n = OCArrayGetCount(rawDims);
        OCMutableArrayRef tmp = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        if (!tmp) {
            if (outError) *outError = STR("Failed to allocate dimensions array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < n; ++i) {
            OCDictionaryRef ddict = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDims, i);
            if (OCGetTypeID(ddict) != OCDictionaryGetTypeID()) {
                if (outError) *outError = STR("Invalid dimension entry (expected dictionary)");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DimensionRef d = DimensionCreateFromDictionary(ddict, &err);
            if (!d) {
                if (outError) {
                    *outError = err
                        ? OCStringCreateCopy(err)
                        : STR("Failed to parse dimension");
                }
                OCRelease(err);
                OCRelease(tmp);
                goto cleanup;
            }
            OCArrayAppendValue(tmp, d);
            OCRelease(d);
        }
        dims = tmp;
    }

    // — dimension_precedence —
    OCIndexArrayRef rawPrec = (OCIndexArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDimensionPrecedenceKey));
    if (rawPrec) {
        dimPrec = OCIndexArrayCreateMutableCopy(rawPrec);
    }

    // — dependent_variables —
    OCArrayRef rawDVs = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDependentVariablesKey));
    if (rawDVs) {
        OCIndex m = OCArrayGetCount(rawDVs);
        OCMutableArrayRef tmp = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        if (!tmp) {
            if (outError) *outError = STR("Failed to allocate dependentVariables array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < m; ++i) {
            OCDictionaryRef dd = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDVs, i);
            if (OCGetTypeID(dd) != OCDictionaryGetTypeID()) {
                if (outError) *outError = STR("Invalid dependent variable entry (expected dictionary)");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DependentVariableRef dv = DependentVariableCreateFromDictionary(dd, &err);
            if (!dv) {
                if (outError) {
                    *outError = err
                        ? OCStringCreateCopy(err)
                        : STR("Failed to parse dependent variable");
                }
                OCRelease(err);
                OCRelease(tmp);
                goto cleanup;
            }
            OCArrayAppendValue(tmp, dv);
            OCRelease(dv);
        }
        dvs = tmp;
    }

    // — tags —
    OCArrayRef rawTags = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetTagsKey));
    if (rawTags) {
        tags = OCArrayCreateMutableCopy(rawTags);
    }

    // — description & title —
    OCStringRef s;
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR(kDatasetDescriptionKey)))) {
        desc = OCStringCreateCopy(s);
    }
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR(kDatasetTitleKey)))) {
        title = OCStringCreateCopy(s);
    }

    // — focus & previous_focus —
    OCDictionaryRef ddict;
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetFocusKey)))) {
        focus = DatumCreateFromDictionary(ddict, outError);
        if (!focus) goto cleanup;
    }
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetPreviousFocusKey)))) {
        prevFocus = DatumCreateFromDictionary(ddict, outError);
        if (!prevFocus) goto cleanup;
    }

    // — metadata —
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetMetadataKey)))) {
        metadata = (OCDictionaryRef)OCTypeDeepCopyMutable(ddict);
        if (!metadata) {
            if (outError) *outError = STR("Failed to copy metadata");
            goto cleanup;
        }
    }

    OCStringRef version   = OCDictionaryGetValue(dict, STR("version"));
    OCStringRef timestamp = OCDictionaryGetValue(dict, STR("timestamp"));
    OCBooleanRef roflag   = OCDictionaryGetValue(dict, STR("read_only"));
    OCDictionaryRef geoDict= OCDictionaryGetValue(dict, STR("geographic_coordinate"));

    // — construct the Dataset —
    ds = DatasetCreate(dims, dimPrec, dvs, tags,
                       desc, title, focus, prevFocus, metadata);

    // overwrite the version/timestamp/readOnly/geo fields
    OCRelease(ds->version);
    ds->version   = OCStringCreateCopy(version);
    OCRelease(ds->timestamp);
    ds->timestamp = OCStringCreateCopy(timestamp);
    ds->readOnly  = (roflag
                     && OCGetTypeID(roflag) == OCBooleanGetTypeID()
                     && OCBooleanGetValue(roflag));
    if (geoDict) {
        OCStringRef err = NULL;
        ds->geographicCoordinate = GeographicCoordinateCreateFromDictionary(geoDict, &err);
        OCRelease(err);
    }

cleanup:
    OCRelease(dims);
    OCRelease(dimPrec);
    OCRelease(dvs);
    OCRelease(tags);
    OCRelease(desc);
    OCRelease(title);
    OCRelease(focus);
    OCRelease(prevFocus);
    OCRelease(metadata);
    return ds;
}
OCDictionaryRef
DatasetDictionaryCreateFromJSON(cJSON *json,
                                OCStringRef *outError)
{
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected a JSON object for Dataset");
        return NULL;
    }
    // ↪ unwrap “csdm” envelope
    cJSON *inner = cJSON_GetObjectItemCaseSensitive(json, kDatasetCsdmEnvelopeKey);
    if (!inner || !cJSON_IsObject(inner)) {
        if (outError) *outError = STR("Missing or invalid \"csdm\" key");
        return NULL;
    }
    json = inner;

    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        if (outError) *outError = STR("Failed to create dictionary");
        return NULL;
    }

    cJSON *entry;

    // version
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetVersionKey);
    if (cJSON_IsString(entry)) {
        OCStringRef v = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kDatasetVersionKey), v);
        OCRelease(v);
    }

    // timestamp
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetTimestampKey);
    if (cJSON_IsString(entry)) {
        OCStringRef t = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kDatasetTimestampKey), t);
        OCRelease(t);
    }

    // read_only
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetReadOnlyKey);
    if (cJSON_IsBool(entry)) {
        OCDictionarySetValue(dict,
                             STR(kDatasetReadOnlyKey),
                             OCBooleanGetWithBool(cJSON_IsTrue(entry)));
    }

    // geographic_coordinate
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetGeoCoordinateKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef gcErr = NULL;
        GeographicCoordinateRef gc =
            GeographicCoordinateCreateFromJSON(entry, &gcErr);
        if (gc) {
            OCDictionaryRef gcDict = GeographicCoordinateCopyAsDictionary(gc);
            OCDictionarySetValue(dict,
                                 STR(kDatasetGeoCoordinateKey),
                                 gcDict);
            OCRelease(gcDict);
            OCRelease(gc);
        } else if (gcErr) {
            // propagate parse error
            if (outError) *outError = OCStringCreateCopy(gcErr);
            OCRelease(gcErr);
            OCRelease(dict);
            return NULL;
        }
        OCRelease(gcErr);
    }

    // tags
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetTagsKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef tagsArr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsString(e)) {
                OCStringRef s = OCStringCreateWithCString(e->valuestring);
                OCArrayAppendValue(tagsArr, s);
                OCRelease(s);
            }
        }
        OCDictionarySetValue(dict, STR(kDatasetTagsKey), tagsArr);
        OCRelease(tagsArr);
    }

    // description & title
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDescriptionKey);
    if (entry && cJSON_IsString(entry)) {
        OCStringRef desc = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kDatasetDescriptionKey), desc);
        OCRelease(desc);
    }
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetTitleKey);
    if (entry && cJSON_IsString(entry)) {
        OCStringRef title = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR(kDatasetTitleKey), title);
        OCRelease(title);
    }

    // dimensions
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDimensionsKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef dimsArr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsObject(e)) {
                OCStringRef dimErr = NULL;
                DimensionRef dim = DimensionCreateFromJSON(e, &dimErr);
                if (!dim) {
                    if (outError && dimErr) *outError = OCStringCreateCopy(dimErr);
                    OCRelease(dimErr);
                    OCRelease(dimsArr);
                    OCRelease(dict);
                    return NULL;
                }
                OCDictionaryRef dd = DimensionCopyAsDictionary(dim);
                OCRelease(dim);
                OCArrayAppendValue(dimsArr, dd);
                OCRelease(dd);
            }
        }
        OCDictionarySetValue(dict, STR(kDatasetDimensionsKey), dimsArr);
        OCRelease(dimsArr);
    }

    // dimension_precedence
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDimensionPrecedenceKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableIndexArrayRef precArr = OCIndexArrayCreateMutable(0);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsNumber(e)) {
                OCIndex idx = (OCIndex)e->valuedouble;
                OCIndexArrayAppendValue(precArr, idx);
            }
        }
        OCDictionarySetValue(dict,
                             STR(kDatasetDimensionPrecedenceKey),
                             precArr);
        OCRelease(precArr);
    }

    // dependent_variables
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDependentVariablesKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef dvsArr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsObject(e)) {
                OCStringRef dvErr = NULL;
                DependentVariableRef dv = DependentVariableCreateFromJSON(e, &dvErr);
                if (!dv) {
                    if (outError && dvErr) *outError = OCStringCreateCopy(dvErr);
                    OCRelease(dvErr);
                    OCRelease(dvsArr);
                    OCRelease(dict);
                    return NULL;
                }
                OCDictionaryRef ddv = DependentVariableCopyAsDictionary(dv);
                OCRelease(dv);
                OCArrayAppendValue(dvsArr, ddv);
                OCRelease(ddv);
            }
        }
        OCDictionarySetValue(dict,
                             STR(kDatasetDependentVariablesKey),
                             dvsArr);
        OCRelease(dvsArr);
    }

    // focus & previous_focus
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetFocusKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef fErr = NULL;
        DatumRef d = DatumCreateFromJSON(entry, &fErr);
        if (!d) {
            if (outError && fErr) *outError = OCStringCreateCopy(fErr);
            OCRelease(fErr);
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef fd = DatumCopyAsDictionary(d);
        OCRelease(d);
        OCDictionarySetValue(dict, STR(kDatasetFocusKey), fd);
        OCRelease(fd);
    }
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetPreviousFocusKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef pfErr = NULL;
        DatumRef d = DatumCreateFromJSON(entry, &pfErr);
        if (!d) {
            if (outError && pfErr) *outError = OCStringCreateCopy(pfErr);
            OCRelease(pfErr);
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef fd = DatumCopyAsDictionary(d);
        OCRelease(d);
        OCDictionarySetValue(dict, STR(kDatasetPreviousFocusKey), fd);
        OCRelease(fd);
    }

    // metadata
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetMetadataKey);
    if (entry && cJSON_IsObject(entry)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(entry, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDatasetMetadataKey), md);
        OCRelease(md);
    }

    return dict;
}

DatasetRef DatasetCreateFromJSON(cJSON *json, OCStringRef *outError) {
    OCDictionaryRef dict = DatasetDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    DatasetRef ds = DatasetCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return ds;
}
#pragma endregion Creators
#pragma region Export/Import
static bool join_path(char *out, size_t size, const char *dir, char sep, const char *relpath) {
    if (!out || !dir || !relpath) return false;
    int len = snprintf(out, size, "%s%c%s", dir, sep, relpath);
    return len > 0 && (size_t)len < size;
}
static bool ensure_directory(const char *dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dir || !*dir) {
        if (outError) *outError = STR("Invalid directory path");
        return false;
    }
    size_t dlen = strnlen(dir, PATH_MAX);
    if (dlen >= PATH_MAX) {
        if (outError) *outError = STR("Directory path too long");
        return false;
    }
    // Copy into a mutable buffer and strip trailing slashes
    char path[PATH_MAX];
    memcpy(path, dir, dlen);
    path[dlen] = '\0';
    while (dlen > 0 && (path[dlen - 1] == '/' || path[dlen - 1] == '\\')) {
        path[--dlen] = '\0';
    }
    // Iteratively create each path segment
    for (size_t i = 1; i <= dlen; ++i) {
        if (path[i] == '/' || path[i] == '\\' || i == dlen) {
            char saved = path[i];
            path[i] = '\0';
            errno = 0;
            if (MKDIR(path) != 0 && errno != EEXIST) {
                if (outError) {
                    OCStringRef fp = OCStringCreateWithCString(path);
                    *outError = OCStringCreateWithFormat(
                        STR("Failed to create directory %@"), fp);
                    OCRelease(fp);
                }
                return false;
            }
            path[i] = saved;
        }
    }
    return true;
}
static uint8_t *read_file_bytes(const char *path, size_t *out_len) {
    if (out_len) *out_len = 0;
    if (!path) return NULL;
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NULL;
    }
    size_t len = (size_t)st.st_size;
    // Guard against malloc(0)
    uint8_t *buf = malloc(len ? len : 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    size_t got = fread(buf, 1, len, f);
    fclose(f);
    if (got != len) {
        free(buf);
        return NULL;
    }
    if (out_len) *out_len = len;
    return buf;
}
static bool ensure_parent_dirs(const char *fullpath, OCStringRef *outError) {
    if (!fullpath) {
        if (outError) *outError = STR("Invalid path");
        return false;
    }
    // Copy since dirname() may modify its argument
    char tmp[PATH_MAX];
    size_t plen = strnlen(fullpath, PATH_MAX);
    if (plen >= PATH_MAX) {
        if (outError) *outError = STR("Path too long");
        return false;
    }
    memcpy(tmp, fullpath, plen);
    tmp[plen] = '\0';
    char *dir = dirname(tmp);
    // If dirname returns "." or empty, nothing to do
    if (!dir || !*dir || (dir[0] == '.' && dir[1] == '\0'))
        return true;
    return ensure_directory(dir, outError);
}
bool ExportDataset(DatasetRef ds,
                   const char *json_path,
                   const char *binary_dir,
                   OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ds || !json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return false;
    }
    // 0) Choose file extension based on whether any DV needs external serialization
    bool hasExternal = false;
    OCIndex dvCount = OCArrayGetCount(DatasetGetDependentVariables(ds));
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = (DependentVariableRef)
            OCArrayGetValueAtIndex(DatasetGetDependentVariables(ds), i);
        if (dv && DependentVariableShouldSerializeExternally(dv)) {
            hasExternal = true;
            break;
        }
    }
    const char *wantExt = hasExternal ? "csdfe" : "csdf";
    const char *dot = strrchr(json_path, '.');
    const char *gotExt = dot ? dot + 1 : "";
    if (strcasecmp(gotExt, wantExt) != 0) {
        if (outError) {
            OCStringRef p = OCStringCreateWithCString(json_path);
            OCStringRef e = OCStringCreateWithCString(wantExt);
            OCStringRef s = OCStringCreateWithCString(
                hasExternal ? "contains" : "does not contain");
            *outError = OCStringCreateWithFormat(
                STR("CSDM requires extension '%@' when file %@ external data; got '%@'"),
                e, s, p);
            OCRelease(p);
            OCRelease(e);
            OCRelease(s);
        }
        return false;
    }
    // 1) Build the dictionary including all CSDM-1.0 fields
    OCDictionaryRef core = DatasetCopyAsDictionary(ds);
    if (!core) {
        if (outError) *outError = STR("Failed to create dictionary from Dataset");
        return false;
    }
    // version
    OCDictionarySetValue((OCMutableDictionaryRef)core,
                         STR(kDatasetVersionKey),
                         STR("1.0"));
    // timestamp
    {
        OCStringRef ts = OCCreateISO8601Timestamp();
        OCDictionarySetValue((OCMutableDictionaryRef)core,
                             STR(kDatasetTimestampKey),
                             ts);
        OCRelease(ts);
    }
    // read_only
    if (DatasetGetReadOnly(ds)) {
        OCDictionarySetValue((OCMutableDictionaryRef)core,
                             STR(kDatasetReadOnlyKey),
                             OCBooleanGetWithBool(true));
    }
    // geographic_coordinate
    if (DatasetGetGeographicCoordinate(ds)) {
        GeographicCoordinateRef gc = DatasetGetGeographicCoordinate(ds);
        OCDictionaryRef gcDict = GeographicCoordinateCopyAsDictionary(gc);
        if (gcDict) {
            OCDictionarySetValue((OCMutableDictionaryRef)core,
                                 STR(kDatasetGeoCoordinateKey),
                                 gcDict);
            OCRelease(gcDict);
        }
    }
    // 2) Wrap under “csdm” envelope
    OCMutableDictionaryRef root = OCDictionaryCreateMutable(1);
    OCDictionarySetValue(root,
                         STR(kDatasetCsdmEnvelopeKey),
                         core);
    OCRelease(core);
    // 3) Convert to JSON
    cJSON *json = OCTypeCopyJSON((OCTypeRef)root);
    OCRelease(root);
    if (!json) {
        if (outError) *outError = STR("Failed to convert dictionary to JSON");
        return false;
    }
    // 4) Serialize JSON to string
    char *json_text = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_text) {
        if (outError) *outError = STR("Failed to generate JSON string");
        return false;
    }
    size_t json_len = strlen(json_text);
    // 5) Ensure parent directories for JSON
    if (!ensure_parent_dirs(json_path, outError)) {
        free(json_text);
        return false;
    }
    // 6) Write JSON to disk
    FILE *jf = fopen(json_path, "wb");
    if (!jf) {
        free(json_text);
        if (outError) *outError = STR("Failed to open JSON output file");
        return false;
    }
    size_t wrote = fwrite(json_text, 1, json_len, jf);
    fclose(jf);
    free(json_text);
    if (wrote != json_len) {
        if (outError) *outError = STR("Error writing JSON file");
        return false;
    }
    // 7) Ensure binary-dir exists
    if (!ensure_directory(binary_dir, outError))
        return false;
    // 8) Export any external DV blobs
    OCArrayRef dvs = DatasetGetDependentVariables(ds);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvs, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError) *outError = STR("External DV missing components_url");
            return false;
        }
        const char *rel = OCStringGetCString(url);
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath),
                       binary_dir, PATH_SEPARATOR, rel)) {
            if (outError) *outError = STR("Binary path too long");
            return false;
        }
        if (!ensure_parent_dirs(fullpath, outError))
            return false;
        OCDataRef blob = DependentVariableCreateCSDMComponentsData(
            dv, DatasetGetDimensions(ds));
        if (!blob) {
            if (outError) *outError = STR("Failed to create binary blob");
            return false;
        }
        FILE *bf = fopen(fullpath, "wb");
        if (!bf) {
            OCRelease(blob);
            if (outError) *outError = STR("Failed to open binary output file");
            return false;
        }
        const void *bytes = OCDataGetBytesPtr(blob);
        size_t len = (size_t)OCDataGetLength(blob);
        bool ok = (fwrite(bytes, 1, len, bf) == len);
        fclose(bf);
        OCRelease(blob);
        if (!ok) {
            if (outError) *outError = STR("Error writing binary blob");
            return false;
        }
    }
    return true;
}
DatasetRef ImportDataset(const char *json_path, const char *binary_dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return NULL;
    }
    // 1) Read entire JSON file into memory
    FILE *jf = fopen(json_path, "rb");
    if (!jf) {
        if (outError) *outError = STR("Failed to open JSON file");
        return NULL;
    }
    if (fseek(jf, 0, SEEK_END) != 0) {
        fclose(jf);
        if (outError) *outError = STR("Failed to seek JSON file");
        return NULL;
    }
    long fsize = ftell(jf);
    rewind(jf);
    char *buffer = malloc((size_t)fsize + 1);
    if (!buffer) {
        fclose(jf);
        if (outError) *outError = STR("Memory allocation failed");
        return NULL;
    }
    size_t got = fread(buffer, 1, (size_t)fsize, jf);
    fclose(jf);
    if (got != (size_t)fsize) {
        free(buffer);
        if (outError) *outError = STR("Failed to read full JSON file");
        return NULL;
    }
    buffer[fsize] = '\0';
    // 2) Parse JSON
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        if (outError) *outError = STR("Invalid JSON format");
        return NULL;
    }
    // 3) Create Dataset object
    DatasetRef ds = DatasetCreateFromJSON(root, outError);
    cJSON_Delete(root);
    if (!ds)
        return NULL;
    // 4) Load each external blob
    OCIndex count = ds && ds->dependentVariables ? OCArrayGetCount(ds->dependentVariables) : 0;
    for (OCIndex i = 0; i < count; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(ds->dependentVariables, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError) *outError = STR("Missing components_url in external DV");
            OCRelease(ds);
            return NULL;
        }
        const char *relpath = OCStringGetCString(url);
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath), binary_dir, PATH_SEPARATOR, relpath)) {
            if (outError) *outError = STR("Binary path too long");
            OCRelease(ds);
            return NULL;
        }
        size_t total_bytes = 0;
        uint8_t *bytes = read_file_bytes(fullpath, &total_bytes);
        if (!bytes) {
            if (outError) {
                OCStringRef pathStr = OCStringCreateWithCString(fullpath);
                *outError = OCStringCreateWithFormat(
                    STR("Failed to read file %@"),
                    pathStr);
                OCRelease(pathStr);
            }
            OCRelease(ds);
            return NULL;
        }
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        if (ncomps == 0) {
            ncomps = DependentVariableComponentsCountFromQuantityType(
                DependentVariableGetQuantityType(dv));
        }
        OCIndex npts = DependentVariableGetSize(dv);
        size_t elemSize = SIQuantityElementSize((SIQuantityRef)dv);
        size_t chunk = (size_t)npts * elemSize;
        if (chunk * (size_t)ncomps != total_bytes) {
            free(bytes);
            if (outError) *outError = STR("Binary size mismatch");
            OCRelease(ds);
            return NULL;
        }
        OCMutableArrayRef comps = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        for (OCIndex ci = 0; ci < ncomps; ++ci) {
            OCMutableDataRef buf = OCDataCreateMutable(chunk);
            if (!buf || !OCDataAppendBytes(buf, bytes + (ci * chunk), chunk)) {
                OCRelease(buf);
                OCRelease(comps);
                free(bytes);
                if (outError) *outError = STR("Failed to create component buffer");
                OCRelease(ds);
                return NULL;
            }
            OCArrayAppendValue(comps, buf);
            OCRelease(buf);
        }
        free(bytes);
        if (!DependentVariableSetComponents(dv, comps)) {
            OCRelease(comps);
            OCRelease(ds);
            if (outError) *outError = STR("Failed to install DV components");
            return NULL;
        }
        OCRelease(comps);
    }
    return ds;
}
#pragma endregion Export / Import
#pragma region Getters/Setters
OCMutableArrayRef DatasetGetDimensions(DatasetRef ds) {
    return ds ? ds->dimensions : NULL;
}
bool DatasetSetDimensions(DatasetRef ds, OCMutableArrayRef dims) {
    if (!ds || !dims) return false;
    OCRelease(ds->dimensions);
    ds->dimensions = (OCMutableArrayRef)OCRetain(dims);
    return true;
}
OCMutableIndexArrayRef DatasetGetDimensionPrecedence(DatasetRef ds) {
    return ds ? ds->dimensionPrecedence : NULL;
}
bool DatasetSetDimensionPrecedence(DatasetRef ds, OCMutableIndexArrayRef order) {
    if (!ds || !order) return false;
    OCRelease(ds->dimensionPrecedence);
    ds->dimensionPrecedence = (OCMutableIndexArrayRef)OCRetain(order);
    return true;
}
OCMutableArrayRef DatasetGetDependentVariables(DatasetRef ds) {
    return ds ? ds->dependentVariables : NULL;
}
bool DatasetSetDependentVariables(DatasetRef ds, OCMutableArrayRef dvs) {
    if (!ds || !dvs) return false;
    OCRelease(ds->dependentVariables);
    ds->dependentVariables = (OCMutableArrayRef)OCRetain(dvs);
    return true;
}
OCMutableArrayRef DatasetGetTags(DatasetRef ds) {
    return ds ? ds->tags : NULL;
}
bool DatasetSetTags(DatasetRef ds, OCMutableArrayRef tags) {
    if (!ds || !tags) return false;
    OCRelease(ds->tags);
    ds->tags = (OCMutableArrayRef)OCRetain(tags);
    return true;
}
OCStringRef DatasetGetDescription(DatasetRef ds) {
    return ds ? ds->description : NULL;
}
bool DatasetSetDescription(DatasetRef ds, OCStringRef desc) {
    if (!ds || !desc) return false;
    OCRelease(ds->description);
    ds->description = OCStringCreateCopy(desc);
    return true;
}
OCStringRef DatasetGetTitle(DatasetRef ds) {
    return ds ? ds->title : NULL;
}
bool DatasetSetTitle(DatasetRef ds, OCStringRef title) {
    if (!ds || !title) return false;
    OCRelease(ds->title);
    ds->title = OCStringCreateCopy(title);
    return true;
}
DatumRef DatasetGetFocus(DatasetRef ds) {
    return ds ? ds->focus : NULL;
}
bool DatasetSetFocus(DatasetRef ds, DatumRef focus) {
    if (!ds) return false;
    OCRelease(ds->focus);
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    return true;
}
DatumRef DatasetGetPreviousFocus(DatasetRef ds) {
    return ds ? ds->previousFocus : NULL;
}
bool DatasetSetPreviousFocus(DatasetRef ds, DatumRef previousFocus) {
    if (!ds) return false;
    OCRelease(ds->previousFocus);
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    return true;
}
OCDictionaryRef DatasetGetMetaData(DatasetRef ds) {
    return ds ? ds->metaData : NULL;
}
bool DatasetSetMetaData(DatasetRef ds, OCDictionaryRef md) {
    if (!ds || !md) return false;
    OCRelease(ds->metaData);
    ds->metaData = (OCDictionaryRef)OCRetain(md);
    return true;
}
#pragma endregion Getters / Setters
#pragma region CSDM-1.0 Fields
OCStringRef DatasetGetVersion(DatasetRef ds) {
    return ds ? ds->version : NULL;
}
bool DatasetSetVersion(DatasetRef ds, OCStringRef v) {
    if (!ds || !v) return false;
    OCRelease(ds->version);
    ds->version = OCStringCreateCopy(v);
    return ds->version != NULL;
}
OCStringRef DatasetGetTimestamp(DatasetRef ds) {
    return ds ? ds->timestamp : NULL;
}
bool DatasetSetTimestamp(DatasetRef ds, OCStringRef ts) {
    if (!ds || !ts) return false;
    OCRelease(ds->timestamp);
    ds->timestamp = OCStringCreateCopy(ts);
    return ds->timestamp != NULL;
}
GeographicCoordinateRef DatasetGetGeographicCoordinate(DatasetRef ds) {
    return ds ? ds->geographicCoordinate : NULL;
}
bool DatasetSetGeographicCoordinate(DatasetRef ds, GeographicCoordinateRef gc) {
    if (!ds) return false;
    OCRelease(ds->geographicCoordinate);
    ds->geographicCoordinate = gc ? (GeographicCoordinateRef)OCRetain(gc) : NULL;
    return true;
}
bool DatasetGetReadOnly(DatasetRef ds) {
    return ds ? ds->readOnly : false;
}
bool DatasetSetReadOnly(DatasetRef ds, bool readOnly) {
    if (!ds) return false;
    ds->readOnly = readOnly;
    return true;
}
#pragma endregion
