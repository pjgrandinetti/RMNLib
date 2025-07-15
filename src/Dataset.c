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
#include <time.h>
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
static void *
impl_DatasetDeepCopy(const void *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    const struct impl_Dataset *src = (const struct impl_Dataset *)ptr;
    // 1) Allocate and zero the destination
    struct impl_Dataset *dst = calloc(1, sizeof(*dst));
    if (dst == NULL) {
        return NULL;
    }
    // 2) Copy the OCBase header (type tag + refcount, etc.)
    memcpy(&dst->base, &src->base, sizeof(OCBase));
    // 3) Deep-copy all OCTypeRef fields via OCTypeDeepCopy:
    dst->version = src->version
                       ? (OCStringRef)OCTypeDeepCopy(src->version)
                       : NULL;
    dst->timestamp = src->timestamp
                         ? (OCStringRef)OCTypeDeepCopy(src->timestamp)
                         : NULL;
    dst->geographicCoordinate = src->geographicCoordinate
                                    ? (GeographicCoordinateRef)OCTypeDeepCopy(src->geographicCoordinate)
                                    : NULL;
    // 4) Primitive field
    dst->readOnly = src->readOnly;
    // 5) Mutable arrays
    dst->dimensions = src->dimensions
                          ? (OCMutableArrayRef)OCTypeDeepCopy(src->dimensions)
                          : NULL;
    dst->dependentVariables = src->dependentVariables
                                  ? (OCMutableArrayRef)OCTypeDeepCopy(src->dependentVariables)
                                  : NULL;
    dst->tags = src->tags
                    ? (OCMutableArrayRef)OCTypeDeepCopy(src->tags)
                    : NULL;
    // 6) More OCTypeRef fields
    dst->description = src->description
                           ? (OCStringRef)OCTypeDeepCopy(src->description)
                           : NULL;
    dst->title = src->title
                     ? (OCStringRef)OCTypeDeepCopy(src->title)
                     : NULL;
    dst->focus = src->focus
                     ? (DatumRef)OCTypeDeepCopy(src->focus)
                     : NULL;
    dst->previousFocus = src->previousFocus
                             ? (DatumRef)OCTypeDeepCopy(src->previousFocus)
                             : NULL;
    dst->dimensionPrecedence = src->dimensionPrecedence
                                   ? (OCMutableIndexArrayRef)OCTypeDeepCopy(src->dimensionPrecedence)
                                   : NULL;
    dst->metaData = src->metaData
                        ? (OCDictionaryRef)OCTypeDeepCopy(src->metaData)
                        : NULL;
    return dst;
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
static bool impl_ValidateDatasetParameters(OCArrayRef dimensions,
                                           OCArrayRef dependentVariables,
                                           OCStringRef *outError) {
    // clear any prior error
    if (outError) *outError = NULL;
    // 1) must have at least one DV
    OCIndex dvCount = dependentVariables
                          ? OCArrayGetCount(dependentVariables)
                          : 0;
    if (dvCount == 0) {
        if (outError)
            *outError = STR("Validation failed: no dependent variables provided");
        return false;
    }
    // 2) compute expected length from dimensions (defaults to 1 if dimensions==NULL)
    OCIndex expectedSize = RMNCalculateSizeFromDimensions(dimensions);
    // For 0D datasets (no dimensions), use the size of the first dependent variable
    if (expectedSize == 1 && (!dimensions || OCArrayGetCount(dimensions) == 0)) {
        DependentVariableRef firstDV = (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, 0);
        if (firstDV && OCGetTypeID(firstDV) == DependentVariableGetTypeID()) {
            expectedSize = DependentVariableGetSize(firstDV);
        }
    }
    // Check for SparseSampling override
    OCIndex sparseSize = -1;
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, i);
        if (!dv) continue;
        SparseSamplingRef ss = DependentVariableGetSparseSampling(dv);
        if (!ss) continue;
        OCIndex fullDimCount = OCArrayGetCount(dimensions);
        OCIndex sparseDimCount = OCIndexSetGetCount(SparseSamplingGetDimensionIndexes(ss));
        if (sparseDimCount == 0) {
            // 0-dimensional sparse sampling: all dimensions are dense
            // Expected size should be the number of sparse grid vertices
            OCIndex flatCount = OCArrayGetCount(SparseSamplingGetSparseGridVertexes(ss));
            sparseSize = flatCount;  // Each vertex represents one data point
            break;
        }
        // For fully sparse datasets (all dimensions are sparse),
        // the expected size equals the actual data size
        if (sparseDimCount == fullDimCount) {
            sparseSize = DependentVariableGetSize(dv);
            break;
        }
        OCIndex fullGridSize = 1;
        for (OCIndex j = 0; j < fullDimCount; ++j) {
            if (!OCIndexSetContainsIndex(SparseSamplingGetDimensionIndexes(ss), j)) {
                DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, j);
                fullGridSize *= DimensionGetCount(d);
            }
        }
        OCIndex flatCount = OCArrayGetCount(SparseSamplingGetSparseGridVertexes(ss));
        OCIndex nVerts = flatCount / sparseDimCount;
        sparseSize = nVerts * fullGridSize;
        break;
    }
    if (sparseSize > 0) {
        expectedSize = sparseSize;
    }
    // 3) validate each dependent variable
    for (OCIndex i = 0; i < dvCount; ++i) {
        const void *obj = OCArrayGetValueAtIndex(dependentVariables, i);
        if (OCGetTypeID(obj) != DependentVariableGetTypeID()) {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("Validation failed: dependent variable at index %ld is not a DependentVariable"),
                    (long)i);
            }
            return false;
        }
        DependentVariableRef dv = (DependentVariableRef)obj;
        OCIndex dvSize = DependentVariableGetSize(dv);
        // if the size doesn't match, but it's still an external DV, skip it here
        if (dvSize != expectedSize) {
            if (DependentVariableShouldSerializeExternally(dv)) {
                // we'll populate its components (and fix the size) later in DatasetCreateWithImport
                continue;
            }
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("Validation failed: size mismatch for DV[%ld]: got %ld, expected %ld"),
                    (long)i, (long)dvSize, (long)expectedSize);
            }
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
    OCDictionaryRef metaData,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    // — allow blank datasets (zero DVs) —
    OCIndex dvCount = dependentVariables ? OCArrayGetCount(dependentVariables) : 0;
    if (dvCount > 0) {
        // only validate when there's at least one DV
        OCStringRef valErr = NULL;
        if (!impl_ValidateDatasetParameters(dimensions,
                                            dependentVariables,
                                            &valErr)) {
            // propagate the specific validation error
            if (outError) *outError = valErr;
            return NULL;
        }
    }
    // allocate
    DatasetRef ds = DatasetAllocate();
    if (!ds) {
        // only set generic if no other error
        if (outError && !*outError)
            *outError = STR("Dataset creation failed: unable to allocate dataset");
        return NULL;
    }
    impl_InitDatasetFields(ds);
    // — copy dimensions —
    if (dimensions) {
        OCIndex nDims = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < nDims; ++i) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
            DimensionRef dCopy = (DimensionRef)OCTypeDeepCopyMutable(d);
            OCArrayAppendValue(ds->dimensions, dCopy);
            OCRelease(dCopy);
        }
    }
    // — copy dependentVariables (may be zero) —
    if (dependentVariables) {
        OCIndex nDVs = OCArrayGetCount(dependentVariables);
        for (OCIndex i = 0; i < nDVs; ++i) {
            DependentVariableRef dv =
                (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, i);
            DependentVariableRef toInsert;
            if (DependentVariableShouldSerializeExternally(dv)) {
                // leave the external DV as‐is (we’ll import its blob later)
                toInsert = (DependentVariableRef)OCRetain(dv);
            } else {
                // deep copy the internal DV - but optimize SparseSampling encoding
                SparseSamplingRef sparseSampling = DependentVariableGetSparseSampling(dv);
                OCStringRef originalEncoding = NULL;
                // Temporarily switch to base64 encoding to avoid OCNumber creation during deep copy
                if (sparseSampling) {
                    originalEncoding = SparseSamplingGetEncoding(sparseSampling);
                    if (originalEncoding) OCRetain(originalEncoding);  // Keep reference to original
                    SparseSamplingSetEncoding(sparseSampling, STR("base64"));
                }
                // Perform the deep copy with optimized encoding
                toInsert = DependentVariableCreateCopy(dv);
                // Also ensure the copied DV uses base64 encoding to avoid future OCNumber creation
                SparseSamplingRef copiedSparseSampling = DependentVariableGetSparseSampling(toInsert);
                if (copiedSparseSampling) {
                    SparseSamplingSetEncoding(copiedSparseSampling, STR("base64"));
                }
                // Restore original encoding on source object
                if (sparseSampling && originalEncoding) {
                    SparseSamplingSetEncoding(sparseSampling, originalEncoding);
                    OCRelease(originalEncoding);
                }
            }
            OCArrayAppendValue(ds->dependentVariables, toInsert);
            OCRelease(toInsert);
        }
    }
    // — copy tags —
    if (tags) {
        OCIndex nTags = OCArrayGetCount(tags);
        for (OCIndex i = 0; i < nTags; ++i) {
            OCStringRef s = (OCStringRef)OCArrayGetValueAtIndex(tags, i);
            OCArrayAppendValue(ds->tags, s);
        }
    }
    // — set dimensionPrecedence (default to 0..N-1 if missing/mismatched) —
    OCIndex dimCount = OCArrayGetCount(ds->dimensions);
    if (dimensionPrecedence && OCIndexArrayGetCount(dimensionPrecedence) == dimCount) {
        for (OCIndex i = 0; i < dimCount; ++i) {
            OCIndex idx = OCIndexArrayGetValueAtIndex(dimensionPrecedence, i);
            OCIndexArrayAppendValue(ds->dimensionPrecedence, idx);
        }
    } else {
        for (OCIndex i = 0; i < dimCount; ++i) {
            OCIndexArrayAppendValue(ds->dimensionPrecedence, i);
        }
    }
    // — copy simple fields —
    ds->description = description ? OCStringCreateCopy(description) : STR("");
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    // — copy metadata if present —
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
        if (outError) *outError = STR("Dataset creation failed: input dictionary is NULL");
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
    // --- dimensions ---
    OCArrayRef rawDims = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDimensionsKey));
    if (rawDims) {
        OCIndex n = OCArrayGetCount(rawDims);
        OCMutableArrayRef tmp = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        if (!tmp) {
            if (outError) *outError = STR("Dataset creation failed: cannot allocate dimensions array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < n; ++i) {
            OCDictionaryRef ddict = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDims, i);
            if (OCGetTypeID(ddict) != OCDictionaryGetTypeID()) {
                if (outError) *outError = STR("Dataset creation failed: invalid dimension entry");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DimensionRef d = DimensionCreateFromDictionary(ddict, &err);
            if (!d) {
                if (outError) {
                    *outError = err
                                    ? OCStringCreateCopy(err)
                                    : STR("Dataset creation failed: error parsing dimension");
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
    // --- dimension precedence ---
    OCIndexArrayRef rawPrec = (OCIndexArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDimensionPrecedenceKey));
    if (rawPrec) {
        dimPrec = OCIndexArrayCreateMutableCopy(rawPrec);
    }
    // --- dependent variables ---
    OCArrayRef rawDVs = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetDependentVariablesKey));
    if (rawDVs) {
        OCIndex m = OCArrayGetCount(rawDVs);
        OCMutableArrayRef tmp = OCArrayCreateMutable(m, &kOCTypeArrayCallBacks);
        if (!tmp) {
            if (outError) *outError = STR("Dataset creation failed: cannot allocate dependent-variables array");
            goto cleanup;
        }
        for (OCIndex i = 0; i < m; ++i) {
            OCDictionaryRef dd = (OCDictionaryRef)OCArrayGetValueAtIndex(rawDVs, i);
            if (OCGetTypeID(dd) != OCDictionaryGetTypeID()) {
                if (outError) *outError = STR("Dataset creation failed: invalid dependent-variable entry");
                OCRelease(tmp);
                goto cleanup;
            }
            OCStringRef err = NULL;
            DependentVariableRef dv = DependentVariableCreateFromDictionary(dd, &err);
            if (!dv) {
                if (outError) {
                    *outError = err
                                    ? OCStringCreateCopy(err)
                                    : STR("Dataset creation failed: error parsing dependent variable");
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
    // If no dependent variables, use an empty array
    if (!dvs) {
        dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    }
    // --- tags ---
    OCArrayRef rawTags = (OCArrayRef)OCDictionaryGetValue(dict, STR(kDatasetTagsKey));
    if (rawTags) {
        tags = OCArrayCreateMutableCopy(rawTags);
    }
    // --- description & title ---
    OCStringRef s;
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR(kDatasetDescriptionKey)))) {
        desc = OCStringCreateCopy(s);
    }
    if ((s = (OCStringRef)OCDictionaryGetValue(dict, STR(kDatasetTitleKey)))) {
        title = OCStringCreateCopy(s);
    }
    // --- focus & previous_focus ---
    OCDictionaryRef ddict;
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetFocusKey)))) {
        focus = DatumCreateFromDictionary(ddict, outError);
        if (!focus) goto cleanup;
    }
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetPreviousFocusKey)))) {
        prevFocus = DatumCreateFromDictionary(ddict, outError);
        if (!prevFocus) goto cleanup;
    }
    // --- metadata ---
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetMetadataKey)))) {
        metadata = (OCDictionaryRef)OCTypeDeepCopyMutable(ddict);
        if (!metadata) {
            if (outError) *outError = STR("Dataset creation failed: cannot copy metadata");
            goto cleanup;
        }
    }
    // Envelope properties
    OCStringRef version = OCDictionaryGetValue(dict, STR(kDatasetVersionKey));
    OCStringRef timestamp = OCDictionaryGetValue(dict, STR(kDatasetTimestampKey));
    OCBooleanRef roflag = OCDictionaryGetValue(dict, STR(kDatasetReadOnlyKey));
    OCDictionaryRef geoDict = OCDictionaryGetValue(dict, STR(kDatasetGeoCoordinateKey));
    // --- create dataset ---
    ds = DatasetCreate(dims, dimPrec, dvs, tags,
                       desc, title, focus, prevFocus, metadata, outError);
    if (!ds) {
        goto cleanup;
    }
    // Overwrite envelope fields
    if (version && OCGetTypeID(version) == OCStringGetTypeID()) {
        OCRelease(ds->version);
        ds->version = OCStringCreateCopy(version);
    }
    if (timestamp && OCGetTypeID(timestamp) == OCStringGetTypeID()) {
        OCRelease(ds->timestamp);
        ds->timestamp = OCStringCreateCopy(timestamp);
    }
    if (roflag && OCGetTypeID(roflag) == OCBooleanGetTypeID()) {
        ds->readOnly = OCBooleanGetValue(roflag);
    }
    if (geoDict && OCGetTypeID(geoDict) == OCDictionaryGetTypeID()) {
        OCStringRef gcErr = NULL;
        ds->geographicCoordinate = GeographicCoordinateCreateFromDictionary(geoDict, &gcErr);
        OCRelease(gcErr);
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
static OCDictionaryRef DatasetDictionaryCreateFromJSON(cJSON *json,
                                                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Top‐level must be an object
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Dataset JSON must be an object");
        return NULL;
    }
    // 2) Unwrap the “csdm” envelope
    cJSON *inner = cJSON_GetObjectItemCaseSensitive(json, kDatasetCsdmEnvelopeKey);
    if (!inner || !cJSON_IsObject(inner)) {
        if (outError) *outError = STR("Missing or invalid \"csdm\" envelope");
        return NULL;
    }
    json = inner;
    // 3) Build a mutable dictionary for all of the fields
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        if (outError) *outError = STR("Out of memory creating dataset dictionary");
        return NULL;
    }
    // version (REQUIRED field in CSDM specification)
    cJSON *entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetVersionKey);
    if (!entry || !cJSON_IsString(entry)) {
        if (outError) *outError = STR("Dataset import failed: missing required \"version\" field");
        OCRelease(dict);
        return NULL;
    }
    OCStringRef v = OCStringCreateWithCString(entry->valuestring);
    OCDictionarySetValue(dict, STR(kDatasetVersionKey), v);
    OCRelease(v);
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
        bool ro = cJSON_IsTrue(entry);
        OCDictionarySetValue(dict,
                             STR(kDatasetReadOnlyKey),
                             OCBooleanGetWithBool(ro));
    }
    // geographic_coordinate
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetGeoCoordinateKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef gcErr = NULL;
        GeographicCoordinateRef gc =
            GeographicCoordinateCreateFromJSON(entry, &gcErr);
        if (!gc) {
            if (outError) *outError = gcErr
                                          ? OCStringCreateCopy(gcErr)
                                          : STR("Failed to parse geographic_coordinate");
            OCRelease(gcErr);
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef gcDict = GeographicCoordinateCopyAsDictionary(gc);
        OCRelease(gc);
        OCDictionarySetValue(dict,
                             STR(kDatasetGeoCoordinateKey),
                             gcDict);
        OCRelease(gcDict);
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
                    if (outError) *outError = dimErr
                                                  ? OCStringCreateCopy(dimErr)
                                                  : STR("Failed to parse a dimension");
                    OCRelease(dimErr);
                    OCRelease(dimsArr);
                    OCRelease(dict);
                    return NULL;
                }
                OCDictionaryRef dd = DimensionCopyAsDictionary(dim);
                if (!dd) {
                    OCRelease(dim);
                    OCRelease(dimsArr);
                    OCRelease(dict);
                    if (outError) *outError = STR("Failed to convert dimension to dictionary");
                    return NULL;
                }
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
    // ————— dependent_variables —————
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDependentVariablesKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableArrayRef dvsArr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        cJSON *e;
        cJSON_ArrayForEach(e, entry) {
            if (cJSON_IsObject(e)) {
                OCStringRef dvErr = NULL;
                // Use the JSON→dict helper to preserve “external”/URL
                OCDictionaryRef ddv =
                    DependentVariableDictionaryCreateFromJSON(e, &dvErr);
                if (!ddv) {
                    if (outError)
                        *outError = dvErr
                                        ? OCStringCreateCopy(dvErr)
                                        : STR("Failed to parse a dependent variable");
                    OCRelease(dvErr);
                    OCRelease(dvsArr);
                    OCRelease(dict);
                    return NULL;
                }
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
            if (outError) *outError = fErr
                                          ? OCStringCreateCopy(fErr)
                                          : STR("Failed to parse focus");
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
            if (outError) *outError = pfErr
                                          ? OCStringCreateCopy(pfErr)
                                          : STR("Failed to parse previous_focus");
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
// ————— DatasetCreateFromJSON —————
DatasetRef DatasetCreateFromJSON(cJSON *root, OCStringRef *outError) {
    if (outError) *outError = NULL;
    // Must have valid JSON object
    if (!root) {
        if (outError) *outError = STR("Input JSON is NULL");
        return NULL;
    }
    // Step 1: Convert JSON to internal dictionary
    OCDictionaryRef dict = DatasetDictionaryCreateFromJSON(root, outError);
    if (!dict) {
        // outError already set by DatasetDictionaryCreateFromJSON
        return NULL;
    }
    // Step 2: Build the Dataset from the dictionary
    DatasetRef ds = DatasetCreateFromDictionary(dict, outError);
    OCRelease(dict);
    if (!ds) {
        // If the dictionary step succeeded but DatasetCreateFromDictionary
        // failed without setting outError, provide a fallback message.
        if (outError && !*outError) {
            *outError = STR("Failed to construct Dataset from dictionary");
        }
        return NULL;
    }
    return ds;
}
#pragma endregion Creators
#pragma region Export/Import
/// Helper: parse a components_url and extract the relative path
/// For URLs like "file:./path/to/file", returns "./path/to/file"
/// For non-file URLs or plain paths, returns the input unchanged
static const char *parse_components_url_path(const char *url) {
    if (!url) return NULL;
    // Check if it starts with "file:"
    if (strncmp(url, "file:", 5) == 0) {
        return url + 5;  // Skip the "file:" prefix
    }
    // Check if it's an HTTP/HTTPS URL - extract just the filename
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        const char *last_slash = strrchr(url, '/');
        if (last_slash && last_slash[1] != '\0') {
            return last_slash + 1;  // Return filename part
        }
    }
    // For any other scheme or plain paths, return as-is
    return url;
}
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
// ————— DatasetExport —————
bool DatasetExport(DatasetRef ds,
                   const char *json_path,
                   const char *binary_dir,
                   OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ds || !json_path || !binary_dir) {
        if (outError) *outError = STR("Invalid arguments");
        return false;
    }
    // 0) decide extension
    bool hasExternal = false;
    OCArrayRef dvsArray = DatasetGetDependentVariables(ds);
    OCIndex dvCount = dvsArray ? OCArrayGetCount(dvsArray) : 0;
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv =
            (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
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
    // 1) build full in-memory dictionary
    OCDictionaryRef core = DatasetCopyAsDictionary(ds);
    if (!core) {
        if (outError) *outError = STR("Failed to create dictionary from Dataset");
        return false;
    }
    // 1a) strip inline components/encoding on externals
    {
        OCStringRef keyDVList = STR(kDatasetDependentVariablesKey);
        OCStringRef keyType = STR(kDependentVariableTypeKey);
        OCStringRef keyExternal = STR(kDependentVariableComponentTypeValueExternal);
        OCStringRef keyComp = STR(kDependentVariableComponentsKey);
        OCStringRef keyEnc = STR(kDependentVariableEncodingKey);
        OCMutableDictionaryRef mcore = (OCMutableDictionaryRef)core;
        OCArrayRef dvDicts = OCDictionaryGetValue(mcore, keyDVList);
        if (dvDicts) {
            OCIndex n = OCArrayGetCount(dvDicts);
            for (OCIndex i = 0; i < n; ++i) {
                OCDictionaryRef dvDict =
                    (OCDictionaryRef)OCArrayGetValueAtIndex(dvDicts, i);
                OCStringRef type =
                    OCDictionaryGetValue(dvDict, keyType);
                if (type && OCStringEqual(type, keyExternal)) {
                    OCDictionaryRemoveValue((OCMutableDictionaryRef)dvDict, keyComp);
                    OCDictionaryRemoveValue((OCMutableDictionaryRef)dvDict, keyEnc);
                }
            }
        }
    }
    // 2) envelope fields
    {
        OCMutableDictionaryRef mcore = (OCMutableDictionaryRef)core;
        OCDictionarySetValue(mcore, STR(kDatasetVersionKey), STR("1.0"));
        {
            OCStringRef ts = OCCreateISO8601Timestamp();
            OCDictionarySetValue(mcore, STR(kDatasetTimestampKey), ts);
            OCRelease(ts);
        }
        if (DatasetGetReadOnly(ds)) {
            OCDictionarySetValue(mcore,
                                 STR(kDatasetReadOnlyKey),
                                 OCBooleanGetWithBool(true));
        }
        if (DatasetGetGeographicCoordinate(ds)) {
            GeographicCoordinateRef gc = DatasetGetGeographicCoordinate(ds);
            OCDictionaryRef gcDict = GeographicCoordinateCopyAsDictionary(gc);
            OCDictionarySetValue(mcore, STR(kDatasetGeoCoordinateKey), gcDict);
            OCRelease(gcDict);
        }
    }
    // 3) wrap under “csdm”
    OCMutableDictionaryRef root = OCDictionaryCreateMutable(1);
    OCDictionarySetValue(root, STR(kDatasetCsdmEnvelopeKey), core);
    OCRelease(core);
    // 4) serialize JSON → file
    cJSON *json = OCTypeCopyJSON((OCTypeRef)root);
    OCRelease(root);
    if (!json) {
        if (outError) *outError = STR("Failed to convert dictionary to JSON");
        return false;
    }
    char *json_text = cJSON_Print(json);
    cJSON_Delete(json);
    if (!json_text) {
        if (outError) *outError = STR("Failed to generate JSON string");
        return false;
    }
    size_t json_len = strlen(json_text);
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
    size_t wrote = fwrite(json_text, 1, json_len, jf);
    fclose(jf);
    free(json_text);
    if (wrote != json_len) {
        if (outError) *outError = STR("Error writing JSON file");
        return false;
    }
    // 5) dump binary blobs
    if (!ensure_directory(binary_dir, outError))
        return false;
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv =
            (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError) *outError = STR("External DV missing components_url");
            return false;
        }
        const char *rel = parse_components_url_path(OCStringGetCString(url));
        if (!rel) {
            if (outError) *outError = STR("Invalid components_url");
            return false;
        }
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
// ————— DatasetCreateWithImport —————
DatasetRef DatasetCreateWithImport(const char *json_path,
                                   const char *binary_dir,
                                   OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json_path || !binary_dir) {
        if (outError)
            *outError = STR("Dataset import failed: invalid arguments");
        return NULL;
    }
    // 1) read + parse JSON
    FILE *jf = fopen(json_path, "rb");
    if (!jf) {
        if (outError) {
            OCStringRef p = OCStringCreateWithCString(json_path);
            *outError = OCStringCreateWithFormat(
                STR("Dataset import failed: cannot open JSON file '%@'"), p);
            OCRelease(p);
        }
        return NULL;
    }
    if (fseek(jf, 0, SEEK_END) != 0) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: cannot seek JSON file");
        return NULL;
    }
    long fsize = ftell(jf);
    if (fsize < 0) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: cannot determine JSON file size");
        return NULL;
    }
    rewind(jf);
    char *buffer = malloc((size_t)fsize + 1);
    if (!buffer) {
        fclose(jf);
        if (outError) *outError = STR("Dataset import failed: memory allocation error");
        return NULL;
    }
    size_t got = fread(buffer, 1, (size_t)fsize, jf);
    fclose(jf);
    if (got != (size_t)fsize) {
        free(buffer);
        if (outError) *outError = STR("Dataset import failed: incomplete read of JSON file");
        return NULL;
    }
    buffer[fsize] = '\0';
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    if (!root) {
        const char *e = cJSON_GetErrorPtr();
        if (outError) {
            if (e) {
                OCStringRef estr = OCStringCreateWithCString(e);
                *outError = OCStringCreateWithFormat(
                    STR("Dataset import failed: JSON parse error at '%@'"), estr);
                OCRelease(estr);
            } else {
                *outError = STR("Dataset import failed: invalid JSON format");
            }
        }
        return NULL;
    }
    // 2) JSON → dictionary → Dataset
    DatasetRef ds = DatasetCreateFromJSON(root, outError);
    cJSON_Delete(root);
    if (!ds) return NULL;
    // 3) compute expected number of points from the dataset’s dimensions
    OCArrayRef dims = DatasetGetDimensions(ds);
    OCIndex expectedSize = RMNCalculateSizeFromDimensions(dims);
    // 4) process dependent variables
    OCArrayRef dvsArray = DatasetGetDependentVariables(ds);
    OCIndex dvCount = dvsArray ? OCArrayGetCount(dvsArray) : 0;
    OCStringRef keyInternal = STR(kDependentVariableComponentTypeValueInternal);
    OCStringRef keyNone = STR(kDependentVariableEncodingValueNone);
    for (OCIndex i = 0; i < dvCount; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvsArray, i);
        if (!dv || !DependentVariableShouldSerializeExternally(dv))
            continue;
        // ensure DV.size is set
        OCIndex npts = DependentVariableGetSize(dv);
        if (npts == 0) {
            npts = expectedSize;
            DependentVariableSetSize(dv, npts);
        }
        // path resolution
        OCStringRef url = DependentVariableGetComponentsURL(dv);
        if (!url) {
            if (outError)
                *outError = STR("Dataset import failed: missing components_url for external variable");
            OCRelease(ds);
            return NULL;
        }
        const char *rel = parse_components_url_path(OCStringGetCString(url));
        if (!rel) {
            if (outError)
                *outError = STR("Dataset import failed: invalid components_url");
            OCRelease(ds);
            return NULL;
        }
        char fullpath[PATH_MAX];
        if (!join_path(fullpath, sizeof(fullpath), binary_dir, PATH_SEPARATOR, rel)) {
            if (outError)
                *outError = STR("Dataset import failed: binary path too long for component");
            OCRelease(ds);
            return NULL;
        }
        size_t total_bytes = 0;
        uint8_t *bytes = read_file_bytes(fullpath, &total_bytes);
        if (!bytes) {
            if (outError) {
                OCStringRef p = OCStringCreateWithCString(fullpath);
                *outError = OCStringCreateWithFormat(
                    STR("Dataset import failed: cannot read binary component '%@'"), p);
                OCRelease(p);
            }
            OCRelease(ds);
            return NULL;
        }
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        if (ncomps == 0) {
            OCStringRef qt = DependentVariableGetQuantityType(dv);
            ncomps = DependentVariableComponentsCountFromQuantityType(qt);
        }
        size_t elemSize = SIQuantityElementSize((SIQuantityRef)dv);
        size_t chunk = (size_t)npts * elemSize;
        if (chunk * (size_t)ncomps != total_bytes) {
            free(bytes);
            if (outError)
                *outError = STR("Dataset import failed: binary size mismatch for component");
            OCRelease(ds);
            return NULL;
        }
        OCMutableArrayRef comps = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        if (!comps) {
            free(bytes);
            if (outError)
                *outError = STR("Dataset import failed: cannot allocate components array");
            OCRelease(ds);
            return NULL;
        }
        for (OCIndex ci = 0; ci < ncomps; ++ci) {
            OCMutableDataRef buf = OCDataCreateMutable(chunk);
            if (!buf || !OCDataAppendBytes(buf, bytes + (ci * chunk), chunk)) {
                OCRelease(buf);
                OCRelease(comps);
                free(bytes);
                if (outError)
                    *outError = STR("Dataset import failed: cannot create component buffer");
                OCRelease(ds);
                return NULL;
            }
            OCArrayAppendValue(comps, buf);
            OCRelease(buf);
        }
        free(bytes);
        if (!DependentVariableSetComponents(dv, comps)) {
            OCRelease(comps);
            if (outError)
                *outError = STR("Dataset import failed: cannot install DV components");
            OCRelease(ds);
            return NULL;
        }
        OCRelease(comps);
        // flip to internal
        DependentVariableSetType(dv, keyInternal);
        DependentVariableSetEncoding(dv, keyNone);
        DependentVariableSetComponentsURL(dv, NULL);
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
