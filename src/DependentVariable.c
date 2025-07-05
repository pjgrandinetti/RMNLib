/* DependentVariable OCType implementation */
#include "DependentVariable.h"
#define DependentVariableComponentsFileName STR("dependent_variable-%ld.data")
#pragma mark - Type Registration
static OCTypeID kDependentVariableID = kOCNotATypeID;
struct impl_DependentVariable {
    OCBase base;
    // SIQuantity Type attributes
    SIUnitRef unit;
    SINumberType numericType;
    // Dependent Variable Type attributes
    OCStringRef name;
    OCStringRef description;
    OCMutableDictionaryRef metaData;
    OCStringRef quantityName;
    OCStringRef quantityType;
    // components...
    OCStringRef type;           // "internal" / "external"
    OCStringRef encoding;       // "base64" / "none" -- used for type = "internal" storage
    OCStringRef componentsURL;  // needed for serialization (external-only)
    OCMutableArrayRef components;
    OCMutableArrayRef componentLabels;
    OCIndexSetRef sparseDimensionIndexes;
    OCMutableArrayRef sparseGridVertexes;
    // weak back-pointer
    OCTypeRef owner;
};
OCTypeID DependentVariableGetTypeID(void) {
    if (kDependentVariableID == kOCNotATypeID)
        kDependentVariableID = OCRegisterType("DependentVariable");
    return kDependentVariableID;
}
#pragma mark - Lifecycle
// Finalizer: release all *owned* references (but not the weak owner)
static void impl_DependentVariableFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_DependentVariable *dv = (struct impl_DependentVariable *)ptr;
    // Dependent-Variable fields
    // string fields
    OCRelease(dv->name);
    OCRelease(dv->type);
    OCRelease(dv->encoding);
    OCRelease(dv->description);
    OCRelease(dv->quantityName);
    OCRelease(dv->quantityType);
    OCRelease(dv->sparseGridVertexes);
    // collection fields
    OCRelease(dv->components);
    OCRelease(dv->componentLabels);
    OCRelease(dv->sparseDimensionIndexes);
    OCRelease(dv->metaData);
    OCRelease(dv->componentsURL);
    // NOTE: dv->owner is a weak back-pointer—do NOT OCRetain or OCRelease it
}
// Equality: compare all significant (owned) fields, but not the weak owner pointer
static bool impl_DependentVariableEqual(const void *a, const void *b) {
    const struct impl_DependentVariable *dvA = a;
    const struct impl_DependentVariable *dvB = b;
    if (!OCTypeEqual(dvA->componentsURL, dvB->componentsURL)) return false;
    if (!dvA || !dvB) return false;
    if (dvA == dvB) return true;
    // 1) compare the SI‐quantity fields
    if (!OCTypeEqual(dvA->unit, dvB->unit)) return false;
    if (dvA->numericType != dvB->numericType) return false;
    // 2) compare all DependentVariable attributes
    if (!OCTypeEqual(dvA->type, dvB->type)) return false;
    if (!OCTypeEqual(dvA->encoding, dvB->encoding)) return false;
    if (!OCTypeEqual(dvA->name, dvB->name)) return false;
    if (!OCTypeEqual(dvA->components, dvB->components)) return false;
    if (!OCTypeEqual(dvA->componentLabels, dvB->componentLabels)) return false;
    if (!OCTypeEqual(dvA->quantityName, dvB->quantityName)) return false;
    if (!OCTypeEqual(dvA->quantityType, dvB->quantityType)) return false;
    if (!OCTypeEqual(dvA->description, dvB->description)) return false;
    if (!OCTypeEqual(dvA->sparseDimensionIndexes, dvB->sparseDimensionIndexes)) return false;
    if (!OCTypeEqual(dvA->sparseGridVertexes, dvB->sparseGridVertexes)) return false;
    if (!OCTypeEqual(dvA->metaData, dvB->metaData)) return false;
    // 3) do NOT compare dvA->owner (weak pointer)
    return true;
}
// Formatting: produce a human-readable description
static OCStringRef impl_DependentVariableCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_DependentVariable *dv = (struct impl_DependentVariable *)cf;
    return OCStringCreateWithFormat(
        STR("<DependentVariable name=\"%@\" components=%lu>"),
        dv->name,
        (unsigned long)OCArrayGetCount(dv->components));
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
// Deep-copy (immutable) via dictionary round-trip
static void *impl_DependentVariableDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    // 1) Serialize the original into a dictionary
    OCDictionaryRef dict = DependentVariableCopyAsDictionary((DependentVariableRef)ptr);
    if (!dict) return NULL;
    // 2) Create a fresh object from that dictionary
    DependentVariableRef copy = DependentVariableCreateFromDictionary(dict, NULL);
    // 3) Clean up
    OCRelease(dict);
    return copy;
}
// Allocate only — do *not* initialize any fields here
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
// internal helper
static void impl_InitDependentVariableFields(DependentVariableRef dv) {
    dv->unit = SIUnitDimensionlessAndUnderived();
    dv->numericType = kSINumberFloat64Type;
    dv->name = STR("");
    dv->type = STR("internal");
    dv->encoding = STR("none");
    dv->components = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    dv->componentLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    dv->quantityName = STR("");
    dv->quantityType = STR("");
    dv->description = STR("");
    dv->sparseDimensionIndexes = OCIndexSetCreateMutable();
    dv->sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    dv->metaData = OCDictionaryCreateMutable(0);
    dv->componentsURL = NULL;  // start out empty
    dv->owner = NULL;
}
static bool validateDependentVariableParameters(
    OCStringRef type,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCArrayRef componentLabels,
    OCIndex componentsCount) {
    // — 0) type must be either "internal" or "external" —
    if (!type ||
        (!OCStringEqual(type, STR("internal")) &&
         !OCStringEqual(type, STR("external")))) {
        return false;
    }
    // — 1) components‐vs‐labels check —
    if (componentLabels) {
        OCIndex labelCount = OCArrayGetCount(componentLabels);
        if (labelCount != componentsCount) return false;
        for (OCIndex i = 0; i < labelCount; i++) {
            const void *obj = OCArrayGetValueAtIndex(componentLabels, i);
            if (OCGetTypeID(obj) != OCStringGetTypeID()) {
                return false;
            }
        }
    }
    // — 2) quantityName vs unit dimensionality —
    if (quantityName) {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim =
            SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim) {
            if (err) OCRelease(err);
            return false;
        }
        SIDimensionalityRef unitDim =
            SIUnitGetDimensionality(unit);
        bool match = SIDimensionalityHasSameReducedDimensionality(
            nameDim, unitDim);
        OCRelease(err);
        if (!match) return false;
    }
    // — 3) quantityType semantics —
    const char *qt = OCStringGetCString(quantityType);
    size_t len = qt ? strlen(qt) : 0;
    // “scalar” → exactly 1 component
    if (len == 6 && strcmp(qt, "scalar") == 0) {
        if (componentsCount != 1) return false;
    }
    // “pixel_N”
    else if (len > 6 && strncmp(qt, "pixel_", 6) == 0) {
        long n = 0;
        if (sscanf(qt + 6, "%ld", &n) != 1 || n != componentsCount)
            return false;
    }
    // “vector_N”
    else if (len > 7 && strncmp(qt, "vector_", 7) == 0) {
        long n = 0;
        if (sscanf(qt + 7, "%ld", &n) != 1 || n != componentsCount)
            return false;
    }
    // “matrix_R_C”
    else if (len > 8 && strncmp(qt, "matrix_", 7) == 0) {
        long r = 0, c = 0;
        if (sscanf(qt + 7, "%ld_%ld", &r, &c) != 2 || r * c != componentsCount)
            return false;
    }
    // “symmetric_matrix_N”
    else if (len > 17 && strncmp(qt, "symmetric_matrix_", 17) == 0) {
        long n = 0;
        if (sscanf(qt + 17, "%ld", &n) != 1 ||
            n * (n + 1) / 2 != componentsCount)
            return false;
    }
    return true;
}
static DependentVariableRef impl_DependentVariableCreate(
    OCStringRef type,            // "internal" or "external"
    OCStringRef name,            // optional name
    OCStringRef description,     // optional description
    SIUnitRef unit,              // optional SIUnitRef, defaults to dimensionless
    OCStringRef quantityName,    // optional quantity name, defaults to SIUnitGuessQuantityName(unit)
    OCStringRef quantityType,    // optional quantity type, defaults to "scalar"
    SINumberType elementType,    // SINumberType, defaults to kSINumberFloat64Type
    OCArrayRef componentLabels,  // optional array of component labels, defaults to "component-0", "component-1", etc.
    OCArrayRef components,       // array of OCDataRef components, all must have the same length
    bool copyComponents,         // if true, copy the components, otherwise retain them
    OCIndex explicitSize,        // size of each component in bytes, only used if components is NULL
    OCStringRef *outError) {
    // 1) figure out how many components, validate OCData types & lengths
    OCIndex componentsCount = 0;
    if (components) {
        componentsCount = OCArrayGetCount(components);
        for (OCIndex i = 0; i < componentsCount; i++) {
            if (OCGetTypeID(OCArrayGetValueAtIndex(components, i)) != OCDataGetTypeID()) {
                if (outError) {
                    *outError = OCStringCreateWithCString(
                        "component at index does not contain OCDataRef");
                }
                return NULL;
            }
        }
        if (componentsCount > 1) {
            uint64_t len0 = OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, 0));
            for (OCIndex i = 1; i < componentsCount; i++) {
                uint64_t leni = OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, i));
                if (leni != len0) {
                    if (outError) {
                        *outError = OCStringCreateWithCString(
                            "component buffers have mismatched lengths");
                    }
                    return NULL;
                }
            }
        }
    } else {
        componentsCount = DependentVariableComponentsCountFromQuantityType(quantityType);
        if (componentsCount == kOCNotFound) {
            if (outError) {
                // assuming OCStringCreateWithFormat exists to embed the bad quantityType
                *outError = OCStringCreateWithFormat(
                    "cannot determine components count for quantityType \"%@\"",
                    quantityType);
            }
            return NULL;
        }
    }
    // 2) semantic validation — now uses the passed‐in `type`
    if (!validateDependentVariableParameters(
            type,
            unit,
            quantityName,
            quantityType,
            componentLabels,
            componentsCount)) {
        return NULL;
    }
    // 3) allocate & init all defaults in one go
    struct impl_DependentVariable *dv = DependentVariableAllocate();
    if (!dv) return NULL;
    impl_InitDependentVariableFields(dv);
    // 3a) set the `type` ivar
    OCRelease(dv->type);
    dv->type = OCStringCreateCopy(type);
    // 3b) set the new `encoding` ivar (only for internal)
    OCRelease(dv->encoding);
    if (OCStringEqual(type, STR("internal"))) {
        dv->encoding = STR("none");
    } else {
        dv->encoding = NULL;
    }
    // 4) now populate every other field exactly once
    dv->numericType = elementType;
    dv->unit = unit ? unit : SIUnitDimensionlessAndUnderived();
    dv->quantityType = quantityType ? OCRetain(quantityType) : STR("");
    dv->name = name ? OCStringCreateCopy(name) : STR("");
    dv->description = description ? OCStringCreateCopy(description) : STR("");
    dv->quantityName = quantityName ? OCRetain(quantityName) : OCStringCreateCopy(SIUnitGuessQuantityName(dv->unit));
    // 5) build the components array
    OCRelease(dv->components);
    dv->components = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (!dv->components) {
        OCRelease(dv);
        return NULL;
    }
    if (components) {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(components, i);
            if (copyComponents) {
                OCMutableDataRef copy = OCDataCreateMutableCopy(0, blob);
                OCArrayAppendValue(dv->components, copy);
                OCRelease(copy);
            } else {
                OCArrayAppendValue(dv->components, blob);
            }
        }
    } else {
        // —— new explicitSize validity check ——
        if (explicitSize <= 0) {
            if (outError) {
                *outError = OCStringCreateWithCString(
                    "invalid explicitSize: must be > 0 when constructing from size");
            }
            OCRelease(dv);
            return NULL;
        }
        // now safe to allocate byteLen
        size_t byteLen = explicitSize * OCNumberTypeSize((OCNumberType)elementType);
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCMutableDataRef buf = OCDataCreateMutable(0);
            OCDataSetLength(buf, byteLen);
            OCArrayAppendValue(dv->components, buf);
            OCRelease(buf);
        }
    }
    // 6) build the componentLabels array
    OCRelease(dv->componentLabels);
    dv->componentLabels = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (!dv->componentLabels) {
        OCRelease(dv);
        return NULL;
    }
    if (componentLabels) {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCStringRef lblCopy = OCStringCreateCopy(
                (OCStringRef)OCArrayGetValueAtIndex(componentLabels, i));
            OCArrayAppendValue(dv->componentLabels, lblCopy);
            OCRelease(lblCopy);
        }
    } else {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCStringRef autoLbl = OCStringCreateWithFormat(STR("component-%ld"), (long)i);
            OCArrayAppendValue(dv->componentLabels, autoLbl);
            OCRelease(autoLbl);
        }
    }
    return (DependentVariableRef)dv;
}
// ————————————————————————————————————————————————————————————————————————
// now the public APIs
DependentVariableRef DependentVariableCreate(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            = */ STR("internal"),
        /* name            = */ name,
        /* description     = */ description,
        /* unit            = */ unit,
        /* quantityName    = */ quantityName,
        /* quantityType    = */ quantityType,
        /* elementType     = */ elementType,
        /* componentLabels = */ componentLabels,
        /* components      = */ components,
        /* copyComponents  = */ true,
        /* explicitSize    = */ (OCIndex)-1,
        /* outError        = */ outError);
}
DependentVariableRef DependentVariableCreateWithComponentsNoCopy(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            = */ STR("internal"),
        /* name            = */ name,
        /* description     = */ description,
        /* unit            = */ unit,
        /* quantityName    = */ quantityName,
        /* quantityType    = */ quantityType,
        /* elementType     = */ elementType,
        /* componentLabels = */ componentLabels,
        /* components      = */ components,
        /* copyComponents  = */ false,
        /* explicitSize    = */ (OCIndex)-1,
        /* outError        = */ outError);
}
DependentVariableRef DependentVariableCreateWithSize(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCIndex size,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            = */ STR("internal"),
        /* name            = */ name,
        /* description     = */ description,
        /* unit            = */ unit,
        /* quantityName    = */ quantityName,
        /* quantityType    = */ quantityType,
        /* elementType     = */ elementType,
        /* componentLabels = */ componentLabels,
        /* components      = */ NULL,
        /* copyComponents  = */ false,
        /* explicitSize    = */ size,
        /* outError        = */ outError);
}
DependentVariableRef DependentVariableCreateDefault(
    OCStringRef quantityType,
    SINumberType elementType,
    OCIndex size,
    OCStringRef *outError) {
    return impl_DependentVariableCreate(
        /* type            = */ STR("internal"),
        /* name            = */ NULL,
        /* description     = */ NULL,
        /* unit            = */ NULL,
        /* quantityName    = */ NULL,
        /* quantityType    = */ quantityType,
        /* elementType     = */ elementType,
        /* componentLabels = */ NULL,
        /* components      = */ NULL,
        /* copyComponents  = */ false,
        /* explicitSize    = */ size,
        /* outError        = */ outError);
}
DependentVariableRef DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCStringRef *outError) {
    OCMutableArrayRef arr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(arr, component);
    DependentVariableRef dv = impl_DependentVariableCreate(
        /* type            = */ STR("internal"),
        /* name            = */ name,
        /* description     = */ description,
        /* unit            = */ unit,
        /* quantityName    = */ quantityName,
        /* quantityType    = */ STR("scalar"),
        /* elementType     = */ elementType,
        /* componentLabels = */ componentLabels,
        /* components      = */ arr,
        /* copyComponents  = */ true,
        /* explicitSize    = */ (OCIndex)-1,
        /* outError        = */ outError);
    OCRelease(arr);
    return dv;
}
DependentVariableRef DependentVariableCreateCopy(DependentVariableRef src) {
    if (!src) return NULL;
    // 1) Serialize the source into a dictionary (captures type, encoding, URL or blobs, etc.)
    OCDictionaryRef dict = DependentVariableCopyAsDictionary(src);
    if (!dict) return NULL;
    // 2) Rehydrate a new object from that dictionary
    DependentVariableRef copy = DependentVariableCreateFromDictionary(dict, NULL);
    // 3) Clean up
    OCRelease(dict);
    return copy;
}
DependentVariableRef DependentVariableCreateComplexCopy(DependentVariableRef src,
                                                        OCTypeRef owner) {
    DependentVariableRef dv = DependentVariableCreateCopy(src);
    if (dv && !SIQuantityIsComplexType((SIQuantityRef)dv)) {
        SINumberType newType = (dv->numericType == kSINumberFloat32Type
                                    ? kSINumberFloat32ComplexType
                                    : kSINumberFloat64ComplexType);
        DependentVariableSetElementType(dv, newType);
    }
    return dv;
}
OCDictionaryRef DependentVariableCopyAsDictionary(DependentVariableRef dv) {
    if (!dv) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) return NULL;
    // 1) type (external vs. internal)
    OCDictionarySetValue(dict, STR("type"), OCStringCreateCopy(dv->type ?: STR("internal")));
    // 1a) if external, record the URL hint—but we STILL embed the raw blobs below
    if (OCStringEqual(dv->type, STR("external")) && dv->componentsURL) {
        OCStringRef urlCopy = OCStringCreateCopy(dv->componentsURL);
        OCDictionarySetValue(dict, STR("components_url"), urlCopy);
        OCRelease(urlCopy);
    }
    // 2) encoding (base64 or none)
    {
        OCStringRef enc = dv->encoding ?: STR("none");
        OCStringRef encCopy = OCStringCreateCopy(enc);
        OCDictionarySetValue(dict, STR("encoding"), encCopy);
        OCRelease(encCopy);
    }
    // 3) components (always embed raw data for round-trip)
    {
        SINumberType et = DependentVariableGetElementType(dv);
        bool isBase64 = dv->encoding && OCStringEqual(dv->encoding, STR("base64"));
        bool isComplex = (et == kSINumberFloat32ComplexType || et == kSINumberFloat64ComplexType);
        OCIndex ncomps = DependentVariableGetComponentCount(dv);
        OCMutableArrayRef compsArr = OCArrayCreateMutable(ncomps, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < ncomps; ++i) {
            OCDataRef blob = DependentVariableGetComponentAtIndex(dv, i);
            if (isBase64) {
                OCStringRef b64 = OCDataCreateBase64EncodedString(blob, OCBase64EncodingOptionsNone);
                OCArrayAppendValue(compsArr, b64);
                OCRelease(b64);
            } else {
                const void *bytes = OCDataGetBytesPtr(blob);
                size_t stride = OCNumberTypeSize((OCNumberType)et);
                OCIndex count = (OCIndex)(OCDataGetLength(blob) / stride);
                OCMutableArrayRef numArr =
                    OCArrayCreateMutable(isComplex ? count * 2 : count, &kOCTypeArrayCallBacks);
                if (!isComplex) {
                    if (et == kSINumberFloat32Type) {
                        float *arr = (float *)bytes;
                        for (OCIndex j = 0; j < count; ++j) {
                            OCNumberRef num = OCNumberCreateWithDouble((double)arr[j]);
                            OCArrayAppendValue(numArr, num);
                            OCRelease(num);
                        }
                    } else {
                        double *arr = (double *)bytes;
                        for (OCIndex j = 0; j < count; ++j) {
                            OCNumberRef num = OCNumberCreateWithDouble(arr[j]);
                            OCArrayAppendValue(numArr, num);
                            OCRelease(num);
                        }
                    }
                } else {
                    if (et == kSINumberFloat32ComplexType) {
                        float complex *arr = (float complex *)bytes;
                        for (OCIndex j = 0; j < count; ++j) {
                            OCNumberRef re = OCNumberCreateWithDouble((double)crealf(arr[j]));
                            OCNumberRef im = OCNumberCreateWithDouble((double)cimagf(arr[j]));
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
        OCDictionarySetValue(dict, STR("components"), compsArr);
        OCRelease(compsArr);
    }
    // 4) name, description
    if (dv->name) {
        OCStringRef c = OCStringCreateCopy(dv->name);
        OCDictionarySetValue(dict, STR("name"), c);
        OCRelease(c);
    }
    if (dv->description) {
        OCStringRef c = OCStringCreateCopy(dv->description);
        OCDictionarySetValue(dict, STR("description"), c);
        OCRelease(c);
    }
    // 5) quantity_name, quantity_type, unit, numeric_type
    if (dv->quantityName) {
        OCStringRef c = OCStringCreateCopy(dv->quantityName);
        OCDictionarySetValue(dict, STR("quantity_name"), c);
        OCRelease(c);
    }
    if (dv->quantityType) {
        OCStringRef c = OCStringCreateCopy(dv->quantityType);
        OCDictionarySetValue(dict, STR("quantity_type"), c);
        OCRelease(c);
    }
    if (dv->unit) {
        OCStringRef s = SIUnitCopySymbol(dv->unit);
        OCDictionarySetValue(dict, STR("unit"), s);
        OCRelease(s);
    }
    {
        OCNumberRef num = OCNumberCreateWithInt((int)DependentVariableGetElementType(dv));
        OCDictionarySetValue(dict, STR("numeric_type"), num);
        OCRelease(num);
    }
    // 6) component_labels
    {
        OCIndex nlab = DependentVariableGetComponentCount(dv);
        OCMutableArrayRef lbls = OCArrayCreateMutable(nlab, &kOCTypeArrayCallBacks);
        for (OCIndex i = 0; i < nlab; ++i) {
            OCStringRef lbl = DependentVariableGetComponentLabelAtIndex(dv, i);
            if (lbl) OCArrayAppendValue(lbls, lbl);
        }
        OCDictionarySetValue(dict, STR("component_labels"), lbls);
        OCRelease(lbls);
    }
    // 7) sparse_sampling
    if (dv->sparseDimensionIndexes || dv->sparseGridVertexes) {
        OCMutableDictionaryRef sp = OCDictionaryCreateMutable(0);
        if (dv->sparseDimensionIndexes) {
            OCArrayRef idxArr = OCIndexSetCreateOCNumberArray(dv->sparseDimensionIndexes);
            if (idxArr) {
                OCDictionarySetValue(sp, STR("dimension_indexes"), idxArr);
                OCRelease(idxArr);
            }
        }
        if (dv->sparseGridVertexes) {
            OCDictionarySetValue(sp, STR("sparse_grid_vertexes"), dv->sparseGridVertexes);
        }
        OCDictionarySetValue(dict, STR("sparse_sampling"), sp);
        OCRelease(sp);
    }
    // 8) metadata
    if (dv->metaData) {
        OCMutableDictionaryRef mdCopy = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(dv->metaData);
        OCDictionarySetValue(dict, STR("metadata"), mdCopy);
        OCRelease(mdCopy);
    }
    return (OCDictionaryRef)dict;
}
DependentVariableRef DependentVariableCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!dict) {
        if (outError) *outError = STR("input dictionary is NULL");
        return NULL;
    }
    // 1) start with a fresh DV and init defaults
    DependentVariableRef dv = DependentVariableAllocate();
    if (!dv) {
        if (outError) *outError = STR("allocation failed");
        return NULL;
    }
    impl_InitDependentVariableFields(dv);
    // 2) type discriminator
    OCStringRef typeStr = OCDictionaryGetValue(dict, STR("type"));
    if (!typeStr) {
        if (outError) *outError = STR("missing \"type\"");
        OCRelease(dv);
        return NULL;
    }
    bool isInternal = OCStringEqual(typeStr, STR("internal"));
    bool isExternal = OCStringEqual(typeStr, STR("external"));
    if (!isInternal && !isExternal) {
        if (outError) *outError = OCStringCreateWithFormat(STR("unknown \"type\": %@"), typeStr);
        OCRelease(dv);
        return NULL;
    }
    DependentVariableSetType(dv, typeStr);
    // 3) optional name / description
    OCStringRef tmp;
    if ((tmp = OCDictionaryGetValue(dict, STR("name"))))
        DependentVariableSetName(dv, tmp);
    if ((tmp = OCDictionaryGetValue(dict, STR("description"))))
        DependentVariableSetDescription(dv, tmp);
    // 4) quantity_name (opt) & quantity_type (req)
    if ((tmp = OCDictionaryGetValue(dict, STR("quantity_name"))))
        DependentVariableSetQuantityName(dv, tmp);
    OCStringRef quantityType = OCDictionaryGetValue(dict, STR("quantity_type"));
    if (!quantityType) {
        if (outError) *outError = STR("missing \"quantity_type\"");
        OCRelease(dv);
        return NULL;
    }
    if (!DependentVariableSetQuantityType(dv, quantityType)) {
        if (outError) *outError = OCStringCreateWithFormat(STR("invalid \"quantity_type\": %@"), quantityType);
        OCRelease(dv);
        return NULL;
    }
    // 5) unit (opt)
    if ((tmp = OCDictionaryGetValue(dict, STR("unit")))) {
        double mult = 1;
        OCStringRef uerr = NULL;
        SIUnitRef u = SIUnitFromExpression(tmp, &mult, &uerr);
        if (uerr) OCRelease(uerr);
        dv->unit = u ? u : SIUnitDimensionlessAndUnderived();
    }
    // 6) numeric_type -> elementType (req)
    OCNumberRef numType = (OCNumberRef)OCDictionaryGetValue(dict, STR("numeric_type"));
    if (!numType ||
        !OCNumberGetValue(numType, kOCNumberIntType, &(int){0})) {
        if (outError) *outError = STR("missing or invalid \"numeric_type\"");
        OCRelease(dv);
        return NULL;
    }
    {
        int it;
        OCNumberGetValue(numType, kOCNumberIntType, &it);
        DependentVariableSetElementType(dv, (SINumberType)it);
    }
    // 7) encoding (for internal only)
    bool isBase64 = false;
    if (isInternal) {
        OCStringRef enc = (OCStringRef)OCDictionaryGetValue(dict, STR("encoding"));
        if (enc && OCStringEqual(enc, STR("base64"))) {
            isBase64 = true;
            DependentVariableSetEncoding(dv, STR("base64"));
        } else {
            DependentVariableSetEncoding(dv, STR("none"));
        }
    }
    // 8) If external, grab the URL hint—but do NOT skip raw components
    if (isExternal) {
        OCStringRef url = OCDictionaryGetValue(dict, STR("components_url"));
        if (url) {
            DependentVariableSetComponentsURL(dv, url);
        }
    }
    // 9) **always** parse the embedded "components" array
    OCArrayRef compArr = (OCArrayRef)OCDictionaryGetValue(dict, STR("components"));
    if (!compArr) {
        if (outError) *outError = STR("missing \"components\"");
        OCRelease(dv);
        return NULL;
    }
    OCIndex count = OCArrayGetCount(compArr);
    OCMutableArrayRef blobs = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    SINumberType et = DependentVariableGetElementType(dv);
    size_t stride = OCNumberTypeSize((OCNumberType)et);
    for (OCIndex i = 0; i < count; ++i) {
        if (isBase64) {
            // decode from b64 string
            OCStringRef b64 = OCArrayGetValueAtIndex(compArr, i);
            OCDataRef data = OCDataCreateFromBase64EncodedString(b64);
            OCArrayAppendValue(blobs, data);
            OCRelease(data);
        } else {
            // numeric array
            OCArrayRef numList = OCArrayGetValueAtIndex(compArr, i);
            OCIndex n = OCArrayGetCount(numList);
            size_t bytes = n * stride;
            OCMutableDataRef data = OCDataCreateMutable(bytes);
            uint8_t *buf = OCDataGetMutableBytes(data);
            for (OCIndex j = 0; j < n; ++j) {
                OCNumberRef num = OCArrayGetValueAtIndex(numList, j);
                double v = 0;
                OCNumberGetValue(num, kOCNumberFloat64Type, &v);
                memcpy(buf + j * stride, &v, stride);
            }
            OCArrayAppendValue(blobs, data);
            OCRelease(data);
        }
    }
    // hand off to the same logic that builds dv->components + labels:
    OCMutableArrayRef labelsCopy = OCArrayCreateMutable(count, &kOCTypeArrayCallBacks);
    OCArrayRef labelArr = (OCArrayRef)OCDictionaryGetValue(dict, STR("component_labels"));
    if (labelArr && OCArrayGetCount(labelArr) == count) {
        for (OCIndex i = 0; i < count; ++i) {
            OCArrayAppendValue(labelsCopy,
                               OCArrayGetValueAtIndex(labelArr, i));
        }
    }
    DependentVariableRef fresh =
        DependentVariableCreateWithComponentsNoCopy(
            dv->name, dv->description, dv->unit,
            dv->quantityName, dv->quantityType,
            et, labelsCopy, blobs, NULL);
    OCRelease(labelsCopy);
    OCRelease(blobs);
    // override the encoding we already set, just to be safe
    if (isInternal) {
        DependentVariableSetEncoding(fresh, isBase64 ? STR("base64") : STR("none"));
    }
    // carry over the components_url for externals
    if (isExternal && dv->componentsURL) {
        DependentVariableSetComponentsURL(fresh, dv->componentsURL);
    }
    OCRelease(dv);
    return fresh;
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
static OCDictionaryRef DependentVariableDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError) {
    if (outError) *outError = NULL;
    if (!json || !cJSON_IsObject(json)) {
        if (outError) *outError = STR("Expected top-level JSON object for DependentVariable");
        return NULL;
    }
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    cJSON *item = NULL;
    // Required: type
    item = cJSON_GetObjectItemCaseSensitive(json, "type");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"type\"");
        OCRelease(dict);
        return NULL;
    }
    OCDictionarySetValue(dict, STR("type"), OCStringCreateWithCString(item->valuestring));
    // Optional: components_url
    item = cJSON_GetObjectItemCaseSensitive(json, "components_url");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("components_url"), OCStringCreateWithCString(item->valuestring));
    }
    // Optional: encoding
    item = cJSON_GetObjectItemCaseSensitive(json, "encoding");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("encoding"), OCStringCreateWithCString(item->valuestring));
    }
    // Required: components
    item = cJSON_GetObjectItemCaseSensitive(json, "components");
    if (!cJSON_IsArray(item)) {
        if (outError) *outError = STR("Missing or invalid \"components\"");
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
    OCDictionarySetValue(dict, STR("components"), components);
    OCRelease(components);
    // Optional: name
    item = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("name"), OCStringCreateWithCString(item->valuestring));
    }
    // Optional: description
    item = cJSON_GetObjectItemCaseSensitive(json, "description");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("description"), OCStringCreateWithCString(item->valuestring));
    }
    // Optional: quantity_name
    item = cJSON_GetObjectItemCaseSensitive(json, "quantity_name");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("quantity_name"), OCStringCreateWithCString(item->valuestring));
    }
    // Required: quantity_type
    item = cJSON_GetObjectItemCaseSensitive(json, "quantity_type");
    if (!cJSON_IsString(item)) {
        if (outError) *outError = STR("Missing or invalid \"quantity_type\"");
        OCRelease(dict);
        return NULL;
    }
    OCDictionarySetValue(dict, STR("quantity_type"), OCStringCreateWithCString(item->valuestring));
    // Optional: unit
    item = cJSON_GetObjectItemCaseSensitive(json, "unit");
    if (cJSON_IsString(item)) {
        OCDictionarySetValue(dict, STR("unit"), OCStringCreateWithCString(item->valuestring));
    }
    // Required: numeric_type
    item = cJSON_GetObjectItemCaseSensitive(json, "numeric_type");
    if (!cJSON_IsNumber(item)) {
        if (outError) *outError = STR("Missing or invalid \"numeric_type\"");
        OCRelease(dict);
        return NULL;
    }
    OCNumberRef num = OCNumberCreateWithInt(item->valueint);
    OCDictionarySetValue(dict, STR("numeric_type"), num);
    OCRelease(num);
    // Optional: component_labels
    item = cJSON_GetObjectItemCaseSensitive(json, "component_labels");
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
        OCDictionarySetValue(dict, STR("component_labels"), labels);
        OCRelease(labels);
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
DependentVariableRef DependentVariableCreateCrossSection(DependentVariableRef dv,
                                                         OCArrayRef dimensions,
                                                         OCIndexPairSetRef indexPairs,
                                                         OCStringRef *outError) {
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
    SINumberType elemType = DependentVariableGetElementType(dv);
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
                case kSINumberFloat32Type:
                    ((float *)dstPtr)[0] = ((float *)srcPtr)[memOff];
                    break;
                case kSINumberFloat64Type:
                    ((double *)dstPtr)[0] = ((double *)srcPtr)[memOff];
                    break;
                case kSINumberFloat32ComplexType:
                    ((float complex *)dstPtr)[0] =
                        ((float complex *)srcPtr)[memOff];
                    break;
                case kSINumberFloat64ComplexType:
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
                    case kSINumberFloat32Type:
                        ((float *)dstPtr)[outOff] = ((float *)srcPtr)[memOff];
                        break;
                    case kSINumberFloat64Type:
                        ((double *)dstPtr)[outOff] = ((double *)srcPtr)[memOff];
                        break;
                    case kSINumberFloat32ComplexType:
                        ((float complex *)dstPtr)[outOff] =
                            ((float complex *)srcPtr)[memOff];
                        break;
                    case kSINumberFloat64ComplexType:
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
bool DependentVariableAppend(DependentVariableRef dv,
                             DependentVariableRef appendedDV,
                             OCStringRef *outError) {
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
    SINumberType et1 = DependentVariableGetElementType(dv);
    SINumberType et2 = DependentVariableGetElementType(appendedDV);
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
OCArrayRef DependentVariableCreatePackedSparseComponentsArray(DependentVariableRef dv,
                                                              OCArrayRef dimensions) {
    if (!dv || !dimensions) return NULL;
    OCIndexSetRef idxs = DependentVariableGetSparseDimensionIndexes(dv);
    OCArrayRef verts = DependentVariableGetSparseGridVertexes(dv);
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
OCDataRef DependentVariableCreateCSDMComponentsData(DependentVariableRef dv, OCArrayRef dimensions) {
    if (!dv) return NULL;
    // 1) Create an empty mutable data buffer
    OCMutableDataRef components = OCDataCreateMutable(0);
    if (!components) return NULL;
    // 2) How many components does the DV have?
    OCIndex nComps = DependentVariableGetComponentCount(dv);
    // 3) Do we have any sparse dims?
    OCIndexSetRef sparseIdxs = DependentVariableGetSparseDimensionIndexes(dv);
    if (sparseIdxs && OCIndexSetGetCount(sparseIdxs) > 0) {
        // pack the sparse components first
        OCArrayRef packed = DependentVariableCreatePackedSparseComponentsArray(dv, dimensions);
        if (packed) {
            OCIndex nPacked = OCArrayGetCount(packed);
            for (OCIndex i = 0; i < nPacked; ++i) {
                OCDataRef comp = (OCDataRef)OCArrayGetValueAtIndex(packed, i);
                const uint8_t *bytes = (const uint8_t *)OCDataGetBytesPtr(comp);
                uint64_t len = OCDataGetLength(comp);
                if (!OCDataAppendBytes(components, bytes, len)) {
                    OCRelease(packed);
                    OCRelease(components);
                    return NULL;
                }
            }
            OCRelease(packed);
        }
        return (OCDataRef)components;
    }
    // 4) Otherwise, just concatenate all raw components
    for (OCIndex i = 0; i < nComps; ++i) {
        OCDataRef comp = DependentVariableGetComponentAtIndex(dv, i);
        const uint8_t *bytes = (const uint8_t *)OCDataGetBytesPtr(comp);
        uint64_t len = OCDataGetLength(comp);
        if (!OCDataAppendBytes(components, bytes, len)) {
            OCRelease(components);
            return NULL;
        }
    }
    return (OCDataRef)components;
}
#pragma mark - Getters & Setters
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
// — Get number of components —
OCIndex
DependentVariableGetComponentCount(DependentVariableRef dv) {
    if (!dv) return 0;
    return OCArrayGetCount(dv->components);
}
// — Access the mutable components array directly —
OCMutableArrayRef
DependentVariableGetComponents(DependentVariableRef dv) {
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
            return false; // mismatched component sizes
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
// — Deep-copy all components (so callers get their own buffers) —
OCMutableArrayRef
DependentVariableCopyComponents(DependentVariableRef dv) {
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
// — Get a single component buffer (no copy) —
OCDataRef
DependentVariableGetComponentAtIndex(DependentVariableRef dv,
                                     OCIndex componentIndex) {
    if (!dv || !dv->components ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(dv->components))
        return NULL;
    return (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
}
// — Swap in a new component buffer (must be same length) —
bool DependentVariableSetComponentAtIndex(DependentVariableRef dv,
                                          OCDataRef newBuf,
                                          OCIndex componentIndex) {
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
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv,
                                             OCDataRef component,
                                             OCIndex idx) {
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
///
/// Remove the component (and its label) at index `idx`.
/// Returns false if out‐of‐range or only one component remains.
///
bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv,
                                             OCIndex idx) {
    if (!dv) return false;
    OCIndex count = OCArrayGetCount(dv->components);
    if (idx >= count || count <= 1)
        return false;
    OCArrayRemoveValueAtIndex(dv->components, idx);
    OCArrayRemoveValueAtIndex(dv->componentLabels, idx);
    updateForComponentCountChange(dv);
    return true;
}
/// Return the “length” (number of elements) of each component buffer
OCIndex
DependentVariableGetSize(DependentVariableRef dv) {
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
/// Resize *all* component buffers to exactly `size` elements,
/// zero‐filling any newly-appended tail.
bool DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize) {
    if (!dv) return false;
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (nComps == 0) return false;
    OCIndex oldSize = DependentVariableGetSize(dv);
    // pick up byte‐size per element
    size_t elemSize;
    switch (dv->numericType) {
        case kSINumberFloat32Type:
            elemSize = sizeof(float);
            break;
        case kSINumberFloat64Type:
            elemSize = sizeof(double);
            break;
        case kSINumberFloat32ComplexType:
            elemSize = 2 * sizeof(float);
            break;
        case kSINumberFloat64ComplexType:
            elemSize = 2 * sizeof(double);
            break;
        default:
            elemSize = 1;
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
// Getter for the “type” field (“internal” or “external”)
OCStringRef DependentVariableGetType(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->type;
}
bool DependentVariableShouldSerializeExternally(DependentVariableRef dv) {
    return dv && OCStringEqual(dv->type, STR("external"));
}

// Setter for the “type” field—returns true on success, false on failure.
// You may wish to further validate that `type` is exactly "internal" or "external".
bool DependentVariableSetType(DependentVariableRef dv, OCStringRef newType) {
    if (!dv || !newType) return false;
    // Copy the incoming string
    OCStringRef copy = OCStringCreateCopy(newType);
    if (!copy) return false;
    // Swap out the old value
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
OCStringRef
DependentVariableGetName(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // If the name field is NULL, return the interned empty string
    return dv->name ? dv->name : STR("");
}
// Setter
// Returns true on success, false on allocation failure or invalid dv.
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
// Getter for the description field
OCStringRef
DependentVariableGetDescription(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    // Return the stored description, or the empty interned string if NULL
    return dv->description ? dv->description : STR("");
}
// Setter for the description field
// Returns true on success (including no-op), false on allocation failure or invalid dv.
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
// Getter
OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv) {
    IF_NO_OBJECT_EXISTS_RETURN(dv, NULL);
    return dv->quantityType;
}
// Setter
// Returns true if quantityType was updated (or was already valid), false on mismatch.
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
OCMutableArrayRef
DependentVariableCreateQuantityTypesArray(DependentVariableRef dv) {
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
// — sparseDimensionIndexes —
OCIndexSetRef DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->sparseDimensionIndexes;
}
bool DependentVariableSetSparseDimensionIndexes(DependentVariableRef dv, OCIndexSetRef idxSet) {
    if (!dv) return false;
    OCRelease(dv->sparseDimensionIndexes);
    dv->sparseDimensionIndexes = idxSet ? OCIndexSetCreateMutableCopy(idxSet) : OCIndexSetCreateMutable();
    return dv->sparseDimensionIndexes != NULL;
}
// — sparseGridVertexes —
OCArrayRef DependentVariableGetSparseGridVertexes(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->sparseGridVertexes;
}
bool DependentVariableSetSparseGridVertexes(DependentVariableRef dv, OCArrayRef verts) {
    if (!dv) return false;
    OCRelease(dv->sparseGridVertexes);
    if (verts) {
        dv->sparseGridVertexes = OCArrayCreateMutableCopy(verts);
    } else {
        dv->sparseGridVertexes = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    }
    return dv->sparseGridVertexes != NULL;
}
OCStringRef
DependentVariableGetQuantityName(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->quantityName;
}
bool DependentVariableSetQuantityName(DependentVariableRef dv,
                                      OCStringRef quantityName) {
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
    if (!validateDependentVariableParameters(dv->type,
                                             dv->unit,
                                             quantityName,
                                             dv->quantityType,
                                             (OCArrayRef)dv->componentLabels,
                                             count)) {
        return false;
    }
    // 4) All good—swap in the new name
    OCRelease(dv->quantityName);
    dv->quantityName = OCStringCreateCopy(quantityName);
    return dv->quantityName != NULL;
}
//-----------------------------------------------------------------------------
// Getter for the componentLabels array.
// Returns the mutable array (as an OCArrayRef) or NULL if dv is NULL.
//-----------------------------------------------------------------------------
OCArrayRef
DependentVariableGetComponentLabels(DependentVariableRef dv) {
    if (!dv) return NULL;
    return (OCArrayRef)dv->componentLabels;
}
//-----------------------------------------------------------------------------
// Setter for the componentLabels array.
// Makes a deep mutable‐copy of the input array (or creates an empty one if NULL).
// Returns true on success, false on NULL dv or allocation failure.
//-----------------------------------------------------------------------------
bool DependentVariableSetComponentLabels(DependentVariableRef dv,
                                         OCArrayRef labels) {
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
OCStringRef
DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv,
                                              OCIndex componentIndex) {
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
//-----------------------------------------------------------------------------
// Direct getter for the componentLabels array at index.
//-----------------------------------------------------------------------------
OCStringRef
DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv,
                                          OCIndex componentIndex) {
    if (!dv) return NULL;
    OCArrayRef labels = (OCArrayRef)dv->componentLabels;
    if (!labels ||
        componentIndex < 0 ||
        componentIndex >= OCArrayGetCount(labels)) {
        return NULL;
    }
    return OCArrayGetValueAtIndex(labels, componentIndex);
}
//-----------------------------------------------------------------------------
// Replace the component label at the given index.
//-----------------------------------------------------------------------------
bool DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv,
                                               OCStringRef newLabel,
                                               OCIndex componentIndex) {
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
SINumberType
DependentVariableGetElementType(DependentVariableRef dv) {
    if (!dv) return kSINumberTypeInvalid;
    return dv->numericType;
}
//-----------------------------------------------------------------------------
// Change the element type of all components.
//   - Reinterprets each old value as the new type,
//     e.g. from float → double, float → complex, etc.
//   - Returns false if:
//       • dv is NULL
//       • there are no components
//       • newType == oldType (no work done)
//-----------------------------------------------------------------------------
bool DependentVariableSetElementType(DependentVariableRef dv,
                                     SINumberType newType) {
    if (!dv) return false;
    SINumberType oldType = dv->numericType;
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
                case kSINumberFloat32Type: {
                    float *src = (float *)oldPtr;
                    float f = src[ei];
                    switch (newType) {
                        case kSINumberFloat32Type:
                            ((float *)tmpBuf)[ei] = f;
                            break;
                        case kSINumberFloat64Type:
                            ((double *)tmpBuf)[ei] = f;
                            break;
                        case kSINumberFloat32ComplexType:
                            ((float complex *)tmpBuf)[ei] = f;
                            break;
                        case kSINumberFloat64ComplexType:
                            ((double complex *)tmpBuf)[ei] = f;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kSINumberFloat64Type: {
                    double *src = (double *)oldPtr;
                    double d = src[ei];
                    switch (newType) {
                        case kSINumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)d;
                            break;
                        case kSINumberFloat64Type:
                            ((double *)tmpBuf)[ei] = d;
                            break;
                        case kSINumberFloat32ComplexType:
                            ((float complex *)tmpBuf)[ei] = (float)d;
                            break;
                        case kSINumberFloat64ComplexType:
                            ((double complex *)tmpBuf)[ei] = d;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kSINumberFloat32ComplexType: {
                    float complex *src = (float complex *)oldPtr;
                    float complex z = src[ei];
                    switch (newType) {
                        case kSINumberFloat32Type:
                            ((float *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kSINumberFloat64Type:
                            ((double *)tmpBuf)[ei] = crealf(z);
                            break;
                        case kSINumberFloat32ComplexType:
                            ((float complex *)tmpBuf)[ei] = z;
                            break;
                        case kSINumberFloat64ComplexType:
                            ((double complex *)tmpBuf)[ei] = z;
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case kSINumberFloat64ComplexType: {
                    double complex *src = (double complex *)oldPtr;
                    double complex z = src[ei];
                    switch (newType) {
                        case kSINumberFloat32Type:
                            ((float *)tmpBuf)[ei] = (float)creal(z);
                            break;
                        case kSINumberFloat64Type:
                            ((double *)tmpBuf)[ei] = creal(z);
                            break;
                        case kSINumberFloat32ComplexType:
                            ((float complex *)tmpBuf)[ei] = (float)creal(z) + (float)cimag(z) * I;
                            break;
                        case kSINumberFloat64ComplexType:
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
bool DependentVariableSetValues(DependentVariableRef dv,
                                OCIndex componentIndex,
                                OCDataRef values) {
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
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN;
    }
    // wrap negative or out‐of‐bounds offsets
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return (float)arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            return (float)crealf(arr[memOffset]);
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            return (float)crealf(arr[memOffset]);
        }
        default:
            return NAN;
    }
}
double
DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv,
                                           OCIndex componentIndex,
                                           OCIndex memOffset) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN;
    }
    memOffset = memOffset % size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return (double)arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            return (double)crealf(arr[memOffset]);
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            return creal(arr[memOffset]);
        }
        default:
            return NAN;
    }
}
float complex
DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv,
                                                 OCIndex componentIndex,
                                                 OCIndex memOffset) {
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
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return (float complex)arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return (float complex)arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            return arr[memOffset];
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            return (float complex)arr[memOffset];
        }
        default:
            return NAN + NAN * I;
    }
}
double complex
DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv,
                                                  OCIndex componentIndex,
                                                  OCIndex memOffset) {
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
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return (double complex)arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return (double complex)arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            return (double complex)crealf(arr[memOffset]);
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            return arr[memOffset];
        }
        default:
            return NAN + NAN * I;
    }
}
double
DependentVariableGetDoubleValueAtMemOffsetForPart(DependentVariableRef dv,
                                                  OCIndex componentIndex,
                                                  OCIndex memOffset,
                                                  complexPart part) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN;
    }
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return (double)arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            float complex v = arr[memOffset];
            switch (part) {
                case kSIRealPart:
                    return crealf(v);
                case kSIImaginaryPart:
                    return cimagf(v);
                case kSIMagnitudePart:
                    return cabsf(v);
                case kSIArgumentPart:
                    return cargf(v);
            }
            break;
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            double complex v = arr[memOffset];
            switch (part) {
                case kSIRealPart:
                    return creal(v);
                case kSIImaginaryPart:
                    return cimag(v);
                case kSIMagnitudePart:
                    return cabs(v);
                case kSIArgumentPart:
                    return carg(v);
            }
            break;
        }
        default:
            break;
    }
    return NAN;
}
float DependentVariableGetFloatValueAtMemOffsetForPart(DependentVariableRef dv,
                                                       OCIndex componentIndex,
                                                       OCIndex memOffset,
                                                       complexPart part) {
    if (!dv) return NAN;
    OCIndex size = DependentVariableGetSize(dv);
    OCIndex nComps = OCArrayGetCount(dv->components);
    if (size == 0 || nComps == 0 ||
        componentIndex < 0 || componentIndex >= nComps) {
        return NAN;
    }
    memOffset %= size;
    if (memOffset < 0) memOffset += size;
    OCDataRef data = (OCDataRef)OCArrayGetValueAtIndex(dv->components, componentIndex);
    const void *bytes = OCDataGetBytesPtr(data);
    switch (dv->numericType) {
        case kSINumberFloat32Type: {
            float *arr = (float *)bytes;
            return arr[memOffset];
        }
        case kSINumberFloat64Type: {
            double *arr = (double *)bytes;
            return (float)arr[memOffset];
        }
        case kSINumberFloat32ComplexType: {
            float complex *arr = (float complex *)bytes;
            float complex v = arr[memOffset];
            switch (part) {
                case kSIRealPart:
                    return crealf(v);
                case kSIImaginaryPart:
                    return cimagf(v);
                case kSIMagnitudePart:
                    return cabsf(v);
                case kSIArgumentPart:
                    return cargf(v);
            }
            break;
        }
        case kSINumberFloat64ComplexType: {
            double complex *arr = (double complex *)bytes;
            double complex v = arr[memOffset];
            switch (part) {
                case kSIRealPart:
                    return (float)creal(v);
                case kSIImaginaryPart:
                    return (float)cimag(v);
                case kSIMagnitudePart:
                    return (float)cabs(v);
                case kSIArgumentPart:
                    return (float)carg(v);
            }
            break;
        }
        default:
            break;
    }
    return NAN;
}
SIScalarRef
DependentVariableCreateValueFromMemOffset(DependentVariableRef dv,
                                          OCIndex componentIndex,
                                          OCIndex memOffset) {
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
        case kSINumberFloat32Type: {
            float v = ((float *)bytes)[memOffset];
            return SIScalarCreateWithFloat(v, dv->unit);
        }
        case kSINumberFloat64Type: {
            double v = ((double *)bytes)[memOffset];
            return SIScalarCreateWithDouble(v, dv->unit);
        }
        case kSINumberFloat32ComplexType: {
            float complex v = ((float complex *)bytes)[memOffset];
            return SIScalarCreateWithFloatComplex(v, dv->unit);
        }
        case kSINumberFloat64ComplexType: {
            double complex v = ((double complex *)bytes)[memOffset];
            return SIScalarCreateWithDoubleComplex(v, dv->unit);
        }
        default:
            return NULL;
    }
}
// -----------------------------------------------------------------------------
// Overwrite the raw component‐array at memOffset with the given scalar.
// -----------------------------------------------------------------------------
bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv,
                                          OCIndex componentIndex,
                                          OCIndex memOffset,
                                          SIScalarRef value,
                                          OCStringRef *error) {
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
        case kSINumberFloat32Type: {
            float v = SIScalarFloatValueInUnit(value, dv->unit, NULL);
            ((float *)bytes)[memOffset] = v;
            break;
        }
        case kSINumberFloat64Type: {
            double v = SIScalarDoubleValueInUnit(value, dv->unit, NULL);
            ((double *)bytes)[memOffset] = v;
            break;
        }
        case kSINumberFloat32ComplexType: {
            float complex v = SIScalarFloatComplexValueInUnit(value, dv->unit, NULL);
            ((float complex *)bytes)[memOffset] = v;
            break;
        }
        case kSINumberFloat64ComplexType: {
            double complex v = SIScalarDoubleComplexValueInUnit(value, dv->unit, NULL);
            ((double complex *)bytes)[memOffset] = v;
            break;
        }
        default:
            return false;
    }
    return true;
}
