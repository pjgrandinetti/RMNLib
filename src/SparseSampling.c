/* SparseSampling OCType implementation */
#include "RMNLibrary.h"
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// SparseSampling dictionary keys
#define kSparseSamplingDimensionIndexesKey "dimension_indexes"
#define kSparseSamplingSparseGridVertexesKey "sparse_grid_vertexes"
#define kSparseSamplingUnsignedIntegerTypeKey "unsigned_integer_type"
#define kSparseSamplingEncodingKey "encoding"
#define kSparseSamplingDescriptionKey "description"
#define kSparseSamplingMetaDataKey "metadata"
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Type registration
static OCTypeID kSparseSamplingID = kOCNotATypeID;
OCTypeID SparseSamplingGetTypeID(void) {
    if (kSparseSamplingID == kOCNotATypeID) {
        kSparseSamplingID = OCRegisterType("SparseSampling");
    }
    return kSparseSamplingID;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// ivar struct
struct impl_SparseSampling {
    OCBase base;
    OCIndexSetRef dimensionIndexes;
    OCMutableArrayRef sparseGridVertexes;  // Array of Index Pairs
    OCNumberType unsignedIntegerType;      // UInt8/16/32/64 only
    OCStringRef encoding;                  // "none" or "base64"
    OCStringRef description;
    OCDictionaryRef metaData;
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
    OCRelease(ss->metaData);
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
    if (!OCTypeEqual(A->metaData, B->metaData))
        return false;
    // Handle NULL sparseGridVertexes explicitly
    if (!A->sparseGridVertexes && !B->sparseGridVertexes)
        return true;
    if (!A->sparseGridVertexes || !B->sparseGridVertexes)
        return false;
    // Compare lengths
    OCIndex count1 = OCArrayGetCount(A->sparseGridVertexes);
    OCIndex count2 = OCArrayGetCount(B->sparseGridVertexes);
    if (count1 != count2)
        return false;
    // Compare each OCIndexPairSetRef
    for (OCIndex i = 0; i < count1; ++i) {
        OCTypeRef p1 = OCArrayGetValueAtIndex(A->sparseGridVertexes, i);
        OCTypeRef p2 = OCArrayGetValueAtIndex(B->sparseGridVertexes, i);
        if (!OCTypeEqual(p1, p2))
            return false;
    }
    return true;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Copy‐formatting description
static OCStringRef impl_SparseSamplingCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_SparseSampling *ss = (const void *)cf;
    return OCStringCreateWithFormat(
        STR("<SparseSampling dims=%@, verts=%lu, uint=%d, enc=%@>"),
        ss->dimensionIndexes,
        (unsigned long)OCArrayGetCount(ss->sparseGridVertexes),
        ss->unsignedIntegerType,
        ss->encoding);
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// JSON serialization
static cJSON *impl_SparseSamplingCreateJSON(const void *obj) {
    if (!obj) return cJSON_CreateNull();
    SparseSamplingRef ss = (SparseSamplingRef)obj;
    OCDictionaryRef dict = SparseSamplingCopyAsDictionary(ss);
    if (!dict) return cJSON_CreateNull();
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    OCRelease(dict);
    return json;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Deep‐copy
static void *impl_SparseSamplingDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    SparseSamplingRef src = (SparseSamplingRef)ptr;
    OCDictionaryRef dict = SparseSamplingCopyAsDictionary(src);
    if (!dict) return NULL;
    OCStringRef err = NULL;
    SparseSamplingRef copy = SparseSamplingCreateFromDictionary(dict, &err);
    OCRelease(dict);
    return copy;
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
        impl_SparseSamplingCreateJSON,
        impl_SparseSamplingDeepCopy,
        impl_SparseSamplingDeepCopy);
}
static void impl_InitSparseSamplingFields(SparseSamplingRef ss) {
    ss->dimensionIndexes = OCIndexSetCreateMutable();
    ss->sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ss->unsignedIntegerType = kOCNumberUInt64Type;
    ss->encoding = STR(kSparseSamplingEncodingValueNone);
    ss->description = STR("");
    ss->metaData = OCDictionaryCreateMutable(0);
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
    // 3) dimensionIndexes must be a valid OCIndexSet with at least one entry
    if (!ss->dimensionIndexes || OCGetTypeID(ss->dimensionIndexes) != OCIndexSetGetTypeID()) {
        if (outError)
            *outError = STR("SparseSampling validation error: dimensionIndexes must be a valid OCIndexSet");
        return false;
    }
    OCIndex ndim = OCIndexSetGetCount(ss->dimensionIndexes);
    if (ndim == 0) {
        if (outError)
            *outError = STR("SparseSampling validation error: dimensionIndexes must contain at least one index");
        return false;
    }
    // 4) sparseGridVertexes must be a valid OCArray of OCIndexPairSetRef
    if (!ss->sparseGridVertexes || OCGetTypeID(ss->sparseGridVertexes) != OCArrayGetTypeID()) {
        if (outError)
            *outError = STR("SparseSampling validation error: sparseGridVertexes must be a valid OCArray of OCIndexPairSet");
        return false;
    }
    OCIndex count = OCArrayGetCount(ss->sparseGridVertexes);
    for (OCIndex i = 0; i < count; ++i) {
        OCTypeRef entry = OCArrayGetValueAtIndex(ss->sparseGridVertexes, i);
        if (!entry || OCGetTypeID(entry) != OCIndexPairSetGetTypeID()) {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("SparseSampling validation error: sparseGridVertexes[%ld] is not a valid OCIndexPairSetRef"),
                    (long)i);
            }
            return false;
        }
        if (OCIndexPairSetGetCount((OCIndexPairSetRef)entry) != ndim) {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("SparseSampling validation error: sparseGridVertexes[%ld] must contain %ld (i,value) pairs"),
                    (long)i, (long)ndim);
            }
            return false;
        }
    }
    return true;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Public creator
SparseSamplingRef SparseSamplingCreate(
    OCIndexSetRef dimensionIndexes,
    OCArrayRef sparseGridVertexes,
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
        ss->dimensionIndexes = OCIndexSetCreateMutableCopy(dimensionIndexes);
    }
    if (sparseGridVertexes) {
        OCRelease(ss->sparseGridVertexes);
        ss->sparseGridVertexes = OCArrayCreateMutableCopy(sparseGridVertexes);
    }
    ss->unsignedIntegerType = unsignedIntegerType;
    OCRelease(ss->encoding);
    ss->encoding = OCStringCreateCopy(encoding);
    OCRelease(ss->description);
    ss->description = description
                          ? OCStringCreateCopy(description)
                          : STR("");
    OCRelease(ss->metaData);
    ss->metaData = metadata
                       ? (OCDictionaryRef)OCTypeDeepCopyMutable(metadata)
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
    if (dimIdxNumbers) {
        OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), dimIdxNumbers);
        OCRelease(dimIdxNumbers);
    }
    // 2. sparse_grid_vertexes: flatten OCArray of OCIndexPairSet into OCNumber array
    OCIndex ndim = OCIndexSetGetCount(ss->dimensionIndexes);
    OCIndex nVerts = OCArrayGetCount(ss->sparseGridVertexes);
    OCMutableArrayRef flatVerts = OCArrayCreateMutable(nVerts * ndim, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < nVerts; ++i) {
        OCIndexPairSetRef vertex = (OCIndexPairSetRef)OCArrayGetValueAtIndex(ss->sparseGridVertexes, i);
        if (!vertex || OCIndexPairSetGetCount(vertex) != ndim) continue;
        OCIndexPair *pairs = OCIndexPairSetGetBytesPtr(vertex);
        for (OCIndex j = 0; j < ndim; ++j) {
            OCNumberRef num = OCNumberCreateWithOCIndex(pairs[j].value);  // Only values define vertex
            OCArrayAppendValue(flatVerts, num);
            OCRelease(num);
        }
    }
    if (OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64))) {
        // Encode flatVerts into Base64 binary form
        OCIndex flatCount = OCArrayGetCount(flatVerts);
        int itemSize = OCNumberTypeSize(ss->unsignedIntegerType);
        OCMutableDataRef bin = OCDataCreateMutable(flatCount * itemSize);
        uint8_t *bytes = OCDataGetMutableBytes(bin);
        for (OCIndex i = 0; i < flatCount; ++i) {
            OCNumberRef num = (OCNumberRef)OCArrayGetValueAtIndex(flatVerts, i);
            switch (ss->unsignedIntegerType) {
                case kOCNumberUInt8Type: {
                    uint8_t v = 0;
                    OCNumberGetValue(num, kOCNumberUInt8Type, &v);
                    bytes[i] = v;
                    break;
                }
                case kOCNumberUInt16Type: {
                    uint16_t v = 0;
                    OCNumberGetValue(num, kOCNumberUInt16Type, &v);
                    ((uint16_t *)bytes)[i] = v;
                    break;
                }
                case kOCNumberUInt32Type: {
                    uint32_t v = 0;
                    OCNumberGetValue(num, kOCNumberUInt32Type, &v);
                    ((uint32_t *)bytes)[i] = v;
                    break;
                }
                case kOCNumberUInt64Type: {
                    uint64_t v = 0;
                    OCNumberGetValue(num, kOCNumberUInt64Type, &v);
                    ((uint64_t *)bytes)[i] = v;
                    break;
                }
                default:
                    break;
            }
        }
        OCStringRef b64 = OCDataCreateBase64EncodedString(bin, 0);
        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), b64);
        OCRelease(b64);
        OCRelease(bin);
    } else {
        // Store flatVerts directly as OCArray of OCNumber
        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), flatVerts);
    }
    OCRelease(flatVerts);
    // 3. unsigned_integer_type
    OCNumberRef numType = OCNumberCreateWithInt(ss->unsignedIntegerType);
    OCDictionarySetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey), numType);
    OCRelease(numType);
    // 4. encoding
    OCDictionarySetValue(dict, STR(kSparseSamplingEncodingKey), ss->encoding);
    // 5. description
    if (ss->description) {
        OCStringRef descCopy = OCStringCreateCopy(ss->description);
        OCDictionarySetValue(dict, STR(kSparseSamplingDescriptionKey), descCopy);
        OCRelease(descCopy);
    }
    // 6. metadata
    if (ss->metaData) {
        OCDictionaryRef mdCopy = (OCDictionaryRef)OCTypeDeepCopyMutable(ss->metaData);
        OCDictionarySetValue(dict, STR(kSparseSamplingMetaDataKey), mdCopy);
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
    OCIndexSetRef dimSet = NULL;
    OCArrayRef idxArr = OCDictionaryGetValue(dict, STR(kSparseSamplingDimensionIndexesKey));
    if (idxArr && OCGetTypeID(idxArr) == OCArrayGetTypeID()) {
        dimSet = OCIndexSetCreateMutable();
        for (OCIndex i = 0; i < OCArrayGetCount(idxArr); ++i) {
            OCNumberRef num = OCArrayGetValueAtIndex(idxArr, i);
            long v = 0;
            if (OCNumberTryGetLong(num, &v)) {
                OCIndexSetAddIndex((OCMutableIndexSetRef)dimSet, (OCIndex)v);
            }
        }
    }
    OCIndex ndim = dimSet ? OCIndexSetGetCount(dimSet) : 0;
    // 2. Parse unsigned_integer_type
    OCNumberType utype = kOCNumberUInt64Type;
    OCNumberRef numType = OCDictionaryGetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey));
    if (numType) {
        int tmp = 0;
        if (OCNumberTryGetInt(numType, &tmp)) {
            utype = (OCNumberType)tmp;
        }
    }
    // 3. Parse encoding
    OCStringRef enc = OCDictionaryGetValue(dict, STR(kSparseSamplingEncodingKey));
    if (!enc) enc = STR(kSparseSamplingEncodingValueNone);
    // 4. Parse sparse_grid_vertexes (OCArray or OCString → OCArray of OCIndexPairSet)
    OCArrayRef flat = NULL;
    OCTypeRef raw = OCDictionaryGetValue(dict, STR(kSparseSamplingSparseGridVertexesKey));
    OCMutableArrayRef gridVerts = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (OCGetTypeID(raw) == OCStringGetTypeID() &&
        OCStringEqual(enc, STR(kSparseSamplingEncodingValueBase64))) {
        OCDataRef bin = OCDataCreateFromBase64EncodedString((OCStringRef)raw);
        if (!bin) {
            if (outError) *outError = STR("Base64 decoding failed");
            OCRelease(dimSet);
            OCRelease(gridVerts);
            return NULL;
        }
        OCIndex totalItems = OCDataGetLength(bin) / OCNumberTypeSize(utype);
        const uint8_t *bytes = OCDataGetBytesPtr(bin);
        for (OCIndex i = 0; i < totalItems / ndim; ++i) {
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
                OCIndexPairSetAddIndexPair(ps, j, value);
            }
            OCArrayAppendValue(gridVerts, ps);
            OCRelease(ps);
        }
        OCRelease(bin);
    } else if (OCGetTypeID(raw) == OCArrayGetTypeID()) {
        flat = (OCArrayRef)raw;
        OCIndex total = OCArrayGetCount(flat);
        if (ndim > 0 && total % ndim == 0) {
            OCIndex vertexCount = total / ndim;
            for (OCIndex i = 0; i < vertexCount; ++i) {
                OCMutableIndexPairSetRef ps = OCIndexPairSetCreateMutable();
                for (OCIndex j = 0; j < ndim; ++j) {
                    OCIndex flatIdx = i * ndim + j;
                    OCNumberRef n = OCArrayGetValueAtIndex(flat, flatIdx);
                    long v = 0;
                    if (OCNumberTryGetLong(n, &v)) {
                        OCIndexPairSetAddIndexPair(ps, j, (OCIndex)v);
                    }
                }
                OCArrayAppendValue(gridVerts, ps);
                OCRelease(ps);
            }
        } else if (outError && ndim > 0) {
            *outError = OCStringCreateWithFormat(
                STR("sparse_grid_vertexes size (%ld) is not divisible by number of dimensions (%ld)"),
                (long)total, (long)ndim);
        }
    }
    // 5. Parse optional fields
    OCStringRef desc = OCDictionaryGetValue(dict, STR(kSparseSamplingDescriptionKey));
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kSparseSamplingMetaDataKey));
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
/**
 * Parse a cJSON tree into an OCDictionary suitable for
 * passing to SparseSamplingCreateFromDictionary().
 */
static OCDictionaryRef
SparseSamplingDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError)
            *outError = STR("Expected top-level JSON object for SparseSampling");
        return NULL;
    }

    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item;

    // --- dimension_indexes → OCArray of OCNumber
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingDimensionIndexesKey);
    OCMutableArrayRef dimIndexes = NULL;
    if (cJSON_IsArray(item)) {
        dimIndexes = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *elem;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                OCNumberRef n = OCNumberCreateWithLong((long)elem->valuedouble);
                OCArrayAppendValue(dimIndexes, n);
                OCRelease(n);
            }
        }
        if (OCArrayGetCount(dimIndexes) > 0) {
            OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), dimIndexes);
        }
        OCRelease(dimIndexes);
    }

    OCIndex ndim = dimIndexes ? OCArrayGetCount(dimIndexes) : 0;

    // --- encoding
    OCStringRef encodingValue = STR(kSparseSamplingEncodingValueNone);
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingEncodingKey);
    if (cJSON_IsString(item)) {
        encodingValue = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSparseSamplingEncodingKey), encodingValue);
        OCRelease(encodingValue);  // retained by dictionary
    }

    // --- required: unsigned_integer_type
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingUnsignedIntegerTypeKey);
    if (!cJSON_IsString(item)) {
        if (outError) {
            *outError = STR("Missing required field: 'unsigned_integer_type' must be a string.");
        }
        OCRelease(dict);
        return NULL;
    }

    const char *ts = item->valuestring;
    OCNumberType utype = kOCNumberUInt64Type;
    bool valid = true;
    if (strcmp(ts, "uint8") == 0)       utype = kOCNumberUInt8Type;
    else if (strcmp(ts, "uint16") == 0) utype = kOCNumberUInt16Type;
    else if (strcmp(ts, "uint32") == 0) utype = kOCNumberUInt32Type;
    else if (strcmp(ts, "uint64") == 0) utype = kOCNumberUInt64Type;
    else valid = false;

    if (!valid) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Invalid 'unsigned_integer_type' value: \"%s\". Must be one of: uint8, uint16, uint32, uint64."),
                ts);
        }
        OCRelease(dict);
        return NULL;
    }

    OCNumberRef ntype = OCNumberCreateWithInt((int)utype);
    OCDictionarySetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey), ntype);
    OCRelease(ntype);

    // --- sparse_grid_vertexes
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingSparseGridVertexesKey);
    if (cJSON_IsString(item) &&
        OCStringEqual(encodingValue, STR(kSparseSamplingEncodingValueBase64))) {
        if (strlen(item->valuestring) == 0) {
            if (outError)
                *outError = STR("Base64 sparse_grid_vertexes is an empty string.");
            OCRelease(dict);
            return NULL;
        }
        OCStringRef b64 = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), b64);
        OCRelease(b64);
    } else if (cJSON_IsArray(item)) {
        OCMutableArrayRef flatVerts = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *elem;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                OCNumberRef n = OCNumberCreateWithLong((long)elem->valuedouble);
                OCArrayAppendValue(flatVerts, n);
                OCRelease(n);
            }
        }

        OCIndex total = OCArrayGetCount(flatVerts);
        if (total == 0) {
            if (outError)
                *outError = STR("sparse_grid_vertexes must not be empty.");
            OCRelease(flatVerts);
            OCRelease(dict);
            return NULL;
        }

        if (ndim == 0 || total % ndim != 0) {
            if (outError)
                *outError = OCStringCreateWithFormat(
                    STR("Expected sparse_grid_vertexes length (%ld) to be divisible by number of dimensions (%ld)"),
                    (long)total, (long)ndim);
            OCRelease(flatVerts);
            OCRelease(dict);
            return NULL;
        }

        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), flatVerts);
        OCRelease(flatVerts);
    }

    // --- description
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef s = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSparseSamplingDescriptionKey), s);
        OCRelease(s);
    }

    // --- metadata
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingMetaDataKey);
    if (cJSON_IsObject(item)) {
        OCMutableDictionaryRef md = OCDictionaryCreateMutable(0);
        cJSON *mdField;
        cJSON_ArrayForEach(mdField, item) {
            if (cJSON_IsString(mdField)) {
                OCStringRef keyStr = OCStringCreateWithCString(mdField->string);
                OCStringRef valStr = OCStringCreateWithCString(mdField->valuestring);
                OCDictionarySetValue(md, keyStr, valStr);
                OCRelease(keyStr);
                OCRelease(valStr);
            }
        }
        OCDictionarySetValue(dict, STR(kSparseSamplingMetaDataKey), md);
        OCRelease(md);
    }

    return dict;
}
/**
 * Build a SparseSamplingRef directly from a cJSON tree
 * by first converting it to an OCDictionary and then
 * calling your existing SparseSamplingCreateFromDictionary().
 */
SparseSamplingRef SparseSamplingCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    OCDictionaryRef dict = SparseSamplingDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    SparseSamplingRef ss = SparseSamplingCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return ss;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Getters / Setters
OCIndexSetRef SparseSamplingGetDimensionIndexes(SparseSamplingRef ss) {
    return ss ? ss->dimensionIndexes : NULL;
}
bool SparseSamplingSetDimensionIndexes(SparseSamplingRef ss, OCIndexSetRef idxs) {
    if (!ss) return false;
    OCRelease(ss->dimensionIndexes);
    ss->dimensionIndexes = idxs
                               ? OCIndexSetCreateMutableCopy(idxs)
                               : OCIndexSetCreateMutable();
    return ss->dimensionIndexes != NULL;
}
OCArrayRef SparseSamplingGetSparseGridVertexes(SparseSamplingRef ss) {
    return ss ? ss->sparseGridVertexes : NULL;
}
bool SparseSamplingSetSparseGridVertexes(SparseSamplingRef ss, OCArrayRef verts) {
    if (!ss) return false;
    OCIndex ndim = ss->dimensionIndexes ? OCIndexSetGetCount(ss->dimensionIndexes) : 0;
    if (verts) {
        OCIndex count = OCArrayGetCount(verts);
        for (OCIndex i = 0; i < count; ++i) {
            OCTypeRef entry = OCArrayGetValueAtIndex(verts, i);
            if (!entry || OCGetTypeID(entry) != OCIndexPairSetGetTypeID()) {
                fprintf(stderr, "[SparseSampling] ERROR: Element at index %ld is not a valid OCIndexPairSet\n", (long)i);
                return false;
            }
            if (ndim > 0 && OCIndexPairSetGetCount((OCIndexPairSetRef)entry) != ndim) {
                fprintf(stderr, "[SparseSampling] ERROR: OCIndexPairSet at index %ld has incorrect size (expected %ld)\n", (long)i, (long)ndim);
                return false;
            }
        }
    }
    OCRelease(ss->sparseGridVertexes);
    ss->sparseGridVertexes = verts
                                 ? OCArrayCreateMutableCopy(verts)
                                 : OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
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
bool SparseSamplingSetDescription(SparseSamplingRef ss, OCStringRef d) {
    if (!ss) return false;
    OCStringRef copy = d ? OCStringCreateCopy(d) : STR("");
    if (!copy) return false;
    OCRelease(ss->description);
    ss->description = copy;
    return true;
}
OCDictionaryRef SparseSamplingGetMetaData(SparseSamplingRef ss) {
    return ss ? ss->metaData : NULL;
}
bool SparseSamplingSetMetaData(SparseSamplingRef ss, OCDictionaryRef md) {
    if (!ss) return false;
    OCRelease(ss->metaData);
    ss->metaData = md
                       ? (OCDictionaryRef)OCTypeDeepCopyMutable(md)
                       : OCDictionaryCreateMutable(0);
    return ss->metaData != NULL;
}
