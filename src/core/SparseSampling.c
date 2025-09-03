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
    OCIndexSetRef dimensionIndexes;
    OCIndexPairSetRef sparseGridVertexes;  // Sparse grid vertex data as index-value pairs
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
        (unsigned long)(ss->sparseGridVertexes ? OCIndexPairSetGetCount(ss->sparseGridVertexes) : 0),
        ss->unsignedIntegerType,
        ss->encoding);
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// JSON serialization
static cJSON *impl_SparseSamplingCopyJSON(const void *obj, bool typed) {
    return SparseSamplingCopyAsJSON((SparseSamplingRef)obj, typed);
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
                                  ? (OCIndexPairSetRef)OCTypeDeepCopy(src->sparseGridVertexes)
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
    ss->sparseGridVertexes = OCIndexPairSetCreateMutable();
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
    OCIndex ndim = OCIndexSetGetCount(ss->dimensionIndexes);
    // Note: Empty dimension indexes (ndim == 0) are allowed
    // 4) sparseGridVertexes must be a valid OCIndexPairSet
    if (!ss->sparseGridVertexes || OCGetTypeID(ss->sparseGridVertexes) != OCIndexPairSetGetTypeID()) {
        if (outError)
            *outError = STR("SparseSampling validation error: sparseGridVertexes must be a valid OCIndexPairSet");
        return false;
    }
    return true;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Public creator
SparseSamplingRef SparseSamplingCreate(
    OCIndexSetRef dimensionIndexes,
    OCIndexPairSetRef sparseGridVertexes,
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
        ss->sparseGridVertexes = (OCIndexPairSetRef)OCTypeDeepCopy(sparseGridVertexes);
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
    // 2. sparse_grid_vertexes: optimize based on encoding type
    OCIndex ndim = OCIndexSetGetCount(ss->dimensionIndexes);
    OCIndex nVerts = OCArrayGetCount(ss->sparseGridVertexes);
    if (OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64))) {
        // Direct binary encoding - avoid creating OCNumber objects
        OCIndex totalCoords = nVerts * ndim;
        int itemSize = OCNumberTypeSize(ss->unsignedIntegerType);
        OCMutableDataRef bin = OCDataCreateMutable(totalCoords * itemSize);
        if (!bin) {
            OCRelease(dict);
            return NULL;
        }
        OCDataSetLength(bin, totalCoords * itemSize);
        uint8_t *bytes = OCDataGetMutableBytes(bin);
        // Direct binary write without intermediate OCNumber objects
        OCIndex writePos = 0;
        for (OCIndex i = 0; i < nVerts; ++i) {
            OCIndexPairSetRef vertex = (OCIndexPairSetRef)OCArrayGetValueAtIndex(ss->sparseGridVertexes, i);
            if (!vertex || OCIndexPairSetGetCount(vertex) != ndim) continue;
            OCIndexPair *pairs = OCIndexPairSetGetBytesPtr(vertex);
            for (OCIndex j = 0; j < ndim; ++j) {
                OCIndex value = pairs[j].value;
                switch (ss->unsignedIntegerType) {
                    case kOCNumberUInt8Type:
                        ((uint8_t *)bytes)[writePos] = (uint8_t)value;
                        break;
                    case kOCNumberUInt16Type:
                        ((uint16_t *)bytes)[writePos] = (uint16_t)value;
                        break;
                    case kOCNumberUInt32Type:
                        ((uint32_t *)bytes)[writePos] = (uint32_t)value;
                        break;
                    case kOCNumberUInt64Type:
                        ((uint64_t *)bytes)[writePos] = (uint64_t)value;
                        break;
                    default:
                        break;
                }
                writePos++;
            }
        }
        OCStringRef b64 = OCDataCreateBase64EncodedString(bin, 0);
        if (b64) {
            OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), b64);
            OCRelease(b64);
        }
        OCRelease(bin);
    } else {
        // For "none" encoding, we still need OCNumber array but optimize creation
        OCMutableArrayRef flatVerts = OCArrayCreateMutable(nVerts * ndim, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < nVerts; ++i) {
            OCIndexPairSetRef vertex = (OCIndexPairSetRef)OCArrayGetValueAtIndex(ss->sparseGridVertexes, i);
            if (!vertex || OCIndexPairSetGetCount(vertex) != ndim) continue;
            OCIndexPair *pairs = OCIndexPairSetGetBytesPtr(vertex);
            for (OCIndex j = 0; j < ndim; ++j) {
                OCNumberRef num = OCNumberCreateWithOCIndex(pairs[j].value);
                OCArrayAppendValue(flatVerts, num);
                OCRelease(num);
            }
        }
        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), flatVerts);
        OCRelease(flatVerts);
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
    // 4. Parse sparse_grid_vertexes (OCArray or OCString → OCArray of OCIndexPairSet)
    OCArrayRef flat = NULL;
    OCTypeRef raw = OCDictionaryGetValue(dict, STR(kSparseSamplingSparseGridVertexesKey));
    OCMutableArrayRef gridVerts = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (OCGetTypeID(raw) == OCStringGetTypeID() &&
        OCStringEqual(enc, STR(kSparseSamplingEncodingValueBase64))) {
        // Optimized binary decoding - avoid intermediate OCNumber array
        OCDataRef bin = OCDataCreateFromBase64EncodedString((OCStringRef)raw);
        if (!bin) {
            if (outError) *outError = STR("Base64 decoding failed");
            OCRelease(dimSet);
            OCRelease(gridVerts);
            return NULL;
        }
        OCIndex totalItems = OCDataGetLength(bin) / OCNumberTypeSize(utype);
        if (ndim > 0 && totalItems % ndim != 0) {
            if (outError)
                *outError = OCStringCreateWithFormat(STR("Binary data size mismatch: %ld items not divisible by %ld dimensions"),
                                                     (long)totalItems, (long)ndim);
            OCRelease(bin);
            OCRelease(dimSet);
            OCRelease(gridVerts);
            return NULL;
        }
        const uint8_t *bytes = OCDataGetBytesPtr(bin);
        OCIndex vertexCount = ndim > 0 ? totalItems / ndim : 0;
        // OPTIMIZATION: Get dimension indexes as raw array instead of OCNumber array
        OCIndex *dimIndices = malloc(ndim * sizeof(OCIndex));
        if (!dimIndices) {
            if (outError) *outError = STR("Memory allocation failed for dimension indices");
            OCRelease(bin);
            OCRelease(dimSet);
            OCRelease(gridVerts);
            return NULL;
        }
        // Extract dimension indices directly from the set without OCNumber conversion
        OCIndex dimIdx = 0;
        for (OCIndex d = 0; d < 100 && dimIdx < ndim; ++d) {  // Reasonable upper bound
            if (OCIndexSetContainsIndex(dimSet, d)) {
                dimIndices[dimIdx++] = d;
            }
        }
        // Direct binary-to-IndexPairSet conversion without OCNumber objects
        for (OCIndex i = 0; i < vertexCount; ++i) {
            OCMutableIndexPairSetRef ps = OCIndexPairSetCreateMutable();
            for (OCIndex j = 0; j < ndim; ++j) {
                OCIndex flatIdx = i * ndim + j;
                OCIndex value = 0;
                switch (utype) {
                    case kOCNumberUInt8Type:
                        value = ((uint8_t *)bytes)[flatIdx];
                        break;
                    case kOCNumberUInt16Type:
                        value = ((uint16_t *)bytes)[flatIdx];
                        break;
                    case kOCNumberUInt32Type:
                        value = ((uint32_t *)bytes)[flatIdx];
                        break;
                    case kOCNumberUInt64Type:
                        value = (OCIndex)((uint64_t *)bytes)[flatIdx];
                        break;
                    default:
                        break;
                }
                // Use pre-extracted dimension index directly
                OCIndexPairSetAddIndexPair(ps, dimIndices[j], value);
            }
            OCArrayAppendValue(gridVerts, ps);
            OCRelease(ps);
        }
        free(dimIndices);  // Clean up raw array
        OCRelease(bin);
    } else if (OCGetTypeID(raw) == OCArrayGetTypeID()) {
        flat = (OCArrayRef)raw;
        OCIndex total = OCArrayGetCount(flat);
        if (ndim > 0 && total % ndim == 0) {
            OCIndex vertexCount = total / ndim;
            // OPTIMIZATION: Get dimension indexes as raw array instead of OCNumber array
            OCIndex *dimIndices = malloc(ndim * sizeof(OCIndex));
            if (!dimIndices) {
                if (outError) *outError = STR("Memory allocation failed for dimension indices");
                OCRelease(dimSet);
                OCRelease(gridVerts);
                return NULL;
            }
            // Extract dimension indices directly from the set without OCNumber conversion
            OCIndex dimIdx = 0;
            for (OCIndex d = 0; d < 100 && dimIdx < ndim; ++d) {  // Reasonable upper bound
                if (OCIndexSetContainsIndex(dimSet, d)) {
                    dimIndices[dimIdx++] = d;
                }
            }
            for (OCIndex i = 0; i < vertexCount; ++i) {
                OCMutableIndexPairSetRef ps = OCIndexPairSetCreateMutable();
                for (OCIndex j = 0; j < ndim; ++j) {
                    OCIndex flatIdx = i * ndim + j;
                    OCNumberRef n = OCArrayGetValueAtIndex(flat, flatIdx);
                    long v = 0;
                    if (OCNumberTryGetLong(n, &v)) {
                        // Use pre-extracted dimension index directly
                        OCIndexPairSetAddIndexPair(ps, dimIndices[j], (OCIndex)v);
                    }
                }
                OCArrayAppendValue(gridVerts, ps);
                OCRelease(ps);
            }
            free(dimIndices);  // Clean up raw array
        } else if (outError && ndim > 0) {
            *outError = OCStringCreateWithFormat(
                STR("sparse_grid_vertexes size (%ld) is not divisible by number of dimensions (%ld)"),
                (long)total, (long)ndim);
        }
    }
    // 5. Parse optional fields
    OCStringRef desc = OCDictionaryGetValue(dict, STR(kSparseSamplingDescriptionKey));
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kSparseSamplingApplicationKey));
    // 6. Construct and validate
    OCStringRef createErr = NULL;
    SparseSamplingRef ss = SparseSamplingCreate(dimSet, gridVerts, utype, enc, desc, md, &createErr);
    OCRelease(dimSet);
    OCRelease(gridVerts);
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

cJSON *SparseSamplingCopyAsJSON(SparseSamplingRef ss, bool typed) {
    if (!ss) return cJSON_CreateNull();
    
    cJSON *json = cJSON_CreateObject();
    if (!json) return cJSON_CreateNull();
    
    // 1. dimension_indexes - use OCIndexSet's built-in JSON serialization
    if (ss->dimensionIndexes) {
        cJSON *dimArray = OCIndexSetCopyAsJSON(ss->dimensionIndexes, typed);
        if (dimArray) {
            cJSON_AddItemToObject(json, kSparseSamplingDimensionIndexesKey, dimArray);
        }
    }
    
    // 2. sparse_grid_vertexes - use OCIndexPairSet's built-in JSON serialization
    if (ss->sparseGridVertexes) {
        cJSON *vertsJson = OCIndexPairSetCopyAsJSON(ss->sparseGridVertexes, typed);
        if (vertsJson) {
            cJSON_AddItemToObject(json, kSparseSamplingSparseGridVertexesKey, vertsJson);
        }
    }
    
    // 3. unsigned_integer_type
    const char *typeName = OCNumberGetTypeName(ss->unsignedIntegerType);
    if (typeName) {
        cJSON_AddStringToObject(json, kSparseSamplingUnsignedIntegerTypeKey, typeName);
    }
    
    // 4. encoding
    if (ss->encoding) {
        const char *encStr = OCStringGetCString(ss->encoding);
        if (encStr) {
            cJSON_AddStringToObject(json, kSparseSamplingEncodingKey, encStr);
        }
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
        cJSON *app = OCTypeCopyJSON((OCTypeRef)ss->application, true);
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
    
    cJSON *item;
    
    // Parse dimension_indexes
    OCMutableIndexSetRef dimensionIndexes = OCIndexSetCreateMutable();
    if (!dimensionIndexes) {
        if (outError) *outError = STR("Failed to create dimension indexes set");
        return NULL;
    }
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingDimensionIndexesKey);
    if (cJSON_IsArray(item)) {
        cJSON *elem;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                OCIndexSetAddIndex(dimensionIndexes, (OCIndex)elem->valuedouble);
            }
        }
    }
    OCIndex ndim = OCIndexSetGetCount(dimensionIndexes);
    
    // Parse encoding (with default)
    OCStringRef encoding = STR(kSparseSamplingEncodingValueBase64);
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingEncodingKey);
    if (cJSON_IsString(item)) {
        encoding = OCStringCreateWithCString(item->valuestring);
        if (!encoding) {
            if (outError) *outError = STR("Failed to create encoding string");
            OCRelease(dimensionIndexes);
            return NULL;
        }
    } else {
        OCRetain(encoding); // Retain the default value for consistency
    }
    
    // Parse required unsigned_integer_type
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingUnsignedIntegerTypeKey);
    if (!cJSON_IsString(item)) {
        if (outError) {
            *outError = STR("Missing required field: 'unsigned_integer_type' must be a string.");
        }
        OCRelease(dimensionIndexes);
        OCRelease(encoding);
        return NULL;
    }
    
    const char *ts = item->valuestring;
    OCNumberType unsignedIntegerType = OCNumberTypeFromName(ts);
    if (unsignedIntegerType == kOCNumberTypeInvalid) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Invalid 'unsigned_integer_type' value: \"%s\". Must be one of: uint8, uint16, uint32, uint64."),
                ts);
        }
        OCRelease(dimensionIndexes);
        OCRelease(encoding);
        return NULL;
    }
    
    // Parse sparse_grid_vertexes
    OCMutableArrayRef sparseGridVertexes = NULL;
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingSparseGridVertexesKey);
    
    if (cJSON_IsString(item) && 
        OCStringEqual(encoding, STR(kSparseSamplingEncodingValueBase64))) {
        // Base64 encoded data
        if (strlen(item->valuestring) == 0) {
            if (outError)
                *outError = STR("Base64 sparse_grid_vertexes is an empty string.");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            return NULL;
        }
        // Parse base64 data - this would typically be decoded into sparse grid vertexes
        // For now, create empty array and handle base64 decoding elsewhere
        sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        if (!sparseGridVertexes) {
            if (outError) *outError = STR("Failed to create sparse grid vertexes array");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            return NULL;
        }
    } else if (cJSON_IsArray(item)) {
        // Array format
        int arraySize = cJSON_GetArraySize(item);
        sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        if (!sparseGridVertexes) {
            if (outError) *outError = STR("Failed to create sparse grid vertexes array");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            return NULL;
        }
        
        // Count valid numbers
        cJSON *elem;
        int validCount = 0;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                validCount++;
            }
        }
        
        if (validCount == 0) {
            if (outError)
                *outError = STR("sparse_grid_vertexes must not be empty.");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            OCRelease(sparseGridVertexes);
            return NULL;
        }
        
        if (ndim == 0 || validCount % ndim != 0) {
            if (outError)
                *outError = OCStringCreateWithFormat(
                    STR("Expected sparse_grid_vertexes length (%d) to be divisible by number of dimensions (%ld)"),
                    validCount, (long)ndim);
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            OCRelease(sparseGridVertexes);
            return NULL;
        }
        
        // Convert flat array to vertex arrays
        OCIndex numVertices = validCount / ndim;
        OCIndex valueIndex = 0;
        
        for (OCIndex v = 0; v < numVertices; v++) {
            OCMutableIndexPairSetRef vertex = OCIndexPairSetCreateMutable();
            if (!vertex) {
                if (outError) *outError = STR("Failed to create vertex index pair set");
                OCRelease(dimensionIndexes);
                OCRelease(encoding);
                OCRelease(sparseGridVertexes);
                return NULL;
            }
            
            for (OCIndex d = 0; d < ndim; d++) {
                // Find the next valid number
                while (valueIndex < arraySize) {
                    elem = cJSON_GetArrayItem(item, valueIndex);
                    valueIndex++;
                    if (cJSON_IsNumber(elem)) {
                        OCIndex dimIndex = d; // Use the dimension index directly
                        OCIndex value = (OCIndex)elem->valuedouble;
                        OCIndexPairSetAddIndexPair(vertex, dimIndex, value);
                        break;
                    }
                }
            }
            
            OCArrayAppendValue(sparseGridVertexes, vertex);
            OCRelease(vertex);
        }
    } else {
        // Default empty array
        sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
        if (!sparseGridVertexes) {
            if (outError) *outError = STR("Failed to create default sparse grid vertexes array");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            return NULL;
        }
    }
    
    // Parse optional description
    OCStringRef description = NULL;
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingDescriptionKey);
    if (cJSON_IsString(item)) {
        description = OCStringCreateWithCString(item->valuestring);
        if (!description) {
            if (outError) *outError = STR("Failed to create description string");
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            OCRelease(sparseGridVertexes);
            return NULL;
        }
    } else if (item) {
        // Field is present but not a string - this is an error
        if (outError) *outError = STR("description field must be a string");
        OCRelease(dimensionIndexes);
        OCRelease(encoding);
        OCRelease(sparseGridVertexes);
        return NULL;
    }
    
    // Parse optional application metadata
    OCMutableDictionaryRef application = NULL;
    item = cJSON_GetObjectItemCaseSensitive(actualJson, kSparseSamplingApplicationKey);
    if (cJSON_IsObject(item)) {
        // Use OCDictionary's native JSON deserialization to handle typed data properly
        OCStringRef parseError = NULL;
        OCDictionaryRef parsedDict = (OCDictionaryRef)OCTypeCreateFromJSONTyped(item, &parseError);
        if (!parsedDict) {
            if (outError) {
                *outError = parseError ? parseError : STR("Failed to parse application metadata");
            }
            OCRelease(dimensionIndexes);
            OCRelease(encoding);
            OCRelease(sparseGridVertexes);
            OCRelease(description);
            return NULL;
        }
        if (OCGetTypeID(parsedDict) == OCDictionaryGetTypeID()) {
            application = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(parsedDict);
            if (!application) {
                if (outError) *outError = STR("Failed to create mutable copy of application metadata");
                OCRelease(parsedDict);
                OCRelease(dimensionIndexes);
                OCRelease(encoding);
                OCRelease(sparseGridVertexes);
                OCRelease(description);
                return NULL;
            }
        }
        OCRelease(parsedDict);
    } else if (item) {
        // Field is present but not an object - this is an error
        if (outError) *outError = STR("application field must be an object");
        OCRelease(dimensionIndexes);
        OCRelease(encoding);
        OCRelease(sparseGridVertexes);
        OCRelease(description);
        return NULL;
    }
    
    // Create the SparseSampling object
    OCStringRef createError = NULL;
    SparseSamplingRef ss = SparseSamplingCreate(
        dimensionIndexes,
        sparseGridVertexes,
        unsignedIntegerType,
        encoding,
        description,
        application,
        &createError
    );
    
    // Clean up
    OCRelease(dimensionIndexes);
    OCRelease(sparseGridVertexes);
    OCRelease(encoding);
    OCRelease(description);
    OCRelease(application);
    
    // Propagate creation error if object creation failed
    if (!ss && outError && createError) {
        *outError = createError;
    }
    
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

OCIndexPairSetRef SparseSamplingGetSparseGridVertexes(SparseSamplingRef ss) {
    return ss ? ss->sparseGridVertexes : NULL;
}
OCIndexPairSetRef SparseSamplingCopySparseGridVertexes(SparseSamplingRef ss) {
    if (!ss || !ss->sparseGridVertexes) return NULL;
    return (OCIndexPairSetRef)OCTypeDeepCopy(ss->sparseGridVertexes);
}
bool SparseSamplingSetSparseGridVertexes(SparseSamplingRef ss, OCIndexPairSetRef verts) {
    if (!ss) return false;
    OCRelease(ss->sparseGridVertexes);
    ss->sparseGridVertexes = verts
                                 ? (OCIndexPairSetRef)OCTypeDeepCopy(verts)
                                 : OCIndexPairSetCreateMutable();
    return ss->sparseGridVertexes != NULL;
}
OCNumberType SparseSamplingGetUnsignedIntegerType(SparseSamplingRef ss) {
    return ss ? ss->unsignedIntegerType : kOCNumberUInt64Type;
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
