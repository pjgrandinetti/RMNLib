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
    OCMutableArrayRef sparseGridVertexes;  // flat array of CFNumber indices
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
    const struct impl_SparseSampling *A = a, *B = b;
    if (!A || !B) return false;
    if (A == B) return true;
    if (A->unsignedIntegerType != B->unsignedIntegerType) return false;
    if (A->encoding != B->encoding &&
        !OCTypeEqual(A->encoding, B->encoding)) return false;
    if (A->dimensionIndexes != B->dimensionIndexes &&
        !OCTypeEqual(A->dimensionIndexes, B->dimensionIndexes)) return false;
    if (A->sparseGridVertexes != B->sparseGridVertexes &&
        !OCTypeEqual(A->sparseGridVertexes, B->sparseGridVertexes)) return false;
    if (A->description != B->description &&
        !OCTypeEqual(A->description, B->description)) return false;
    if (A->metaData != B->metaData &&
        !OCTypeEqual(A->metaData, B->metaData)) return false;
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
    if (!ss) return true;  // nothing to validate
    // 1) unsignedIntegerType must be one of the four unsigned types
    switch (ss->unsignedIntegerType) {
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            break;
        default:
            if (outError)
                *outError = STR(
                    "SparseSampling validation error: "
                    "unsignedIntegerType must be UInt8/16/32/64");
            return false;
    }
    // 2) encoding must be "none" or "base64"
    if (!ss->encoding ||
        (!OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueNone)) &&
         !OCStringEqual(ss->encoding, STR(kSparseSamplingEncodingValueBase64)))) {
        if (outError)
            *outError = STR(
                "SparseSampling validation error: "
                "encoding must be \"none\" or \"base64\"");
        return false;
    }
    // 3) dimensionIndexes must exist
    if (!ss->dimensionIndexes) {
        if (outError)
            *outError = STR(
                "SparseSampling validation error: missing dimensionIndexes");
        return false;
    }
    // 4) sparseGridVertexes must exist and be a flat array of integers
    if (!ss->sparseGridVertexes) {
        if (outError)
            *outError = STR(
                "SparseSampling validation error: missing sparseGridVertexes");
        return false;
    }
    OCIndex ndim = OCIndexSetGetCount(ss->dimensionIndexes);
    if (ndim == 0) {
        if (outError)
            *outError = STR(
                "SparseSampling validation error: dimensionIndexes must contain at least one index");
        return false;
    }
    OCIndex total = OCArrayGetCount(ss->sparseGridVertexes);
    if (total % ndim != 0) {
        if (outError)
            *outError = OCStringCreateWithFormat(
                STR("SparseSampling validation error: "
                    "sparseGridVertexes length (%ld) is not divisible by number of sparse dimensions (%ld)"),
                (long)total, (long)ndim);
        return false;
    }
    for (OCIndex i = 0; i < total; ++i) {
        OCTypeRef v = OCArrayGetValueAtIndex(ss->sparseGridVertexes, i);
        if (OCGetTypeID(v) != OCNumberGetTypeID()) {
            if (outError)
                *outError = STR(
                    "SparseSampling validation error: "
                    "sparseGridVertexes must contain only integer numbers");
            return false;
        }
    }
    // All checks passed
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
    // 1) Validate unsignedIntegerType
    switch (unsignedIntegerType) {
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            break;
        default:
            if (outError) {
                *outError = STR(
                    "Invalid unsigned_integer_type: must be one of "
                    "kOCNumberUInt8Type, kOCNumberUInt16Type, "
                    "kOCNumberUInt32Type or kOCNumberUInt64Type");
            }
            return NULL;
    }
    // 2) Validate encoding
    if (!encoding ||
        (!OCStringEqual(encoding, STR(kSparseSamplingEncodingValueNone)) &&
         !OCStringEqual(encoding, STR(kSparseSamplingEncodingValueBase64)))) {
        if (outError) {
            *outError = STR(
                "Invalid encoding: must be \"" kSparseSamplingEncodingValueNone
                "\" or \"" kSparseSamplingEncodingValueBase64 "\"");
        }
        return NULL;
    }
    // 3) Allocate & initialize default fields
    struct impl_SparseSampling *ss = SparseSamplingAllocate();
    if (!ss) {
        if (outError) *outError = STR("Allocation failed");
        return NULL;
    }
    impl_InitSparseSamplingFields((SparseSamplingRef)ss);
    // 4) Swap in provided values
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
    if (!validateSparseSampling((SparseSamplingRef)ss, outError)) {
        OCRelease(ss);
        return NULL;
    }
    return (SparseSamplingRef)ss;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Dictionary → object
SparseSamplingRef SparseSamplingCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("input dictionary is NULL");
        return NULL;
    }
    // 1) Pull out each field from the dictionary
    OCArrayRef idxArr = OCDictionaryGetValue(dict, STR(kSparseSamplingDimensionIndexesKey));
    OCIndexSetRef dims = NULL;
    if (idxArr && OCGetTypeID(idxArr) == OCArrayGetTypeID()) {
        dims = OCIndexSetCreateMutable();
        OCIndex n = OCArrayGetCount(idxArr);
        for (OCIndex i = 0; i < n; ++i) {
            OCNumberRef num = OCArrayGetValueAtIndex(idxArr, i);
            long v = 0;
            OCNumberTryGetLong(num, &v);
            OCIndexSetAddIndex((OCMutableIndexSetRef)dims, (OCIndex)v);
        }
    }
    OCArrayRef verts = OCDictionaryGetValue(dict, STR(kSparseSamplingSparseGridVertexesKey));
    OCNumberRef numType = OCDictionaryGetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey));
    OCNumberType uintType = kOCNumberUInt64Type;
    if (numType) {
        int tmp = 0;
        OCNumberTryGetInt(numType, &tmp);
        uintType = (OCNumberType)tmp;
    }
    OCStringRef enc = OCDictionaryGetValue(dict, STR(kSparseSamplingEncodingKey));
    OCStringRef desc = OCDictionaryGetValue(dict, STR(kSparseSamplingDescriptionKey));
    OCDictionaryRef md = OCDictionaryGetValue(dict, STR(kSparseSamplingMetaDataKey));
    // 2) Create the SparseSampling object
    OCStringRef createErr = NULL;
    SparseSamplingRef ss =
        SparseSamplingCreate(
            dims,   // may be NULL
            verts,  // may be NULL
            uintType,
            enc ?: STR(kSparseSamplingEncodingValueNone),
            desc,
            md,
            &createErr);
    if (dims) OCRelease(dims);
    if (!ss) {
        printf("[DEBUG] SparseSamplingCreateFromDictionary failed: %s\n",
               createErr ? OCStringGetCString(createErr) : "(null)");
        if (outError) *outError = createErr;
        return NULL;
    }
    // 3) Validate before returning
    if (!validateSparseSampling(ss, outError)) {
        OCRelease(ss);
        return NULL;
    }
    return ss;
}
/*–––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––*/
// Dictionary serialization
OCDictionaryRef SparseSamplingCopyAsDictionary(SparseSamplingRef ss) {
    if (!ss) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    // dimension_indexes
    OCArrayRef idxArr = OCIndexSetCreateOCNumberArray(ss->dimensionIndexes);
    if (idxArr) {
        OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), idxArr);
        OCRelease(idxArr);
    }
    printf("[DEBUG] Parsed dimension_indexes count = %llu\n",
       (unsigned long long)OCArrayGetCount(idxArr));

    // sparse_grid_vertexes
    OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), ss->sparseGridVertexes);
    // unsigned_integer_type
    OCNumberRef num = OCNumberCreateWithInt(ss->unsignedIntegerType);
    OCDictionarySetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey), num);
    OCRelease(num);
    // encoding
    OCDictionarySetValue(dict, STR(kSparseSamplingEncodingKey), ss->encoding);
    // description
    if (ss->description) {
        OCStringRef d = OCStringCreateCopy(ss->description);
        OCDictionarySetValue(dict, STR(kSparseSamplingDescriptionKey), d);
        OCRelease(d);
    }
    // metadata
    if (ss->metaData) {
        OCDictionaryRef mdc = (OCDictionaryRef)OCTypeDeepCopyMutable(ss->metaData);
        OCDictionarySetValue(dict, STR(kSparseSamplingMetaDataKey), mdc);
        OCRelease(mdc);
    }
    return dict;
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

    // --- dimension_indexes → array of integers
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingDimensionIndexesKey);
    if (cJSON_IsArray(item)) {
        OCMutableArrayRef idxArr = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *elem;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                OCNumberRef n = OCNumberCreateWithLong((long)elem->valuedouble);
                OCArrayAppendValue(idxArr, n);
                OCRelease(n);
            }
        }
        OCDictionarySetValue(dict, STR(kSparseSamplingDimensionIndexesKey), idxArr);
        OCRelease(idxArr);
    }

    // --- encoding
    OCStringRef encodingValue = STR(kSparseSamplingEncodingValueNone);  // default
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingEncodingKey);
    if (cJSON_IsString(item)) {
        encodingValue = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kSparseSamplingEncodingKey), encodingValue);
        OCRelease(encodingValue);  // safe since dictionary retains
    }

    // --- unsigned_integer_type
    OCNumberType utype = kOCNumberUInt64Type;
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingUnsignedIntegerTypeKey);
    if (cJSON_IsString(item)) {
        const char *ts = item->valuestring;
        if (strcmp(ts, "uint8") == 0)       utype = kOCNumberUInt8Type;
        else if (strcmp(ts, "uint16") == 0) utype = kOCNumberUInt16Type;
        else if (strcmp(ts, "uint32") == 0) utype = kOCNumberUInt32Type;
        else if (strcmp(ts, "uint64") == 0) utype = kOCNumberUInt64Type;
    }
    OCNumberRef ntype = OCNumberCreateWithInt((int)utype);
    OCDictionarySetValue(dict, STR(kSparseSamplingUnsignedIntegerTypeKey), ntype);
    OCRelease(ntype);

    // --- sparse_grid_vertexes
    item = cJSON_GetObjectItemCaseSensitive(json, kSparseSamplingSparseGridVertexesKey);
    if (cJSON_IsArray(item)) {
        // plain array of integers
        OCMutableArrayRef verts = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *elem;
        cJSON_ArrayForEach(elem, item) {
            if (cJSON_IsNumber(elem)) {
                OCNumberRef n = OCNumberCreateWithLong((long)elem->valuedouble);
                OCArrayAppendValue(verts, n);
                OCRelease(n);
            }
        }
        printf("[DEBUG] Decoded sparse vertexes: count = %lu\n", (unsigned long)OCArrayGetCount(verts));
        OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), verts);
        OCRelease(verts);
    } else if (cJSON_IsString(item) &&
               OCStringEqual(encodingValue, STR(kSparseSamplingEncodingValueBase64))) {
        OCStringRef b64 = OCStringCreateWithCString(item->valuestring);
        OCDataRef decoded = OCDataCreateFromBase64EncodedString(b64);
        OCRelease(b64);

        if (decoded) {
            int itemSize = OCNumberTypeSize(utype);
            printf("[DEBUG] Using itemSize = %d bytes for OCNumberType = %d\n", itemSize, utype);
            uint64_t count = OCDataGetLength(decoded) / itemSize;
            printf("[DEBUG] Decoded sparse vertex data size = %llu bytes (count = %llu)\n",
                   (unsigned long long)OCDataGetLength(decoded),
                   (unsigned long long)count);

            const uint8_t *bytes = OCDataGetBytesPtr(decoded);
            OCMutableArrayRef verts = OCArrayCreateMutable((OCIndex)count, &kOCTypeArrayCallBacks);
            for (uint64_t i = 0; i < count; ++i) {
                OCNumberRef n = NULL;
                switch (utype) {
                    case kOCNumberUInt8Type:
                        n = OCNumberCreateWithUInt8(*(uint8_t *)(bytes + i));
                        break;
                    case kOCNumberUInt16Type:
                        n = OCNumberCreateWithUInt16(*(uint16_t *)(bytes + i * 2));
                        break;
                    case kOCNumberUInt32Type:
                        n = OCNumberCreateWithUInt32(*(uint32_t *)(bytes + i * 4));
                        break;
                    case kOCNumberUInt64Type:
                        n = OCNumberCreateWithUInt64(*(uint64_t *)(bytes + i * 8));
                        break;
                    default:
                        break;
                }
                if (n) {
                    OCArrayAppendValue(verts, n);
                    OCRelease(n);
                }
            }
            printf("[DEBUG] Parsed sparse vertex count = %llu\n", (unsigned long long)OCArrayGetCount(verts));
            OCDictionarySetValue(dict, STR(kSparseSamplingSparseGridVertexesKey), verts);
            OCRelease(verts);
            OCRelease(decoded);
        } else {
            printf("[ERROR] Base64 decoding of sparse vertexes failed\n");
        }
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
