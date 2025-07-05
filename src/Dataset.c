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
#pragma region Type Registration
static OCTypeID kDatasetID = kOCNotATypeID;
struct impl_Dataset {
    OCBase base;
    // CSDM attributes
    OCMutableArrayRef dimensions;          // array of Dimensions.
    OCMutableArrayRef dependentVariables;  // Array with dependentVariables. Each element is a DependentVariable
    OCMutableArrayRef tags;
    OCStringRef description;
    // RMN extra attributes below
    OCStringRef title;
    DatumRef focus;
    DatumRef previousFocus;
    OCMutableIndexArrayRef dimensionPrecedence;  // ordered array of indexes, representing dimension precedence.
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
    OCRelease(ds->metaData);
}
static bool impl_DatasetEqual(const void *a, const void *b) {
    const DatasetRef A = (const DatasetRef)a;
    const DatasetRef B = (const DatasetRef)b;
    if (!A || !B) return false;
    if (!OCTypeEqual(A->dimensions, B->dimensions)) return false;
    if (!OCTypeEqual(A->dependentVariables, B->dependentVariables)) return false;
    if (!OCTypeEqual(A->tags, B->tags)) return false;
    if (!OCTypeEqual(A->description, B->description)) return false;
    if (!OCTypeEqual(A->title, B->title)) return false;
    if (!OCTypeEqual(A->focus, B->focus)) return false;
    if (!OCTypeEqual(A->previousFocus, B->previousFocus)) return false;
    if (!OCTypeEqual(A->dimensionPrecedence, B->dimensionPrecedence)) return false;
    if (!OCTypeEqual(A->metaData, B->metaData)) return false;
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
cJSON *impl_DatasetCreateJSON(const void *obj) {
    if (!obj) return cJSON_CreateNull();
    DatasetRef ds = (DatasetRef)obj;
    OCDictionaryRef dict = DatasetCopyAsDictionary(ds);
    if (!dict) return cJSON_CreateNull();
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    OCRelease(dict);
    return json;
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
    // — tags —
    if (ds->tags) {
        OCMutableArrayRef tags_copy = OCArrayCreateMutableCopy(ds->tags);
        if (tags_copy) {
            OCDictionarySetValue(dict, STR("tags"), tags_copy);
            OCRelease(tags_copy);
        }
    }
    // — description & title —
    if (ds->description) {
        OCStringRef dcopy = OCStringCreateCopy(ds->description);
        OCDictionarySetValue(dict, STR("description"), dcopy);
        OCRelease(dcopy);
    }
    if (ds->title) {
        OCStringRef tcopy = OCStringCreateCopy(ds->title);
        OCDictionarySetValue(dict, STR("title"), tcopy);
        OCRelease(tcopy);
    }
    // — dimensions —
    if (ds->dimensions) {
        OCIndex n = OCArrayGetCount(ds->dimensions);
        OCMutableArrayRef dims_arr = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < n; ++i) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(ds->dimensions, i);
            OCDictionaryRef d_dict = DimensionCopyAsDictionary(d);
            OCArrayAppendValue(dims_arr, d_dict);
            OCRelease(d_dict);
        }
        OCDictionarySetValue(dict, STR("dimensions"), dims_arr);
        OCRelease(dims_arr);
    }
    // — dimension_precedence —
    if (ds->dimensionPrecedence) {
        OCIndexArrayRef prec_copy = OCIndexArrayCreateMutableCopy(ds->dimensionPrecedence);
        OCDictionarySetValue(dict, STR("dimension_precedence"), prec_copy);
        OCRelease(prec_copy);
    }
    // — dependent_variables —
    if (ds->dependentVariables) {
        OCIndex m = OCArrayGetCount(ds->dependentVariables);
        OCMutableArrayRef dvs_arr = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < m; ++i) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(ds->dependentVariables, i);
            DependentVariableRef copy = DependentVariableCreateCopy(dv);
            DependentVariableSetType(copy, STR("internal"));
            OCDictionaryRef dv_dict = DependentVariableCopyAsDictionary(copy);
            OCArrayAppendValue(dvs_arr, dv_dict);
            OCRelease(dv_dict);
            OCRelease(copy);
        }
        OCDictionarySetValue(dict, STR("dependent_variables"), dvs_arr);
        OCRelease(dvs_arr);
    }
    // — focus & previous_focus —
    if (ds->focus) {
        OCDictionaryRef fdict = DatumCopyAsDictionary(ds->focus);
        OCDictionarySetValue(dict, STR("focus"), fdict);
        OCRelease(fdict);
    }
    if (ds->previousFocus) {
        OCDictionaryRef pf = DatumCopyAsDictionary(ds->previousFocus);
        OCDictionarySetValue(dict, STR("previous_focus"), pf);
        OCRelease(pf);
    }
    // — metadata: just deep-copy the existing dictionary —
    if (ds->metaData) {
        OCDictionaryRef meta_copy = (OCDictionaryRef)OCTypeDeepCopyMutable(ds->metaData);
        if (meta_copy) {
            OCDictionarySetValue(dict, STR("metadata"), meta_copy);
            OCRelease(meta_copy);
        }
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
    OCArrayRef rawDims = (OCArrayRef)OCDictionaryGetValue(dict, STR("dimensions"));
    if (rawDims) {
        OCIndex n = OCArrayGetCount(rawDims);
        OCMutableArrayRef tmp = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        if (!tmp) {
            *outError = STR("Failed to allocate dimensions array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < n; ++i) {
            OCDictionaryRef ddict = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDims, i);
            if (OCGetTypeID(ddict) != OCDictionaryGetTypeID()) {
                *outError = STR("Invalid dimension entry (expected dictionary)");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DimensionRef d = DimensionCreateFromDictionary(ddict, &err);
            if (!d) {
                *outError = err ? OCStringCreateCopy(err) : STR("Failed to parse dimension");
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
    OCIndexArrayRef rawPrec = (OCIndexArrayRef)OCDictionaryGetValue(dict, STR("dimension_precedence"));
    if (rawPrec) {
        dimPrec = OCIndexArrayCreateMutableCopy(rawPrec);
    }
    // — dependent_variables —
    OCArrayRef rawDVs = (OCArrayRef)OCDictionaryGetValue(dict, STR("dependent_variables"));
    if (rawDVs) {
        OCIndex m = OCArrayGetCount(rawDVs);
        OCMutableArrayRef tmp = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        if (!tmp) {
            *outError = STR("Failed to allocate dependentVariables array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < m; ++i) {
            OCDictionaryRef dd = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDVs, i);
            if (OCGetTypeID(dd) != OCDictionaryGetTypeID()) {
                *outError = STR("Invalid dependent variable entry (expected dictionary)");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DependentVariableRef dv = DependentVariableCreateFromDictionary(dd, &err);
            if (!dv) {
                *outError = err ? OCStringCreateCopy(err) : STR("Failed to parse dependent variable");
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
    OCArrayRef rawTags = (OCArrayRef)OCDictionaryGetValue(dict, STR("tags"));
    if (rawTags) {
        tags = OCArrayCreateMutableCopy(rawTags);
    }
    // — description & title —
    OCStringRef s;
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR("description")))) {
        desc = OCStringCreateCopy(s);
    }
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR("title")))) {
        title = OCStringCreateCopy(s);
    }
    // — focus & previous_focus —
    OCDictionaryRef ddict;
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("focus")))) {
        focus = DatumCreateFromDictionary(ddict, outError);
        if (!focus) goto cleanup;
    }
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("previous_focus")))) {
        prevFocus = DatumCreateFromDictionary(ddict, outError);
        if (!prevFocus) goto cleanup;
    }
    // — metadata — deep-copy existing dictionary
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata")))) {
        metadata = (OCDictionaryRef)OCTypeDeepCopyMutable(ddict);
        if (!metadata) {
            *outError = STR("Failed to copy metadata");
            goto cleanup;
        }
    }
    // — construct the Dataset —
    ds = DatasetCreate(dims, dimPrec, dvs, tags, desc, title, focus, prevFocus, metadata);
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
OCDictionaryRef DatasetDictionaryCreateFromJSON(cJSON *json,
                                                OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected a JSON object for Dataset");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        if (outError) *outError = STR("Failed to create dictionary");
        return NULL;
    }
    // --- tags ---
    cJSON *entry = cJSON_GetObjectItem(json, "tags");
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef tagsArr =
            OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsString(e)) {
                OCStringRef s = OCStringCreateWithCString(e->valuestring);
                OCArrayAppendValue(tagsArr, s);
                OCRelease(s);
            }
        }
        OCDictionarySetValue(dict, STR("tags"), tagsArr);
        OCRelease(tagsArr);
    }
    // --- description & title ---
    entry = cJSON_GetObjectItem(json, "description");
    if (entry && cJSON_IsString(entry)) {
        OCStringRef desc = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR("description"), desc);
        OCRelease(desc);
    }
    entry = cJSON_GetObjectItem(json, "title");
    if (entry && cJSON_IsString(entry)) {
        OCStringRef title = OCStringCreateWithCString(entry->valuestring);
        OCDictionarySetValue(dict, STR("title"), title);
        OCRelease(title);
    }
    // --- dimensions ---
    entry = cJSON_GetObjectItem(json, "dimensions");
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef dimsArr =
            OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsObject(e)) {
                OCStringRef err = NULL;
                DimensionRef dim = DimensionCreateFromJSON(e, &err);
                if (!dim) {
                    if (outError && err) *outError = OCStringCreateCopy(err);
                    OCRelease(err);
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
        OCDictionarySetValue(dict, STR("dimensions"), dimsArr);
        OCRelease(dimsArr);
    }
    // --- dimension_precedence ---
    entry = cJSON_GetObjectItem(json, "dimension_precedence");
    if (entry && cJSON_IsArray(entry)) {
        OCMutableIndexArrayRef precArr = OCIndexArrayCreateMutable(0);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsNumber(e)) {
                OCIndex idx = (OCIndex)e->valuedouble;
                OCIndexArrayAppendValue(precArr, idx);
            }
        }
        OCDictionarySetValue(dict, STR("dimension_precedence"), precArr);
        OCRelease(precArr);
    }
    // --- dependent_variables ---
    entry = cJSON_GetObjectItem(json, "dependent_variables");
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef dvsArr =
            OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsObject(e)) {
                OCStringRef err = NULL;
                DependentVariableRef dv =
                    DependentVariableCreateFromJSON(e, &err);
                if (!dv) {
                    if (outError && err) *outError = OCStringCreateCopy(err);
                    OCRelease(err);
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
        OCDictionarySetValue(dict, STR("dependent_variables"), dvsArr);
        OCRelease(dvsArr);
    }
    // --- focus & previous_focus ---
    entry = cJSON_GetObjectItem(json, "focus");
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef err = NULL;
        DatumRef d = DatumCreateFromJSON(entry, &err);
        if (!d) {
            if (outError && err) *outError = OCStringCreateCopy(err);
            OCRelease(err);
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef fd = DatumCopyAsDictionary(d);
        OCRelease(d);
        OCDictionarySetValue(dict, STR("focus"), fd);
        OCRelease(fd);
    }
    entry = cJSON_GetObjectItem(json, "previous_focus");
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef err = NULL;
        DatumRef d = DatumCreateFromJSON(entry, &err);
        if (!d) {
            if (outError && err) *outError = OCStringCreateCopy(err);
            OCRelease(err);
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef fd = DatumCopyAsDictionary(d);
        OCRelease(d);
        OCDictionarySetValue(dict, STR("previous_focus"), fd);
        OCRelease(fd);
    }
    // --- metadata ---
    entry = cJSON_GetObjectItem(json, "metadata");
    if (entry && cJSON_IsObject(entry)) {
        OCDictionaryRef md = OCMetadataCreateFromJSON(entry, outError);
        if (!md) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR("metadata"), md);
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
bool ExportDataset(DatasetRef ds, const char *json_path, const char *binary_dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ds || !json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return false;
    }
    // — 0) Decide if any DV is external, so we know which extension we require —
    bool hasExternal = false;
    OCIndex dvCount = DatasetGetDependentVariableCount(ds);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = DatasetGetDependentVariableAtIndex(ds, i);
        if (dv && DependentVariableShouldSerializeExternally(dv)) {
            hasExternal = true;
            break;
        }
    }

    // Check user-supplied json_path extension:
    const char *dot     = strrchr(json_path, '.');
    const char *gotExt  = dot ? dot + 1 : "";
    const char *wantExt = hasExternal ? "csdfe" : "csdf";
    if (strcasecmp(gotExt, wantExt) != 0) {
        if (outError) {
            OCStringRef pathStr = OCStringCreateWithCString(json_path);
            OCStringRef extStr  = OCStringCreateWithCString(wantExt);
            OCStringRef stateStr= OCStringCreateWithCString(hasExternal ? "contains" : "does not contain");
            *outError = OCStringCreateWithFormat(
                STR("CSDM requires %@ extension when the file %s external data; got %@"),
                extStr, stateStr, pathStr);
            OCRelease(pathStr);
            OCRelease(extStr);
            OCRelease(stateStr);
        }
        return false;
    }
    // 1) Serialize to JSON via an intermediate dictionary
    OCDictionaryRef dict = DatasetCopyAsDictionary(ds);
    if (!dict) {
        if (outError) *outError = STR("Failed to create dictionary from Dataset");
        return false;
    }
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    OCRelease(dict);
    if (!json) {
        if (outError) *outError = STR("Failed to convert dictionary to JSON");
        return false;
    }
    // 2) Render JSON to string
    char *json_text = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (!json_text) {
        if (outError) *outError = STR("Failed to generate JSON string");
        return false;
    }
    size_t json_len = strlen(json_text);
    // 3) Ensure parent dirs for JSON and write it out
    if (!ensure_parent_dirs(json_path, outError)) {
        free(json_text);
        return false;
    }
    FILE *jf = fopen(json_path, "wb");
    if (!jf) {
        free(json_text);
        if (outError) *outError = STR("Failed to open JSON output file");
        return false;
    }
    size_t written = fwrite(json_text, 1, json_len, jf);
    fclose(jf);
    free(json_text);
    if (written != json_len) {
        if (outError) *outError = STR("Failed writing JSON file");
        return false;
    }
    // 4) Ensure binary directory exists
    if (!ensure_directory(binary_dir, outError))
        return false;
    // 5) Export each external DependentVariable
    OCArrayRef dims = DatasetGetDimensions(ds);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = DatasetGetDependentVariableAtIndex(ds, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError) *outError = STR("External DV missing components_url");
            return false;
        }
        // build full path for the blob
        const char *relpath = OCStringGetCString(url);
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath), binary_dir, PATH_SEPARATOR, relpath)) {
            if (outError) *outError = STR("Binary path too long");
            return false;
        }
        if (!ensure_parent_dirs(fullpath, outError))
            return false;
        // serialize the blob
        OCDataRef blob = DependentVariableCreateCSDMComponentsData(dv, dims);
        if (!blob) {
            if (outError) *outError = STR("Failed to create binary blob for DV");
            return false;
        }
        // write it
        FILE *bf = fopen(fullpath, "wb");
        if (!bf) {
            OCRelease(blob);
            if (outError) *outError = STR("Failed to open binary output file");
            return false;
        }
        const void *bytes = OCDataGetBytesPtr(blob);
        size_t len = (size_t)OCDataGetLength(blob);
        bool success = (fwrite(bytes, 1, len, bf) == len);
        fclose(bf);
        OCRelease(blob);
        if (!success) {
            if (outError) *outError = STR("Error writing binary blob");
            return false;
        }
    }
    return true;
}
DatasetRef ImportDataset(const char *json_path,
                         const char *binary_dir,
                         OCStringRef *outError) {
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
    OCIndex count = DatasetGetDependentVariableCount(ds);
    for (OCIndex i = 0; i < count; ++i) {
        DependentVariableRef dv = DatasetGetDependentVariableAtIndex(ds, i);
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
