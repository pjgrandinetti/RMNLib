// ============================================================================
// Dataset.c
// ============================================================================
#include "RMNLibrary.h"
static OCTypeID kDatasetID = kOCNotATypeID;
// SIScalar Opaque Type
struct impl_Dataset {
    OCBase base;
    // CSDM attributes
    OCMutableArrayRef dimensions;          // array of PSDimensions, each representing a uniformly sampled dimension.
    OCMutableArrayRef dependentVariables;  // Array with dependentVariables. Each element is a PSDependentVariable
    OCMutableArrayRef tags;
    OCStringRef description;
    // RMN extra attributes below
    OCStringRef title;
    DatumRef focus;
    DatumRef previousFocus;
    OCMutableIndexArrayRef dimensionPrecedence;  // ordered array of indexes, representing dimension precedence.
    OCDictionaryRef metaData;
    OCMutableDictionaryRef operations;
    // ***** End Persistent Attributes
    bool base64;
};
//==============================================================================
// MARK: - Type Registration
//==============================================================================
OCTypeID DatasetGetTypeID(void) {
    if (kDatasetID == kOCNotATypeID)
        kDatasetID = OCRegisterType("Dataset");
    return kDatasetID;
}
//==============================================================================
// MARK: - Finalizer, Equal, Description, DeepCopy
//==============================================================================
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
    OCRelease(ds->metaData);
    OCRelease(ds->operations);
}
static bool impl_DatasetEqual(const void *a, const void *b) {
    const DatasetRef A = (const DatasetRef)a;
    const DatasetRef B = (const DatasetRef)b;
    if (!A || !B) return false;
    if (!OCTypeEqual(A->dimensions, B->dimensions)) return false;
    if (!OCTypeEqual(A->dependentVariables, B->dependentVariables)) return false;
    if (!OCTypeEqual(A->tags, B->tags)) return false;
    if (!OCTypeEqual(A->description, B->description)) return false;
    if (!OCTypeEqual(A->title, B->title)) return false;
    if (!OCTypeEqual(A->focus, B->focus)) return false;
    if (!OCTypeEqual(A->previousFocus, B->previousFocus)) return false;
    if (!OCTypeEqual(A->dimensionPrecedence, B->dimensionPrecedence)) return false;
    if (!OCTypeEqual(A->metaData, B->metaData)) return false;
    if (!OCTypeEqual(A->operations, B->operations)) return false;
    // base64 is a plain bool, compare directly
    return A->base64 == B->base64;
}
static OCStringRef impl_DatasetCopyFormattingDesc(OCTypeRef cf) {
    DatasetRef ds = (DatasetRef)cf;
    if (!ds) return STR("<Dataset: NULL>");
    return OCStringCreateWithFormat(
        STR("<Dataset dims=%lu vars=%lu tags=%lu title=\"%@\">"),
        (unsigned long)OCArrayGetCount(ds->dimensions),
        (unsigned long)OCArrayGetCount(ds->dependentVariables),
        (unsigned long)OCArrayGetCount(ds->tags),
        ds->title);
}
static void *impl_DatasetDeepCopy(const void *ptr) {
    if (!ptr) return NULL;
    DatasetRef src = (DatasetRef)ptr;
    OCDictionaryRef dict = DatasetCopyAsDictionary(src);
    DatasetRef copy = DatasetCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}
//==============================================================================
// MARK: - Allocation & Initialization
//==============================================================================
static struct impl_Dataset *DatasetAllocate(void) {
    return OCTypeAlloc(
        struct impl_Dataset,
        DatasetGetTypeID(),
        impl_DatasetFinalize,
        impl_DatasetEqual,
        impl_DatasetCopyFormattingDesc,
        impl_DatasetDeepCopy,
        impl_DatasetDeepCopy);
}
static void impl_InitDatasetFields(DatasetRef ds) {
    ds->dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->dependentVariables = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    ds->description = STR("");
    ds->title = STR("");
    ds->focus = NULL;
    ds->previousFocus = NULL;
    ds->dimensionPrecedence = OCIndexArrayCreateMutable(0);
    ds->metaData = OCDictionaryCreateMutable(0);
    ds->operations = OCDictionaryCreateMutable(0);
    ds->base64 = false;
}
//==============================================================================
// Validate that
//  • dependentVariables is non-NULL & non-empty,
//  • every element is a DependentVariableRef,
//  • if dimensions != NULL, every element is some DimensionRef subtype,
//  • all dependentVariables have the same total “size” as produced by RMNCalculateSizeFromDimensions.
//==============================================================================
static bool impl_ValidateDatasetParameters(OCArrayRef dimensions,
                                           OCArrayRef dependentVariables) {
    // must have at least one dependent variable
    if (!dependentVariables || OCArrayGetCount(dependentVariables) == 0)
        return false;
    // compute expected length from dimensions (defaults to 1 if dimensions==NULL)
    OCIndex expectedSize = RMNCalculateSizeFromDimensions(dimensions);
    // if dimensions provided, check that each is a DimensionRef
    if (dimensions) {
        OCIndex nDims = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < nDims; i++) {
            const void *obj = OCArrayGetValueAtIndex(dimensions, i);
            OCTypeID tid = OCGetTypeID(obj);
            if (tid != DimensionGetTypeID() && tid != LabeledDimensionGetTypeID() && tid != SIMonotonicDimensionGetTypeID() && tid != SILinearDimensionGetTypeID()) {
                return false;
            }
        }
    }
    // now validate each dependent variable
    OCIndex dvCount = OCArrayGetCount(dependentVariables);
    for (OCIndex i = 0; i < dvCount; i++) {
        const void *obj = OCArrayGetValueAtIndex(dependentVariables, i);
        if (OCGetTypeID(obj) != DependentVariableGetTypeID())
            return false;
        DependentVariableRef dv = (DependentVariableRef)obj;
        OCIndex dvSize = DependentVariableGetSize(dv);
        if (dvSize != expectedSize) {
            fprintf(stderr,
                    "RMNValidateDatasetParameters: size mismatch for DV at index %ld: got %ld, expected %ld\n",
                    (long)i, (long)dvSize, (long)expectedSize);
            return false;
        }
    }
    return true;
}
//==============================================================================
// MARK: - Public Constructor
//==============================================================================
DatasetRef
DatasetCreate(
    OCArrayRef dimensions,
    OCArrayRef dimensionPrecedence,
    OCArrayRef dependentVariables,
    OCArrayRef tags,
    OCStringRef description,
    OCStringRef title,
    DatumRef focus,
    DatumRef previousFocus,
    OCDictionaryRef operations,
    OCDictionaryRef metaData) {
    // 1) validate inputs
    if (!impl_ValidateDatasetParameters(dimensions, dependentVariables))
        return NULL;
    // 2) allocate & initialize defaults
    DatasetRef ds = DatasetAllocate();
    if (!ds) return NULL;
    impl_InitDatasetFields(ds);
    // 3) copy dimensions array
    if (dimensions) {
        OCIndex n = OCArrayGetCount(dimensions);
        for (OCIndex i = 0; i < n; i++) {
            DimensionRef d = (DimensionRef)OCArrayGetValueAtIndex(dimensions, i);
            DimensionRef dc = (DimensionRef)OCTypeDeepCopyMutable(d);
            OCArrayAppendValue(ds->dimensions, dc);
            OCRelease(dc);
        }
    }
    // 4) copy dependentVariables
    {
        OCIndex n = OCArrayGetCount(dependentVariables);
        for (OCIndex i = 0; i < n; i++) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dependentVariables, i);
            DependentVariableRef copy = DependentVariableCreateCopy(dv);
            OCArrayAppendValue(ds->dependentVariables, copy);
            OCRelease(copy);
        }
    }
    // 5) copy tags
    if (tags) {
        OCIndex n = OCArrayGetCount(tags);
        for (OCIndex i = 0; i < n; i++) {
            OCStringRef s = (OCStringRef)OCArrayGetValueAtIndex(tags, i);
            OCArrayAppendValue(ds->tags, s);
        }
    }
    // 6) build or default dimensionPrecedence
    OCIndex dimCount = OCArrayGetCount(ds->dimensions);
    if (dimensionPrecedence && OCArrayGetCount(dimensionPrecedence) == dimCount) {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCNumberRef idx = (OCNumberRef)OCArrayGetValueAtIndex(dimensionPrecedence, i);
            OCIndex tmpIdx = 0;
            OCNumberGetValue(idx, kOCNumberIntType, &tmpIdx);
            OCIndexArrayAppendValue(ds->dimensionPrecedence, tmpIdx);
        }
    } else {
        for (OCIndex i = 0; i < dimCount; i++) {
            OCIndexArrayAppendValue(ds->dimensionPrecedence, i);
        }
    }
    // 7) copy simple fields (cast retain back into DatumRef)
    ds->description = description ? OCStringCreateCopy(description) : STR("");
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    // 8) copy operations & metaData
    if (operations) {
        OCRelease(ds->operations);
        ds->operations = OCTypeDeepCopyMutable(operations);
    }
    if (metaData) {
        OCRelease(ds->metaData);
        ds->metaData = OCTypeDeepCopyMutable(metaData);
    }
    return ds;
}
DatasetRef DatasetCreateFromDictionary(OCDictionaryRef dict) {
    if (!dict) return NULL;
    DatasetRef ds = DatasetAllocate();
    if (!ds) return NULL;
    impl_InitDatasetFields(ds);
    // swap in any serialized fields...
    OCArrayRef tmpArr;
    OCStringRef tmpStr;
    OCDictionaryRef tmpDict;
    DatumRef tmpDatum;
    OCIndexArrayRef tmpIdxArr;
    if ((tmpArr = (OCArrayRef)OCDictionaryGetValue(dict, STR("dimensions")))) {
        OCRelease(ds->dimensions);
        ds->dimensions = OCTypeDeepCopyMutable(tmpArr);
    }
    if ((tmpArr = (OCArrayRef)OCDictionaryGetValue(dict, STR("dependentVariables")))) {
        OCRelease(ds->dependentVariables);
        ds->dependentVariables = OCTypeDeepCopyMutable(tmpArr);
    }
    if ((tmpArr = (OCArrayRef)OCDictionaryGetValue(dict, STR("tags")))) {
        OCRelease(ds->tags);
        ds->tags = OCTypeDeepCopyMutable(tmpArr);
    }
    if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR("description")))) {
        OCRelease(ds->description);
        ds->description = OCStringCreateCopy(tmpStr);
    }
    if ((tmpStr = (OCStringRef)OCDictionaryGetValue(dict, STR("title")))) {
        OCRelease(ds->title);
        ds->title = OCStringCreateCopy(tmpStr);
    }
    if ((tmpDatum = (DatumRef)OCDictionaryGetValue(dict, STR("focus")))) {
        OCRelease(ds->focus);
        ds->focus = (DatumRef)OCTypeDeepCopyMutable(tmpDatum);
    }
    if ((tmpDatum = (DatumRef)OCDictionaryGetValue(dict, STR("previousFocus")))) {
        OCRelease(ds->previousFocus);
        ds->previousFocus = (DatumRef)OCTypeDeepCopyMutable(tmpDatum);
    }
    if ((tmpIdxArr = (OCIndexArrayRef)OCDictionaryGetValue(dict, STR("dimensionPrecedence")))) {
        OCRelease(ds->dimensionPrecedence);
        ds->dimensionPrecedence = OCIndexArrayCreateMutableCopy(tmpIdxArr);
    }
    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("metaData")))) {
        OCRelease(ds->metaData);
        ds->metaData = OCTypeDeepCopyMutable(tmpDict);
    }
    if ((tmpDict = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("operations")))) {
        OCRelease(ds->operations);
        ds->operations = OCTypeDeepCopyMutable(tmpDict);
    }
    OCBooleanRef bobj = (OCBooleanRef)OCDictionaryGetValue(dict, STR("base64"));
    ds->base64 = bobj ? OCBooleanGetValue(bobj) : false;
    return ds;
}
OCDictionaryRef DatasetCopyAsDictionary(DatasetRef ds) {
    if (!ds) return NULL;
    OCMutableDictionaryRef dict = OCDictionaryCreateMutable(0);
    // ... serialize each field back into dict ...
    OCDictionarySetValue(dict, STR("dimensions"), ds->dimensions);
    OCDictionarySetValue(dict, STR("dependentVariables"), ds->dependentVariables);
    OCDictionarySetValue(dict, STR("tags"), ds->tags);
    OCDictionarySetValue(dict, STR("description"), ds->description);
    OCDictionarySetValue(dict, STR("title"), ds->title);
    if (ds->focus) OCDictionarySetValue(dict, STR("focus"), ds->focus);
    if (ds->previousFocus) OCDictionarySetValue(dict, STR("previousFocus"), ds->previousFocus);
    OCDictionarySetValue(dict, STR("dimensionPrecedence"), ds->dimensionPrecedence);
    OCDictionarySetValue(dict, STR("metaData"), ds->metaData);
    OCDictionarySetValue(dict, STR("operations"), ds->operations);
    OCDictionarySetValue(dict, STR("base64"), OCBooleanGetWithBool(ds->base64));
    return dict;
}
