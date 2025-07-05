// ============================================================================
// Dataset.c
// ============================================================================
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "RMNLibrary.h"
// Provide a default file pattern for DV blobs if not defined elsewhere
#ifndef COMPONENTS_FILE_PATTERN
#define COMPONENTS_FILE_PATTERN "component_%ld.bin"
#endif
static OCTypeID kDatasetID = kOCNotATypeID;
// SIScalar Opaque Type
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
//==============================================================================
// MARK: - Type Registration
//==============================================================================
OCTypeID DatasetGetTypeID(void) {
    if (kDatasetID == kOCNotATypeID)
        kDatasetID = OCRegisterType("Dataset");
    return kDatasetID;
}
//==============================================================================
// MARK: - Finalizer, Equal, Description, DeepCopy
//==============================================================================
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
        STR("<Dataset dims=%lu vars=%lu tags=%lu title=\"%@\">"),
        (unsigned long)OCArrayGetCount(ds->dimensions),
        (unsigned long)OCArrayGetCount(ds->dependentVariables),
        (unsigned long)OCArrayGetCount(ds->tags),
        ds->title);
}
cJSON *impl_DatasetCreateJSON(const void *obj) {
    const DatasetRef ds = (const DatasetRef)obj;
    if (!ds) return cJSON_CreateNull();
    cJSON *json = cJSON_CreateObject();
    if (!json) return NULL;
    // dimensions (OCMutableArrayRef)
    if (ds->dimensions) {
        cJSON *dims_json = OCTypeCopyJSON((OCTypeRef)ds->dimensions);
        if (dims_json)
            cJSON_AddItemToObject(json, "dimensions", dims_json);
    }
    // dependentVariables (OCMutableArrayRef)
    if (ds->dependentVariables) {
        cJSON *depvars_json = OCTypeCopyJSON((OCTypeRef)ds->dependentVariables);
        if (depvars_json)
            cJSON_AddItemToObject(json, "dependentVariables", depvars_json);
    }
    // tags (OCMutableArrayRef)
    if (ds->tags) {
        cJSON *tags_json = OCTypeCopyJSON((OCTypeRef)ds->tags);
        if (tags_json)
            cJSON_AddItemToObject(json, "tags", tags_json);
    }
    // description (OCStringRef)
    if (ds->description) {
        cJSON *desc_json = OCTypeCopyJSON((OCTypeRef)ds->description);
        if (desc_json)
            cJSON_AddItemToObject(json, "description", desc_json);
    }
    // title (OCStringRef)
    if (ds->title) {
        cJSON *title_json = OCTypeCopyJSON((OCTypeRef)ds->title);
        if (title_json)
            cJSON_AddItemToObject(json, "title", title_json);
    }
    // focus (DatumRef)
    if (ds->focus) {
        cJSON *focus_json = OCTypeCopyJSON((OCTypeRef)ds->focus);
        if (focus_json)
            cJSON_AddItemToObject(json, "focus", focus_json);
    }
    // previousFocus (DatumRef)
    if (ds->previousFocus) {
        cJSON *prevfocus_json = OCTypeCopyJSON((OCTypeRef)ds->previousFocus);
        if (prevfocus_json)
            cJSON_AddItemToObject(json, "previousFocus", prevfocus_json);
    }
    // dimensionPrecedence (OCMutableIndexArrayRef)
    if (ds->dimensionPrecedence) {
        cJSON *dp_json = OCTypeCopyJSON((OCTypeRef)ds->dimensionPrecedence);
        if (dp_json)
            cJSON_AddItemToObject(json, "dimensionPrecedence", dp_json);
    }
    // metaData (OCDictionaryRef)
    if (ds->metaData) {
        cJSON *meta_json = OCTypeCopyJSON((OCTypeRef)ds->metaData);
        if (meta_json)
            cJSON_AddItemToObject(json, "metaData", meta_json);
    }
    return json;
}
static void *impl_DatasetDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    DatasetRef src = (DatasetRef)ptr;
    OCDictionaryRef dict = DatasetCopyAsDictionary(src, NULL);
    if (!dict) return NULL;

    OCStringRef err = NULL;
    DatasetRef copy = DatasetCreateFromDictionary(dict, &err);
    OCRelease(dict);
    OCRelease(err);  // Discard error, since deep copy failure isn't recoverable here

    return copy;
}
//==============================================================================
// MARK: - Allocation & Initialization
//==============================================================================
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
//==============================================================================
// Validate that
//  • dependentVariables is non-NULL & non-empty,
//  • every element is a DependentVariableRef,
//  • if dimensions != NULL, every element is some DimensionRef subtype,
//  • all dependentVariables have the same total “size” as produced by RMNCalculateSizeFromDimensions.
//==============================================================================
static bool impl_ValidateDatasetParameters(OCArrayRef dimensions,
                                           OCArrayRef dependentVariables) {
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
//==============================================================================
// MARK: - Public Constructor
//==============================================================================
DatasetRef DatasetCreate(
    OCArrayRef dimensions,
    OCArrayRef dimensionPrecedence,
    OCArrayRef dependentVariables,
    OCArrayRef tags,
    OCStringRef description,
    OCStringRef title,
    DatumRef focus,
    DatumRef previousFocus,
    OCDictionaryRef metaData) {
    // 1) validate inputs
    if (!impl_ValidateDatasetParameters(dimensions, dependentVariables))
        return NULL;
    // 2) allocate & initialize defaults
    DatasetRef ds = DatasetAllocate();
    if (!ds) return NULL;
    impl_InitDatasetFields(ds);
    // 3) copy dimensions array
    if (dimensions) {
        OCIndex n = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < n; i++) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
            DimensionRef dc = (DimensionRef)OCTypeDeepCopyMutable(d);
            OCArrayAppendValue(ds->dimensions, dc);
            OCRelease(dc);
        }
    }
    // 4) copy dependentVariables
    {
        OCIndex n = OCArrayGetCount(dependentVariables);
        for (OCIndex i = 0; i < n; i++) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, i);
            DependentVariableRef copy = DependentVariableCreateCopy(dv);
            OCArrayAppendValue(ds->dependentVariables, copy);
            OCRelease(copy);
        }
    }
    // 5) copy tags
    if (tags) {
        OCIndex n = OCArrayGetCount(tags);
        for (OCIndex i = 0; i < n; i++) {
            OCStringRef s = (OCStringRef)OCArrayGetValueAtIndex(tags, i);
            OCArrayAppendValue(ds->tags, s);
        }
    }
    // 6) build or default dimensionPrecedence
    OCIndex dimCount = OCArrayGetCount(ds->dimensions);
    if (dimensionPrecedence && OCArrayGetCount(dimensionPrecedence) == dimCount) {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCNumberRef idx = (OCNumberRef)OCArrayGetValueAtIndex(dimensionPrecedence, i);
            OCIndex tmpIdx = 0;
            OCNumberGetValue(idx, kOCNumberIntType, &tmpIdx);
            OCIndexArrayAppendValue(ds->dimensionPrecedence, tmpIdx);
        }
    } else {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCIndexArrayAppendValue(ds->dimensionPrecedence, i);
        }
    }
    // 7) copy simple fields (cast retain back into DatumRef)
    ds->description = description ? OCStringCreateCopy(description) : STR("");
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    // 8) copy metaData
    if (metaData) {
        OCRelease(ds->metaData);
        ds->metaData = OCTypeDeepCopyMutable(metaData);
    }
    return ds;
}
DatasetRef DatasetCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError && !*outError) *outError = STR("Input dictionary is NULL");
        return NULL;
    }

    DatasetRef ds = DatasetAllocate();
    if (!ds) {
        if (outError && !*outError) *outError = STR("Failed to allocate Dataset");
        return NULL;
    }

    impl_InitDatasetFields(ds);

    // — dimensions —
    OCArrayRef rawDims = (OCArrayRef)OCDictionaryGetValue(dict, STR("dimensions"));
    if (rawDims) {
        OCRelease(ds->dimensions);
        ds->dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        if (!ds->dimensions) {
            OCRelease(ds);
            if (outError && !*outError) *outError = STR("Failed to create dimensions array");
            return NULL;
        }

        OCIndex n = OCArrayGetCount(rawDims);
        for (OCIndex i = 0; i < n; i++) {
            OCTypeRef item = OCArrayGetValueAtIndex(rawDims, i);
            if (!item || OCGetTypeID(item) != OCDictionaryGetTypeID()) {
                if (outError && !*outError) *outError = STR("Invalid dimension entry (expected dictionary)");
                OCRelease(ds);
                return NULL;
            }

            OCStringRef err = NULL;
            DimensionRef d = DimensionCreateFromDictionary((OCDictionaryRef)item, &err);
            if (!d) {
                if (outError && err && !*outError) *outError = OCStringCreateCopy(err);
                OCRelease(err);
                OCRelease(ds);
                return NULL;
            }
            OCRelease(err);

            if (!OCArrayAppendValue(ds->dimensions, d)) {
                OCRelease(d);
                if (outError && !*outError) *outError = STR("Failed to append Dimension");
                OCRelease(ds);
                return NULL;
            }
            OCRelease(d);
        }
    }

    // — dimensionPrecedence —
    OCIndexArrayRef rawPrec = (OCIndexArrayRef)OCDictionaryGetValue(dict, STR("dimension_precedence"));
    if (rawPrec) {
        OCRelease(ds->dimensionPrecedence);
        ds->dimensionPrecedence = OCIndexArrayCreateMutableCopy(rawPrec);
        if (!ds->dimensionPrecedence) {
            if (outError && !*outError) *outError = STR("Failed to copy dimension_precedence");
            OCRelease(ds);
            return NULL;
        }
    }

    // — dependentVariables —
    OCArrayRef rawDVs = (OCArrayRef)OCDictionaryGetValue(dict, STR("dependent_variables"));
    if (rawDVs) {
        OCRelease(ds->dependentVariables);
        ds->dependentVariables = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        if (!ds->dependentVariables) {
            if (outError && !*outError) *outError = STR("Failed to create dependentVariables array");
            OCRelease(ds);
            return NULL;
        }

        OCIndex m = OCArrayGetCount(rawDVs);
        for (OCIndex i = 0; i < m; i++) {
            OCTypeRef item = OCArrayGetValueAtIndex(rawDVs, i);
            if (!item || OCGetTypeID(item) != OCDictionaryGetTypeID()) {
                if (outError && !*outError) *outError = STR("Invalid dependent variable entry (expected dictionary)");
                OCRelease(ds);
                return NULL;
            }

            OCStringRef err = NULL;
            DependentVariableRef dv = DependentVariableCreateFromDictionary((OCDictionaryRef)item, &err);
            if (!dv) {
                if (outError && err && !*outError) *outError = OCStringCreateCopy(err);
                OCRelease(err);
                OCRelease(ds);
                return NULL;
            }
            OCRelease(err);

            if (!OCArrayAppendValue(ds->dependentVariables, dv)) {
                OCRelease(dv);
                if (outError && !*outError) *outError = STR("Failed to append DependentVariable");
                OCRelease(ds);
                return NULL;
            }
            OCRelease(dv);
        }
    }

    // — tags —
    OCArrayRef rawTags = (OCArrayRef)OCDictionaryGetValue(dict, STR("tags"));
    if (rawTags) {
        OCRelease(ds->tags);
        ds->tags = OCArrayCreateMutableCopy(rawTags);
        if (!ds->tags) {
            if (outError && !*outError) *outError = STR("Failed to copy tags array");
            OCRelease(ds);
            return NULL;
        }
    }

    // — description & title —
    OCStringRef tmpStr;
    if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR("description")))) {
        OCRelease(ds->description);
        ds->description = OCStringCreateCopy(tmpStr);
    }
    if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR("title")))) {
        OCRelease(ds->title);
        ds->title = OCStringCreateCopy(tmpStr);
    }

    // — focus & previous_focus —
    OCDictionaryRef tmpDict;
    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("focus")))) {
        OCRelease(ds->focus);
        ds->focus = DatumCreateFromDictionary(tmpDict, outError);
        if (!ds->focus && outError && *outError) {
            OCRelease(ds);
            return NULL;
        }
    }

    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("previous_focus")))) {
        OCRelease(ds->previousFocus);
        ds->previousFocus = DatumCreateFromDictionary(tmpDict, outError);
        if (!ds->previousFocus && outError && *outError) {
            OCRelease(ds);
            return NULL;
        }
    }

    // — meta_data —
    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metadata")))) {
        OCRelease(ds->metaData);
        ds->metaData = (OCDictionaryRef)OCTypeDeepCopyMutable(tmpDict);
        if (!ds->metaData) {
            if (outError && !*outError) *outError = STR("Failed to copy metadata dictionary");
            OCRelease(ds);
            return NULL;
        }
    }

    return ds;
}
/// Ensures the directory `dir` exists (you’ll need to implement or link your favorite mkdir-recursive).
static bool ensure_directory(const char *dir, OCStringRef *outError);
/// Export the entire dataset:
///   - writes JSON to `json_path`
///   - for each DV with type == "external", writes its blobs to `binary_dir`/components_url
bool ExportDataset(DatasetRef ds, const char *json_path, const char *binary_dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ds || !json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return false;
    }
    // 1) Build the cJSON DOM for the dataset (no I/O here)
    cJSON *root = DatasetCreateJSON((OCTypeRef)ds);
    if (!root) {
        if (outError) *outError = STR("Failed to serialize Dataset to JSON");
        return false;
    }
    // 2) Render JSON to string
    char *json_text = cJSON_Print(root);
    cJSON_Delete(root);
    if (!json_text) {
        if (outError) *outError = STR("Failed to render JSON text");
        return false;
    }
    // 3) Write the JSON file
    FILE *jf = fopen(json_path, "wb");
    if (!jf) {
        free(json_text);
        if (outError) *outError = STR("Unable to open JSON output file");
        return false;
    }
    fwrite(json_text, 1, strlen(json_text), jf);
    fclose(jf);
    free(json_text);
    // 4) Make sure the binary export directory exists
    if (!ensure_directory(binary_dir, outError)) {
        return false;
    }
    // 5) Walk all dependent variables, export externals
    OCIndex dvCount = DatasetGetDependentVariableCount(ds);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = DatasetGetDependentVariableAtIndex(ds, i);
        if (!dv) continue;
        if (DependentVariableShouldSerializeExternally(dv)) {
            OCStringRef url = DependentVariableGetComponentsURL(dv);
            if (!url) {
                if (outError) *outError = STR("external DV missing components_url");
                return false;
            }
            const char *relpath = OCStringGetCString(url);
            char fullpath[4096];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", binary_dir, relpath);
            // Ensure parent directories exist
            if (!ensure_parent_dirs(fullpath, outError)) {
                return false;
            }
            FILE *bf = fopen(fullpath, "wb");
            if (!bf) {
                if (outError) {
                    OCStringRef tmpPath = OCStringCreateWithCString(fullpath);
                    *outError = OCStringCreateWithFormat(
                        STR("Unable to open binary output file \"%@\""), tmpPath);
                    OCRelease(tmpPath);
                }
                return false;
            }
            OCIndex ncomps = DependentVariableGetComponentCount(dv);
            for (OCIndex ci = 0; ci < ncomps; ++ci) {
                OCDataRef blob = DependentVariableGetComponentAtIndex(dv, ci);
                const void *bytes = OCDataGetBytesPtr(blob);
                uint64_t len = OCDataGetLength(blob);
                if (fwrite(bytes, 1, (size_t)len, bf) != len) {
                    fclose(bf);
                    if (outError) *outError = STR("Failed writing binary component");
                    OCRelease(ds); 
                    return false;
                }
            }
            fclose(bf);
        }
    }
    return true;
}
/// Read an entire file into a malloc()’d buffer; returns NULL on error.
static uint8_t *read_file_bytes(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    struct stat st;
    if (fstat(fileno(f), &st) != 0) {
        fclose(f);
        return NULL;
    }
    size_t len = (size_t)st.st_size;
    uint8_t *buf = malloc(len);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    if (fread(buf, 1, len, f) != len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_len = len;
    return buf;
}
/// Ensure that all directories in `fullpath` exist (mkdir -p style).
static bool ensure_parent_dirs(const char *fullpath, OCStringRef *outError) {
    char tmp[4096];
    strncpy(tmp, fullpath, sizeof(tmp));
    tmp[sizeof(tmp) - 1] = 0;
    char *dir = dirname(tmp);
    struct stat st;
    if (stat(dir, &st) == 0) return true;  // already exists
    // recursively create parent
    if (!ensure_parent_dirs(dir, outError)) return false;
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        if (outError) {
            OCStringRef tmp = OCStringCreateWithCString(dir);
            *outError = OCStringCreateWithFormat(
                STR("Failed to create directory \"%@\""), tmp);
            OCRelease(tmp);
        }
        return false;
    }
    return true;
}
/// Inverse of ExportDataset: reads JSON + external blobs and reconstructs a DatasetRef.
DatasetRef ImportDataset(const char *json_path, const char *binary_dir, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return NULL;
    }
    // Step 1: Load JSON file
    FILE *jf = fopen(json_path, "rb");
    if (!jf) {
        if (outError) *outError = STR("Unable to open JSON file");
        return NULL;
    }
    fseek(jf, 0, SEEK_END);
    long fsize = ftell(jf);
    fseek(jf, 0, SEEK_SET);
    char *json_text = malloc((size_t)fsize + 1);
    if (!json_text) {
        fclose(jf);
        if (outError) *outError = STR("Memory allocation failed for JSON buffer");
        return NULL;
    }
    size_t read_bytes = fread(json_text, 1, (size_t)fsize, jf);
    fclose(jf);
    if (read_bytes != (size_t)fsize) {
        free(json_text);
        if (outError) *outError = STR("Failed to read entire JSON file");
        return NULL;
    }
    json_text[fsize] = '\0';
    // Step 2: Parse JSON
    cJSON *root = cJSON_Parse(json_text);
    free(json_text);
    if (!root) {
        if (outError) *outError = STR("Invalid JSON");
        return NULL;
    }
    // Step 3: Convert JSON → OCDictionary
    OCTypeRef obj = OCTypeCreateFromJSON((OCTypeRef)root);
    cJSON_Delete(root);
    if (!obj || OCGetTypeID(obj) != OCDictionaryGetTypeID()) {
        if (outError) *outError = STR("Expected JSON object at top level");
        if (obj) OCRelease(obj);
        return NULL;
    }
    OCDictionaryRef dsDict = (OCDictionaryRef)obj;
    DatasetRef ds = DatasetCreateFromDictionary(dsDict, outError);
    OCRelease(dsDict);
    if (!ds) return NULL;
    // Step 4: Load external binary data for dependent variables
    OCIndex ndv = DatasetGetDependentVariableCount(ds);
    for (OCIndex i = 0; i < ndv; ++i) {
        DependentVariableRef dv = DatasetGetDependentVariableAtIndex(ds, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv)) continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        const char *rel = url ? OCStringGetCString(url) : NULL;
        if (!rel || strlen(rel) == 0) {
            if (outError) *outError = STR("Missing or invalid components_url in external DV");
            OCRelease(ds);
            return NULL;
        }
        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", binary_dir, rel);
        full[sizeof(full) - 1] = '\0';
        size_t total_bytes = 0;
        uint8_t *all = read_file_bytes(full, &total_bytes);
        if (!all) {
            if (outError) {
                OCStringRef tmpPath = OCStringCreateWithCString(full);
                *outError = OCStringCreateWithFormat(STR("Failed to read binary file \"%@\""), tmpPath);
                OCRelease(tmpPath);
            }
            OCRelease(ds);
            return NULL;
        }
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        if (ncomps == 0) {
            ncomps = DependentVariableComponentsCountFromQuantityType(
                DependentVariableGetQuantityType(dv));
        }
        size_t elemSize = SIQuantityElementSize((SIQuantityRef)dv);
        OCIndex npts = DependentVariableGetSize(dv);
        size_t chunk = (size_t)npts * elemSize;
        if (chunk * (size_t)ncomps != total_bytes) {
            free(all);
            if (outError && !*outError)
                *outError = STR("Binary size mismatch for DV");
            OCRelease(ds);
            return NULL;
        }
        OCMutableArrayRef comps = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        if (!comps) {
            free(all);
            if (outError) *outError = STR("Failed to allocate components array");
            OCRelease(ds);
            return NULL;
        }
        for (OCIndex ci = 0; ci < ncomps; ++ci) {
            OCMutableDataRef buf = OCDataCreateMutable(chunk);
            if (!buf || !OCDataAppendBytes(buf, all + (size_t)ci * chunk, chunk)) {
                OCRelease(buf);
                OCRelease(comps);
                free(all);
                if (outError) *outError = STR("Failed to create component buffer");
                OCRelease(ds);
                return NULL;
            }
            OCArrayAppendValue(comps, buf);
            OCRelease(buf);
        }
        free(all);
        if (!DependentVariableSetComponents(dv, comps)) {
            if (outError && !*outError)
                *outError = STR("Failed to install components into DependentVariable");
            OCRelease(comps);
            OCRelease(ds);
            return NULL;
        }
        OCRelease(comps);
    }
    return ds;
}
OCDictionaryRef DatasetCopyAsDictionary(DatasetRef ds, const char *exportDirectory) {
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
        OCStringRef desc_copy = OCStringCreateCopy(ds->description);
        if (desc_copy) {
            OCDictionarySetValue(dict, STR("description"), desc_copy);
            OCRelease(desc_copy);
        }
    }
    if (ds->title) {
        OCStringRef title_copy = OCStringCreateCopy(ds->title);
        if (title_copy) {
            OCDictionarySetValue(dict, STR("title"), title_copy);
            OCRelease(title_copy);
        }
    }

    // — dimensions —
    if (ds->dimensions) {
        OCIndex n = OCArrayGetCount(ds->dimensions);
        OCMutableArrayRef dims_arr = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        if (dims_arr) {
            for (OCIndex i = 0; i < n; ++i) {
                DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(ds->dimensions, i);
                OCDictionaryRef d_dict = DimensionCopyAsDictionary(d);
                if (d_dict) {
                    OCArrayAppendValue(dims_arr, d_dict);
                    OCRelease(d_dict);
                }
            }
            OCDictionarySetValue(dict, STR("dimensions"), dims_arr);
            OCRelease(dims_arr);
        }
    }

    // — dimension_precedence —
    if (ds->dimensionPrecedence) {
        OCIndexArrayRef prec_copy = OCIndexArrayCreateMutableCopy(ds->dimensionPrecedence);
        if (prec_copy) {
            OCDictionarySetValue(dict, STR("dimension_precedence"), prec_copy);
            OCRelease(prec_copy);
        }
    }

    // — dependent_variables —
    if (ds->dependentVariables) {
        OCStringRef err = NULL;
        if (exportDirectory) {
            if (!DatasetExportDependentVariableBlobs(ds, exportDirectory, &err)) {
                // optionally log or retain `err` for diagnostics
                OCRelease(err);
            }
        }

        OCIndex m = OCArrayGetCount(ds->dependentVariables);
        OCMutableArrayRef dvs_arr = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        if (dvs_arr) {
            for (OCIndex i = 0; i < m; ++i) {
                DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(ds->dependentVariables, i);
                OCDictionaryRef dv_dict = DependentVariableCopyAsDictionary(dv);
                if (dv_dict) {
                    OCArrayAppendValue(dvs_arr, dv_dict);
                    OCRelease(dv_dict);
                }
            }
            OCDictionarySetValue(dict, STR("dependent_variables"), dvs_arr);
            OCRelease(dvs_arr);
        }
    }

    // — focus & previous_focus —
    if (ds->focus) {
        OCDictionaryRef fdict = DatumCopyAsDictionary(ds->focus);
        if (fdict) {
            OCDictionarySetValue(dict, STR("focus"), fdict);
            OCRelease(fdict);
        }
    }
    if (ds->previousFocus) {
        OCDictionaryRef pf = DatumCopyAsDictionary(ds->previousFocus);
        if (pf) {
            OCDictionarySetValue(dict, STR("previous_focus"), pf);
            OCRelease(pf);
        }
    }

    // — metadata —
    if (ds->metaData) {
        OCMutableDictionaryRef md_copy = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(ds->metaData);
        if (md_copy) {
            OCDictionarySetValue(dict, STR("metadata"), md_copy);
            OCRelease(md_copy);
        }
    }

    return (OCDictionaryRef)dict;
}
//-----------------------
// Accessors & Mutators
//-----------------------
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
