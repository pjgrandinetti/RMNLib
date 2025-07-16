/* DependentVariable OCType implementation */
#include "DependentVariable.h"
#pragma region Type Registration
static OCTypeID kDependentVariableID = kOCNotATypeID;
struct impl_DependentVariable {
    OCBase base;
    // SIQuantity Type attributes
    SIUnitRef unit;
    OCNumberType numericType;
    // Dependent Variable Type attributes
    OCStringRef name;
    OCStringRef description;
    OCMutableDictionaryRef metaData;
    OCStringRef quantityName;
    OCStringRef quantityType;
    // components...
    OCStringRef type;                   // "internal" / "external"
    OCStringRef encoding;               // "base64" / "none"
    OCStringRef componentsURL;          // for external‐only
    OCMutableArrayRef components;       // And OCArray of OCDataRef types
    OCMutableArrayRef componentLabels;  // And OCArray of OCStringRef types
    // sparse‐sampling metadata
    SparseSamplingRef sparseSampling;
    // weak back‐pointer
    OCTypeRef owner;
};
OCTypeID DependentVariableGetTypeID(void) {
    if (kDependentVariableID == kOCNotATypeID)
        kDependentVariableID = OCRegisterType("DependentVariable");
    return kDependentVariableID;
}
static void impl_DependentVariableFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_DependentVariable *dv = (struct impl_DependentVariable *)ptr;
    // --- string fields ---
    OCRelease(dv->name);
    OCRelease(dv->type);
    OCRelease(dv->encoding);
    OCRelease(dv->description);
    OCRelease(dv->quantityName);
    OCRelease(dv->quantityType);
    OCRelease(dv->componentsURL);
    // --- sparse‐sampling ---
    OCRelease(dv->sparseSampling);
    // --- collection fields ---
    OCRelease(dv->components);
    OCRelease(dv->componentLabels);
    OCRelease(dv->metaData);
    // NOTE: dv->owner is a weak back-pointer — do NOT OCRelease it
}
static bool DependentVariableComponentsAreEqual(const struct impl_DependentVariable *a,
                                                const struct impl_DependentVariable *b) {
    if (!a || !b) return false;
    OCIndex nA = DependentVariableGetComponentCount((DependentVariableRef)a);
    OCIndex nB = DependentVariableGetComponentCount((DependentVariableRef)b);
    if (nA != nB) return false;
    for (OCIndex i = 0; i < nA; ++i) {
        OCDataRef dA = DependentVariableGetComponentAtIndex((DependentVariableRef)a, i);
        OCDataRef dB = DependentVariableGetComponentAtIndex((DependentVariableRef)b, i);
        if (!dA || !dB) return false;
        size_t lenA = OCDataGetLength(dA);
        size_t lenB = OCDataGetLength(dB);
        if (lenA != lenB) return false;
        const void *pA = OCDataGetBytesPtr(dA);
        const void *pB = OCDataGetBytesPtr(dB);
        if (memcmp(pA, pB, lenA) != 0) return false;
    }
    return true;
}
static bool impl_DependentVariableEqual(const void *a, const void *b) {
    const struct impl_DependentVariable *dvA = a;
    const struct impl_DependentVariable *dvB = b;
    if (!dvA || !dvB) return false;
    if (dvA == dvB) return true;
    // 1) URL, unit, numeric type, type, encoding, name
    if (dvA->componentsURL != dvB->componentsURL &&
        !OCTypeEqual(dvA->componentsURL, dvB->componentsURL)) return false;
    if (dvA->unit != dvB->unit &&
        !OCTypeEqual(dvA->unit, dvB->unit)) return false;
    if (dvA->numericType != dvB->numericType) return false;
    if (dvA->type != dvB->type &&
        !OCTypeEqual(dvA->type, dvB->type)) return false;
    if (dvA->encoding != dvB->encoding &&
        !OCTypeEqual(dvA->encoding, dvB->encoding)) return false;
    if (dvA->name != dvB->name &&
        !OCTypeEqual(dvA->name, dvB->name)) return false;
    // 2) raw components & labels
    if (!DependentVariableComponentsAreEqual(dvA, dvB)) return false;
    if (dvA->componentLabels != dvB->componentLabels &&
        !OCTypeEqual(dvA->componentLabels, dvB->componentLabels)) return false;
    // 3) quantity & description
    if (dvA->quantityName != dvB->quantityName &&
        !OCTypeEqual(dvA->quantityName, dvB->quantityName)) return false;
    if (dvA->quantityType != dvB->quantityType &&
        !OCTypeEqual(dvA->quantityType, dvB->quantityType)) return false;
    if (dvA->description != dvB->description &&
        !OCTypeEqual(dvA->description, dvB->description)) return false;
    // 4) sparseSampling (new combined field)
    if (dvA->sparseSampling != dvB->sparseSampling &&
        !OCTypeEqual(dvA->sparseSampling, dvB->sparseSampling)) return false;
    // 5) metadata
    if (dvA->metaData != dvB->metaData &&
        !OCTypeEqual(dvA->metaData, dvB->metaData)) return false;
    return true;
}
static OCStringRef impl_DependentVariableCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_DependentVariable *dv = (struct impl_DependentVariable *)cf;
    // Build a little summary of sparseSampling, or "none" if absent
    OCStringRef sparseDesc = dv->sparseSampling
                                 ? OCTypeCopyFormattingDesc((OCTypeRef)dv->sparseSampling)
                                 : STR("none");
    OCStringRef desc = OCStringCreateWithFormat(
        STR("<DependentVariable name=\"%@\" components=%lu sparse_sampling=%@>"),
        dv->name,
        (unsigned long)OCArrayGetCount(dv->components),
        sparseDesc);
    if (dv->sparseSampling) OCRelease(sparseDesc);
    return desc;
}
cJSON *impl_DependentVariableCreateJSON(const void *obj) {
    if (!obj) return cJSON_CreateNull();
    // 1) Copy to an OCDictionary
    //    — we pass NULL for dimensions (no context),
    //      external = false (embed components inline),
    //      base64Encoding = false (emit JSON arrays)
    OCDictionaryRef dict = DependentVariableCopyAsDictionary(
        (DependentVariableRef)obj);
    if (!dict) return NULL;
    // 2) Use the generic OCType → cJSON converter
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    // 3) Clean up
    OCRelease(dict);
    return json;
}
static void *impl_DependentVariableDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    const struct impl_DependentVariable *src = (const struct impl_DependentVariable *)ptr;
    struct impl_DependentVariable *dst = calloc(1, sizeof(*dst));
    if (!dst) return NULL;
    // 1) Copy base
    memcpy(&dst->base, &src->base, sizeof(OCBase));
    // 2) SIQuantity Type attributes
    dst->unit = src->unit ? OCRetain(src->unit) : NULL;
    dst->numericType = src->numericType;
    // 3) Dependent Variable Type attributes
    dst->name = src->name ? OCStringCreateCopy(src->name) : NULL;
    dst->description = src->description ? OCStringCreateCopy(src->description) : NULL;
    dst->metaData = src->metaData ? OCDictionaryCreateMutableCopy(src->metaData) : NULL;
    dst->quantityName = src->quantityName ? OCStringCreateCopy(src->quantityName) : NULL;
    dst->quantityType = src->quantityType ? OCStringCreateCopy(src->quantityType) : NULL;
    // 4) Components metadata
    dst->type = src->type ? OCStringCreateCopy(src->type) : NULL;
    dst->encoding = src->encoding ? OCStringCreateCopy(src->encoding) : NULL;
    dst->componentsURL = src->componentsURL ? OCStringCreateCopy(src->componentsURL) : NULL;
    // 5) Deep‐copy the components array using OCTypeDeepCopy
    if (src->components) {
        OCIndex count = OCArrayGetCount(src->components);
        dst->components = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
        if (!dst->components) goto fail;
        for (OCIndex i = 0; i < count; ++i) {
            OCTypeRef elem = OCArrayGetValueAtIndex(src->components, i);
            OCTypeRef elemCopy = OCTypeDeepCopy(elem);
            if (!elemCopy) goto fail;
            OCArrayAppendValue(dst->components, elemCopy);
            OCRelease(elemCopy);
        }
    } else {
        dst->components = NULL;
    }
    // 6) Copy component labels
    if (src->componentLabels) {
        OCIndex count = OCArrayGetCount(src->componentLabels);
        dst->componentLabels = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
        if (!dst->componentLabels) goto fail;
        for (OCIndex i = 0; i < count; ++i) {
            OCStringRef lbl = (OCStringRef)OCArrayGetValueAtIndex(src->componentLabels, i);
            OCStringRef lblCopy = OCStringCreateCopy(lbl);
            if (!lblCopy) goto fail;
            OCArrayAppendValue(dst->componentLabels, lblCopy);
            OCRelease(lblCopy);
        }
    } else {
        dst->componentLabels = NULL;
    }
    // 7) Sparse‐sampling metadata
    dst->sparseSampling = src->sparseSampling
                              ? OCTypeDeepCopy(src->sparseSampling)
                              : NULL;
    // 8) Weak back‐pointer
    dst->owner = src->owner;
    return dst;
fail:
    // Clean up any allocated fields if a failure occurs
    OCRelease(dst->unit);
    OCRelease(dst->name);
    OCRelease(dst->description);
    OCRelease(dst->metaData);
    OCRelease(dst->quantityName);
    OCRelease(dst->quantityType);
    OCRelease(dst->type);
    OCRelease(dst->encoding);
    OCRelease(dst->componentsURL);
    OCRelease(dst->components);
    OCRelease(dst->componentLabels);
    OCRelease(dst->sparseSampling);
    free(dst);
    return NULL;
}
static struct impl_DependentVariable *DependentVariableAllocate(void) {
    return OCTypeAlloc(
        struct impl_DependentVariable,
        DependentVariableGetTypeID(),
        impl_DependentVariableFinalize,
        impl_DependentVariableEqual,
        impl_DependentVariableCopyFormattingDesc,
        impl_DependentVariableCreateJSON,
        impl_DependentVariableDeepCopy,
        impl_DependentVariableDeepCopy);
}
static void impl_InitDependentVariableFields(DependentVariableRef dv) {
    // SIQuantity defaults
    dv->unit = SIUnitDimensionlessAndUnderived();
    dv->numericType = kOCNumberFloat64Type;
    // Basic DV fields
    dv->name = STR("");
    dv->description = STR("");
    dv->quantityName = STR("");
    dv->quantityType = STR("");
    dv->metaData = OCDictionaryCreateMutable(0);
    // Storage mode defaults
    dv->type = STR(kDependentVariableComponentTypeValueInternal);
    dv->encoding = STR(kDependentVariableEncodingValueBase64);
    dv->componentsURL = NULL;
    // Components & labels
    dv->components = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    dv->componentLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    // Sparse-sampling: start out with NO sparseSampling attached
    dv->sparseSampling = NULL;
    // weak back-pointer
    dv->owner = NULL;
}
static bool validateDependentVariableParameters(
    OCStringRef type,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCArrayRef componentLabels,
    OCIndex componentsCount,
    SparseSamplingRef sparseSampling) {
    // 0) type must be either "internal" or "external"
    if (!type ||
        (!OCStringEqual(type, STR(kDependentVariableComponentTypeValueInternal)) &&
         !OCStringEqual(type, STR(kDependentVariableComponentTypeValueExternal)))) {
        return false;
    }
    // 1) components‐vs‐labels
    if (componentLabels) {
        OCIndex labelCount = OCArrayGetCount(componentLabels);
        if (labelCount != componentsCount) return false;
        for (OCIndex i = 0; i < labelCount; ++i) {
            if (OCGetTypeID(OCArrayGetValueAtIndex(componentLabels, i)) != OCStringGetTypeID()) {
                return false;
            }
        }
    }
    // 2) quantityName vs unit dimensionality
    if (quantityName) {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim =
            SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim) {
            OCRelease(err);
            return false;
        }
        bool match = SIDimensionalityHasSameReducedDimensionality(
            nameDim, SIUnitGetDimensionality(unit));
        OCRelease(err);
        if (!match) return false;
    }
    // 3) quantityType semantics (“scalar”, “vector_N”, etc.)
    const char *qt = OCStringGetCString(quantityType);
    size_t len = qt ? strlen(qt) : 0;
    if (len == 6 && strcmp(qt, "scalar") == 0) {
        if (componentsCount != 1) return false;
    } else if (len > 6 && strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) != 1 || n != componentsCount)
            return false;
    } else if (len > 7 && strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) != 1 || n != componentsCount)
            return false;
    } else if (len > 7 && strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) != 2 || r * c != componentsCount)
            return false;
    } else if (len > 17 && strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) != 1 ||
            n * (n + 1) / 2 != componentsCount)
            return false;
    } else {
        // unknown quantityType
        return false;
    }
    // 4) sparse‐sampling consistency
    if (sparseSampling) {
        OCStringRef err = NULL;
        bool valid = validateSparseSampling(sparseSampling, &err);
        if (!valid) {
            OCRelease(err);
            return false;
        }
        if (err) OCRelease(err);  // ✅ release even on success
    }
    return true;
}
#pragma endregion Type Registration
#pragma region Creators
#pragma mark — Core Creator
static DependentVariableRef impl_DependentVariableCreate(
    OCStringRef type,  // "internal" or "external"
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,    // numericType
    OCStringRef encoding,        // "none", "base64", or "raw"
    OCStringRef componentsURL,   // only for external
    OCArrayRef components,       // array of OCDataRef
    bool copyComponents,         // whether to deep-copy blobs
    OCIndex explicitSize,        // elements-per-component if components==NULL
    OCArrayRef componentLabels,  // array of OCStringRef
    SparseSamplingRef sparseSampling,
    bool copySparseSampling,   // NEW: whether to deep-copy SparseSampling
    OCDictionaryRef metaData,  // application‐specific annotations
    OCStringRef *outError) {
    bool isExternal = type && OCStringEqual(type, STR(kDependentVariableComponentTypeValueExternal));
    // 0) internal must have either buffers or positive explicitSize
    if (!isExternal && !components && explicitSize <= 0) {
        if (outError) *outError = STR(
                          "DependentVariableCreate: must supply either component buffers or an explicitSize > 0");
        return NULL;
    }
    // 1) determine component count
    OCIndex componentsCount;
    if (isExternal) {
        componentsCount = DependentVariableComponentsCountFromQuantityType(quantityType);
        if (componentsCount == kOCNotFound) {
            if (outError) *outError = STR("DependentVariableCreate: invalid quantityType for external");
            return NULL;
        }
    } else if (components) {
        componentsCount = OCArrayGetCount(components);
        // validate OCDataRef & uniform length
        for (OCIndex i = 0; i < componentsCount; ++i) {
            if (OCGetTypeID(OCArrayGetValueAtIndex(components, i)) != OCDataGetTypeID()) {
                if (outError) *outError = STR("DependentVariableCreate: component element is not OCDataRef");
                return NULL;
            }
        }
        uint64_t L0 = componentsCount ? OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, 0)) : 0;
        for (OCIndex i = 1; i < componentsCount; ++i) {
            if (OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, i)) != L0) {
                if (outError) *outError = STR("DependentVariableCreate: mismatched component‐buffer lengths");
                return NULL;
            }
        }
    } else {
        componentsCount = DependentVariableComponentsCountFromQuantityType(quantityType);
        if (componentsCount == kOCNotFound) {
            if (outError) {
                *outError = OCStringCreateWithFormat(
                    STR("DependentVariableCreate: cannot determine count for quantityType %@"),
                    quantityType);
            }
            return NULL;
        }
    }
    // 2) semantic validation
    if (!validateDependentVariableParameters(
            type, unit, quantityName, quantityType,
            componentLabels, componentsCount,
            sparseSampling)) {
        return NULL;
    }
    // 3) allocate & init
    struct impl_DependentVariable *dv = DependentVariableAllocate();
    if (!dv) return NULL;
    impl_InitDependentVariableFields(dv);
    // 4) copy in everything
    OCRelease(dv->type);
    dv->type = OCStringCreateCopy(type);
    OCRelease(dv->encoding);
    dv->encoding = encoding ? OCStringCreateCopy(encoding)
                            : STR(kDependentVariableEncodingValueBase64);
    OCRelease(dv->componentsURL);
    dv->componentsURL = componentsURL ? OCStringCreateCopy(componentsURL)
                                      : NULL;
    dv->numericType = elementType;
    dv->unit = unit ? unit : SIUnitDimensionlessAndUnderived();
    OCRelease(dv->quantityName);
    dv->quantityName = quantityName ? OCStringCreateCopy(quantityName)
                                    : OCStringCreateCopy(SIUnitGuessQuantityName(dv->unit));
    OCRelease(dv->quantityType);
    dv->quantityType = quantityType ? OCStringCreateCopy(quantityType)
                                    : STR("scalar");
    OCRelease(dv->name);
    dv->name = name ? OCStringCreateCopy(name) : STR("");
    OCRelease(dv->description);
    dv->description = description ? OCStringCreateCopy(description) : STR("");
    OCRelease(dv->metaData);
    dv->metaData = metaData
                       ? (OCMutableDictionaryRef)OCTypeDeepCopyMutable(metaData)
                       : OCDictionaryCreateMutable(0);
    // 5) build components array
    OCRelease(dv->components);
    dv->components = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (!dv->components) {
        OCRelease(dv);
        return NULL;
    }
    if (!isExternal) {
        if (components) {
            for (OCIndex i = 0; i < componentsCount; ++i) {
                OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(components, i);
                if (copyComponents) {
                    OCMutableDataRef cp = OCDataCreateMutableCopy(0, blob);
                    OCArrayAppendValue(dv->components, cp);
                    OCRelease(cp);
                } else {
                    OCArrayAppendValue(dv->components, blob);
                }
            }
        } else {
            size_t eltSize = OCNumberTypeSize(elementType);
            size_t byteLen = (size_t)explicitSize * eltSize;
            for (OCIndex i = 0; i < componentsCount; ++i) {
                OCMutableDataRef buf = OCDataCreateMutable(0);
                OCDataSetLength(buf, byteLen);
                OCArrayAppendValue(dv->components, buf);
                OCRelease(buf);
            }
        }
    }
    // 6) build componentLabels
    OCRelease(dv->componentLabels);
    dv->componentLabels = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (!dv->componentLabels) {
        OCRelease(dv);
        return NULL;
    }
    if (componentLabels) {
        for (OCIndex i = 0; i < componentsCount; ++i) {
            OCArrayAppendValue(dv->componentLabels,
                               OCArrayGetValueAtIndex(componentLabels, i));
        }
    } else {
        for (OCIndex i = 0; i < componentsCount; ++i) {
            OCStringRef autoLbl = OCStringCreateWithFormat(STR("component-%ld"), (long)i);
            OCArrayAppendValue(dv->componentLabels, autoLbl);
            OCRelease(autoLbl);
        }
    }
    // 7) SparseSampling: deep‐copy only if requested, otherwise retain reference
    OCRelease(dv->sparseSampling);
    if (sparseSampling) {
        if (copySparseSampling) {
            dv->sparseSampling = (SparseSamplingRef)OCTypeDeepCopyMutable(sparseSampling);
        } else {
            dv->sparseSampling = (SparseSamplingRef)OCRetain(sparseSampling);
        }
    } else {
        dv->sparseSampling = NULL;
    }
    return (DependentVariableRef)dv;
}
#pragma mark — Public Factories
DependentVariableRef DependentVariableCreate(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            */ STR(kDependentVariableComponentTypeValueInternal),
        /* name            */ name,
        /* description     */ description,
        /* unit            */ unit,
        /* quantityName    */ quantityName,
        /* quantityType    */ quantityType,
        /* elementType     */ elementType,
        /* encoding        */ NULL,  // default to base64
        /* componentsURL   */ NULL,  // not used for internal
        /* components      */ components,
        /* copyComponents  */ true,         // deep‐copy blobs
        /* explicitSize    */ (OCIndex)-1,  // ignored when components != NULL
        /* componentLabels */ componentLabels,
        /* sparseSampling  */ NULL,     // no sparse‐sampling by default
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData        */ NULL,     // no extra metadata by default
        /* outError        */ outError);
}
DependentVariableRef DependentVariableCreateWithComponentsNoCopy(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            */ STR(kDependentVariableComponentTypeValueInternal),
        /* name            */ name,
        /* description     */ description,
        /* unit            */ unit,
        /* quantityName    */ quantityName,
        /* quantityType    */ quantityType,
        /* elementType     */ elementType,
        /* encoding        */ NULL,  // default to base64
        /* componentsURL   */ NULL,  // not used for internal
        /* components      */ components,
        /* copyComponents  */ false,        // no copy
        /* explicitSize    */ (OCIndex)-1,  // ignored when components != NULL
        /* componentLabels */ componentLabels,
        /* sparseSampling  */ NULL,     // none by default
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData        */ NULL,     // none by default
        /* outError        */ outError);
}
DependentVariableRef DependentVariableCreateWithSize(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCIndex size,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            */ STR(kDependentVariableComponentTypeValueInternal),
        /* name            */ name,
        /* description     */ description,
        /* unit            */ unit,
        /* quantityName    */ quantityName,
        /* quantityType    */ quantityType,
        /* elementType     */ elementType,
        /* encoding        */ NULL,   // default to "base64"
        /* componentsURL   */ NULL,   // not used for internal
        /* components      */ NULL,   // no blobs, use explicitSize
        /* copyComponents  */ false,  // irrelevant when components==NULL
        /* explicitSize    */ size,   // allocate this many elements
        /* componentLabels */ componentLabels,
        /* sparseSampling  */ NULL,     // none by default
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData        */ NULL,     // none by default
        /* outError        */ outError);
}
DependentVariableRef DependentVariableCreateDefault(
    OCStringRef quantityType,
    OCNumberType elementType,
    OCIndex size,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type               */ STR(kDependentVariableComponentTypeValueInternal),
        /* name               */ NULL,
        /* description        */ NULL,
        /* unit               */ NULL,
        /* quantityName       */ NULL,
        /* quantityType       */ quantityType,
        /* elementType        */ elementType,
        /* encoding           */ NULL,  // default to "base64"
        /* componentsURL      */ NULL,  // not used for internal
        /* components         */ NULL,  // no blobs, use explicitSize
        /* copyComponents     */ false,
        /* explicitSize       */ size,
        /* componentLabels    */ NULL,  // default labels
        /* sparseSampling     */ NULL,  // none
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData           */ NULL,  // none
        /* outError           */ outError);
}
DependentVariableRef DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCStringRef *outError) {
    OCMutableArrayRef arr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(arr, component);
    DependentVariableRef dv = impl_DependentVariableCreate(
        /* type               */ STR(kDependentVariableComponentTypeValueInternal),
        /* name               */ name,
        /* description        */ description,
        /* unit               */ unit,
        /* quantityName       */ quantityName,
        /* quantityType       */ STR("scalar"),
        /* elementType        */ elementType,
        /* encoding           */ NULL,  // default to "base64"
        /* componentsURL      */ NULL,  // not used for internal
        /* components         */ arr,
        /* copyComponents     */ true,
        /* explicitSize       */ (OCIndex)-1,
        /* componentLabels    */ componentLabels,
        /* sparseSampling     */ NULL,  // none
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData           */ NULL,  // none
        /* outError           */ outError);
    OCRelease(arr);
    return dv;
}
DependentVariableRef DependentVariableCreateExternal(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCStringRef componentsURL,
    OCStringRef *outError) {
    if (componentsURL == NULL) {
        if (outError) {
            *outError = STR(
                "DependentVariableCreateExternal: "
                "must supply a non-NULL componentsURL for external variables");
        }
        return NULL;
    }
    return impl_DependentVariableCreate(
        /* type               */ STR(kDependentVariableComponentTypeValueExternal),
        /* name               */ name,
        /* description        */ description,
        /* unit               */ unit,
        /* quantityName       */ quantityName,
        /* quantityType       */ quantityType,
        /* elementType        */ elementType,
        /* encoding           */ NULL,  // default to "base64"
        /* componentsURL      */ componentsURL,
        /* components         */ NULL,  // no inline blobs for external
        /* copyComponents     */ false,
        /* explicitSize       */ 0,     // ignored for external
        /* componentLabels    */ NULL,  // not used for external
        /* sparseSampling     */ NULL,  // no sparse-sampling by default
        /* copySparseSampling */ true,  // deep-copy if present
        /* metaData           */ NULL,  // no extra metadata
        /* outError           */ outError);
}
DependentVariableRef DependentVariableCreateCopy(DependentVariableRef src) {
    return impl_DependentVariableDeepCopy(src);
}
DependentVariableRef DependentVariableCreateComplexCopy(DependentVariableRef src,
                                                        OCTypeRef owner) {
    if (!src) return NULL;
    // 1) Make a deep copy of the source
    DependentVariableRef dv = DependentVariableCreateCopy(src);
    if (!dv) return NULL;
    // 2) Assign the owner (weak back-pointer)
    DependentVariableSetOwner(dv, owner);
    // 3) If it isn’t already a complex type, upgrade its element type
    if (!SIQuantityIsComplexType((SIQuantityRef)dv)) {
        OCNumberType base = DependentVariableGetElementType(dv);
        OCNumberType complexType =
            (base == kOCNumberFloat32Type
                 ? kOCNumberComplex64Type
                 : kOCNumberComplex128Type);
        DependentVariableSetElementType(dv, complexType);
    }
    return dv;
}
OCDictionaryRef DependentVariableCopyAsDictionary(DependentVariableRef dv) {
    if (!dv) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) return NULL;
    // 1) type (external vs. internal)
    {
        OCStringRef t = dv->type ?: STR(kDependentVariableComponentTypeValueInternal);
        OCStringRef tCopy = OCStringCreateCopy(t);
        OCDictionarySetValue(dict, STR(kDependentVariableTypeKey), tCopy);
        OCRelease(tCopy);
    }
    // 1a) if external, record the URL hint—but we STILL embed the raw blobs below
    if (OCStringEqual(dv->type, STR(kDependentVariableComponentTypeValueExternal)) && dv->componentsURL) {
        OCStringRef urlCopy = OCStringCreateCopy(dv->componentsURL);
        OCDictionarySetValue(dict, STR(kDependentVariableComponentsURLKey), urlCopy);
        OCRelease(urlCopy);
    }
    // 2) encoding (base64, none, or raw)
    {
        OCStringRef enc = dv->encoding ?: STR(kDependentVariableEncodingValueBase64);
        OCStringRef encCopy = OCStringCreateCopy(enc);
        OCDictionarySetValue(dict, STR(kDependentVariableEncodingKey), encCopy);
        OCRelease(encCopy);
    }
    // 3) components (always embed raw data for round-trip)
    {
        OCNumberType et = DependentVariableGetElementType(dv);
        bool isBase64 = dv->encoding && OCStringEqual(dv->encoding, STR(kDependentVariableEncodingValueBase64));
        bool isRaw = dv->encoding && OCStringEqual(dv->encoding, STR(kDependentVariableEncodingValueRaw));
        bool isComplex = (et == kOCNumberComplex64Type || et == kOCNumberComplex128Type);
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        OCMutableArrayRef compsArr = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < ncomps; ++i) {
            OCDataRef blob = DependentVariableGetComponentAtIndex(dv, i);
            if (isRaw) {
                // If raw, we just store the OCDataRef directly
                OCArrayAppendValue(compsArr, blob);
                continue;
            } else if (isBase64) {
                OCStringRef b64 = OCDataCreateBase64EncodedString(blob, OCBase64EncodingOptionsNone);
                OCArrayAppendValue(compsArr, b64);
                OCRelease(b64);
            } else {
                const void *bytes = OCDataGetBytesPtr(blob);
                size_t stride = OCNumberTypeSize(et);
                OCIndex count = (OCIndex)(OCDataGetLength(blob) / stride);
                OCMutableArrayRef numArr = OCArrayCreateMutable(isComplex ? count * 2 : count,
                                                                &kOCTypeArrayCallBacks);
                if (!isComplex) {
                    switch (et) {
                        case kOCNumberSInt8Type: {
                            int8_t *arr = (int8_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithSInt8(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberSInt16Type: {
                            int16_t *arr = (int16_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithSInt16(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberSInt32Type: {
                            int32_t *arr = (int32_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithSInt32(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberSInt64Type: {
                            int64_t *arr = (int64_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithSInt64(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberUInt8Type: {
                            uint8_t *arr = (uint8_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithUInt8(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberUInt16Type: {
                            uint16_t *arr = (uint16_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithUInt16(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberUInt32Type: {
                            uint32_t *arr = (uint32_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithUInt32(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberUInt64Type: {
                            uint64_t *arr = (uint64_t *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithUInt64(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberFloat32Type: {
                            float *arr = (float *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithFloat(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        case kOCNumberFloat64Type: {
                            double *arr = (double *)bytes;
                            for (OCIndex j = 0; j < count; ++j) {
                                OCNumberRef n = OCNumberCreateWithDouble(arr[j]);
                                OCArrayAppendValue(numArr, n);
                                OCRelease(n);
                            }
                            break;
                        }
                        default:
                            break;
                    }
                } else {
                    if (et == kOCNumberComplex64Type) {
                        float complex *arr = (float complex *)bytes;
                        for (OCIndex j = 0; j < count; ++j) {
                            OCNumberRef re = OCNumberCreateWithFloat((float)crealf(arr[j]));
                            OCNumberRef im = OCNumberCreateWithFloat((float)cimagf(arr[j]));
                            OCArrayAppendValue(numArr, re);
                            OCArrayAppendValue(numArr, im);
                            OCRelease(re);
                            OCRelease(im);
                        }
                    } else {
                        double complex *arr = (double complex *)bytes;
                        for (OCIndex j = 0; j < count; ++j) {
                            OCNumberRef re = OCNumberCreateWithDouble(creal(arr[j]));
                            OCNumberRef im = OCNumberCreateWithDouble(cimag(arr[j]));
                            OCArrayAppendValue(numArr, re);
                            OCArrayAppendValue(numArr, im);
                            OCRelease(re);
                            OCRelease(im);
                        }
                    }
                }
                OCArrayAppendValue(compsArr, numArr);
                OCRelease(numArr);
            }
        }
        OCDictionarySetValue(dict, STR(kDependentVariableComponentsKey), compsArr);
        OCRelease(compsArr);
    }
    // 4) name, description
    if (dv->name) {
        OCStringRef c = OCStringCreateCopy(dv->name);
        OCDictionarySetValue(dict, STR(kDependentVariableNameKey), c);
        OCRelease(c);
    }
    if (dv->description) {
        OCStringRef c = OCStringCreateCopy(dv->description);
        OCDictionarySetValue(dict, STR(kDependentVariableDescriptionKey), c);
        OCRelease(c);
    }
    // 5) quantity_name, quantity_type, unit, numeric_type
    if (dv->quantityName) {
        OCStringRef c = OCStringCreateCopy(dv->quantityName);
        OCDictionarySetValue(dict, STR(kDependentVariableQuantityNameKey), c);
        OCRelease(c);
    }
    if (dv->quantityType) {
        OCStringRef c = OCStringCreateCopy(dv->quantityType);
        OCDictionarySetValue(dict, STR(kDependentVariableQuantityTypeKey), c);
        OCRelease(c);
    }
    if (dv->unit) {
        OCStringRef s = SIUnitCopySymbol(dv->unit);
        OCDictionarySetValue(dict, STR(kDependentVariableUnitKey), s);
        OCRelease(s);
    }
    {
        const char *typeName = OCNumberGetTypeName(DependentVariableGetElementType(dv));
        if (typeName) {
            OCStringRef typeStr = OCStringCreateWithCString(typeName);
            OCDictionarySetValue(dict, STR(kDependentVariableNumericTypeKey), typeStr);
            OCRelease(typeStr);
        }
    }
    // 6) component_labels
    {
        OCIndex nlab = DependentVariableGetComponentCount(dv);
        OCMutableArrayRef lbls = OCArrayCreateMutable(nlab, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < nlab; ++i) {
            OCStringRef lbl = DependentVariableGetComponentLabelAtIndex(dv, i);
            if (lbl) OCArrayAppendValue(lbls, lbl);
        }
        OCDictionarySetValue(dict, STR(kDependentVariableComponentLabelsKey), lbls);
        OCRelease(lbls);
    }
    // 7) sparse_sampling (new single OCType)
    if (dv->sparseSampling) {
        OCDictionaryRef spDict = SparseSamplingCopyAsDictionary(dv->sparseSampling);
        if (spDict) {
            OCDictionarySetValue(dict, STR(kDependentVariableSparseSamplingKey), spDict);
            OCRelease(spDict);
        }
    }
    // 8) metadata
    if (dv->metaData) {
        OCMutableDictionaryRef mdCopy = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(dv->metaData);
        OCDictionarySetValue(dict, STR(kDependentVariableMetaDataKey), mdCopy);
        OCRelease(mdCopy);
    }
    return (OCDictionaryRef)dict;
}
DependentVariableRef DependentVariableCreateFromDictionary(OCDictionaryRef dict,
                                                           OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("DependentVariableCreateFromDictionary: input dictionary is NULL");
        return NULL;
    }
    // 1) type
    OCStringRef type = OCDictionaryGetValue(dict, STR(kDependentVariableTypeKey));
    bool isExternal = type && OCStringEqual(type, STR(kDependentVariableComponentTypeValueExternal));
    // 2) required quantityType & numericType
    OCStringRef quantityType = OCDictionaryGetValue(dict, STR(kDependentVariableQuantityTypeKey));
    OCStringRef numType = OCDictionaryGetValue(dict, STR(kDependentVariableNumericTypeKey));
    if (!type || !quantityType || !numType) {
        if (outError) *outError = STR("DependentVariableCreateFromDictionary: missing required fields");
        return NULL;
    }
    // 3) components array for internal
    OCArrayRef compArr = OCDictionaryGetValue(dict, STR(kDependentVariableComponentsKey));
    if (!isExternal && !compArr) {
        if (outError) *outError = STR(
                          "DependentVariableCreateFromDictionary: missing \"components\" for internal variable");
        return NULL;
    }
    // 4) componentsURL for external
    OCStringRef componentsURL = NULL;
    if (isExternal) {
        componentsURL = OCDictionaryGetValue(dict, STR(kDependentVariableComponentsURLKey));
        if (!componentsURL) {
            if (outError) *outError = STR(
                              "DependentVariableCreateFromDictionary: missing \"components_url\" for external variable");
            return NULL;
        }
    }
    // 5) parse numeric_type → elementType
    OCNumberType elementType = kOCNumberTypeInvalid;
    if (OCGetTypeID(numType) == OCStringGetTypeID()) {
        const char *typeName = OCStringGetCString(numType);
        elementType = OCNumberTypeFromName(typeName);
        if (elementType == kOCNumberTypeInvalid) {
            if (outError) *outError = STR("DependentVariableCreateFromDictionary: invalid numeric_type string");
            return NULL;
        }
    } else {
        if (outError) *outError = STR("DependentVariableCreateFromDictionary: numeric_type must be a string or number");
        return NULL;
    }
    // 6) optional fields
    OCStringRef name = OCDictionaryGetValue(dict, STR(kDependentVariableNameKey));
    OCStringRef desc = OCDictionaryGetValue(dict, STR(kDependentVariableDescriptionKey));
    OCStringRef quantityName = OCDictionaryGetValue(dict, STR(kDependentVariableQuantityNameKey));
    OCStringRef unitExpr = OCDictionaryGetValue(dict, STR(kDependentVariableUnitKey));
    OCArrayRef labelArr = OCDictionaryGetValue(dict, STR(kDependentVariableComponentLabelsKey));
    OCStringRef encoding = OCDictionaryGetValue(dict, STR(kDependentVariableEncodingKey));
    bool isBase64 = encoding && OCStringEqual(encoding, STR(kDependentVariableEncodingValueBase64));
    bool isRaw = encoding && OCStringEqual(encoding, STR(kDependentVariableEncodingValueRaw));
    // 7) SIUnitFromExpression
    SIUnitRef unit = NULL;
    if (unitExpr) {
        double mult = 1.0;
        OCStringRef uerr = NULL;
        unit = SIUnitFromExpression(unitExpr, &mult, &uerr);
        if (uerr) OCRelease(uerr);
    }
    // 8) deserialize components (for internal only)
    OCIndex count = compArr ? OCArrayGetCount(compArr) : 0;
    OCMutableArrayRef components = OCArrayCreateMutable(isExternal ? 0 : count,
                                                        &kOCTypeArrayCallBacks);
    if (!components) {
        if (outError) *outError = STR(
                          "DependentVariableCreateFromDictionary: cannot allocate components array");
        return NULL;
    }
    if (!isExternal) {
        for (OCIndex i = 0; i < count; ++i) {
            if (isRaw) {
                // If raw, we just store the OCDataRef directly
                OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(compArr, i);
                if (blob && OCDataGetLength(blob) > 0) {
                    OCDataRef data = OCDataCreateMutableCopy(0, blob);
                    OCArrayAppendValue(components, data);
                    OCRelease(data);
                }
            } else if (isBase64) {
                OCStringRef b64 = OCArrayGetValueAtIndex(compArr, i);
                OCDataRef data = OCDataCreateFromBase64EncodedString(b64);
                if (data && OCDataGetLength(data) > 0)
                    OCArrayAppendValue(components, data);
                if (data) OCRelease(data);
            } else {
                OCArrayRef numList = OCArrayGetValueAtIndex(compArr, i);
                OCIndex n = OCArrayGetCount(numList);
                OCMutableDataRef data = OCDataCreateMutable(0);
                for (OCIndex j = 0; j < n; ++j) {
                    OCNumberRef num = OCArrayGetValueAtIndex(numList, j);
                    double v = 0;
                    OCNumberTryGetDouble(num, &v);
                    switch (elementType) {
                        case kOCNumberSInt8Type: {
                            int8_t i8 = (int8_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&i8, sizeof(i8));
                            break;
                        }
                        case kOCNumberSInt16Type: {
                            int16_t i16 = (int16_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&i16, sizeof(i16));
                            break;
                        }
                        case kOCNumberSInt32Type: {
                            int32_t i32 = (int32_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&i32, sizeof(i32));
                            break;
                        }
                        case kOCNumberSInt64Type: {
                            int64_t i64 = (int64_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&i64, sizeof(i64));
                            break;
                        }
                        case kOCNumberUInt8Type: {
                            uint8_t u8 = (uint8_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&u8, sizeof(u8));
                            break;
                        }
                        case kOCNumberUInt16Type: {
                            uint16_t u16 = (uint16_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&u16, sizeof(u16));
                            break;
                        }
                        case kOCNumberUInt32Type: {
                            uint32_t u32 = (uint32_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&u32, sizeof(u32));
                            break;
                        }
                        case kOCNumberUInt64Type: {
                            uint64_t u64 = (uint64_t)v;
                            OCDataAppendBytes(data, (uint8_t const *)&u64, sizeof(u64));
                            break;
                        }
                        case kOCNumberFloat32Type: {
                            float f = (float)v;
                            OCDataAppendBytes(data, (uint8_t const *)&f, sizeof(f));
                            break;
                        }
                        case kOCNumberFloat64Type: {
                            double d = v;
                            OCDataAppendBytes(data, (uint8_t const *)&d, sizeof(d));
                            break;
                        }
                        case kOCNumberComplex64Type: {
                            float re = (float)v, im = 0;
                            if (j + 1 < n) {
                                OCNumberRef next = OCArrayGetValueAtIndex(numList, ++j);
                                OCNumberTryGetDouble(next, &v);
                                im = (float)v;
                            }
                            float complex z = re + im * I;
                            OCDataAppendBytes(data, (uint8_t const *)&z, sizeof(z));
                            break;
                        }
                        case kOCNumberComplex128Type: {
                            double re = v, im = 0;
                            if (j + 1 < n) {
                                OCNumberRef next = OCArrayGetValueAtIndex(numList, ++j);
                                OCNumberTryGetDouble(next, &v);
                                im = v;
                            }
                            double complex z = re + im * I;
                            OCDataAppendBytes(data, (uint8_t const *)&z, sizeof(z));
                            break;
                        }
                        default:
                            break;
                    }
                }
                if (OCDataGetLength(data) > 0)
                    OCArrayAppendValue(components, data);
                OCRelease(data);
            }
        }
        if ((OCIndex)OCArrayGetCount(components) != count) {
            OCRelease(components);
            if (outError) *outError = STR(
                              "DependentVariableCreateFromDictionary: component deserialization failed");
            return NULL;
        }
    }
    // 8b) pull sparseSampling & metaData out of dict
    SparseSamplingRef sparseSampling = NULL;
    {
        OCDictionaryRef spDict =
            OCDictionaryGetValue(dict, STR(kDependentVariableSparseSamplingKey));
        if (spDict) {
            OCStringRef spErr = NULL;
            sparseSampling = SparseSamplingCreateFromDictionary(spDict, &spErr);
            if (!sparseSampling) {
                if (outError) *outError = spErr
                                              ? OCStringCreateCopy(spErr)
                                              : STR("DependentVariableCreateFromDictionary: invalid sparse_sampling");
                if (spErr) OCRelease(spErr);
                OCRelease(components);
                return NULL;
            }
        }
    }
    OCDictionaryRef metaData =
        (OCDictionaryRef)OCDictionaryGetValue(dict, STR(kDependentVariableMetaDataKey));
    // 9) call core creator with encoding & metaData
    DependentVariableRef dv = impl_DependentVariableCreate(
        /* type            */ type,
        /* name            */ name,
        /* description     */ desc,
        /* unit            */ unit,
        /* quantityName    */ quantityName,
        /* quantityType    */ quantityType,
        /* elementType     */ elementType,
        /* encoding        */ encoding,
        /* componentsURL   */ componentsURL,
        /* components      */ components,
        /* copyComponents  */ false,
        /* explicitSize    */ isExternal ? 0 : -1,
        /* componentLabels */ labelArr,
        /* sparseSampling  */ sparseSampling,
        /* copySparseSampling */ false,  // OPTIMIZATION: Don't deep copy during initial creation!
        /* metaData        */ metaData,
        /* outError        */ outError);
    OCRelease(components);
    if (sparseSampling) OCRelease(sparseSampling);
    return dv;
}
cJSON *DependentVariableCreateJSON(DependentVariableRef dv) {
    if (!dv) {
        // if you prefer, return NULL here instead of cJSON null
        return cJSON_CreateNull();
    }
    // 1) Turn the DV into an OCDictionary (no I/O, just in-memory)
    OCDictionaryRef dict = DependentVariableCopyAsDictionary(dv);
    if (!dict) {
        // serialization-to-dictionary failed
        return cJSON_CreateNull();
    }
    // 2) Convert that dictionary into a cJSON tree
    cJSON *json = OCTypeCopyJSON((OCTypeRef)dict);
    // 3) Clean up
    OCRelease(dict);
    // 4) Return the cJSON object (caller is responsible for cJSON_Delete)
    return json;
}
OCDictionaryRef DependentVariableDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError)
            *outError = STR("Expected top-level JSON object for DependentVariable");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) {
        if (outError) *outError = STR("Failed to allocate dictionary");
        return NULL;
    }
    // 1) Required: "type"
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableTypeKey);
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"type\"");
        OCRelease(dict);
        return NULL;
    }
    bool isExternal = (strcmp(item->valuestring, kDependentVariableComponentTypeValueExternal) == 0);
    {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableTypeKey), tmp);
        OCRelease(tmp);
    }
    // 2) Optional: "components_url"
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableComponentsURLKey);
    if (isExternal && !cJSON_IsString(item)) {
        if (outError) *outError = STR("External DependentVariable requires \"components_url\"");
        OCRelease(dict);
        return NULL;
    }
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableComponentsURLKey), tmp);
        OCRelease(tmp);
    }
    // 3) Optional: "encoding"
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableEncodingKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableEncodingKey), tmp);
        OCRelease(tmp);
    }
    // 4) "components" — required for internal only
    if (!isExternal) {
        item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableComponentsKey);
        if (!cJSON_IsArray(item)) {
            if (outError) *outError = STR("Missing or invalid \"components\" for internal DependentVariable");
            OCRelease(dict);
            return NULL;
        }
        OCMutableArrayRef components = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *comp = NULL;
        cJSON_ArrayForEach(comp, item) {
            if (cJSON_IsString(comp)) {
                OCStringRef s = OCStringCreateWithCString(comp->valuestring);
                OCArrayAppendValue(components, s);
                OCRelease(s);
            } else if (cJSON_IsArray(comp)) {
                OCMutableArrayRef numArr = OCArrayCreateMutable(cJSON_GetArraySize(comp), &kOCTypeArrayCallBacks);
                cJSON *val = NULL;
                cJSON_ArrayForEach(val, comp) {
                    if (cJSON_IsNumber(val)) {
                        OCNumberRef n = OCNumberCreateWithDouble(val->valuedouble);
                        OCArrayAppendValue(numArr, n);
                        OCRelease(n);
                    }
                }
                OCArrayAppendValue(components, numArr);
                OCRelease(numArr);
            }
        }
        OCDictionarySetValue(dict, STR(kDependentVariableComponentsKey), components);
        OCRelease(components);
    }
    // 5) Optional: name, description
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableNameKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableNameKey), tmp);
        OCRelease(tmp);
    }
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableDescriptionKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableDescriptionKey), tmp);
        OCRelease(tmp);
    }
    // 6) Optional: quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableQuantityNameKey);
    if (cJSON_IsString(item) && item->valuestring[0] != '\0') {
        OCStringRef qname = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableQuantityNameKey), qname);
        OCRelease(qname);
    }
    // 7) Required: quantity_type
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableQuantityTypeKey);
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"quantity_type\"");
        OCRelease(dict);
        return NULL;
    }
    {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableQuantityTypeKey), tmp);
        OCRelease(tmp);
    }
    // 8) Optional: unit
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableUnitKey);
    if (cJSON_IsString(item)) {
        OCStringRef tmp = OCStringCreateWithCString(item->valuestring);
        OCDictionarySetValue(dict, STR(kDependentVariableUnitKey), tmp);
        OCRelease(tmp);
    } else {
        OCStringRef qname = (OCStringRef)OCDictionaryGetValue(dict, STR(kDependentVariableQuantityNameKey));
        if (qname && OCStringEqual(qname, kSIQuantityDimensionless)) {
            SIUnitRef u = SIUnitDimensionlessAndUnderived();
            OCStringRef sym = SIUnitCreateSymbol(u);
            OCDictionarySetValue(dict, STR(kDependentVariableUnitKey), sym);
            OCRelease(sym);
        }
    }
    // 9) Required: numeric_type
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableNumericTypeKey);
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"numeric_type\"");
        OCRelease(dict);
        return NULL;
    }
    {
        const char *ts = item->valuestring;
        OCNumberType code = OCNumberTypeFromName(ts);
        if (code == -1) {
            if (outError)
                *outError = STR("Unrecognized \"numeric_type\"");
            OCRelease(dict);
            return NULL;
        }
        OCStringRef typeStr = OCStringCreateWithCString(ts);
        OCDictionarySetValue(dict, STR(kDependentVariableNumericTypeKey), typeStr);
        OCRelease(typeStr);
    }
    // 10) Optional: component_labels
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableComponentLabelsKey);
    if (cJSON_IsArray(item)) {
        OCMutableArrayRef labels = OCArrayCreateMutable(cJSON_GetArraySize(item), &kOCTypeArrayCallBacks);
        cJSON *label = NULL;
        cJSON_ArrayForEach(label, item) {
            if (cJSON_IsString(label)) {
                OCStringRef lbl = OCStringCreateWithCString(label->valuestring);
                OCArrayAppendValue(labels, lbl);
                OCRelease(lbl);
            }
        }
        OCDictionarySetValue(dict, STR(kDependentVariableComponentLabelsKey), labels);
        OCRelease(labels);
    } else {
        // default: N empty labels based on quantity type
        OCStringRef qt = (OCStringRef)OCDictionaryGetValue(dict, STR(kDependentVariableQuantityTypeKey));
        OCIndex n = DependentVariableComponentsCountFromQuantityType(qt);
        OCMutableArrayRef labels = OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < n; ++i) {
            OCArrayAppendValue(labels, STR(""));
        }
        OCDictionarySetValue(dict, STR(kDependentVariableComponentLabelsKey), labels);
        OCRelease(labels);
    }
    // 11) Optional: sparse_sampling
    item = cJSON_GetObjectItemCaseSensitive(json, kDependentVariableSparseSamplingKey);
    if (cJSON_IsObject(item)) {
        SparseSamplingRef ss = SparseSamplingCreateFromJSON(item, outError);
        if (!ss) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionaryRef spDict = SparseSamplingCopyAsDictionary(ss);
        OCRelease(ss);
        if (!spDict) {
            OCRelease(dict);
            return NULL;
        }
        OCDictionarySetValue(dict, STR(kDependentVariableSparseSamplingKey), spDict);
        OCRelease(spDict);
    }
    return dict;
}
DependentVariableRef DependentVariableCreateFromJSON(cJSON *json, OCStringRef *outError) {
    OCDictionaryRef dict = DependentVariableDictionaryCreateFromJSON(json, outError);
    if (!dict) return NULL;
    DependentVariableRef dv = DependentVariableCreateFromDictionary(dict, outError);
    OCRelease(dict);
    return dv;
}
DependentVariableRef DependentVariableCreateCrossSection(DependentVariableRef dv, OCArrayRef dimensions, OCIndexPairSetRef indexPairs, OCStringRef *outError) {
    // 0) bail if caller already has an error
    if (outError && *outError) return NULL;
    if (!dv) return NULL;
    OCIndex allDimsCount = OCArrayGetCount(dimensions);
    OCIndex fixedCount = OCIndexPairSetGetCount(indexPairs);
    OCIndex freeCount = allDimsCount - fixedCount;
    if (freeCount < 0 || freeCount > allDimsCount) return NULL;
    // 1) no dims fixed? just copy
    if (freeCount == allDimsCount) {
        return DependentVariableCreateCopy(dv);
    }
    // 2) build an OCIndexSet of the *fixed* dimension‐indices
    OCIndexSetRef fixedDims =
        OCIndexPairSetCreateIndexSetOfIndexes(indexPairs);
    // 3) compute cross‐section length = product of all *free* dims
    OCIndex crossSize =
        RMNCalculateSizeFromDimensionsIgnoring(dimensions, fixedDims);
    // 4) allocate output DV of the right size
    DependentVariableRef outDV =
        DependentVariableCreateWithSize(
            DependentVariableGetName(dv),
            DependentVariableGetDescription(dv),
            dv->unit,
            DependentVariableGetQuantityName(dv),
            DependentVariableGetQuantityType(dv),
            DependentVariableGetElementType(dv),
            DependentVariableGetComponentLabels(dv),
            crossSize,
            /* owner */ NULL);
    if (!outDV) {
        OCRelease(fixedDims);
        return NULL;
    }
    // 5) prepare coord & npts arrays
    OCIndex *coords = calloc((size_t)allDimsCount, sizeof(OCIndex));
    OCIndex *npts = calloc((size_t)allDimsCount, sizeof(OCIndex));
    for (OCIndex d = 0; d < allDimsCount; d++) {
        DimensionRef dim = (DimensionRef)OCArrayGetValueAtIndex(dimensions, d);
        // fixed dims get their specified coordinate; others start at 0
        coords[d] = OCIndexPairSetContainsIndex(indexPairs, d)
                        ? OCIndexPairSetValueForIndex(indexPairs, d)
                        : 0;
        // record the total length along each dimension
        npts[d] = DimensionGetCount(dim);
    }
    // 6) for each component buffer, walk the cross‐section
    OCNumberType elemType = DependentVariableGetElementType(dv);
    OCIndex nComps = DependentVariableGetComponentCount(dv);
    for (OCIndex ci = 0; ci < nComps; ci++) {
        OCDataRef srcBlob = DependentVariableGetComponentAtIndex(dv, ci);
        OCMutableDataRef dstBlob = (OCMutableDataRef)
            DependentVariableGetComponentAtIndex(outDV, ci);
        const void *srcPtr = OCDataGetBytesPtr(srcBlob);
        void *dstPtr = OCDataGetMutableBytes(dstBlob);
        if (freeCount == 0) {
            // single‐point slice
            OCIndex memOff = RMNGridMemOffsetFromIndexes(dimensions, coords);
            switch (elemType) {
                case kOCNumberFloat32Type:
                    ((float *)dstPtr)[0] = ((float *)srcPtr)[memOff];
                    break;
                case kOCNumberFloat64Type:
                    ((double *)dstPtr)[0] = ((double *)srcPtr)[memOff];
                    break;
                case kOCNumberComplex64Type:
                    ((float complex *)dstPtr)[0] =
                        ((float complex *)srcPtr)[memOff];
                    break;
                case kOCNumberComplex128Type:
                    ((double complex *)dstPtr)[0] =
                        ((double complex *)srcPtr)[memOff];
                    break;
                default:
                    break;
            }
        } else {
            // full slice: for each output offset, decode the *free* dims
            for (OCIndex outOff = 0; outOff < crossSize; outOff++) {
                setIndexesForReducedMemOffsetIgnoringDimensions(outOff, coords, allDimsCount, npts, fixedDims);
                OCIndex memOff =
                    RMNGridMemOffsetFromIndexes(dimensions, coords);
                switch (elemType) {
                    case kOCNumberFloat32Type:
                        ((float *)dstPtr)[outOff] = ((float *)srcPtr)[memOff];
                        break;
                    case kOCNumberFloat64Type:
                        ((double *)dstPtr)[outOff] = ((double *)srcPtr)[memOff];
                        break;
                    case kOCNumberComplex64Type:
                        ((float complex *)dstPtr)[outOff] =
                            ((float complex *)srcPtr)[memOff];
                        break;
                    case kOCNumberComplex128Type:
                        ((double complex *)dstPtr)[outOff] =
                            ((double complex *)srcPtr)[memOff];
                        break;
                    default:
                        break;
                }
            }
        }
    }
    free(coords);
    free(npts);
    OCRelease(fixedDims);
    return outDV;
}
bool DependentVariableAppend(DependentVariableRef dv, DependentVariableRef appendedDV, OCStringRef *outError) {
    // 0) if caller already has an error, bail
    if (outError && *outError) return false;
    // 1) sanity
    if (!dv || !appendedDV) return false;
    // 2) must have matching physical dimensionality
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)dv,
                                                (SIQuantityRef)appendedDV)) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Append Error: Incompatible dimensionalities."));
        }
        return false;
    }
    // 3) must have the same element type
    OCNumberType et1 = DependentVariableGetElementType(dv);
    OCNumberType et2 = DependentVariableGetElementType(appendedDV);
    if (et1 != et2) {
        if (outError) {
            *outError = OCStringCreateWithFormat(
                STR("Append Error: Incompatible numeric types."));
        }
        return false;
    }
    // 4) component‐count checks
    OCIndex n1 = DependentVariableGetComponentCount(dv);
    OCIndex n2 = DependentVariableGetComponentCount(appendedDV);
    if (n1 == 0 || n2 == 0) return false;
    // allow either exactly matching or a single “broadcast” component on dv
    if (n1 != n2 && n1 != 1) return false;
    // 5) for each component, append raw bytes
    for (OCIndex ci = 0; ci < n1; ++ci) {
        // dest is always mutable
        OCMutableDataRef dest = (OCMutableDataRef)
            DependentVariableGetComponentAtIndex(dv, ci);
        // pick matching source, or 0 if appendedDV has exactly one component
        OCIndex srcIdx = (n2 != 1 ? ci : 0);
        OCDataRef src = DependentVariableGetComponentAtIndex(appendedDV, srcIdx);
        OCDataAppendBytes(
            dest,
            OCDataGetBytesPtr(src),
            OCDataGetLength(src));
    }
    return true;
}
OCArrayRef DependentVariableCreatePackedSparseComponentsArray(DependentVariableRef dv, OCArrayRef dimensions) {
    if (!dv || !dimensions) return NULL;
    SparseSamplingRef ss = DependentVariableGetSparseSampling(dv);
    OCIndexSetRef idxs = SparseSamplingGetDimensionIndexes(ss);
    OCArrayRef verts = SparseSamplingGetSparseGridVertexes(ss);
    if (!idxs || !verts) return NULL;
    OCIndex nVerts = OCArrayGetCount(verts);
    // 1) Build an “output” DV of exactly the right size (nVerts)
    DependentVariableRef outDV =
        DependentVariableCreateWithSize(
            DependentVariableGetName(dv),
            DependentVariableGetDescription(dv),
            dv->unit,
            DependentVariableGetQuantityName(dv),
            DependentVariableGetQuantityType(dv),
            DependentVariableGetElementType(dv),
            DependentVariableGetComponentLabels(dv),
            nVerts,
            /* owner */ NULL);
    if (!outDV) return NULL;
    OCStringRef err = NULL;
    // 2) Cross‐section each sparse vertex into outDV
    for (OCIndex iVert = 0; iVert < nVerts; ++iVert) {
        OCIndexPairSetRef pairs = (OCIndexPairSetRef)OCArrayGetValueAtIndex(verts, iVert);
        DependentVariableRef slice =
            DependentVariableCreateCrossSection(dv, dimensions, pairs, &err);
        if (slice) {
            DependentVariableAppend(outDV, slice, &err);
            OCRelease(slice);
        }
    }
    // 3) Extract & return a _deep_ mutable copy of the packed components
    OCArrayRef packed =
        OCArrayCreateMutableCopy(outDV->components);
    OCRelease(outDV);
    return packed;
}
OCDataRef DependentVariableCreateCSDMComponentsData(DependentVariableRef dv,
                                                    OCArrayRef dimensions) {
    if (!dv) return NULL;
    // 1) Allocate the output buffer
    OCMutableDataRef buffer = OCDataCreateMutable(0);
    if (!buffer) return NULL;
    // 2) Decide whether we're packing just the sparse points...
    OCArrayRef sourceArray = NULL;
    bool ownsArray = false;
    SparseSamplingRef ss = DependentVariableGetSparseSampling(dv);
    if (ss) {
        OCIndexSetRef idxs = SparseSamplingGetDimensionIndexes(ss);
        OCArrayRef verts = SparseSamplingGetSparseGridVertexes(ss);
        if (idxs && OCIndexSetGetCount(idxs) > 0 && verts) {
            sourceArray = DependentVariableCreatePackedSparseComponentsArray(dv, dimensions);
            ownsArray = true;  // we'll need to release it
        }
    }
    // 3) …otherwise just concatenate ALL components
    if (!sourceArray) {
        sourceArray = dv->components;
    }
    // 4) Append every chunk in sourceArray to our buffer
    OCIndex n = OCArrayGetCount(sourceArray);
    for (OCIndex i = 0; i < n; ++i) {
        OCDataRef chunk = (OCDataRef)OCArrayGetValueAtIndex(sourceArray, i);
        const uint8_t *ptr = OCDataGetBytesPtr(chunk);
        uint64_t len = OCDataGetLength(chunk);
        if (!OCDataAppendBytes(buffer, ptr, len)) {
            goto fail;
        }
    }
    // 5) Clean up and return
    if (ownsArray) OCRelease(sourceArray);
    return (OCDataRef)buffer;
fail:
    if (ownsArray) OCRelease(sourceArray);
    OCRelease(buffer);
    return NULL;
}
#pragma endregion Creators
#pragma region Tests, Getters & Setters
bool DependentVariableIsScalarType(DependentVariableRef dv) {
    if (!dv || !dv->quantityType) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    return qt && strcmp(qt, "scalar") == 0;
}
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount) {
    if (!dv || !dv->quantityType || !outCount) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “vector_N”
    if (strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) == 1) {
            *outCount = (OCIndex)n;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount) {
    if (!dv || !dv->quantityType || !outCount) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “pixel_N”
    if (strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) == 1) {
            *outCount = (OCIndex)n;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsMatrixType(DependentVariableRef dv, OCIndex *outRows, OCIndex *outCols) {
    if (!dv || !dv->quantityType || !outRows || !outCols) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “matrix_R_C”
    if (strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) == 2) {
            *outRows = (OCIndex)r;
            *outCols = (OCIndex)c;
            return true;
        }
    }
    return false;
}
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv, OCIndex *outN) {
    if (!dv || !dv->quantityType || !outN) return false;
    const char *qt = OCStringGetCString(dv->quantityType);
    if (!qt) return false;
    // “symmetric_matrix_N”
    if (strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) == 1) {
            *outN = (OCIndex)n;
            return true;
        }
    }
    return false;
}
OCIndex DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType) {
    if (!quantityType) return 1;  // default scalar
    const char *qt = OCStringGetCString(quantityType);
    if (!qt) return 1;
    // scalar
    if (strcmp(qt, "scalar") == 0) {
        return 1;
    }
    // pixel_N
    if (strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) == 1) {
            return (OCIndex)n;
        }
    }
    // vector_N
    if (strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) == 1) {
            return (OCIndex)n;
        }
    }
    // matrix_R_C
    if (strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) == 2) {
            return (OCIndex)(r * c);
        }
    }
    // symmetric_matrix_N
    if (strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) == 1) {
            return (OCIndex)(n * (n + 1) / 2);
        }
    }
    return (OCIndex)kOCNotFound;
}
OCIndex DependentVariableGetComponentCount(DependentVariableRef dv) {
    if (!dv) return 0;
    return OCArrayGetCount(dv->components);
}
OCMutableArrayRef DependentVariableGetComponents(DependentVariableRef dv) {
    return dv ? dv->components : NULL;
}
bool DependentVariableSetComponents(DependentVariableRef dv, OCArrayRef newComponents) {
    if (!dv || !newComponents) return false;
    OCIndex count = OCArrayGetCount(newComponents);
    if (count == 0) return false;
    // Validate each component is OCDataRef and has matching size
    uint64_t expectedLength = 0;
    for (OCIndex i = 0; i < count; ++i) {
        OCTypeRef obj = OCArrayGetValueAtIndex(newComponents, i);
        if (OCGetTypeID(obj) != OCDataGetTypeID()) {
            return false;
        }
        OCDataRef data = (OCDataRef)obj;
        uint64_t len = OCDataGetLength(data);
        if (i == 0) {
            expectedLength = len;
        } else if (len != expectedLength) {
            return false;  // mismatched component sizes
        }
    }
    // Install the new components (retain or copy, depending on usage needs)
    OCMutableArrayRef newArray = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < count; ++i) {
        OCArrayAppendValue(newArray, OCArrayGetValueAtIndex(newComponents, i));
    }
    OCRelease(dv->components);
    dv->components = newArray;
    // Rebuild or adjust component labels
    OCRelease(dv->componentLabels);
    dv->componentLabels = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < count; ++i) {
        OCStringRef lbl = OCStringCreateWithFormat(STR("component-%ld"), (long)i);
        OCArrayAppendValue(dv->componentLabels, lbl);
        OCRelease(lbl);
    }
    // Update quantityType if necessary
    OCRelease(dv->quantityType);
    if (count == 1) {
        dv->quantityType = STR("scalar");
    } else {
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    }
    return true;
}
OCMutableArrayRef DependentVariableCopyComponents(DependentVariableRef dv) {
    if (!dv || !dv->components) return NULL;
    OCIndex n = OCArrayGetCount(dv->components);
    OCMutableArrayRef copy =
        OCArrayCreateMutable(n, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < n; i++) {
        OCDataRef buf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, i);
        // Deep‐copy the data buffer:
        OCDataRef bufCopy = (OCDataRef)OCTypeDeepCopyMutable(buf);
        if (bufCopy) {
            OCArrayAppendValue(copy, bufCopy);
            OCRelease(bufCopy);
        }
    }
    return copy;
}
OCDataRef DependentVariableGetComponentAtIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv || !dv->components ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(dv->components))
        return NULL;
    return (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
}
bool DependentVariableSetComponentAtIndex(DependentVariableRef dv, OCDataRef newBuf, OCIndex componentIndex) {
    if (!dv || !dv->components || !newBuf) return false;
    OCIndex n = OCArrayGetCount(dv->components);
    if (componentIndex < 0 || componentIndex >= n) return false;
    OCDataRef oldBuf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    if (OCDataGetLength(newBuf) != OCDataGetLength(oldBuf)) return false;
    OCArraySetValueAtIndex(dv->components, componentIndex, newBuf);
    return true;
}
static void updateForComponentCountChange(DependentVariableRef dv) {
    OCIndex count = OCArrayGetCount(dv->components);
    const char *qt = OCStringGetCString(dv->quantityType);
    // "scalar" stays scalar
    if (strncmp(qt, "vector", 6) == 0) {
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    } else if (strncmp(qt, "pixel", 5) == 0) {
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("pixel_%ld"), (long)count);
    } else if (strncmp(qt, "matrix", 6) == 0 ||
               strncmp(qt, "symmetric_matrix", 16) == 0) {
        // PS fell back to vector for nonuniform grids
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateWithFormat(STR("vector_%ld"), (long)count);
    }
}
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv, OCDataRef component, OCIndex idx) {
    if (!dv || !component) return false;
    OCMutableArrayRef comps = dv->components;
    if (!comps) return false;
    OCIndex count = OCArrayGetCount(comps);
    // valid positions are 0…count
    if (idx < 0 || idx > count) return false;
    // if there's already at least one component, enforce matching byte‐length
    if (count > 0) {
        OCDataRef first = (OCDataRef)OCArrayGetValueAtIndex(comps, 0);
        if (OCDataGetLength(component) != OCDataGetLength(first)) {
            return false;
        }
    }
    OCArrayInsertValueAtIndex(comps, idx, component);
    updateForComponentCountChange(dv);
    return true;
}
bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv, OCIndex idx) {
    if (!dv) return false;
    OCIndex count = OCArrayGetCount(dv->components);
    if (idx >= count || count <= 1)
        return false;
    OCArrayRemoveValueAtIndex(dv->components, idx);
    OCArrayRemoveValueAtIndex(dv->componentLabels, idx);
    updateForComponentCountChange(dv);
    return true;
}
OCIndex DependentVariableGetSize(DependentVariableRef dv) {
    if (!dv) return 0;
    OCIndex componentsCount = OCArrayGetCount(dv->components);
    if (componentsCount == 0) return 0;
    OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(dv->components, 0);
    size_t byteLength = OCDataGetLength(blob);
    // element size in bytes for this variable’s unit/quantity
    size_t eltSize = SIQuantityElementSize((SIQuantityRef)dv);
    if (eltSize == 0) return 0;
    return (OCIndex)(byteLength / eltSize);
}
bool DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize) {
    if (!dv) return false;
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0) return false;
    OCIndex oldSize = DependentVariableGetSize(dv);
    // pick up byte‐size per element
    size_t elemSize;
    switch (dv->numericType) {
        // 8-bit integers
        case kOCNumberSInt8Type:
        case kOCNumberUInt8Type:
            elemSize = 1;
            break;
        // 16-bit integers
        case kOCNumberSInt16Type:
        case kOCNumberUInt16Type:
            elemSize = 2;
            break;
        // 32-bit integers
        case kOCNumberSInt32Type:
        case kOCNumberUInt32Type:
            elemSize = 4;
            break;
        // 64-bit integers
        case kOCNumberSInt64Type:
        case kOCNumberUInt64Type:
            elemSize = 8;
            break;
        // IEEE floats
        case kOCNumberFloat32Type:
            elemSize = sizeof(float);
            break;
        case kOCNumberFloat64Type:
            elemSize = sizeof(double);
            break;
        // Complex (real+imaginary)
        case kOCNumberComplex64Type:
            elemSize = 2 * sizeof(float);
            break;
        case kOCNumberComplex128Type:
            elemSize = 2 * sizeof(double);
            break;
        default:
            // Should never happen if numericType is valid
            elemSize = 0;
            break;
    }
    OCIndex newByteLen = newSize * elemSize;
    // if shrinking, just cut each buffer down
    if (newSize < oldSize) {
        for (OCIndex i = 0; i < nComps; i++) {
            OCMutableDataRef buf =
                (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, i);
            OCDataSetLength(buf, newByteLen);
        }
        return true;
    }
    // else we must grow: copy-and-replace each buffer, then zero its tail
    for (OCIndex i = 0; i < nComps; i++) {
        OCDataRef oldBuf = (OCDataRef)OCArrayGetValueAtIndex(dv->components, i);
        // deep-mutable copy
        OCMutableDataRef newBuf =
            (OCMutableDataRef)OCTypeDeepCopyMutable(oldBuf);
        if (!newBuf) return false;
        // resize to the new byte length
        OCDataSetLength(newBuf, newByteLen);
        // install it
        OCArraySetValueAtIndex(dv->components, i, newBuf);
        // zero out the newly-appended region
        uint8_t *bytes = (uint8_t *)OCDataGetMutableBytes(newBuf);
        size_t offset = (size_t)oldSize * elemSize;
        size_t count = (size_t)(newSize - oldSize) * elemSize;
        memset(bytes + offset, 0, count);
        OCRelease(newBuf);
    }
    return true;
}
OCStringRef DependentVariableGetEncoding(DependentVariableRef dv) {
    return dv ? dv->encoding : NULL;
}
bool DependentVariableSetEncoding(DependentVariableRef dv, OCStringRef newEnc) {
    if (!dv || !newEnc) return false;
    OCStringRef copy = OCStringCreateCopy(newEnc);
    if (!copy) return false;
    OCRelease(dv->encoding);
    dv->encoding = copy;
    return true;
}
OCStringRef DependentVariableGetType(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->type;
}
bool DependentVariableShouldSerializeExternally(DependentVariableRef dv) {
    return dv && OCStringEqual(dv->type, STR(kDependentVariableComponentTypeValueExternal));
}
bool DependentVariableSetType(DependentVariableRef dv, OCStringRef newType) {
    if (!dv || !newType) return false;
    // Validate newType against allowed values
    if (!OCStringEqual(newType, STR(kDependentVariableComponentTypeValueInternal)) &&
        !OCStringEqual(newType, STR(kDependentVariableComponentTypeValueExternal))) {
        return false;
    }
    OCStringRef copy = OCStringCreateCopy(newType);
    if (!copy) return false;
    OCRelease(dv->type);
    dv->type = copy;
    return true;
}
OCStringRef DependentVariableGetComponentsURL(DependentVariableRef dv) {
    return dv ? dv->componentsURL : NULL;
}
bool DependentVariableSetComponentsURL(DependentVariableRef dv, OCStringRef url) {
    if (!dv) return false;
    OCStringRef copy = url ? OCStringCreateCopy(url) : NULL;
    OCRelease(dv->componentsURL);
    dv->componentsURL = copy;
    return true;
}
OCStringRef DependentVariableGetName(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // If the name field is NULL, return the interned empty string
    return dv->name ? dv->name : STR("");
}
bool DependentVariableSetName(DependentVariableRef dv, OCStringRef newName) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    // If it’s literally the same pointer, nothing to do
    if (dv->name == newName) {
        return true;
    }
    // Copy the incoming string (or default to "")
    OCStringRef copy = newName
                           ? OCStringCreateCopy(newName)
                           : STR("");
    if (!copy) {
        // allocation failed
        return false;
    }
    // Replace old name
    OCRelease(dv->name);
    dv->name = copy;
    return true;
}
OCStringRef DependentVariableGetDescription(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // Return the stored description, or the empty interned string if NULL
    return dv->description ? dv->description : STR("");
}
bool DependentVariableSetDescription(DependentVariableRef dv, OCStringRef newDesc) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    // If it's literally the same object, nothing to do
    if (dv->description == newDesc) {
        return true;
    }
    // Prepare a copy (or get the interned empty string)
    OCStringRef copy = newDesc
                           ? OCStringCreateCopy(newDesc)
                           : STR("");
    if (!copy) {
        return false;
    }
    // Swap it in
    OCRelease(dv->description);
    dv->description = copy;
    return true;
}
OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    return dv->quantityType;
}
bool DependentVariableSetQuantityType(DependentVariableRef dv, OCStringRef qt) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    if (!qt) return false;
    const char *cstr = OCStringGetCString(qt);
    if (!cstr) return false;
    size_t len = strlen(cstr);
    OCIndex compCount = OCArrayGetCount(dv->components);
    // scalar
    if (strcmp(cstr, "scalar") == 0) {
        if (compCount != 1) return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // pixel_N
    if (len > 5 && strncmp(cstr, "pixel", 5) == 0) {
        long n;
        if (sscanf(cstr, "pixel_%ld", &n) != 1 || n != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // vector_N
    if (len > 6 && strncmp(cstr, "vector", 6) == 0) {
        long n;
        if (sscanf(cstr, "vector_%ld", &n) != 1 || n != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // matrix_N_M
    if (len > 6 && strncmp(cstr, "matrix", 6) == 0) {
        long n, m;
        if (sscanf(cstr, "matrix_%ld_%ld", &n, &m) != 2 || n * m != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    // symmetric_matrix_N
    if (len > 16 && strncmp(cstr, "symmetric_matrix", 16) == 0) {
        long n;
        if (sscanf(cstr, "symmetric_matrix_%ld", &n) != 1 || (n * (n + 1) / 2) != compCount)
            return false;
        OCRelease(dv->quantityType);
        dv->quantityType = OCRetain(qt);
        return true;
    }
    return false;
}
OCMutableArrayRef DependentVariableCreateQuantityTypesArray(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    OCIndex count = OCArrayGetCount(dv->components);
    OCMutableArrayRef types = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!types) return NULL;
    // scalar
    if (count == 1) {
        OCArrayAppendValue(types, STR("scalar"));
    }
    // vector_N
    OCStringRef vstr = OCStringCreateWithFormat(
        STR("vector_%ld"),
        (long)count);
    OCArrayAppendValue(types, vstr);
    OCRelease(vstr);
    // pixel_N
    OCStringRef pstr = OCStringCreateWithFormat(
        STR("pixel_%ld"),
        (long)count);
    OCArrayAppendValue(types, pstr);
    OCRelease(pstr);
    // symmetric_matrix_N  if N*(N+1)/2 == count
    for (OCIndex n = 1; n < count; ++n) {
        if (n * (n + 1) / 2 == count) {
            OCStringRef sstr = OCStringCreateWithFormat(
                STR("symmetric_matrix_%ld"),
                (long)n);
            OCArrayAppendValue(types, sstr);
            OCRelease(sstr);
        }
    }
    // matrix_M_N  if M*N == count
    for (OCIndex m = 1; m <= count; ++m) {
        for (OCIndex n = 1; n <= count; ++n) {
            if (m * n == count) {
                OCStringRef mstr = OCStringCreateWithFormat(
                    STR("matrix_%ld_%ld"),
                    (long)m, (long)n);
                OCArrayAppendValue(types, mstr);
                OCRelease(mstr);
            }
        }
    }
    return types;
}
OCDictionaryRef DependentVariableGetMetaData(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->metaData;
}
bool DependentVariableSetMetaData(DependentVariableRef dv, OCDictionaryRef dict) {
    if (!dv) return false;
    OCRelease(dv->metaData);
    dv->metaData = dict ? OCTypeDeepCopyMutable(dict) : OCDictionaryCreateMutable(0);
    return dv->metaData != NULL;
}
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->owner;
}
bool DependentVariableSetOwner(DependentVariableRef dv, OCTypeRef owner) {
    if (!dv) return false;
    dv->owner = owner;
    return true;
}
SparseSamplingRef DependentVariableGetSparseSampling(DependentVariableRef dv) {
    return dv ? dv->sparseSampling : NULL;
}
bool DependentVariableSetSparseSampling(DependentVariableRef dv,
                                        SparseSamplingRef ss) {
    if (!dv) return false;
    // Release any existing sparseSampling
    OCRelease(dv->sparseSampling);
    if (ss) {
        // Deep-copy the provided SparseSampling
        dv->sparseSampling = (SparseSamplingRef)OCTypeDeepCopyMutable(ss);
        return dv->sparseSampling != NULL;
    } else {
        // Clearing out sparseSampling
        dv->sparseSampling = NULL;
        return true;
    }
}
OCStringRef DependentVariableGetQuantityName(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->quantityName;
}
bool DependentVariableSetQuantityName(DependentVariableRef dv, OCStringRef quantityName) {
    if (!dv || !quantityName) return false;
    // 1) Check that the name corresponds to a known dimensionality
    OCStringRef err = NULL;
    SIDimensionalityRef qDim = SIDimensionalityForQuantity(quantityName, &err);
    if (!qDim) {
        if (err) OCRelease(err);
        return false;
    }
    OCRelease(err);
    // 2) If it’s already set to exactly that same object, we’re done
    if (dv->quantityName == quantityName) return true;
    // 3) Validate everything else still lines up (unit, type, labels, count)
    OCIndex count = OCArrayGetCount(dv->components);
    if (!validateDependentVariableParameters(
            dv->type, dv->unit, quantityName,
            dv->quantityType, dv->componentLabels, count,
            dv->sparseSampling))
        return false;
    // 4) All good—swap in the new name
    OCRelease(dv->quantityName);
    dv->quantityName = OCStringCreateCopy(quantityName);
    return dv->quantityName != NULL;
}
OCArrayRef DependentVariableGetComponentLabels(DependentVariableRef dv) {
    if (!dv) return NULL;
    return (OCArrayRef)dv->componentLabels;
}
bool DependentVariableSetComponentLabels(DependentVariableRef dv, OCArrayRef labels) {
    if (!dv) return false;
    // release old labels
    OCRelease(dv->componentLabels);
    if (labels) {
        // deep‐mutable copy of the incoming array
        dv->componentLabels = OCArrayCreateMutableCopy(labels);
    } else {
        // no labels → empty mutable array
        dv->componentLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    }
    return dv->componentLabels != NULL;
}
OCStringRef DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv) return NULL;
    OCStringRef name = dv->name;
    OCArrayRef labels = (OCArrayRef)dv->componentLabels;
    OCStringRef componentName = NULL;
    if (labels && componentIndex >= 0 && componentIndex < OCArrayGetCount(labels)) {
        componentName = OCArrayGetValueAtIndex(labels, componentIndex);
    }
    // If we have both a non‐empty name and a componentName, join them:
    if (name && OCStringGetLength(name) > 0 && componentName) {
        return OCStringCreateWithFormat(
            STR("%@ : %@"),
            name,
            componentName);
    }
    // Otherwise just copy the component label (or fall back to empty string).
    if (componentName) {
        return OCStringCreateCopy(componentName);
    } else {
        return STR("");
    }
}
OCStringRef DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv, OCIndex componentIndex) {
    if (!dv) return NULL;
    OCArrayRef labels = (OCArrayRef)dv->componentLabels;
    if (!labels ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(labels)) {
        return NULL;
    }
    return OCArrayGetValueAtIndex(labels, componentIndex);
}
bool DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv, OCStringRef newLabel, OCIndex componentIndex) {
    if (!dv || !newLabel) return false;
    OCArrayRef comps = (OCArrayRef)dv->components;
    OCMutableArrayRef labels = dv->componentLabels;
    OCIndex count = comps ? OCArrayGetCount(comps) : 0;
    if (componentIndex < 0 || componentIndex >= count) {
        return false;
    }
    // Just overwrite the slot; OCArray uses retain/release automatically.
    OCArraySetValueAtIndex(labels, componentIndex, newLabel);
    return true;
}
OCNumberType DependentVariableGetElementType(DependentVariableRef dv) {
    if (!dv) return kOCNumberTypeInvalid;
    return dv->numericType;
}
bool DependentVariableSetElementType(DependentVariableRef dv, OCNumberType newType) {
    if (!dv) return false;
    OCNumberType oldType = dv->numericType;
    if (oldType == newType) return true;
    OCMutableArrayRef comps = dv->components;
    if (!comps) return false;
    OCIndex nComps = OCArrayGetCount(comps);
    OCIndex nElems = DependentVariableGetSize(dv);
    size_t newBytes = nElems * OCNumberTypeSize((OCNumberType)newType);
    for (OCIndex ci = 0; ci < nComps; ci++) {
        OCMutableDataRef oldData = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *oldPtr = OCDataGetMutableBytes(oldData);
        OCMutableDataRef newData = OCDataCreateMutable(0);
        void *tmpBuf = malloc(newBytes);
        // copy/convert element-by-element:
        for (OCIndex ei = 0; ei < nElems; ei++) {
            switch (oldType) {
                case kOCNumberFloat32Type: {
                    float *src = (float *)oldPtr;
                    float f = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = f;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = f;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberFloat64Type: {
                    double *src = (double *)oldPtr;
                    double d = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)d;
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = d;
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = (float)d;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = d;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberComplex64Type: {
                    float complex *src = (float complex *)oldPtr;
                    float complex z = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = z;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = z;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kOCNumberComplex128Type: {
                    double complex *src = (double complex *)oldPtr;
                    double complex z = src[ei];
                    switch (newType) {
                        case kOCNumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)creal(z);
                            break;
                        case kOCNumberFloat64Type:
                            ((double *)tmpBuf)[ei] = creal(z);
                            break;
                        case kOCNumberComplex64Type:
                            ((float complex *)tmpBuf)[ei] = (float)creal(z) + (float)cimag(z) * I;
                            break;
                        case kOCNumberComplex128Type:
                            ((double complex *)tmpBuf)[ei] = z;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        // append and swap in
        OCDataAppendBytes(newData, tmpBuf, newBytes);
        free(tmpBuf);
        OCArraySetValueAtIndex(comps, ci, newData);
        OCRelease(newData);
    }
    dv->numericType = newType;
    return true;
}
bool DependentVariableSetValues(DependentVariableRef dv, OCIndex componentIndex, OCDataRef values) {
    // NULL‐check
    if (!dv) return false;
    // Bounds check
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0 || componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }
    // Ensure the new data blob is the same length as the existing components
    {
        OCDataRef first = (OCDataRef)OCArrayGetValueAtIndex(dv->components, 0);
        size_t oldLen = OCDataGetLength(first);
        size_t newLen = OCDataGetLength(values);
        if (oldLen != newLen) return false;
    }
    // If it’s literally the same object, nothing to do
    OCMutableDataRef oldValues = (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    if (oldValues == values) return true;
    // Otherwise make a mutable copy and swap it in
    OCIndex length = OCDataGetLength(values);
    OCMutableDataRef newValues = OCDataCreateMutableCopy(length, values);
    OCArraySetValueAtIndex(dv->components, componentIndex, newValues);
    OCRelease(newValues);
    return true;
}
float DependentVariableGetFloatValueAtMemOffset(DependentVariableRef dv,
                                                OCIndex componentIndex,
                                                OCIndex memOffset) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 ||
        nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps) {
        return NAN;
    }
    // wrap negative or out‐of‐bounds offsets
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        // 8-bit integers
        case kOCNumberSInt8Type: {
            int8_t *arr = (int8_t *)bytes;
            return (float)arr[memOffset];
        }
        case kOCNumberUInt8Type: {
            uint8_t *arr = (uint8_t *)bytes;
            return (float)arr[memOffset];
        }
        // 16-bit integers
        case kOCNumberSInt16Type: {
            int16_t *arr = (int16_t *)bytes;
            return (float)arr[memOffset];
        }
        case kOCNumberUInt16Type: {
            uint16_t *arr = (uint16_t *)bytes;
            return (float)arr[memOffset];
        }
        // 32-bit integers
        case kOCNumberSInt32Type: {
            int32_t *arr = (int32_t *)bytes;
            return (float)arr[memOffset];
        }
        case kOCNumberUInt32Type: {
            uint32_t *arr = (uint32_t *)bytes;
            return (float)arr[memOffset];
        }
        // 64-bit integers
        case kOCNumberSInt64Type: {
            int64_t *arr = (int64_t *)bytes;
            return (float)arr[memOffset];
        }
        case kOCNumberUInt64Type: {
            uint64_t *arr = (uint64_t *)bytes;
            return (float)arr[memOffset];
        }
        // IEEE floats
        case kOCNumberFloat32Type: {
            float *arr = (float *)bytes;
            return arr[memOffset];
        }
        case kOCNumberFloat64Type: {
            double *arr = (double *)bytes;
            return (float)arr[memOffset];
        }
        // Complex (return the real part)
        case kOCNumberComplex64Type: {
            float complex *arr = (float complex *)bytes;
            return (float)crealf(arr[memOffset]);
        }
        case kOCNumberComplex128Type: {
            double complex *arr = (double complex *)bytes;
            return (float)crealf(arr[memOffset]);
        }
        default:
            return NAN;
    }
}
double DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv,
                                                  OCIndex componentIndex,
                                                  OCIndex memOffset) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 ||
        nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps) {
        return NAN;
    }
    // wrap negative or out‐of‐bounds offsets
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        // 8-bit integers
        case kOCNumberSInt8Type:
            return (double)((int8_t *)bytes)[memOffset];
        case kOCNumberUInt8Type:
            return (double)((uint8_t *)bytes)[memOffset];
        // 16-bit integers
        case kOCNumberSInt16Type:
            return (double)((int16_t *)bytes)[memOffset];
        case kOCNumberUInt16Type:
            return (double)((uint16_t *)bytes)[memOffset];
        // 32-bit integers
        case kOCNumberSInt32Type:
            return (double)((int32_t *)bytes)[memOffset];
        case kOCNumberUInt32Type:
            return (double)((uint32_t *)bytes)[memOffset];
        // 64-bit integers
        case kOCNumberSInt64Type:
            return (double)((int64_t *)bytes)[memOffset];
        case kOCNumberUInt64Type:
            return (double)((uint64_t *)bytes)[memOffset];
        // IEEE floats
        case kOCNumberFloat32Type:
            return (double)((float *)bytes)[memOffset];
        case kOCNumberFloat64Type:
            return ((double *)bytes)[memOffset];
        // Complex (return the real part)
        case kOCNumberComplex64Type:
            return (double)crealf(((float complex *)bytes)[memOffset]);
        case kOCNumberComplex128Type:
            return creal(((double complex *)bytes)[memOffset]);
        default:
            return NAN;
    }
}
float complex DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    if (!dv) return NAN + NAN * I;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN + NAN * I;
    }
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float *arr = (float *)bytes;
            return (float complex)arr[memOffset];
        }
        case kOCNumberFloat64Type: {
            double *arr = (double *)bytes;
            return (float complex)arr[memOffset];
        }
        case kOCNumberComplex64Type: {
            float complex *arr = (float complex *)bytes;
            return arr[memOffset];
        }
        case kOCNumberComplex128Type: {
            double complex *arr = (double complex *)bytes;
            return (float complex)arr[memOffset];
        }
        default:
            return NAN + NAN * I;
    }
}
double complex DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    if (!dv) return NAN + NAN * I;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN + NAN * I;
    }
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float *arr = (float *)bytes;
            return (double complex)arr[memOffset];
        }
        case kOCNumberFloat64Type: {
            double *arr = (double *)bytes;
            return (double complex)arr[memOffset];
        }
        case kOCNumberComplex64Type: {
            float complex *arr = (float complex *)bytes;
            return (double complex)crealf(arr[memOffset]);
        }
        case kOCNumberComplex128Type: {
            double complex *arr = (double complex *)bytes;
            return arr[memOffset];
        }
        default:
            return NAN + NAN * I;
    }
}
double DependentVariableGetDoubleValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex componentIndex,
    OCIndex memOffset,
    complexPart part) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 ||
        nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps) {
        return NAN;
    }
    // wrap and normalize offset
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    double realv = 0.0;
    double imagv = 0.0;
    switch (dv->numericType) {
        // signed integers
        case kOCNumberSInt8Type:
            realv = (double)((int8_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt16Type:
            realv = (double)((int16_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt32Type:
            realv = (double)((int32_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt64Type:
            realv = (double)((int64_t *)bytes)[memOffset];
            break;
        // unsigned integers
        case kOCNumberUInt8Type:
            realv = (double)((uint8_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt16Type:
            realv = (double)((uint16_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt32Type:
            realv = (double)((uint32_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt64Type:
            realv = (double)((uint64_t *)bytes)[memOffset];
            break;
        // floats
        case kOCNumberFloat32Type:
            realv = (double)((float *)bytes)[memOffset];
            break;
        case kOCNumberFloat64Type:
            realv = ((double *)bytes)[memOffset];
            break;
        // complex floats
        case kOCNumberComplex64Type: {
            float complex *arr = (float complex *)bytes;
            float complex v = arr[memOffset];
            realv = crealf(v);
            imagv = cimagf(v);
            break;
        }
        // complex doubles
        case kOCNumberComplex128Type: {
            double complex *arr = (double complex *)bytes;
            double complex v = arr[memOffset];
            realv = creal(v);
            imagv = cimag(v);
            break;
        }
        default:
            return NAN;
    }
    // now pick the requested part
    switch (part) {
        case kSIRealPart:
            return realv;
        case kSIImaginaryPart:
            return imagv;
        case kSIMagnitudePart:
            return hypot(realv, imagv);
        case kSIArgumentPart:
            return atan2(imagv, realv);
    }
    return NAN;
}
float DependentVariableGetFloatValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex componentIndex,
    OCIndex memOffset,
    complexPart part) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 ||
        nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps) {
        return NAN;
    }
    // wrap and normalize offset
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    float realv = 0.0f;
    float imagv = 0.0f;
    switch (dv->numericType) {
        // signed integers
        case kOCNumberSInt8Type:
            realv = (float)((int8_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt16Type:
            realv = (float)((int16_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt32Type:
            realv = (float)((int32_t *)bytes)[memOffset];
            break;
        case kOCNumberSInt64Type:
            realv = (float)((int64_t *)bytes)[memOffset];
            break;
        // unsigned integers
        case kOCNumberUInt8Type:
            realv = (float)((uint8_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt16Type:
            realv = (float)((uint16_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt32Type:
            realv = (float)((uint32_t *)bytes)[memOffset];
            break;
        case kOCNumberUInt64Type:
            realv = (float)((uint64_t *)bytes)[memOffset];
            break;
        // floats
        case kOCNumberFloat32Type:
            realv = ((float *)bytes)[memOffset];
            break;
        case kOCNumberFloat64Type:
            realv = (float)((double *)bytes)[memOffset];
            break;
        // complex floats
        case kOCNumberComplex64Type: {
            float complex *arr = (float complex *)bytes;
            float complex v = arr[memOffset];
            realv = crealf(v);
            imagv = cimagf(v);
            break;
        }
        // complex doubles
        case kOCNumberComplex128Type: {
            double complex *arr = (double complex *)bytes;
            double complex v = arr[memOffset];
            realv = (float)creal(v);
            imagv = (float)cimag(v);
            break;
        }
        default:
            return NAN;
    }
    // now pick and return the requested part as a float
    switch (part) {
        case kSIRealPart:
            return realv;
        case kSIImaginaryPart:
            return imagv;
        case kSIMagnitudePart:
            return hypotf(realv, imagv);
        case kSIArgumentPart:
            return atan2f(imagv, realv);
    }
    return NAN;
}
SIScalarRef DependentVariableCreateValueFromMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset) {
    if (!dv) return NULL;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NULL;
    }
    // wrap memOffset into [0..size)
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    // grab the raw bytes
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float v = ((float *)bytes)[memOffset];
            return SIScalarCreateWithFloat(v, dv->unit);
        }
        case kOCNumberFloat64Type: {
            double v = ((double *)bytes)[memOffset];
            return SIScalarCreateWithDouble(v, dv->unit);
        }
        case kOCNumberComplex64Type: {
            float complex v = ((float complex *)bytes)[memOffset];
            return SIScalarCreateWithFloatComplex(v, dv->unit);
        }
        case kOCNumberComplex128Type: {
            double complex v = ((double complex *)bytes)[memOffset];
            return SIScalarCreateWithDoubleComplex(v, dv->unit);
        }
        default:
            return NULL;
    }
}
bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv, OCIndex componentIndex, OCIndex memOffset, SIScalarRef value, OCStringRef *error) {
    // if caller already set *error, bail
    if (error && *error) return false;
    if (!dv) return false;
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }
    // dimensionality check
    if (!SIQuantityHasSameReducedDimensionality((SIQuantityRef)dv,
                                                (SIQuantityRef)value)) {
        if (error) {
            *error = STR("DependentVariableSetValueAtMemOffset: Incompatible dimensionalities");
        }
        return false;
    }
    OCIndex size = DependentVariableGetSize(dv);
    if (size == 0) return false;
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    // grab a mutable pointer into the data
    OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    void *bytes = OCDataGetMutableBytes(data);
    switch (dv->numericType) {
        case kOCNumberFloat32Type: {
            float v = SIScalarFloatValueInUnit(value, dv->unit, NULL);
            ((float *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberFloat64Type: {
            double v = SIScalarDoubleValueInUnit(value, dv->unit, NULL);
            ((double *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberComplex64Type: {
            float complex v = SIScalarFloatComplexValueInUnit(value, dv->unit, NULL);
            ((float complex *)bytes)[memOffset] = v;
            break;
        }
        case kOCNumberComplex128Type: {
            double complex v = SIScalarDoubleComplexValueInUnit(value, dv->unit, NULL);
            ((double complex *)bytes)[memOffset] = v;
            break;
        }
        default:
            return false;
    }
    return true;
}
#pragma endregion Tests, Getters &Setters
#pragma region Conversion and Manipulation
/**
 * @brief Convert all component data in a dependent variable to a new unit.
 *        Integer‐typed dependent variables cannot be converted and will error.
 * @param dv      The dependent variable to convert.
 * @param unit    The target unit (must have the same reduced dimensionality).
 * @param error   On failure, receives an OCStringRef describing the problem; may be NULL.
 *                   Caller should release *error if non-NULL.
 * @return true on success, false on error.
 * @ingroup RMNLib
 */
bool DependentVariableConvertToUnit(DependentVariableRef dv,
                                    SIUnitRef unit,
                                    OCStringRef *error) {
    /* Bail if caller passed in an existing error string */
    if (error && *error) return false;
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    /* Retrieve components and ensure there is data to convert */
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t count = OCArrayGetCount(comps);
    if (count == 0) {
        if (error) {
            *error = STR("Convert to Unit failed: no data components present.");
        }
        return false;
    }
    /* Determine element type and reject integer types */
    OCNumberType etype = DependentVariableGetElementType(dv);
    switch (etype) {
        case kOCNumberSInt8Type:
        case kOCNumberSInt16Type:
        case kOCNumberSInt32Type:
        case kOCNumberSInt64Type:
        case kOCNumberUInt8Type:
        case kOCNumberUInt16Type:
        case kOCNumberUInt32Type:
        case kOCNumberUInt64Type:
            if (error) {
                *error = STR("Invalid operation: cannot convert integer-typed dependent variable to another unit.");
            }
            return false;
        default:
            break;
    }
    /* Check dimensionality compatibility */
    SIUnitRef oldUnit = SIQuantityGetUnit((SIQuantityRef) dv);
    SIDimensionalityRef oldDim = SIUnitGetDimensionality(oldUnit);
    SIDimensionalityRef newDim = SIUnitGetDimensionality(unit);
    if (!SIDimensionalityHasSameReducedDimensionality(oldDim, newDim)) {
        if (error) {
            *error = STR("Convert to Unit failed: incompatible dimensionalities.");
        }
        return false;
    }
    /* Compute conversion factor and update stored unit */
    double factor = SIUnitConversion(oldUnit, unit);
    SIQuantitySetUnit((SIMutableQuantityRef) dv, unit);
    /* Scale each component in a simple loop */
    uint64_t size = DependentVariableGetSize(dv);
    for (uint64_t ci = 0; ci < count; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (etype) {
            case kOCNumberFloat32Type: {
                float *arr = (float *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= (float)factor;
                }
                break;
            }
            case kOCNumberFloat64Type: {
                double *arr = (double *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= factor;
                }
                break;
            }
            case kOCNumberComplex64Type: {
                float complex *arr = (float complex *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= (float)factor;
                }
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *arr = (double complex *)bytes;
                for (uint64_t i = 0; i < size; ++i) {
                    arr[i] *= factor;
                }
                break;
            }
            default:
                /* All integer types handled above; no action here */
                break;
        }
    }
    return true;
}
bool DependentVariableSetValuesToZero(DependentVariableRef dv, int64_t componentIndex) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0) return false;
    // Determine the range of components to clear
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        if ((uint64_t)componentIndex >= nComps) return false;
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    // For each selected component, just memset its entire byte buffer to zero
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        uint64_t byteCount = OCDataGetLength(data);
        memset(bytes, 0, byteCount);
    }
    return true;
}

bool DependentVariableZeroPartInRange(DependentVariableRef dv,
                                 OCIndex componentIndex,
                                 OCRange range,
                                 complexPart part)
{
    if (!dv) return false;

    OCArrayRef comps = dv->components;
    OCIndex nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 || componentIndex < 0 || componentIndex >= nComps) {
        return false;
    }

    OCIndex totalSize = DependentVariableGetSize(dv);
    if (range.location < 0
     || range.length  < 0
     || range.location + range.length > totalSize) {
        return false;
    }

    OCIndex startComp = componentIndex;
    OCIndex endComp   = componentIndex + 1;

    for (OCIndex ci = startComp; ci < endComp; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *ptr = OCDataGetMutableBytes(data);
        OCNumberType etype = DependentVariableGetElementType(dv);

        switch (etype) {

          case kOCNumberFloat32Type:
            if (part == kSIRealPart || part == kSIMagnitudePart) {
                /* scale the subvector at &ptr[range.location] by 0.0f */
                cblas_sscal((int)range.length,
                            0.0f,
                            (float*)ptr + range.location,
                            1);
            }
            break;

          case kOCNumberFloat64Type:
            if (part == kSIRealPart || part == kSIMagnitudePart) {
                cblas_dscal((int)range.length,
                            0.0,
                            (double*)ptr + range.location,
                            1);
            }
            break;

          case kOCNumberComplex64Type: {
            float complex *cptr = (float complex*)ptr;
            switch (part) {
              case kSIRealPart:
                /* zero real entries, keep imag: real at stride=2 offset 0 */
                cblas_sscal((int)range.length,
                            0.0f,
                            (float*)cptr + 2*range.location,
                            2);
                break;
              case kSIImaginaryPart:
                /* zero imag entries: offset 1, stride=2 */
                cblas_sscal((int)range.length,
                            0.0f,
                            (float*)cptr + 2*range.location + 1,
                            2);
                break;
              case kSIMagnitudePart:
                /* zero both real & imag: treat as 2*length contiguous */
                cblas_sscal((int)(2*range.length),
                            0.0f,
                            (float*)cptr + 2*range.location,
                            1);
                break;
              case kSIArgumentPart:
                /* must compute abs and zero imag by hand */
                for (OCIndex i = range.location; i < range.location + range.length; ++i) {
                  float complex v = cptr[i];
                  float m = cabsf(v);
                  cptr[i] = m + 0.0f * I;
                }
                break;
            }
            break;
          }

          case kOCNumberComplex128Type: {
            double complex *cptr = (double complex*)ptr;
            switch (part) {
              case kSIRealPart:
                cblas_dscal((int)range.length,
                            0.0,
                            (double*)cptr + 2*range.location,
                            2);
                break;
              case kSIImaginaryPart:
                cblas_dscal((int)range.length,
                            0.0,
                            (double*)cptr + 2*range.location + 1,
                            2);
                break;
              case kSIMagnitudePart:
                cblas_dscal((int)(2*range.length),
                            0.0,
                            (double*)cptr + 2*range.location,
                            1);
                break;
              case kSIArgumentPart:
                for (OCIndex i = range.location; i < range.location + range.length; ++i) {
                  double complex v = cptr[i];
                  double m = cabs(v);
                  cptr[i] = m + 0.0 * I;
                }
                break;
            }
            break;
          }

          default:
            /* integer types or unsupported */
            return false;
        }
    }

    return true;
}

bool DependentVariableTakeAbsoluteValue(DependentVariableRef dv,
                                        int64_t componentIndex) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0 ||
        (componentIndex >= 0 && (uint64_t)componentIndex >= nComps)) {
        return false;
    }
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    uint64_t size = DependentVariableGetSize(dv);
    OCNumberType origEtype = DependentVariableGetElementType(dv);
    OCNumberType newEtype = origEtype;
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (origEtype) {
            case kOCNumberSInt8Type: {
                int8_t *arr = (int8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt16Type: {
                int16_t *arr = (int16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt32Type: {
                int32_t *arr = (int32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberSInt64Type: {
                int64_t *arr = (int64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = arr[i] < 0 ? -arr[i] : arr[i];
                break;
            }
            case kOCNumberUInt8Type:
            case kOCNumberUInt16Type:
            case kOCNumberUInt32Type:
            case kOCNumberUInt64Type:
                /* Unsigned types are already non-negative */
                break;
            case kOCNumberFloat32Type: {
                float *arr = (float *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = fabsf(arr[i]);
                break;
            }
            case kOCNumberFloat64Type: {
                double *arr = (double *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = fabs(arr[i]);
                break;
            }
            case kOCNumberComplex64Type: {
                float complex *arr = (float complex *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = cabsf(arr[i]); /* real=|z|, imag=0 */
                newEtype = kOCNumberFloat32Type;
                break;
            }
            case kOCNumberComplex128Type: {
                double complex *arr = (double complex *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = cabs(arr[i]); /* real=|z|, imag=0 */
                newEtype = kOCNumberFloat64Type;
                break;
            }
            default:
                return false;
        }
    }
    if (newEtype != origEtype) {
        DependentVariableSetElementType(dv, newEtype);
    }
    return true;
}
bool DependentVariableMultiplyValuesByDimensionlessComplexConstant(DependentVariableRef dv,
                                                                   int64_t componentIndex,
                                                                   double complex constant) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, false);
    OCArrayRef comps = DependentVariableGetComponents(dv);
    uint64_t nComps = OCArrayGetCount(comps);
    if (nComps == 0 ||
        (componentIndex >= 0 && (uint64_t)componentIndex >= nComps)) {
        return false;
    }
    uint64_t lower = 0, upper = nComps;
    if (componentIndex >= 0) {
        lower = (uint64_t)componentIndex;
        upper = lower + 1;
    }
    uint64_t size = DependentVariableGetSize(dv);
    OCNumberType etype = DependentVariableGetElementType(dv);
    /* Prepare BLAS scalars */
    float scalar_f32 = (float)creal(constant);
    double scalar_f64 = creal(constant);
    float scalar_c32[2] = {(float)creal(constant), (float)cimag(constant)};
    double scalar_c64[2] = {creal(constant), cimag(constant)};
    for (uint64_t ci = lower; ci < upper; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);
        uint8_t *bytes = OCDataGetMutableBytes(data);
        switch (etype) {
            case kOCNumberSInt8Type: {
                int8_t *arr = (int8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt16Type: {
                int16_t *arr = (int16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt32Type: {
                int32_t *arr = (int32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberSInt64Type: {
                int64_t *arr = (int64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (int64_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt8Type: {
                uint8_t *arr = (uint8_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint8_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt16Type: {
                uint16_t *arr = (uint16_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint16_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt32Type: {
                uint32_t *arr = (uint32_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint32_t)(arr[i] * constant);
                break;
            }
            case kOCNumberUInt64Type: {
                uint64_t *arr = (uint64_t *)bytes;
                for (uint64_t i = 0; i < size; ++i)
                    arr[i] = (uint64_t)(arr[i] * constant);
                break;
            }
            case kOCNumberFloat32Type:
                cblas_sscal((int)size, scalar_f32, (float *)bytes, 1);
                break;
            case kOCNumberFloat64Type:
                cblas_dscal((int)size, scalar_f64, (double *)bytes, 1);
                break;
            case kOCNumberComplex64Type:
                cblas_cscal((int)size,
                            (const void *)scalar_c32,
                            (float _Complex *)bytes,
                            1);
                break;
            case kOCNumberComplex128Type:
                cblas_zscal((int)size,
                            (const void *)scalar_c64,
                            (double _Complex *)bytes,
                            1);
                break;
            default:
                /* Unsupported element type */
                return false;
        }
    }
    return true;
}

/**
 * @brief Extracts a specific complex component (real, imaginary, magnitude, or argument)
 *        from a DependentVariable’s data, replacing each value accordingly.
 *
 * @param dv               The DependentVariable to modify.
 * @param componentIndex   Index of the component to operate on (0-based).  
 *                         If negative, the operation applies to all components in sequence.
 * @param part             Which part of each element to retain:
 *                         - kSIRealPart      : keep real part, zero imaginary  
 *                         - kSIImaginaryPart : keep imaginary part, zero real  
 *                         - kSIMagnitudePart : replace with magnitude (abs)  
 *                         - kSIArgumentPart  : replace with argument (phase)  
 *
 * @return true if the data was successfully transformed; false if inputs are invalid
 *         or the variable’s numeric type does not support the requested component.
 *
 * @ingroup DependentVariable
 *
 * @code
 * // Convert component 1 of myDV to its magnitude values:
 * bool ok = DependentVariableTakeComplexPart(myDV, 1, kSIMagnitudePart);
 * @endcode
 */
bool
DependentVariableTakeComplexPart(DependentVariableRef dv,
                                 OCIndex componentIndex,
                                 complexPart part)
{
    if (!dv) return false;

    OCArrayRef comps = dv->components;
    OCIndex nComps = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 ||
        componentIndex < 0 ||
        componentIndex >= nComps)
    {
        return false;
    }

    OCIndex size = DependentVariableGetSize(dv);
    OCRange fullRange = { .location = 0, .length = size };

    switch (dv->numericType) {

      case kOCNumberFloat32Type:
      case kOCNumberFloat64Type:
        switch (part) {
          case kSIRealPart:
            return true;
          case kSIImaginaryPart:
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIRealPart);
            break;
          case kSIMagnitudePart:
            DependentVariableTakeAbsoluteValue(dv, componentIndex);
            break;
          case kSIArgumentPart:
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIRealPart);
            break;
        }
        break;

      case kOCNumberComplex64Type:
        switch (part) {
          case kSIRealPart:
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
            break;
          case kSIImaginaryPart:
            DependentVariableMultiplyValuesByDimensionlessComplexConstant(dv, componentIndex, -I);
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
            break;
          case kSIMagnitudePart:
            DependentVariableTakeAbsoluteValue(dv, componentIndex);
            break;
          case kSIArgumentPart: {
            OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, componentIndex);
            float complex *buf = (float complex*)OCDataGetMutableBytes(data);
            for (OCIndex i = 0; i < size; ++i) {
                buf[i] = cargf(buf[i]);
            }
            break;
          }
        }
        break;

      case kOCNumberComplex128Type:
        switch (part) {
          case kSIRealPart:
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
            break;
          case kSIImaginaryPart:
            DependentVariableMultiplyValuesByDimensionlessComplexConstant(dv, componentIndex, -I);
            DependentVariableZeroPartInRange(dv, componentIndex, fullRange, kSIImaginaryPart);
            break;
          case kSIMagnitudePart:
            DependentVariableTakeAbsoluteValue(dv, componentIndex);
            break;
          case kSIArgumentPart: {
            OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, componentIndex);
            double complex *buf = (double complex*)OCDataGetMutableBytes(data);
            for (OCIndex i = 0; i < size; ++i) {
                buf[i] = carg(buf[i]);
            }
            break;
          }
        }
        break;

      default:
        // integer or unsupported types
        return false;
    }

    if (componentIndex < 0) {
        if (dv->numericType == kOCNumberComplex64Type) {
            DependentVariableSetElementType(dv, kOCNumberFloat32Type);
        }
        else if (dv->numericType == kOCNumberComplex128Type) {
            DependentVariableSetElementType(dv, kOCNumberFloat64Type);
        }
    }

    return true;
}

bool
DependentVariableConjugate(DependentVariableRef dv,
                           OCIndex            componentIndex)
{
    if (!dv) return false;

    OCArrayRef comps   = dv->components;
    OCIndex   nComps   = comps ? OCArrayGetCount(comps) : 0;
    if (nComps == 0 || componentIndex >= nComps) {
        return false;
    }

    size_t totalSize = DependentVariableGetSize(dv);

    OCIndex lo = componentIndex >= 0 ? componentIndex : 0;
    OCIndex hi = componentIndex >= 0 ? componentIndex + 1  : nComps;

    for (OCIndex ci = lo; ci < hi; ++ci) {
        OCMutableDataRef data = (OCMutableDataRef)OCArrayGetValueAtIndex(comps, ci);

        switch (dv->numericType) {
            case kOCNumberFloat32Type:
            case kOCNumberFloat64Type:
                // real‐only, nothing to do
                break;

            case kOCNumberComplex64Type: {
                // interleaved [r0,i0, r1,i1, …]
                float complex *buf = (float complex*)CFDataGetMutableByte(data);
                float *imag = ((float*)buf) + 1;
                // negate each imaginary entry:
                //      imag[j] = -imag[j],  j=0..totalSize-1, stride=2
                cblas_sscal((int)totalSize, -1.0f, imag, 2);
                break;
            }

            case kOCNumberComplex128Type: {
                double complex *buf = (double complex*)CFDataGetMutableByte(data);
                double *imag = ((double*)buf) + 1;
                cblas_dscal((int)totalSize, -1.0, imag, 2);
                break;
            }

            default:
                return false;  // integers & others not supported
        }
    }

    return true;
}

#pragma endregion Conversion and Manipulation