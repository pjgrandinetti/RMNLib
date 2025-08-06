/* DependentVariable OCType implementation */
#include "DependentVariable.h"
#include "DependentVariable_private.h"
#pragma region Type Registration
static OCTypeID kDependentVariableID = kOCNotATypeID;
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
static bool impl_DependentVariableComponentsAreEqual(const struct impl_DependentVariable *a,
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
    if (!impl_DependentVariableComponentsAreEqual(dvA, dvB)) return false;
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
bool validateDependentVariableParameters(
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
    OCNumberType numericType,    // was elementType
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
    dv->numericType = numericType;
    dv->unit = unit ? unit : SIUnitDimensionlessAndUnderived();
    OCRelease(dv->quantityName);
    dv->quantityName = quantityName ? OCStringCreateCopy(quantityName)
                                    : SIUnitCreateQuantityNameGuess(dv->unit);
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
            size_t eltSize = OCNumberTypeSize(numericType);
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
    OCNumberType numericType,
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
        /* numericType     */ numericType,
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
    OCNumberType numericType,
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
        /* numericType     */ numericType,
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
    OCNumberType numericType,
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
        /* numericType     */ numericType,
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
    OCNumberType numericType,
    OCIndex size,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type               */ STR(kDependentVariableComponentTypeValueInternal),
        /* name               */ NULL,
        /* description        */ NULL,
        /* unit               */ NULL,
        /* quantityName       */ NULL,
        /* quantityType       */ quantityType,
        /* numericType        */ numericType,
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
    OCNumberType numericType,
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
        /* numericType        */ numericType,
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
    OCNumberType numericType,
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
        /* numericType        */ numericType,
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
DependentVariableRef DependentVariableCopy(DependentVariableRef src) {
    return impl_DependentVariableDeepCopy(src);
}
DependentVariableRef DependentVariableCreateComplexCopy(DependentVariableRef src,
                                                        OCTypeRef owner) {
    if (!src) return NULL;
    // 1) Make a deep copy of the source
    DependentVariableRef dv = DependentVariableCopy(src);
    if (!dv) return NULL;
    // 2) Assign the owner (weak back-pointer)
    DependentVariableSetOwner(dv, owner);
    // 3) If it isn’t already a complex type, upgrade its element type
    if (!SIQuantityIsComplexType((SIQuantityRef)dv)) {
        OCNumberType base = DependentVariableGetNumericType(dv);
        OCNumberType complexType =
            (base == kOCNumberFloat32Type
                 ? kOCNumberComplex64Type
                 : kOCNumberComplex128Type);
        DependentVariableSetNumericType(dv, complexType);
    }
    return dv;
}
DependentVariableRef DependentVariableCreateMinimal(
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType numericType,
    OCArrayRef components,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type               */ STR(kDependentVariableComponentTypeValueInternal),
        /* name               */ NULL,  // minimal - no name
        /* description        */ NULL,  // minimal - no description
        /* unit               */ unit,
        /* quantityName       */ quantityName,
        /* quantityType       */ quantityType,
        /* numericType        */ numericType,
        /* encoding           */ NULL,  // default to base64
        /* componentsURL      */ NULL,  // not used for internal
        /* components         */ components,
        /* copyComponents     */ true,         // deep-copy blobs
        /* explicitSize       */ (OCIndex)-1,  // ignored when components != NULL
        /* componentLabels    */ NULL,         // minimal - auto-generate labels
        /* sparseSampling     */ NULL,         // minimal - no sparse sampling
        /* copySparseSampling */ true,         // deep-copy if present
        /* metaData           */ NULL,         // minimal - no metadata
        /* outError           */ outError);
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
        OCNumberType et = DependentVariableGetNumericType(dv);
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
        const char *typeName = OCNumberGetTypeName(DependentVariableGetNumericType(dv));
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
    // 5) parse numeric_type → numericType
    OCNumberType numericType = kOCNumberTypeInvalid;
    if (OCGetTypeID(numType) == OCStringGetTypeID()) {
        const char *typeName = OCStringGetCString(numType);
        numericType = OCNumberTypeFromName(typeName);
        if (numericType == kOCNumberTypeInvalid) {
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
                    switch (numericType) {
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
        /* numericType     */ numericType,
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
            OCStringRef sym = SIUnitCopySymbol(u);
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
