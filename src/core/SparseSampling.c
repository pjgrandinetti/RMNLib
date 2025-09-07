/* SparseSampling OCType implementation */
#include "../RMNLibrary.h"
#include "SparseSampling_private.h"
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// SparseSampling dictionary keys
#define kSparseSamplingDimensionIndexesKey "dimension_indexes"
#define kSparseSamplingSparseGridVertexesKey "sparse_grid_vertexes"
#define kSparseSamplingUnsignedIntegerTypeKey "unsigned_integer_type"
#define kSparseSamplingEncodingKey "encoding"
#define kSparseSamplingDescriptionKey "description"
#define kSparseSamplingApplicationKey "application"
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Type registration
static OCTypeID kSparseSamplingID = kOCNotATypeID;
OCTypeID SparseSamplingGetTypeID(void) {
    if (kSparseSamplingID == kOCNotATypeID) {
        kSparseSamplingID = OCRegisterType("SparseSampling", (OCTypeRef (*)(cJSON *, OCStringRef *))SparseSamplingCreateFromJSON);
    }
    return kSparseSamplingID;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// ivar struct
struct impl_SparseSampling {
    OCBase base;
    OCIndexSetRef dimensionIndexes;        // Defines which dimensions are sparse (count = dimensionCount)
    OCDataRef sparseGridVertexes;          // Flattened vertex coordinate data
    OCNumberType unsignedIntegerType;      // UInt8/16/32/64 only
    OCStringRef encoding;                  // "none" or "base64"
    OCStringRef description;
    OCMutableDictionaryRef application;
};
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Finalize & Equal
static void impl_SparseSamplingFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_SparseSampling *ss = (struct impl_SparseSampling *)ptr;
    OCRelease(ss->dimensionIndexes);
    OCRelease(ss->sparseGridVertexes);
    OCRelease(ss->encoding);
    OCRelease(ss->description);
    OCRelease(ss->application);
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Helper functions for derived values
OCIndex SparseSamplingGetDimensionCount(SparseSamplingRef ss) {
    if (!ss || !ss->dimensionIndexes) return 0;
    return OCIndexSetGetCount(ss->dimensionIndexes);
}

size_t SparseSamplingGetVertexCount(SparseSamplingRef ss) {
    if (!ss || !ss->sparseGridVertexes) {
        return 0;
    }

    OCIndex dimensionCount = SparseSamplingGetDimensionCount(ss);
    if (dimensionCount == 0) return 0;

    OCIndex elementSize = OCNumberTypeSize(ss->unsignedIntegerType);
    if (elementSize == 0) return 0;

    OCIndex totalBytes = OCDataGetLength(ss->sparseGridVertexes);
    OCIndex totalElements = totalBytes / elementSize;

    OCIndex result = totalElements / dimensionCount;
    return result;
}

/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
static bool impl_SparseSamplingEqual(const void *a, const void *b) {
    const struct impl_SparseSampling *A = (const struct impl_SparseSampling *)a;
    const struct impl_SparseSampling *B = (const struct impl_SparseSampling *)b;
    if (!A || !B) return false;
    if (A == B) return true;
    // Compare primitive field directly
    if (A->unsignedIntegerType != B->unsignedIntegerType)
        return false;
    // Compare OCTypeRefs using OCTypeEqual (handles NULL cases)
    if (!OCTypeEqual(A->encoding, B->encoding))
        return false;
    if (!OCTypeEqual(A->description, B->description))
        return false;
    if (!OCTypeEqual(A->dimensionIndexes, B->dimensionIndexes))
        return false;
    if (!OCTypeEqual(A->application, B->application))
        return false;
    // Compare sparseGridVertexes OCIndexPairSetRef
    if (!OCTypeEqual(A->sparseGridVertexes, B->sparseGridVertexes))
        return false;
    return true;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Copy‐formatting description
static OCStringRef impl_SparseSamplingCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_SparseSampling *ss = (const void *)cf;
    return OCStringCreateWithFormat(
        STR("<SparseSampling dims=%@, verts=%lu, uint=%d, enc=%@>"),
        ss->dimensionIndexes,
        (unsigned long)SparseSamplingGetVertexCount((SparseSamplingRef)ss),
        ss->unsignedIntegerType,
        ss->encoding);
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// JSON serialization
static cJSON *impl_SparseSamplingCopyJSON(const void *obj, bool typed, OCStringRef *outError) {
    return SparseSamplingCopyAsJSON((SparseSamplingRef)obj, typed, outError);
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Deep‐copy
static void *
impl_SparseSamplingDeepCopy(const void *ptr) {
    if (ptr == NULL) {
        return NULL;
    }
    const struct impl_SparseSampling *src =
        (const struct impl_SparseSampling *)ptr;
    // Allocate a zeroed struct so we can bail early without uninitialized fields
    struct impl_SparseSampling *dst =
        calloc(1, sizeof(*dst));
    if (dst == NULL) {
        return NULL;
    }
    // Copy the OCBase header (type tag & ref-count)
    memcpy(&dst->base, &src->base, sizeof(OCBase));
    // Deep-copy each OCTypeRef field via OCTypeDeepCopy:
    dst->dimensionIndexes = src->dimensionIndexes
                                ? (OCIndexSetRef)OCTypeDeepCopy(src->dimensionIndexes)
                                : NULL;
    dst->sparseGridVertexes = src->sparseGridVertexes
                                  ? (OCDataRef)OCTypeDeepCopy(src->sparseGridVertexes)
                                  : NULL;
    // Primitive field
    dst->unsignedIntegerType = src->unsignedIntegerType;
    // Strings and dictionaries
    dst->encoding = src->encoding
                        ? (OCStringRef)OCTypeDeepCopy(src->encoding)
                        : NULL;
    dst->description = src->description
                           ? (OCStringRef)OCTypeDeepCopy(src->description)
                           : NULL;
    dst->application = src->application
                           ? (OCMutableDictionaryRef)OCTypeDeepCopy(src->application)
                           : NULL;
    return dst;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Allocator + init
static struct impl_SparseSampling *SparseSamplingAllocate(void) {
    return OCTypeAlloc(
        struct impl_SparseSampling,
        SparseSamplingGetTypeID(),
        impl_SparseSamplingFinalize,
        impl_SparseSamplingEqual,
        impl_SparseSamplingCopyFormattingDesc,
        impl_SparseSamplingCopyJSON,
        impl_SparseSamplingDeepCopy,
        impl_SparseSamplingDeepCopy);
}
static void impl_InitSparseSamplingFields(SparseSamplingRef ss) {
    ss->dimensionIndexes = OCIndexSetCreateMutable();
    ss->sparseGridVertexes = OCDataCreate(NULL, 0);
    ss->unsignedIntegerType = kOCNumberUInt64Type;
    ss->encoding = STR(kSparseSamplingEncodingValueBase64);
    ss->description = STR("");
    ss->application = OCDictionaryCreateMutable(0);
}
bool validateSparseSampling(SparseSamplingRef ss, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!ss) return true;  // Allow null object as trivially valid
    // 1) Check unsignedIntegerType is one of the allowed unsigned types
    switch (ss->unsignedIntegerType) {
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            break;
        default:
            if (outError)
                *outError = STR("SparseSampling validation error: unsignedIntegerType must be one of UInt8, UInt16, UInt32, or UInt64");
            return false;
    }
    // 2) encoding must be either "none" or "base64"
    if (!ss->encoding ||
        OCGetTypeID(ss->encoding) != OCStringGetTypeID() ||
        (!OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueNone)) &&
         !OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64)))) {
        if (outError)
            *outError = STR("SparseSampling validation error: encoding must be \"none\" or \"base64\"");
        return false;
    }
    // 3) dimensionIndexes must be a valid OCIndexSet (empty is allowed)
    if (!ss->dimensionIndexes || OCGetTypeID(ss->dimensionIndexes) != OCIndexSetGetTypeID()) {
        if (outError)
            *outError = STR("SparseSampling validation error: dimensionIndexes must be a valid OCIndexSet");
        return false;
    }
    // Note: Empty dimension indexes are allowed
    // 4) sparseGridVertexes must be a valid OCDataRef
    if (!ss->sparseGridVertexes || OCGetTypeID(ss->sparseGridVertexes) != OCDataGetTypeID()) {
        if (outError)
            *outError = STR("SparseSampling validation error: sparseGridVertexes must be a valid OCDataRef");
        return false;
    }

    // 5) Validate that data size is consistent with dimension count and integer type
    OCIndex dimensionCount = SparseSamplingGetDimensionCount(ss);
    if (dimensionCount > 0) {
        OCIndex elementSize = OCNumberTypeSize(ss->unsignedIntegerType);
        OCIndex totalBytes = OCDataGetLength(ss->sparseGridVertexes);

        if (elementSize > 0 && (totalBytes % (dimensionCount * elementSize) != 0)) {
            if (outError)
                *outError = STR("SparseSampling validation error: data size is not consistent with dimension count and integer type");
            return false;
        }
    }
    return true;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Public creator
SparseSamplingRef SparseSamplingCreate(
    OCIndexSetRef dimensionIndexes,
    OCDataRef sparseGridVertexes,
    OCNumberType unsignedIntegerType,
    OCStringRef encoding,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    // 1. Validate unsignedIntegerType
    switch (unsignedIntegerType) {
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            break;
        default:
            if (outError) {
                *outError = STR("SparseSamplingCreate: invalid unsignedIntegerType (must be UInt8/16/32/64)");
            }
            return NULL;
    }
    // 2. Validate encoding string
    if (!encoding ||
        (!OCStringEqual(encoding, STR(kSparseSamplingEncodingValueNone)) &&
         !OCStringEqual(encoding, STR(kSparseSamplingEncodingValueBase64)))) {
        if (outError) {
            *outError = STR("SparseSamplingCreate: encoding must be \"none\" or \"base64\"");
        }
        return NULL;
    }
    // 3. Allocate and initialize default values
    struct impl_SparseSampling *ss = SparseSamplingAllocate();
    if (!ss) {
        if (outError) *outError = STR("SparseSamplingCreate: allocation failed");
        return NULL;
    }
    impl_InitSparseSamplingFields((SparseSamplingRef)ss);
    // 4. Assign provided values
    if (dimensionIndexes) {
        OCRelease(ss->dimensionIndexes);
        // Handle empty index sets specially since OCIndexSetCreateMutableCopy returns NULL for empty sets
        if (OCIndexSetGetCount(dimensionIndexes) == 0) {
            ss->dimensionIndexes = OCIndexSetCreateMutable();  // Create new empty mutable set
        } else {
            ss->dimensionIndexes = OCIndexSetCreateMutableCopy(dimensionIndexes);
        }
    }
    if (sparseGridVertexes) {
        OCRelease(ss->sparseGridVertexes);
        ss->sparseGridVertexes = (OCDataRef)OCTypeDeepCopy(sparseGridVertexes);
    }
    ss->unsignedIntegerType = unsignedIntegerType;
    OCRelease(ss->encoding);
    ss->encoding = OCStringCreateCopy(encoding);
    OCRelease(ss->description);
    ss->description = description
                          ? OCStringCreateCopy(description)
                          : OCStringCreateCopy(STR(""));
    OCRelease(ss->application);
    ss->application = metadata
                          ? (OCMutableDictionaryRef)OCTypeDeepCopyMutable(metadata)
                          : OCDictionaryCreateMutable(0);
    // 5. Final structural validation
    if (!validateSparseSampling((SparseSamplingRef)ss, outError)) {
        OCRelease(ss);
        return NULL;
    }
    return (SparseSamplingRef)ss;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Dictionary serialization
OCDictionaryRef SparseSamplingCopyAsDictionary(SparseSamplingRef ss) {
    if (!ss) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    // 1. dimension_indexes → OCArray of OCNumber from OCIndexSet
    OCArrayRef dimIdxNumbers = OCIndexSetCreateOCNumberArray(ss->dimensionIndexes);
    // Always add the dimension_indexes key, even if empty
    if (dimIdxNumbers) {
        OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), dimIdxNumbers);
        OCRelease(dimIdxNumbers);
    } else {
        // Create empty array if OCIndexSetCreateOCNumberArray returned NULL
        OCMutableArrayRef emptyArray = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), emptyArray);
        OCRelease(emptyArray);
    }
    // 2. sparse_grid_vertexes: convert OCData to array of vertex arrays
    if (ss->sparseGridVertexes) {
        OCIndex dimensionCount = SparseSamplingGetDimensionCount(ss);
        OCIndex vertexCount = SparseSamplingGetVertexCount(ss);

        if (dimensionCount > 0 && vertexCount > 0) {
            OCMutableArrayRef vertexesArray = OCArrayCreateMutable(vertexCount, &kOCTypeArrayCallBacks);

            // Convert OCData to array based on encoding
            bool isBase64 = OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64));

            if (isBase64) {
                // Store as base64 encoded string
                OCStringRef b64String = OCDataCreateBase64EncodedString(ss->sparseGridVertexes, OCBase64EncodingOptionsNone);
                if (b64String) {
                    OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), b64String);
                    OCRelease(b64String);
                }
            } else {
                // Convert to array of arrays format: [[dim1, dim2, ...], [dim1, dim2, ...], ...]
                OCStringRef conversionError = NULL;
                OCArrayRef numberArray = OCNumberCreateArrayFromData(ss->sparseGridVertexes, ss->unsignedIntegerType, &conversionError);

                if (numberArray && OCArrayGetCount(numberArray) == dimensionCount * vertexCount) {
                    for (OCIndex v = 0; v < vertexCount; v++) {
                        OCMutableArrayRef vertex = OCArrayCreateMutable(dimensionCount, &kOCTypeArrayCallBacks);
                        for (OCIndex d = 0; d < dimensionCount; d++) {
                            OCIndex flatIndex = v * dimensionCount + d;
                            OCNumberRef coord = OCArrayGetValueAtIndex(numberArray, flatIndex);
                            if (coord) {
                                OCArrayAppendValue(vertex, coord);
                            }
                        }
                        OCArrayAppendValue(vertexesArray, vertex);
                        OCRelease(vertex);
                    }
                }

                if (numberArray) OCRelease(numberArray);
                if (conversionError) OCRelease(conversionError);
            }

            if (!isBase64) {
                OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), vertexesArray);
            }
            OCRelease(vertexesArray);
        }
    }
    // 3. unsigned_integer_type
    const char *typeName = OCNumberGetTypeName(ss->unsignedIntegerType);
    if (typeName) {
        OCStringRef typeStr = OCStringCreateWithCString(typeName);
        OCDictionarySetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey), typeStr);
        OCRelease(typeStr);
    }
    // 4. encoding
    OCDictionarySetValue(dict, STR(kSparseSamplingEncodingKey), ss->encoding);
    // 5. description
    if (ss->description) {
        OCStringRef descCopy = OCStringCreateCopy(ss->description);
        OCDictionarySetValue(dict, STR(kSparseSamplingDescriptionKey), descCopy);
        OCRelease(descCopy);
    }
    // 6. metadata
    if (ss->application) {
        OCDictionaryRef mdCopy = (OCDictionaryRef)OCTypeDeepCopyMutable(ss->application);
        OCDictionarySetValue(dict, STR(kSparseSamplingApplicationKey), mdCopy);
        OCRelease(mdCopy);
    }
    return dict;
}

// Dictionary → object
SparseSamplingRef SparseSamplingCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("input dictionary is NULL");
        return NULL;
    }
    // 1. Parse dimension_indexes (OCArray of OCNumber → OCIndexSet)
    OCMutableIndexSetRef dimSet = OCIndexSetCreateMutable();  // Create mutable like SparseSamplingCreate does
    OCArrayRef idxArr = OCDictionaryGetValue(dict, STR(kSparseSamplingDimensionIndexesKey));
    if (idxArr && OCGetTypeID(idxArr) == OCArrayGetTypeID()) {
        for (OCIndex i = 0; i < OCArrayGetCount(idxArr); ++i) {
            OCNumberRef num = OCArrayGetValueAtIndex(idxArr, i);
            long v = 0;
            if (OCNumberTryGetLong(num, &v)) {
                OCIndexSetAddIndex(dimSet, (OCIndex)v);
            }
        }
    }
    OCIndex ndim = dimSet ? OCIndexSetGetCount(dimSet) : 0;
    // 2. Parse unsigned_integer_type
    OCNumberType utype = kOCNumberUInt64Type;
    OCStringRef typeStr = OCDictionaryGetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey));
    if (typeStr && OCGetTypeID(typeStr) == OCStringGetTypeID()) {
        const char *typeName = OCStringGetCString(typeStr);
        OCNumberType parsed = OCNumberTypeFromName(typeName);
        if (parsed != -1) {
            utype = parsed;
        }
    }
    // 3. Parse encoding
    OCStringRef enc = OCDictionaryGetValue(dict, STR(kSparseSamplingEncodingKey));
    if (!enc) enc = STR(kSparseSamplingEncodingValueBase64);
    // 4. Parse sparse_grid_vertexes from array of arrays or base64 string
    OCDataRef sparseGridVerts = NULL;
    OCTypeRef sparseGridValue = OCDictionaryGetValue(dict, STR(kSparseSamplingSparseGridVertexesKey));

    if (sparseGridValue) {
        if (OCGetTypeID(sparseGridValue) == OCStringGetTypeID()) {
            // Base64 encoded string
            sparseGridVerts = OCDataCreateFromBase64EncodedString((OCStringRef)sparseGridValue);
        } else if (OCGetTypeID(sparseGridValue) == OCArrayGetTypeID()) {
            // Array of vertex arrays: [[dim1, dim2, ...], [dim1, dim2, ...], ...]
            OCArrayRef vertexArray = (OCArrayRef)sparseGridValue;
            OCIndex vertexCount = OCArrayGetCount(vertexArray);

            if (vertexCount > 0 && ndim > 0) {
                // Create flat array of all coordinates
                OCMutableArrayRef flatNumbers = OCArrayCreateMutable(vertexCount * ndim, &kOCTypeArrayCallBacks);

                for (OCIndex v = 0; v < vertexCount; v++) {
                    OCArrayRef vertex = OCArrayGetValueAtIndex(vertexArray, v);
                    if (vertex && OCGetTypeID(vertex) == OCArrayGetTypeID()) {
                        OCIndex coordCount = OCArrayGetCount(vertex);
                        if (coordCount == ndim) {
                            for (OCIndex d = 0; d < ndim; d++) {
                                OCNumberRef coord = OCArrayGetValueAtIndex(vertex, d);
                                if (coord) {
                                    OCArrayAppendValue(flatNumbers, coord);
                                }
                            }
                        }
                    }
                }

                // Convert flat number array to OCData
                if (OCArrayGetCount(flatNumbers) == vertexCount * ndim) {
                    OCStringRef conversionError = NULL;
                    sparseGridVerts = OCNumberCreateDataFromArray(flatNumbers, utype, &conversionError);
                    if (conversionError) OCRelease(conversionError);
                }

                OCRelease(flatNumbers);
            }
        }
    }

    if (!sparseGridVerts) {
        sparseGridVerts = OCDataCreate(NULL, 0);
    }
    // 5. Parse optional fields
    OCStringRef desc = OCDictionaryGetValue(dict, STR(kSparseSamplingDescriptionKey));
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kSparseSamplingApplicationKey));
    // 6. Construct and validate
    OCStringRef createErr = NULL;
    SparseSamplingRef ss = SparseSamplingCreate(dimSet, sparseGridVerts, utype, enc, desc, md, &createErr);
    OCRelease(dimSet);
    OCRelease(sparseGridVerts);
    if (!ss) {
        if (outError) *outError = createErr;
        return NULL;
    }
    if (!validateSparseSampling(ss, outError)) {
        OCRelease(ss);
        return NULL;
    }
    return ss;
}

cJSON *SparseSamplingCopyAsJSON(SparseSamplingRef ss, bool typed, OCStringRef *outError) {
    if (!ss) return cJSON_CreateNull();

    cJSON *json = cJSON_CreateObject();
    if (!json) return cJSON_CreateNull();

    // 1. dimension_indexes - use OCIndexSet's built-in JSON serialization
    if (ss->dimensionIndexes) {
        cJSON *dimArray = OCIndexSetCopyAsJSON(ss->dimensionIndexes, typed, outError);
        if (dimArray) {
            cJSON_AddItemToObject(json, kSparseSamplingDimensionIndexesKey, dimArray);
        }
    }


    // 2. sparse_grid_vertexes - convert OCData to JSON format
    if (ss->sparseGridVertexes) {
        OCStringRef dataError = NULL;
        cJSON *vertexesJSON = NULL;

        if (ss->encoding && OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64))) {
            // For base64 encoding, use OCDataCopyAsJSON with base64 encoding
            OCJSONEncoding originalEncoding = OCDataCopyEncoding(ss->sparseGridVertexes);
            OCDataSetEncoding((OCMutableDataRef)ss->sparseGridVertexes, OCJSONEncodingBase64);
            vertexesJSON = OCDataCopyAsJSON(ss->sparseGridVertexes, typed, &dataError);
            OCDataSetEncoding((OCMutableDataRef)ss->sparseGridVertexes, originalEncoding);
        } else {
            // For "none" encoding, use SparseSampling schema knowledge to create proper format
            if (typed) {
                // For typed JSON with "none" encoding, create array of typed OCNumbers
                // This preserves the encoding="none" semantics in typed format
                OCArrayRef numberArray = OCNumberCreateArrayFromData(ss->sparseGridVertexes, ss->unsignedIntegerType, &dataError);
                if (numberArray) {
                    vertexesJSON = OCArrayCopyAsJSON(numberArray, true, &dataError);
                    OCRelease(numberArray);
                }
            } else {
                // For untyped JSON with "none" encoding, create plain JSON array
                OCArrayRef numberArray = OCNumberCreateArrayFromData(ss->sparseGridVertexes, ss->unsignedIntegerType, &dataError);
                if (numberArray) {
                    vertexesJSON = OCArrayCopyAsJSON(numberArray, false, &dataError);
                    OCRelease(numberArray);
                }
            }
        }

        if (vertexesJSON) {
            cJSON_AddItemToObject(json, kSparseSamplingSparseGridVertexesKey, vertexesJSON);
        }

        if (dataError) OCRelease(dataError);
    }

    // 3. encoding - always include for both typed and untyped to preserve round-trip accuracy
    if (ss->encoding) {
        const char *encStr = OCStringGetCString(ss->encoding);
        if (encStr) {
            cJSON_AddStringToObject(json, kSparseSamplingEncodingKey, encStr);
        }
    }

    // 4. unsigned_integer_type
    const char *typeName = OCNumberGetTypeName(ss->unsignedIntegerType);
    if (typeName) {
        cJSON_AddStringToObject(json, kSparseSamplingUnsignedIntegerTypeKey, typeName);
    }

    // 5. description
    if (ss->description) {
        const char *descStr = OCStringGetCString(ss->description);
        if (descStr) {
            cJSON_AddStringToObject(json, kSparseSamplingDescriptionKey, descStr);
        }
    }

    // 6. application metadata
    if (ss->application) {
        // CRITICAL REQUIREMENT: application ivar in ALL RMNLib types MUST ALWAYS be encoded
        // into JSON as typed=true, NO EXCEPTIONS. Even if the rest of the JSON is untyped,
        // application must always remain typed to preserve complex nested type information.
        cJSON *app = OCTypeCopyJSON((OCTypeRef)ss->application, true, outError);
        if (app) {
            cJSON_AddItemToObject(json, kSparseSamplingApplicationKey, app);
        }
    }

    if (typed) {
        // Wrap in typed object format
        cJSON *wrapper = cJSON_CreateObject();
        if (wrapper) {
            cJSON_AddStringToObject(wrapper, "type", "SparseSampling");
            cJSON_AddItemToObject(wrapper, "value", json);
            return wrapper;
        } else {
            cJSON_Delete(json);
            return cJSON_CreateNull();
        }
    }

    return json;
}

SparseSamplingRef SparseSamplingCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;

    if (!json) {
        if (outError) *outError = STR("Input JSON is NULL");
        return NULL;
    }

    // Declare variables at top to avoid uninitialized warnings with goto
    OCIndexSetRef dimensionIndexes = NULL;
    OCDataRef sparseGridVertexes = NULL;
    OCStringRef encoding = STR(kSparseSamplingEncodingValueBase64); // Default encoding
    OCStringRef description = NULL;
    OCDictionaryRef application = NULL;

    cJSON *actualJson = json;

    // Check if this is a typed JSON object (has "type" and "value" fields)
    if (cJSON_IsObject(json)) {
        cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(json, "type");
        cJSON *valueItem = cJSON_GetObjectItemCaseSensitive(json, "value");

        if (typeItem && cJSON_IsString(typeItem) &&
            strcmp(typeItem->valuestring, "SparseSampling") == 0 &&
            valueItem && cJSON_IsObject(valueItem)) {
            // This is a typed JSON, use the value part
            actualJson = valueItem;
        }
    }

    if (!cJSON_IsObject(actualJson)) {
        if (outError) *outError = STR("Expected top-level JSON object for SparseSampling");
        return NULL;
    }

    // Initialize all variables for cleanup
    SparseSamplingRef ss = NULL;
    OCNumberType unsignedIntegerType = kOCNumberUInt64Type;

    // Parse unsigned_integer_type FIRST (needed for sparse_grid_vertexes parsing)
    // This field is REQUIRED for sparse sampling objects
    cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingUnsignedIntegerTypeKey);
    if (!typeItem || !cJSON_IsString(typeItem)) {
        if (outError) {
            *outError = STR("SparseSampling object missing required 'unsigned_integer_type' field. Must be one of: uint8, uint16, uint32, uint64.");
        }
        goto cleanup;
    }

    const char *typeName = typeItem->valuestring;
    OCNumberType parsed = OCNumberTypeFromName(typeName);
    if (parsed == kOCNumberTypeInvalid) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Invalid 'unsigned_integer_type' value: \"%s\". Must be one of: uint8, uint16, uint32, uint64."),
                typeName);
        }
        goto cleanup;
    }
    unsignedIntegerType = parsed;

    // Parse dimension_indexes using OCIndexSet's JSON parser
    cJSON *dimItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingDimensionIndexesKey);
    if (dimItem) {
        OCStringRef parseError = NULL;
        dimensionIndexes = (OCIndexSetRef)OCIndexSetCreateFromJSON(dimItem, &parseError);
        if (!dimensionIndexes && parseError) {
            if (outError) *outError = parseError;
            goto cleanup;
        }
    }
    if (!dimensionIndexes) {
        dimensionIndexes = OCIndexSetCreateMutable();
    }

    // Parse sparse_grid_vertexes from JSON
    cJSON *vertsItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingSparseGridVertexesKey);
    if (vertsItem) {
        OCStringRef dataParseError = NULL;

        // Determine if this is typed or untyped JSON
        bool isTypedJSON = (actualJson != json); // If we unwrapped a typed object, this is typed JSON

        if (isTypedJSON) {
            // For typed JSON, first check if there's an explicit encoding field
            cJSON *encItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingEncodingKey);
            if (cJSON_IsString(encItem)) {
                if (strcmp(encItem->valuestring, "base64") == 0) {
                    encoding = STR(kSparseSamplingEncodingValueBase64);
                } else if (strcmp(encItem->valuestring, "none") == 0) {
                    encoding = STR(kSparseSamplingEncodingValueNone);
                }
            }

            // For typed JSON, we need to handle both OCData and OCArray typed formats
            // Check if this is an OCData typed object
            if (cJSON_IsObject(vertsItem)) {
                cJSON *typeItem = cJSON_GetObjectItemCaseSensitive(vertsItem, "type");
                if (typeItem && cJSON_IsString(typeItem) && strcmp(typeItem->valuestring, "OCData") == 0) {
                    // OCData typed format - use OCDataCreateFromJSON
                    sparseGridVertexes = OCDataCreateFromJSON(vertsItem, &dataParseError);
                    // Note: Don't override encoding if explicitly set above
                    if (!cJSON_IsString(encItem) && sparseGridVertexes) {
                        encoding = STR(kSparseSamplingEncodingValueBase64);
                    }
                } else if (typeItem && cJSON_IsString(typeItem) && strcmp(typeItem->valuestring, "OCArray") == 0) {
                    // OCArray typed format - manually parse the homogeneous format
                    cJSON *valueItem = cJSON_GetObjectItemCaseSensitive(vertsItem, "value");
                    if (valueItem && cJSON_IsArray(valueItem)) {
                        // Parse the plain JSON array from the "value" field
                        OCArrayRef numberArray = OCArrayOfNumbersCreateFromJSON(valueItem, unsignedIntegerType, &dataParseError);
                        if (numberArray) {
                            sparseGridVertexes = OCNumberCreateDataFromArray(numberArray, unsignedIntegerType, &dataParseError);
                            if (sparseGridVertexes) {
                                // Note: Don't override encoding if explicitly set above
                                if (!cJSON_IsString(encItem) && sparseGridVertexes) {
                                    encoding = STR(kSparseSamplingEncodingValueNone);
                                }
                            }
                            OCRelease(numberArray);
                        }
                    } else {
                        if (outError) *outError = STR("OCArray typed format missing 'value' array");
                        goto cleanup;
                    }
                } else {
                    if (outError) *outError = STR("Unrecognized typed JSON format for sparse_grid_vertexes");
                    goto cleanup;
                }
            } else if (cJSON_IsArray(vertsItem)) {
                // Heterogeneous typed array - parse as array then convert to data
                OCArrayRef numberArray = OCArrayCreateFromJSONTyped(vertsItem, &dataParseError);
                if (numberArray) {
                    sparseGridVertexes = OCNumberCreateDataFromArray(numberArray, unsignedIntegerType, &dataParseError);
                    // Note: Don't override encoding if explicitly set above
                    if (!cJSON_IsString(encItem) && sparseGridVertexes) {
                        encoding = STR(kSparseSamplingEncodingValueNone);
                    }
                    OCRelease(numberArray);
                }
            } else {
                if (outError) *outError = STR("Invalid typed JSON format for sparse_grid_vertexes");
                goto cleanup;
            }
        } else {
            // For untyped JSON, check the encoding field to know how to parse
            cJSON *encItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingEncodingKey);
            bool isBase64Encoding = false;
            if (cJSON_IsString(encItem) && strcmp(encItem->valuestring, "base64") == 0) {
                isBase64Encoding = true;
            }

            if (isBase64Encoding && cJSON_IsString(vertsItem)) {
                // Base64 encoded string - use OCDataCreateFromJSON
                sparseGridVertexes = OCDataCreateFromJSON(vertsItem, &dataParseError);
                if (sparseGridVertexes) {
                    encoding = STR(kSparseSamplingEncodingValueBase64);
                }
            } else if (!isBase64Encoding && cJSON_IsArray(vertsItem)) {
                // Array format with "none" encoding - use OCArrayOfNumbersCreateFromJSON
                OCArrayRef numberArray = OCArrayOfNumbersCreateFromJSON(vertsItem, unsignedIntegerType, &dataParseError);
                if (numberArray) {
                    sparseGridVertexes = OCNumberCreateDataFromArray(numberArray, unsignedIntegerType, &dataParseError);
                    if (sparseGridVertexes) {
                        encoding = STR(kSparseSamplingEncodingValueNone);
                    }
                    OCRelease(numberArray);
                }
            } else {
                // Format mismatch - set error and return
                if (outError) {
                    if (isBase64Encoding) {
                        *outError = STR("Encoding is 'base64' but sparse_grid_vertexes is not a string");
                    } else {
                        *outError = STR("Encoding is 'none' but sparse_grid_vertexes is not an array");
                    }
                }
                goto cleanup;
            }
        }

        if (dataParseError) OCRelease(dataParseError);
    }

    // Parse optional description
    cJSON *descItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingDescriptionKey);
    if (cJSON_IsString(descItem)) {
        description = OCStringCreateWithCString(descItem->valuestring);
        if (!description) {
            if (outError) *outError = STR("Failed to create description string");
            goto cleanup;
        }
    }

    // Parse optional application metadata using OCDictionary's JSON parser
    cJSON *appItem = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingApplicationKey);
    if (cJSON_IsObject(appItem)) {
        OCStringRef parseError = NULL;
        application = (OCDictionaryRef)OCDictionaryCreateFromJSONTyped(appItem, &parseError);
        if (!application && parseError) {
            if (outError) *outError = parseError;
            goto cleanup;
        }
    }

    // Create the SparseSampling object
    OCStringRef createError = NULL;
    ss = SparseSamplingCreate(
        dimensionIndexes,
        sparseGridVertexes,
        unsignedIntegerType,
        encoding,
        description,
        application,
        &createError
    );

    // Propagate creation error if object creation failed
    if (!ss && outError) {
        if (createError) {
            *outError = createError;
        } else if (!*outError) {
            *outError = STR("Failed to create SparseSampling object");
        }
    }

cleanup:
    // Clean up all allocated resources
    OCRelease(dimensionIndexes);
    OCRelease(sparseGridVertexes);
    OCRelease(description);
    OCRelease(application);

    return ss;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Getters / Setters
OCIndexSetRef SparseSamplingGetDimensionIndexes(SparseSamplingRef ss) {
    return ss ? ss->dimensionIndexes : NULL;
}
OCIndexSetRef SparseSamplingCopyDimensionIndexes(SparseSamplingRef ss) {
    if (!ss || !ss->dimensionIndexes) return NULL;
    return (OCIndexSetRef)OCTypeDeepCopy(ss->dimensionIndexes);
}
bool SparseSamplingSetDimensionIndexes(SparseSamplingRef ss, OCIndexSetRef idxs) {
    if (!ss) return false;
    OCRelease(ss->dimensionIndexes);
    ss->dimensionIndexes = idxs
                               ? OCIndexSetCreateMutableCopy(idxs)
                               : OCIndexSetCreateMutable();
    return ss->dimensionIndexes != NULL;
}

OCDataRef SparseSamplingGetSparseGridVertexes(SparseSamplingRef ss) {
    return ss ? ss->sparseGridVertexes : NULL;
}
OCDataRef SparseSamplingCopySparseGridVertexes(SparseSamplingRef ss) {
    if (!ss || !ss->sparseGridVertexes) return NULL;
    return (OCDataRef)OCTypeDeepCopy(ss->sparseGridVertexes);
}
bool SparseSamplingSetSparseGridVertexes(SparseSamplingRef ss, OCDataRef verts) {
    if (!ss) return false;
    OCRelease(ss->sparseGridVertexes);
    ss->sparseGridVertexes = verts
                                 ? (OCDataRef)OCTypeDeepCopy(verts)
                                 : OCDataCreate(NULL, 0);
    return ss->sparseGridVertexes != NULL;
}
OCNumberType SparseSamplingGetUnsignedIntegerType(SparseSamplingRef ss) {
    return ss ? ss->unsignedIntegerType : kOCNumberUInt64Type;
}
bool SparseSamplingSetUnsignedIntegerType(SparseSamplingRef ss, OCNumberType t) {
    if (!ss) return false;
    switch (t) {
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            ss->unsignedIntegerType = t;
            return true;
        default:
            return false;
    }
}
OCStringRef SparseSamplingGetEncoding(SparseSamplingRef ss) {
    return ss ? ss->encoding : NULL;
}
OCStringRef SparseSamplingCopyEncoding(SparseSamplingRef ss) {
    if (!ss || !ss->encoding) return NULL;
    return OCStringCreateCopy(ss->encoding);
}
bool SparseSamplingSetEncoding(SparseSamplingRef ss, OCStringRef enc) {
    if (!ss || !enc) return false;
    if (!OCStringEqual(enc, STR(kSparseSamplingEncodingValueNone)) &&
        !OCStringEqual(enc, STR(kSparseSamplingEncodingValueBase64))) {
        return false;
    }
    OCRelease(ss->encoding);
    ss->encoding = OCStringCreateCopy(enc);
    return true;
}
OCStringRef SparseSamplingGetDescription(SparseSamplingRef ss) {
    return ss ? ss->description : NULL;
}
OCStringRef SparseSamplingCopyDescription(SparseSamplingRef ss) {
    if (!ss || !ss->description) return NULL;
    return OCStringCreateCopy(ss->description);
}
bool SparseSamplingSetDescription(SparseSamplingRef ss, OCStringRef d) {
    if (!ss) return false;
    OCStringRef copy = d ? OCStringCreateCopy(d) : OCStringCreateCopy(STR(""));
    if (!copy) return false;
    OCRelease(ss->description);
    ss->description = copy;
    return true;
}
OCDictionaryRef SparseSamplingGetApplicationMetaData(SparseSamplingRef ss) {
    return ss ? ss->application : NULL;
}
bool SparseSamplingSetApplicationMetaData(SparseSamplingRef ss, OCDictionaryRef md) {
    if (!ss) return false;
    OCRelease(ss->application);
    ss->application = md
                          ? (OCMutableDictionaryRef)OCTypeDeepCopyMutable(md)
                          : OCDictionaryCreateMutable(0);
    return ss->application != NULL;
}

// Note: SparseSamplingGetVertexCount and SparseSamplingGetDimensionCount are already defined as static helpers above

bool SparseSamplingGetVertexAtIndex(SparseSamplingRef ss, OCIndex vertexIndex, OCIndex *outCoords) {
    if (!ss || !ss->sparseGridVertexes || !outCoords) return false;

    OCIndex dimensionCount = SparseSamplingGetDimensionCount(ss);
    OCIndex vertexCount = SparseSamplingGetVertexCount(ss);

    if (vertexIndex >= vertexCount || dimensionCount == 0) return false;

    // Get raw data pointer and element size
    const uint8_t *data = OCDataGetBytesPtr(ss->sparseGridVertexes);
    OCIndex elementSize = OCNumberTypeSize(ss->unsignedIntegerType);

    if (!data || elementSize == 0) return false;

    // Calculate offset to the vertex data
    OCIndex vertexOffset = vertexIndex * dimensionCount * elementSize;

    // Extract coordinates directly from raw data based on integer type
    for (OCIndex d = 0; d < dimensionCount; d++) {
        OCIndex coordOffset = vertexOffset + (d * elementSize);

        switch (ss->unsignedIntegerType) {
            case kOCNumberUInt8Type: {
                const uint8_t *ptr = (const uint8_t *)(data + coordOffset);
                outCoords[d] = (OCIndex)*ptr;
                break;
            }
            case kOCNumberUInt16Type: {
                const uint16_t *ptr = (const uint16_t *)(data + coordOffset);
                outCoords[d] = (OCIndex)*ptr;
                break;
            }
            case kOCNumberUInt32Type: {
                const uint32_t *ptr = (const uint32_t *)(data + coordOffset);
                outCoords[d] = (OCIndex)*ptr;
                break;
            }
            case kOCNumberUInt64Type: {
                const uint64_t *ptr = (const uint64_t *)(data + coordOffset);
                outCoords[d] = (OCIndex)*ptr;
                break;
            }
            default:
                return false;
        }
    }

    return true;
}
