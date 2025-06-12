/* DependentVariable OCType implementation */
#include "DependentVariable.h"

static OCTypeID kDependentVariableID = kOCNotATypeID;
struct impl_DependentVariable {
    OCBase base;
    OCStringRef         name;
    OCMutableArrayRef   components;
    OCMutableArrayRef   componentLabels;
    OCStringRef         quantityName;
    OCStringRef         quantityType;
    OCStringRef         description;
    OCIndexSetRef       sparseDimensionIndexes;
    OCStringRef         sparseGridVertexes;
    OCMutableDictionaryRef  metaData;
    OCTypeRef              dataset;
};

OCTypeID DependentVariableGetTypeID(void) {
    if (kDependentVariableID == kOCNotATypeID)
        kDependentVariableID = OCRegisterType("DependentVariable");
    return kDependentVariableID;
}


// Finalizer: release all owned references
static void impl_DependentVariableFinalize(const void *ptr) {
    if (!ptr) return;
    struct impl_DependentVariable *dv = (struct impl_DependentVariable *)ptr;
    OCRelease(dv->name);
    OCRelease(dv->components);
    OCRelease(dv->componentLabels);
    OCRelease(dv->quantityName);
    OCRelease(dv->quantityType);
    OCRelease(dv->description);
    OCRelease(dv->sparseDimensionIndexes);
    OCRelease(dv->sparseGridVertexes);
    OCRelease(dv->metaData);
    OCRelease(dv->dataset);
}

// Equality: compare all significant fields
static bool impl_DependentVariableEqual(const void *a, const void *b) {
    const struct impl_DependentVariable *dvA = a;
    const struct impl_DependentVariable *dvB = b;
    if (!dvA || !dvB) return false;
    if (dvA == dvB) return true;
    return OCTypeEqual(dvA->name, dvB->name)
        && OCTypeEqual(dvA->components, dvB->components)
        && OCTypeEqual(dvA->componentLabels, dvB->componentLabels)
        && OCTypeEqual(dvA->quantityName, dvB->quantityName)
        && OCTypeEqual(dvA->quantityType, dvB->quantityType)
        && OCTypeEqual(dvA->description, dvB->description)
        && OCTypeEqual(dvA->sparseDimensionIndexes, dvB->sparseDimensionIndexes)
        && OCTypeEqual(dvA->sparseGridVertexes, dvB->sparseGridVertexes)
        && OCTypeEqual(dvA->metaData, dvB->metaData)
        && OCTypeEqual(dvA->dataset, dvB->dataset);
}

// Formatting: produce a human-readable description
static OCStringRef impl_DependentVariableCopyFormattingDesc(OCTypeRef cf) {
    const struct impl_DependentVariable *dv = (struct impl_DependentVariable *)cf;
    return OCStringCreateWithFormat(
        STR("<DependentVariable name=\"%@\" components=%lu>"),
        dv->name,
        (unsigned long)OCArrayGetCount(dv->components)
    );
}

// Deep-copy (immutable) via dictionary round-trip
static void *
impl_DependentVariableDeepCopy(const void *ptr)
{
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

// Deep-copy mutable variant (currently same)
static void *impl_DependentVariableDeepCopyMutable(const void *ptr) {
    return impl_DependentVariableDeepCopy(ptr);
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
        impl_DependentVariableDeepCopyMutable);
    // Initialize fields
    obj->name                 = STR("");
    obj->components           = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    obj->componentLabels      = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    obj->quantityName         = STR("");
    obj->quantityType         = STR("");
    obj->description          = STR("");
    obj->sparseDimensionIndexes = OCIndexSetCreateMutable();
    obj->sparseGridVertexes   = STR("");
    obj->metaData             = OCDictionaryCreateMutable(0);
    obj->dataset              = NULL;
    return obj;
}

DependentVariableRef
DependentVariableCreate(
    OCStringRef            name,
    OCArrayRef             components,
    OCArrayRef             componentLabels,
    OCStringRef            quantityName,
    OCStringRef            quantityType,
    OCStringRef            description,
    OCIndexSetRef          sparseDimensionIndexes,
    OCStringRef            sparseGridVertexes,
    OCMutableDictionaryRef metadata)
{
    // Allocate with all defaults in place
    struct impl_DependentVariable *dv = DependentVariableAllocate();
    if (!dv) return NULL;

    // Replace name
    if (name) {
        OCRelease(dv->name);
        dv->name = OCStringCreateCopy(name);
        if (!dv->name) { OCRelease(dv); return NULL; }
    }

    // Replace components array
    if (components) {
        OCRelease(dv->components);
        dv->components = (OCMutableArrayRef)OCTypeDeepCopyMutable(components);
        if (!dv->components) { OCRelease(dv); return NULL; }
    }

    // Replace componentLabels array
    if (componentLabels) {
        OCRelease(dv->componentLabels);
        dv->componentLabels = (OCMutableArrayRef)OCTypeDeepCopyMutable(componentLabels);
        if (!dv->componentLabels) { OCRelease(dv); return NULL; }
    }

    // Replace quantityName
    if (quantityName) {
        OCRelease(dv->quantityName);
        dv->quantityName = OCStringCreateCopy(quantityName);
        if (!dv->quantityName) { OCRelease(dv); return NULL; }
    }

    // Replace quantityType
    if (quantityType) {
        OCRelease(dv->quantityType);
        dv->quantityType = OCStringCreateCopy(quantityType);
        if (!dv->quantityType) { OCRelease(dv); return NULL; }
    }

    // Replace description
    if (description) {
        OCRelease(dv->description);
        dv->description = OCStringCreateCopy(description);
        if (!dv->description) { OCRelease(dv); return NULL; }
    }

    // Replace sparseDimensionIndexes
    if (sparseDimensionIndexes) {
        OCRelease(dv->sparseDimensionIndexes);
        dv->sparseDimensionIndexes = OCIndexSetCreateMutableCopy(sparseDimensionIndexes);
        if (!dv->sparseDimensionIndexes) { OCRelease(dv); return NULL; }
    }

    // Replace sparseGridVertexes
    if (sparseGridVertexes) {
        OCRelease(dv->sparseGridVertexes);
        dv->sparseGridVertexes = OCStringCreateCopy(sparseGridVertexes);
        if (!dv->sparseGridVertexes) { OCRelease(dv); return NULL; }
    }

    // Replace metadata dictionary
    if (metadata) {
        OCRelease(dv->metaData);
        dv->metaData = (OCMutableDictionaryRef)OCTypeDeepCopyMutable(metadata);
        if (!dv->metaData) { OCRelease(dv); return NULL; }
    }

    // Leave dv->dataset == NULL; user can call setter later if desired

    return (DependentVariableRef)dv;
}

OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv) {
    if (!dv) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    if (!dict) return NULL;

    // — Strings —
    if (dv->name) {
        OCStringRef tmp = OCStringCreateCopy(dv->name);
        OCDictionarySetValue(dict, STR("name"), tmp);
        OCRelease(tmp);
    }
    if (dv->quantityName) {
        OCStringRef tmp = OCStringCreateCopy(dv->quantityName);
        OCDictionarySetValue(dict, STR("quantityName"), tmp);
        OCRelease(tmp);
    }
    if (dv->quantityType) {
        OCStringRef tmp = OCStringCreateCopy(dv->quantityType);
        OCDictionarySetValue(dict, STR("quantityType"), tmp);
        OCRelease(tmp);
    }
    if (dv->description) {
        OCStringRef tmp = OCStringCreateCopy(dv->description);
        OCDictionarySetValue(dict, STR("description"), tmp);
        OCRelease(tmp);
    }
    if (dv->sparseGridVertexes) {
        OCStringRef tmp = OCStringCreateCopy(dv->sparseGridVertexes);
        OCDictionarySetValue(dict, STR("sparseGridVertexes"), tmp);
        OCRelease(tmp);
    }

    // — Collections —
    if (dv->components) {
        OCMutableArrayRef a = OCTypeDeepCopyMutable(dv->components);
        if (a) {
            OCDictionarySetValue(dict, STR("components"), a);
            OCRelease(a);
        }
    }
    if (dv->componentLabels) {
        OCMutableArrayRef a = OCTypeDeepCopyMutable(dv->componentLabels);
        if (a) {
            OCDictionarySetValue(dict, STR("componentLabels"), a);
            OCRelease(a);
        }
    }
    if (dv->sparseDimensionIndexes) {
        OCMutableIndexSetRef s = OCIndexSetCreateMutableCopy(dv->sparseDimensionIndexes);
        if (s) {
            OCDictionarySetValue(dict, STR("sparseDimensionIndexes"), s);
            OCRelease(s);
        }
    }
    if (dv->metaData) {
        OCDictionaryRef m = OCTypeDeepCopyMutable(dv->metaData);
        if (m) {
            OCDictionarySetValue(dict, STR("metaData"), m);
            OCRelease(m);
        }
    }

    // — Dataset (opaque) —
    if (dv->dataset) {
        // deep‐copy via the OCType copyDeep hook
        OCTypeRef copy = (OCTypeRef)OCTypeDeepCopy(dv->dataset);
        if (copy) {
            OCDictionarySetValue(dict, STR("dataset"), copy);
            OCRelease(copy);
        }
    }

    return (OCDictionaryRef)dict;
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
    OCStringRef            tmpStr;
    OCArrayRef             tmpArr;
    OCIndexSetRef          tmpSet;
    OCDictionaryRef        tmpDict;
    OCTypeRef              tmpType;

    #define SWAP_STRING(key, field)                                       \
        if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR(key)))) { \
            OCRelease(dv->field);                                         \
            dv->field = OCStringCreateCopy(tmpStr);                       \
        }

    #define SWAP_ARRAY(key, field)                                        \
        if ((tmpArr = (OCArrayRef)OCDictionaryGetValue(dict, STR(key)))) { \
            OCRelease(dv->field);                                         \
            dv->field = OCTypeDeepCopyMutable(tmpArr);                    \
        }

    #define SWAP_DICT(key, field)                                         \
        if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR(key)))) { \
            OCRelease(dv->field);                                         \
            dv->field = OCTypeDeepCopyMutable(tmpDict);                   \
        }

    #define SWAP_SET(key, field)                                          \
        if ((tmpSet = (OCIndexSetRef)OCDictionaryGetValue(dict, STR(key)))) { \
            OCRelease(dv->field);                                         \
            dv->field = OCIndexSetCreateMutableCopy(tmpSet);              \
        }

    SWAP_STRING("name",                   name);
    SWAP_STRING("quantityName",           quantityName);
    SWAP_STRING("quantityType",           quantityType);
    SWAP_STRING("description",            description);
    SWAP_STRING("sparseGridVertexes",     sparseGridVertexes);

    SWAP_ARRAY("components",              components);
    SWAP_ARRAY("componentLabels",         componentLabels);
    SWAP_SET("sparseDimensionIndexes",    sparseDimensionIndexes);
    SWAP_DICT("metaData",                 metaData);

    #undef SWAP_STRING
    #undef SWAP_ARRAY
    #undef SWAP_DICT
    #undef SWAP_SET

    // 3) Dataset (opaque): expect the same key
    if ((tmpType = (OCTypeRef)OCDictionaryGetValue(dict, STR("dataset")))) {
        OCRelease(dv->dataset);
        dv->dataset = (OCTypeRef)OCTypeDeepCopy(tmpType);
    }

    return (DependentVariableRef)dv;
}
