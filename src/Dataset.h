#ifndef DATASET_H
#define DATASET_H

#include "RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct impl_Dataset *DatasetRef;

///-----------------------------
/// Type registration & creation
///-----------------------------

/// Returns the OCTypeID for Dataset
OCTypeID                DatasetGetTypeID(void);

/// Allocate & initialize an empty, zero-dimensional Dataset
DatasetRef              DatasetCreateEmpty(void);

/// Construct a Dataset with everything specified.
/// Returns NULL on validation failure (mismatched sizes, bad types, etc).
DatasetRef
DatasetCreate(
    OCArrayRef      dimensions,          // array of DimensionRef subclasses
    OCArrayRef      dimensionPrecedence, // array of OCNumberRefs, length == dimensions count
    OCArrayRef      dependentVariables,  // array of DependentVariableRef
    OCArrayRef      tags,                // array of OCStringRef
    OCStringRef     description,
    OCStringRef     title,
    DatumRef        focus,
    DatumRef        previousFocus,
    OCDictionaryRef operations,
    OCDictionaryRef metaData
);

/// Rehydrate a Dataset from its dictionary form (as emitted by CopyAsDictionary)
DatasetRef              DatasetCreateFromDictionary(OCDictionaryRef dict);

/// Serialize a Dataset to a dictionary for persistence or deep-copy
OCDictionaryRef         DatasetCopyAsDictionary(DatasetRef ds);

/// Deep-copy a Dataset (via CopyAsDictionary + CreateFromDictionary)
static inline DatasetRef
DatasetCreateCopy(DatasetRef ds) {
    if (!ds) return NULL;
    OCDictionaryRef dict = DatasetCopyAsDictionary(ds);
    DatasetRef     copy = DatasetCreateFromDictionary(dict);
    OCRelease(dict);
    return copy;
}

///---------------------------------
/// Accessors & mutators
///---------------------------------

// — dimensions —
OCMutableArrayRef       DatasetGetDimensions(DatasetRef ds);
bool                    DatasetSetDimensions(DatasetRef ds,
                                              OCMutableArrayRef dims);

// — dependent variables —
OCMutableArrayRef       DatasetGetDependentVariables(DatasetRef ds);
bool                    DatasetSetDependentVariables(DatasetRef ds,
                                                     OCMutableArrayRef dvs);

// — tags —
OCMutableArrayRef       DatasetGetTags(DatasetRef ds);
bool                    DatasetSetTags(DatasetRef ds,
                                       OCMutableArrayRef tags);

// — description —
OCStringRef             DatasetGetDescription(DatasetRef ds);
bool                    DatasetSetDescription(DatasetRef ds,
                                              OCStringRef desc);

// — title —
OCStringRef             DatasetGetTitle(DatasetRef ds);
bool                    DatasetSetTitle(DatasetRef ds,
                                         OCStringRef title);

// — focus & previous focus —
DatumRef                DatasetGetFocus(DatasetRef ds);
bool                    DatasetSetFocus(DatasetRef ds,
                                        DatumRef focus);
DatumRef                DatasetGetPreviousFocus(DatasetRef ds);
bool                    DatasetSetPreviousFocus(DatasetRef ds,
                                                DatumRef previousFocus);

// — dimension precedence (ordered indexes) —
OCMutableIndexArrayRef  DatasetGetDimensionPrecedence(DatasetRef ds);
bool                    DatasetSetDimensionPrecedence(DatasetRef ds,
                                                      OCMutableIndexArrayRef order);

// — metadata dictionary —
OCDictionaryRef         DatasetGetMetaData(DatasetRef ds);
bool                    DatasetSetMetaData(DatasetRef ds,
                                           OCDictionaryRef md);

// — operations dictionary —
OCMutableDictionaryRef  DatasetGetOperations(DatasetRef ds);
bool                    DatasetSetOperations(DatasetRef ds,
                                             OCMutableDictionaryRef ops);

// — base64 flag —
bool                    DatasetGetBase64(DatasetRef ds);
bool                    DatasetSetBase64(DatasetRef ds,
                                          bool base64);

#ifdef __cplusplus
}
#endif

#endif // DATASET_H
