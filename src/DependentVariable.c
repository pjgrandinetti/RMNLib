/* DependentVariable OCType implementation */
#include "DependentVariable.h"
#define DependentVariableComponentsFileName STR("dependent_variable-%ld.data")
static OCTypeID kDependentVariableID = kOCNotATypeID;
struct impl_DependentVariable {
    OCBase base;
    // SIQuantity Type attributes
    SIUnitRef unit;
    SINumberType type;
    // Dependent Variable Type attributes
    OCStringRef name;
    OCStringRef description;
    OCMutableDictionaryRef metaData;
    OCStringRef quantityName;
    OCStringRef quantityType;
    OCMutableArrayRef components;
    OCMutableArrayRef componentLabels;
    OCIndexSetRef sparseDimensionIndexes;
    OCStringRef sparseGridVertexes;
    // weak reference to the dataset this variable belongs to.  Do not retain!
    OCTypeRef owner;
};
OCTypeID DependentVariableGetTypeID(void) {
    if (kDependentVariableID == kOCNotATypeID)
        kDependentVariableID = OCRegisterType("DependentVariable");
    return kDependentVariableID;
}
// Finalizer: release all *owned* references (but not the weak owner)
static void impl_DependentVariableFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_DependentVariable *dv = (struct impl_DependentVariable *)ptr;
    // Dependent-Variable fields
    OCRelease(dv->name);
    OCRelease(dv->components);
    OCRelease(dv->componentLabels);
    OCRelease(dv->quantityName);
    OCRelease(dv->quantityType);
    OCRelease(dv->description);
    OCRelease(dv->sparseDimensionIndexes);
    OCRelease(dv->sparseGridVertexes);
    OCRelease(dv->metaData);
    // NOTE: dv->owner is a weak back-pointer—do NOT OCRetain or OCRelease it
}
// Equality: compare all significant (owned) fields, but not the weak owner pointer
static bool impl_DependentVariableEqual(const void *a, const void *b) {
    const struct impl_DependentVariable *dvA = a;
    const struct impl_DependentVariable *dvB = b;
    if (!dvA || !dvB) return false;
    if (dvA == dvB) return true;
    // 1) compare the SI‐quantity fields
    if (!OCTypeEqual(dvA->unit, dvB->unit)) return false;
    if (dvA->type != dvB->type) return false;
    // 2) compare all DependentVariable attributes
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
// Deep-copy (immutable) via dictionary round-trip
static void *
impl_DependentVariableDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    // 1) Serialize the original into a dictionary
    OCDictionaryRef dict = DependentVariableCopyAsDictionary((DependentVariableRef)ptr);
    if (!dict) return NULL;
    // 2) Create a fresh object from that dictionary
    DependentVariableRef copy = DependentVariableCreateFromDictionary(dict);
    // 3) Clean up
    OCRelease(dict);
    return copy;
}
// Allocate & initialize defaults
static struct impl_DependentVariable *DependentVariableAllocate(void) {
    struct impl_DependentVariable *obj = OCTypeAlloc(
        struct impl_DependentVariable,
        DependentVariableGetTypeID(),
        impl_DependentVariableFinalize,
        impl_DependentVariableEqual,
        impl_DependentVariableCopyFormattingDesc,
        impl_DependentVariableDeepCopy,
        impl_DependentVariableDeepCopy);
    // — SIQuantity fields —
    // default to the unit‐less SI unit and a double‐precision scalar
    obj->unit = SIUnitDimensionlessAndUnderived();
    obj->type = kSINumberFloat64Type;
    // — DependentVariable fields —
    obj->name = STR("");
    obj->components = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    obj->componentLabels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    obj->quantityName = STR("");
    obj->quantityType = STR("");
    obj->description = STR("");
    obj->sparseDimensionIndexes = OCIndexSetCreateMutable();
    obj->sparseGridVertexes = STR("");
    obj->metaData = OCDictionaryCreateMutable(0);
    // weak back‐pointer to owner dataset
    obj->owner = NULL;
    return obj;
}
static bool validateDependentVariableParameters(
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCArrayRef componentLabels,
    OCIndex componentsCount) {
    // --- 1) components‐vs‐labels check ---
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
    // --- 2) quantityName vs unit dimensionality ---
    if (quantityName) {
        OCStringRef err = NULL;
        SIDimensionalityRef nameDim =
            SIDimensionalityForQuantity(quantityName, &err);
        if (!nameDim) {
            if (err) OCRelease(err);
            return false;
        }
        SIDimensionalityRef unitDim =
            SIQuantityGetUnitDimensionality((SIQuantityRef)unit);
        bool match = SIDimensionalityHasSameReducedDimensionality(
            nameDim, unitDim);
        OCRelease(err);
        if (!match) return false;
    }
    // --- 3) quantityType semantics ---
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
// internal helper
static DependentVariableRef
_DependentVariableCreateInternal(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    bool copyComponents,
    OCIndex explicitSize,
    OCTypeRef owner) {
    // 1) figure out how many components, validate CFData / OCData types & lengths
    OCIndex componentsCount = 0;
    if (components) {
        componentsCount = OCArrayGetCount(components);
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCTypeRef obj = OCArrayGetValueAtIndex(components, i);
            if (OCGetTypeID(obj) != OCDataGetTypeID()) return NULL;
        }
        if (componentsCount > 1) {
            uint64_t len0 = OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, 0));
            for (OCIndex i = 1; i < componentsCount; i++) {
                if (OCDataGetLength((OCDataRef)OCArrayGetValueAtIndex(components, i)) != len0)
                    return NULL;
            }
        }
    } else {
        // derive count from the quantityType string
        componentsCount = DependentVariableComponentsCountFromQuantityType(quantityType);
        if (componentsCount == kOCNotFound) return NULL;
    }
    // 2) validate dimensionality / labels / type semantics
    if (!validateDependentVariableParameters(
            unit,
            quantityName,
            quantityType,
            componentLabels,
            componentsCount)) {
        return NULL;
    }
    // 3) allocate & default‐init
    DependentVariableRef dv = DependentVariableAllocate();
    // 4) populate basic fields
    dv->type = elementType;
    dv->unit = unit ? unit : SIUnitDimensionlessAndUnderived();
    dv->quantityType = OCRetain(quantityType);
    dv->name = name ? OCStringCreateCopy(name) : STR("");
    dv->description = description ? OCStringCreateCopy(description) : STR("");
    dv->quantityName = quantityName
                           ? OCRetain(quantityName)
                           : OCStringCreateCopy(SIUnitGuessQuantityName(dv->unit));
    // 5) build / copy component buffers
    dv->components = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (components) {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(components, i);
            if (copyComponents) {
                OCMutableDataRef copy = OCDataCreateMutableCopy(/*capacity*/ 0, blob);
                OCArrayAppendValue(dv->components, copy);
                OCRelease(copy);
            } else {
                OCArrayAppendValue(dv->components, blob);
            }
        }
    } else {
        // size‐based creation
        size_t byteLen = explicitSize * OCNumberTypeSize((OCNumberType)elementType);
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCMutableDataRef buf = OCDataCreateMutable(/*capacity*/ 0);
            OCDataSetLength(buf, byteLen);
            OCArrayAppendValue(dv->components, buf);
            OCRelease(buf);
        }
    }
    // 6) build componentLabels
    dv->componentLabels = OCArrayCreateMutable(componentsCount, &kOCTypeArrayCallBacks);
    if (componentLabels) {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCStringRef lbl = (OCStringRef)OCArrayGetValueAtIndex(componentLabels, i);
            OCStringRef copyLbl = OCStringCreateCopy(lbl);
            OCArrayAppendValue(dv->componentLabels, copyLbl);
            OCRelease(copyLbl);
        }
    } else {
        for (OCIndex i = 0; i < componentsCount; i++) {
            OCStringRef autoLbl = OCStringCreateWithFormat(
                STR("component-%ld"), (long)i);
            OCArrayAppendValue(dv->componentLabels, autoLbl);
            OCRelease(autoLbl);
        }
    }
    // 7) metadata, owner
    dv->metaData = OCDictionaryCreateMutable(0);
    dv->owner = owner;  // weak back‐pointer - check if octpe is a DatasetRef
    return dv;
}
// ————————————————————————————————————————————————————————————————————————
// now the public APIs
DependentVariableRef
DependentVariableCreateWithComponentsNoCopy(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCTypeRef owner) {
    return _DependentVariableCreateInternal(
        name, description, unit,
        quantityName, quantityType, elementType,
        componentLabels, components,
        /*copy=*/false,
        /*explicitSize=*/-1,
        owner);
}
DependentVariableRef
DependentVariableCreate(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCTypeRef owner) {
    return _DependentVariableCreateInternal(
        name, description, unit,
        quantityName, quantityType, elementType,
        componentLabels, components,
        /*copy=*/true,
        /*explicitSize=*/-1,
        owner);
}
DependentVariableRef
DependentVariableCreateWithSize(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCIndex size,
    OCTypeRef owner) {
    return _DependentVariableCreateInternal(
        name, description, unit,
        quantityName, quantityType, elementType,
        componentLabels,
        /*components=*/NULL,
        /*copy=*/false,
        size,
        owner);
}
DependentVariableRef
DependentVariableCreateDefault(
    OCStringRef quantityType,
    SINumberType elementType,
    OCIndex size,
    OCTypeRef owner) {
    return DependentVariableCreateWithSize(
        /*name=*/NULL,
        /*description=*/NULL,
        /*unit=*/NULL,
        /*quantityName=*/NULL,
        quantityType,
        elementType,
        /*componentLabels=*/NULL,
        size,
        owner);
}
DependentVariableRef
DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCTypeRef owner) {
    OCMutableArrayRef arr = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(arr, component);
    DependentVariableRef dv = DependentVariableCreate(
        name, description, unit,
        quantityName, STR("scalar"), elementType,
        componentLabels, arr,
        owner);
    OCRelease(arr);
    return dv;
}
DependentVariableRef
DependentVariableCreateCopy(DependentVariableRef src) {
    if (!src) return NULL;
    return DependentVariableCreate(
        src->name,
        src->description,
        src->unit,
        src->quantityName,
        src->quantityType,
        src->type,
        src->componentLabels,
        src->components,
        NULL);
}
DependentVariableRef
DependentVariableCreateComplexCopy(DependentVariableRef src,
                                   OCTypeRef owner) {
    DependentVariableRef dv = DependentVariableCreateCopy(src);
    if (dv && !SIQuantityIsComplexType((SIQuantityRef) dv)) {
        SINumberType newType = (dv->type == kSINumberFloat32Type
                                    ? kSINumberFloat32ComplexType
                                    : kSINumberFloat64ComplexType);
        DependentVariableSetElementType(dv, newType);
    }
    return dv;
}
// ————————————————————————————————————————————————————————————————————————
// Reconstruct a DependentVariable from the above dictionary format
// ————————————————————————————————————————————————————————————————————————
DependentVariableRef
DependentVariableCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    // 1) Allocate fresh object with defaults
    struct impl_DependentVariable *dv = DependentVariableAllocate();
    // 2) Pull out each field and swap in
    OCStringRef tmpStr;
    OCArrayRef tmpArr;
    OCIndexSetRef tmpSet;
    OCDictionaryRef tmpDict;
#define SWAP_STRING(key, field)                                         \
    if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR(key)))) { \
        OCRelease(dv->field);                                           \
        dv->field = OCStringCreateCopy(tmpStr);                         \
    }
#define SWAP_ARRAY(key, field)                                         \
    if ((tmpArr = (OCArrayRef)OCDictionaryGetValue(dict, STR(key)))) { \
        OCRelease(dv->field);                                          \
        dv->field = OCTypeDeepCopyMutable(tmpArr);                     \
    }
#define SWAP_DICT(key, field)                                                \
    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(key)))) { \
        OCRelease(dv->field);                                                \
        dv->field = OCTypeDeepCopyMutable(tmpDict);                          \
    }
#define SWAP_SET(key, field)                                              \
    if ((tmpSet = (OCIndexSetRef)OCDictionaryGetValue(dict, STR(key)))) { \
        OCRelease(dv->field);                                             \
        dv->field = OCIndexSetCreateMutableCopy(tmpSet);                  \
    }
    SWAP_STRING("name", name);
    SWAP_STRING("quantityName", quantityName);
    SWAP_STRING("quantityType", quantityType);
    SWAP_STRING("description", description);
    SWAP_STRING("sparseGridVertexes", sparseGridVertexes);
    SWAP_ARRAY("components", components);
    SWAP_ARRAY("componentLabels", componentLabels);
    SWAP_SET("sparseDimensionIndexes", sparseDimensionIndexes);
    SWAP_DICT("metaData", metaData);
#undef SWAP_STRING
#undef SWAP_ARRAY
#undef SWAP_DICT
#undef SWAP_SET
    return (DependentVariableRef)dv;
}
//-----------------------------------------------------------------------------
// Serialize a DependentVariable to a dictionary (for deep-copy & tests)
//-----------------------------------------------------------------------------
OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv)
{
    if (!dv) return NULL;

    // 1) Create a mutable dictionary with default callbacks
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);

    // 2) Simple string fields
    OCDictionarySetValue(dict, STR("name"),                dv->name);
    OCDictionarySetValue(dict, STR("description"),         dv->description);
    OCDictionarySetValue(dict, STR("quantityName"),        dv->quantityName);
    OCDictionarySetValue(dict, STR("quantityType"),        dv->quantityType);
    OCDictionarySetValue(dict, STR("sparseGridVertexes"),  dv->sparseGridVertexes);

    // 3) Components array (deep-copy each OCData)
    OCIndex nComps = OCArrayGetCount(dv->components);
    OCMutableArrayRef compArr = OCArrayCreateMutable(nComps, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < nComps; ++i) {
        OCDataRef blob = (OCDataRef)OCArrayGetValueAtIndex(dv->components, i);
        OCMutableDataRef copy = OCDataCreateMutableCopy(0, blob);
        OCArrayAppendValue(compArr, copy);
        OCRelease(copy);
    }
    OCDictionarySetValue(dict, STR("components"), compArr);
    OCRelease(compArr);

    // 4) Component labels (shallow-copy strings)
    OCMutableArrayRef lblArr = OCArrayCreateMutable(nComps, &kOCTypeArrayCallBacks);
    for (OCIndex i = 0; i < nComps; ++i) {
        OCStringRef lbl = (OCStringRef)OCArrayGetValueAtIndex(dv->componentLabels, i);
        OCArrayAppendValue(lblArr, lbl);
    }
    OCDictionarySetValue(dict, STR("componentLabels"), lblArr);
    OCRelease(lblArr);

    // 5) Sparse indexes (if any)
    if (dv->sparseDimensionIndexes) {
        OCIndexSetRef copySet = OCIndexSetCreateMutableCopy(dv->sparseDimensionIndexes);
        OCDictionarySetValue(dict, STR("sparseDimensionIndexes"), copySet);
        OCRelease(copySet);
    }

    // 6) Metadata (deep-copy dictionary)
    if (dv->metaData) {
        OCDictionaryRef mdCopy = OCTypeDeepCopyMutable(dv->metaData);
        OCDictionarySetValue(dict, STR("metaData"), mdCopy);
        OCRelease(mdCopy);
    }

    return (OCDictionaryRef)dict;
}
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
bool
DependentVariableInsertComponentAtIndex(DependentVariableRef dv,
                                        OCDataRef          component,
                                        OCIndex            idx)
{
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
    switch (dv->type) {
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
        uint8_t *bytes = (uint8_t *) OCDataGetMutableBytesPtr(newBuf);
        size_t offset = (size_t)oldSize * elemSize;
        size_t count = (size_t)(newSize - oldSize) * elemSize;
        memset(bytes + offset, 0, count);
        OCRelease(newBuf);
    }
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
OCStringRef
DependentVariableGetQuantityType(DependentVariableRef dv) {
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
OCDictionaryRef
DependentVariableGetMetaData(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->metaData;
}

bool
DependentVariableSetMetaData(DependentVariableRef dv,
                             OCDictionaryRef      dict)
{
    if (!dv) return false;
    OCRelease(dv->metaData);
    dv->metaData = dict ? OCTypeDeepCopyMutable(dict) : OCDictionaryCreateMutable(0);
    return dv->metaData != NULL;
}
// — sparseDimensionIndexes —
OCIndexSetRef
DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->sparseDimensionIndexes;
}

bool
DependentVariableSetSparseDimensionIndexes(DependentVariableRef dv,
                                           OCIndexSetRef        idxSet)
{
    if (!dv) return false;
    OCRelease(dv->sparseDimensionIndexes);
    dv->sparseDimensionIndexes = idxSet
        ? OCIndexSetCreateMutableCopy(idxSet)
        : OCIndexSetCreateMutable();
    return dv->sparseDimensionIndexes != NULL;
}

// — sparseGridVertexes —
OCStringRef
DependentVariableGetSparseGridVertexes(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->sparseGridVertexes;
}

bool
DependentVariableSetSparseGridVertexes(DependentVariableRef dv,
                                       OCStringRef         s)
{
    if (!dv) return false;
    OCRelease(dv->sparseGridVertexes);
    dv->sparseGridVertexes = s
        ? OCStringCreateCopy(s)
        : STR("");
    return dv->sparseGridVertexes != NULL;
}

// — (optional) owner —
OCTypeRef
DependentVariableGetOwner(DependentVariableRef dv) {
    if (!dv) return NULL;
    return dv->owner;
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
    if (!validateDependentVariableParameters(
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
    return dv->type;
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
                                     SINumberType newType) 
                                     {
    if (!dv) return false;
    SINumberType oldType = dv->type;
    if (oldType == newType) return true;
    OCMutableArrayRef comps = dv->components;
    if (!comps) return false;
    OCIndex nComps = OCArrayGetCount(comps);
    OCIndex nElems = DependentVariableGetSize(dv);
    size_t newBytes = nElems * OCNumberTypeSize((OCNumberType) newType);
    for (OCIndex ci = 0; ci < nComps; ci++) {
        OCMutableDataRef oldData = (OCMutableDataRef) OCArrayGetValueAtIndex(comps, ci);
        uint8_t *oldPtr = OCDataGetMutableBytesPtr(oldData);
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
    dv->type = newType;
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    switch (dv->type) {
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
    void *bytes = OCDataGetMutableBytesPtr(data);
    switch (dv->type) {
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
