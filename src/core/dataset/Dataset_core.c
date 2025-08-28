/**
 * @file Dataset_core.c
 * @brief Core infrastructure for Dataset type
 *
 * This module handles type registration, lifecycle management, and core
 * infrastructure functions for Dataset.
 *
 * Functions include:
 * - Type registration (GetTypeID function)
 * - Object lifecycle (finalize, equal, copy)
 * - Object allocation and creation
 * - JSON serialization and deserialization
 * - Base field initialization
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../../RMNLibrary.h"
#include "Dataset_private.h"
#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
#pragma region Type Registration
// ============================================================================

static OCTypeID kDatasetID = kOCNotATypeID;

OCTypeID DatasetGetTypeID(void) {
    if (kDatasetID == kOCNotATypeID)
        kDatasetID = OCRegisterType("Dataset");
    return kDatasetID;
}

// ============================================================================
#pragma region Lifecycle Management
// ============================================================================

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
    OCRelease(ds->application);
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
    if (A->application != B->application &&
        !OCTypeEqual(A->application, B->application)) return false;
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
    // Build the "core" dictionary
    OCDictionaryRef core = DatasetCopyAsDictionary(ds);
    if (!core) return cJSON_CreateNull();
    // Wrap under the "csdm" envelope
    cJSON *root = cJSON_CreateObject();
    cJSON *inner = OCTypeCopyJSON((OCTypeRef)core);
    cJSON_AddItemToObject(root,
                          kDatasetCsdmEnvelopeKey,
                          inner);
    OCRelease(core);
    return root;
}

static void *impl_DatasetDeepCopy(const void *ptr) {
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
    dst->application = src->application
                           ? (OCMutableDictionaryRef)OCTypeDeepCopy(src->application)
                           : NULL;
    return dst;
}

// ============================================================================
#pragma region Object Allocation and Initialization
// ============================================================================

struct impl_Dataset *DatasetAllocate(void) {
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

void impl_InitDatasetFields(DatasetRef ds) {
    // Initialize all required fields to non-NULL values
    ds->dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->dependentVariables = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->description = STR("");
    ds->title = STR("");
    ds->dimensionPrecedence = OCIndexArrayCreateMutable(0);
    ds->version = STR("1.0");
    ds->timestamp = OCCreateISO8601Timestamp();
    ds->readOnly = false;
    ds->application = OCDictionaryCreateMutable(0);
    
    // These fields are conceptually optional and set to NULL initially
    // TODO: If strict no-NULL policy is required, consider creating empty/sentinel objects
    ds->focus = NULL;
    ds->previousFocus = NULL;
    ds->geographicCoordinate = NULL;
}

// ============================================================================
#pragma region Validation Functions
// ============================================================================

bool impl_ValidateDatasetParameters(OCArrayRef dimensions,
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

// ============================================================================
#pragma region Creation Functions
// ============================================================================

DatasetRef DatasetCreate(
    OCArrayRef dimensions,
    OCIndexArrayRef dimensionPrecedence,
    OCArrayRef dependentVariables,
    OCArrayRef tags,
    OCStringRef description,
    OCStringRef title,
    DatumRef focus,
    DatumRef previousFocus,
    OCDictionaryRef application,
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
                // leave the external DV as‐is (we'll import its blob later)
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
                toInsert = DependentVariableCopy(dv);
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
    // Release and reassign description and title (since they were initialized in impl_InitDatasetFields)
    OCRelease(ds->description);
    OCRelease(ds->title);
    ds->description = description ? OCStringCreateCopy(description) : STR("");
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    
    // For focus and previousFocus, keep existing NULL values from init if parameters are NULL
    // These are conceptually optional data points that may not exist
    if(focus) {
        OCIndex dependentVariableIndex = 0;
        OCIndex componentIndex = 0;
        OCIndex memOffset = 0;
        DatumRef testDatum = DatasetCreateDatumFromMemOffset(ds, dependentVariableIndex, componentIndex, memOffset);
        
        if(DatumHasSameReducedDimensionalities(focus, testDatum)) {
            ds->focus = (DatumRef) OCRetain(focus);
        } else {
            ds->focus = (DatumRef) OCRetain(testDatum);
        }
        OCRelease(testDatum);
    }
    // If focus is NULL, keep ds->focus as NULL (already initialized to NULL)
    
    if(previousFocus) {
        OCIndex dependentVariableIndex = 0;
        OCIndex componentIndex = 0;
        OCIndex memOffset = 0;
        DatumRef testDatum = DatasetCreateDatumFromMemOffset(ds, dependentVariableIndex, componentIndex, memOffset);
        
        if(DatumHasSameReducedDimensionalities(previousFocus, testDatum)) {
            ds->previousFocus = (DatumRef) OCRetain(previousFocus);
        } else {
            ds->previousFocus = (DatumRef) OCRetain(testDatum);
        }
        OCRelease(testDatum);
    }
    // If previousFocus is NULL, keep ds->previousFocus as NULL (already initialized to NULL)
    
    // — copy metadata if present —
    if (application) {
        OCRelease(ds->application);
        ds->application = OCTypeDeepCopyMutable(application);
    }
    return ds;
}

DatasetRef DatasetCreateMinimal(
    OCArrayRef dimensions,
    OCArrayRef dependentVariables,
    OCStringRef *outError) {
    // Call the full DatasetCreate function with default values for all optional parameters
    return DatasetCreate(
        dimensions,          // dimensions
        NULL,                // dimensionPrecedence (use natural order)
        dependentVariables,  // dependentVariables
        NULL,                // tags (empty)
        NULL,                // description (empty)
        NULL,                // title (empty)
        NULL,                // focus
        NULL,                // previousFocus
        NULL,                // application
        outError             // outError
    );
}

DatasetRef DatasetCreateEmpty(OCStringRef *outError) {
    return DatasetCreate(
        NULL,  // dimensions
        NULL,  // dimensionPrecedence
        NULL,  // dependentVariables
        NULL,  // tags
        NULL,  // description
        NULL,  // title
        NULL,  // focus
        NULL,  // previousFocus
        NULL,  // metaData
        outError);
}

DatasetRef DatasetCreateCopy(DatasetRef ds) {
    if (!ds) return NULL;
    return (DatasetRef)impl_DatasetDeepCopy(ds);
}

// ============================================================================
#pragma region JSON Helper Functions
// ============================================================================

static OCDictionaryRef DatasetDictionaryCreateFromJSON(cJSON *json,
                                                       OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1) Top‐level must be an object
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Dataset JSON must be an object");
        return NULL;
    }
    // 2) Unwrap the "csdm" envelope
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
        GeographicCoordinateRef gc = GeographicCoordinateCreateFromJSON(entry, &gcErr);
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
    // Process remaining fields using OCType JSON conversion
    // For arrays and other complex types, we need to parse them properly
    const char *arrayFields[] = {
        kDatasetDimensionsKey,
        kDatasetDependentVariablesKey,
        kDatasetTagsKey,
        NULL
    };
    for (int i = 0; arrayFields[i]; i++) {
        entry = cJSON_GetObjectItemCaseSensitive(json, arrayFields[i]);
        if (entry && cJSON_IsArray(entry)) {
            // Convert cJSON array to OCArray
            OCMutableArrayRef array = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
            if (array) {
                cJSON *item = NULL;
                cJSON_ArrayForEach(item, entry) {
                    if (strcmp(arrayFields[i], kDatasetDimensionsKey) == 0 && cJSON_IsObject(item)) {
                        OCStringRef dimErr = NULL;
                        DimensionRef dim = DimensionCreateFromJSON(item, &dimErr);
                        if (dim) {
                            // Convert dimension to dictionary for storage
                            OCDictionaryRef dd = DimensionCopyAsDictionary(dim);
                            if (dd) {
                                OCArrayAppendValue(array, dd);
                                OCRelease(dd);
                            }
                            OCRelease(dim);
                        } else if (dimErr) {
                            if (outError && !*outError) *outError = OCStringCreateCopy(dimErr);
                            OCRelease(dimErr);
                            OCRelease(array);
                            OCRelease(dict);
                            return NULL;
                        }
                    } else if (strcmp(arrayFields[i], kDatasetDependentVariablesKey) == 0 && cJSON_IsObject(item)) {
                        OCStringRef dvErr = NULL;
                        // Use the JSON→dict helper to preserve "external"/URL
                        OCDictionaryRef ddv = DependentVariableDictionaryCreateFromJSON(item, &dvErr);
                        if (ddv) {
                            OCArrayAppendValue(array, ddv);
                            OCRelease(ddv);
                        } else if (dvErr) {
                            if (outError && !*outError) *outError = OCStringCreateCopy(dvErr);
                            OCRelease(dvErr);
                            OCRelease(array);
                            OCRelease(dict);
                            return NULL;
                        }
                    } else if (strcmp(arrayFields[i], kDatasetTagsKey) == 0 && cJSON_IsString(item)) {
                        OCStringRef tag = OCStringCreateWithCString(item->valuestring);
                        OCArrayAppendValue(array, tag);
                        OCRelease(tag);
                    }
                }
                OCDictionarySetValue(dict, OCStringCreateWithCString(arrayFields[i]), array);
                OCRelease(array);
            }
        }
    }
    // Simple string fields
    const char *stringFields[] = {
        kDatasetDescriptionKey,
        kDatasetTitleKey,
        NULL
    };
    for (int i = 0; stringFields[i]; i++) {
        entry = cJSON_GetObjectItemCaseSensitive(json, stringFields[i]);
        if (entry && cJSON_IsString(entry)) {
            OCStringRef str = OCStringCreateWithCString(entry->valuestring);
            OCDictionarySetValue(dict, OCStringCreateWithCString(stringFields[i]), str);
            OCRelease(str);
        }
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
    // dimension_precedence (array of integers)
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetDimensionPrecedenceKey);
    if (entry && cJSON_IsArray(entry)) {
        OCMutableIndexArrayRef indexArray = OCIndexArrayCreateMutable(0);
        if (indexArray) {
            cJSON *item = NULL;
            cJSON_ArrayForEach(item, entry) {
                if (cJSON_IsNumber(item)) {
                    OCIndexArrayAppendValue(indexArray, (OCIndex)item->valueint);
                }
            }
            OCDictionarySetValue(dict, STR(kDatasetDimensionPrecedenceKey), indexArray);
            OCRelease(indexArray);
        }
    }
    // focus and previous_focus (Datum objects)
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetFocusKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef focusErr = NULL;
        DatumRef focus = DatumCreateFromJSON(entry, &focusErr);
        if (focus) {
            OCDictionaryRef focusDict = DatumCopyAsDictionary(focus);
            OCDictionarySetValue(dict, STR(kDatasetFocusKey), focusDict);
            OCRelease(focusDict);
            OCRelease(focus);
        } else if (focusErr) {
            OCRelease(focusErr);
        }
    }
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetPreviousFocusKey);
    if (entry && cJSON_IsObject(entry)) {
        OCStringRef pfErr = NULL;
        DatumRef prevFocus = DatumCreateFromJSON(entry, &pfErr);
        if (prevFocus) {
            OCDictionaryRef pfDict = DatumCopyAsDictionary(prevFocus);
            OCDictionarySetValue(dict, STR(kDatasetPreviousFocusKey), pfDict);
            OCRelease(pfDict);
            OCRelease(prevFocus);
        } else if (pfErr) {
            OCRelease(pfErr);
        }
    }
    // application metadata (generic object)
    entry = cJSON_GetObjectItemCaseSensitive(json, kDatasetApplicationKey);
    if (entry && cJSON_IsObject(entry)) {
        // Convert cJSON object to OCDictionary
        OCMutableDictionaryRef appDict = OCDictionaryCreateMutable(0);
        if (appDict) {
            cJSON *appItem = NULL;
            cJSON_ArrayForEach(appItem, entry) {
                if (appItem->string && cJSON_IsString(appItem)) {
                    OCStringRef key = OCStringCreateWithCString(appItem->string);
                    OCStringRef value = OCStringCreateWithCString(appItem->valuestring);
                    OCDictionarySetValue(appDict, key, value);
                    OCRelease(key);
                    OCRelease(value);
                }
            }
            OCDictionarySetValue(dict, STR(kDatasetApplicationKey), appDict);
            OCRelease(appDict);
        }
    }
    return (OCDictionaryRef)dict;
}

// ============================================================================
#pragma region Serialization Functions
// ============================================================================

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
            DependentVariableRef copy = DependentVariableCopy(dv);
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
    if (ds->application) {
        OCDictionaryRef meta_copy = (OCDictionaryRef)OCTypeDeepCopyMutable(ds->application);
        OCDictionarySetValue(dict,
                             STR(kDatasetApplicationKey),
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
    if ((ddict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDatasetApplicationKey)))) {
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
        if (!ds->geographicCoordinate && gcErr) {
            if (outError && !*outError) *outError = OCStringCreateCopy(gcErr);
            OCRelease(gcErr);
        }
    }
cleanup:
    // cleanup temporary allocations
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


DatumRef DatasetCreateDatumFromMemOffset(DatasetRef theDataset,
                                             OCIndex dependentVariableIndex,
                                             OCIndex componentIndex,
                                             OCIndex memOffset)
{
    IF_NO_OBJECT_EXISTS_RETURN(theDataset,NULL);
    DependentVariableRef theDV = (DependentVariableRef) OCArrayGetValueAtIndex(theDataset->dependentVariables, dependentVariableIndex);
    if (!theDV) return NULL;
    SIScalarRef response = DependentVariableCreateValueFromMemOffset(theDV, componentIndex, memOffset);
    if (!response) return NULL;
    
    DatumRef datum = DatumCreate(response, dependentVariableIndex, componentIndex, memOffset, (OCTypeRef)theDataset, NULL);
    if(response) OCRelease(response);
    return datum;
}


#pragma endregion

#ifdef __cplusplus
}
#endif
