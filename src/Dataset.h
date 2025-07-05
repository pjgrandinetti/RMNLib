#ifndef DATASET_H
#define DATASET_H

#include "Datum.h"
#include "OCArray.h"
#include "OCDictionary.h"
#include "OCIndexArray.h"
#include "OCString.h"
#include "OCType.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque Dataset type
typedef struct impl_Dataset *DatasetRef;

/// Type ID registration
OCTypeID DatasetGetTypeID(void);

/// Create a new Dataset with the given contents.
/// Returns NULL on validation failure (e.g. no dependent‐variables, size mismatches, bad types).
DatasetRef DatasetCreate(
    OCArrayRef     dimensions,
    OCIndexArrayRef dimensionPrecedence,
    OCArrayRef     dependentVariables,
    OCArrayRef     tags,
    OCStringRef    description,
    OCStringRef    title,
    DatumRef       focus,
    DatumRef       previousFocus,
    OCDictionaryRef metaData);

/// Re-instantiate a Dataset from the dictionary form produced by DatasetCopyAsDictionary
DatasetRef DatasetCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/// Serialize into a dictionary (deep-copyable, for persistence or DatasetCreateCopy)
OCDictionaryRef DatasetCopyAsDictionary(DatasetRef ds);
/// Shorthand deep-copy via CopyAsDictionary + CreateFromDictionary
static inline DatasetRef DatasetCreateCopy(DatasetRef ds) {
    if (!ds) return NULL;
    OCStringRef err = NULL;
    OCDictionaryRef d = DatasetCopyAsDictionary(ds);
    DatasetRef c = DatasetCreateFromDictionary(d, &err);
    OCRelease(d);
    if (!c) {
        OCRelease(err);  // optional: log or inspect err before releasing
        return NULL;
    }
    return c;
}

///-----------------------
/// Accessors & Mutators
///-----------------------

/// dimensions
OCMutableArrayRef DatasetGetDimensions(DatasetRef ds);
bool DatasetSetDimensions(DatasetRef ds,
                          OCMutableArrayRef dims);

/// dimension precedence
OCMutableIndexArrayRef DatasetGetDimensionPrecedence(DatasetRef ds);
bool DatasetSetDimensionPrecedence(DatasetRef ds,
                                   OCMutableIndexArrayRef order);

/// dependent-variables
OCMutableArrayRef DatasetGetDependentVariables(DatasetRef ds);
bool DatasetSetDependentVariables(DatasetRef ds,
                                  OCMutableArrayRef dvs);
OCIndex DatasetGetDependentVariableCount(DatasetRef ds);
DependentVariableRef DatasetGetDependentVariableAtIndex(DatasetRef ds, OCIndex index);

/// tags
OCMutableArrayRef DatasetGetTags(DatasetRef ds);
bool DatasetSetTags(DatasetRef ds,
                    OCMutableArrayRef tags);

/// description
OCStringRef DatasetGetDescription(DatasetRef ds);
bool DatasetSetDescription(DatasetRef ds,
                           OCStringRef desc);

/// title
OCStringRef DatasetGetTitle(DatasetRef ds);
bool DatasetSetTitle(DatasetRef ds,
                     OCStringRef title);

/// focus & previous‐focus
DatumRef DatasetGetFocus(DatasetRef ds);
bool DatasetSetFocus(DatasetRef ds,
                     DatumRef focus);
DatumRef DatasetGetPreviousFocus(DatasetRef ds);
bool DatasetSetPreviousFocus(DatasetRef ds, DatumRef previousFocus);

/// metadata dictionary
OCDictionaryRef DatasetGetMetaData(DatasetRef ds);
bool DatasetSetMetaData(DatasetRef ds,
                        OCDictionaryRef md);

/// base64‐flag
bool DatasetGetBase64(DatasetRef ds);
bool DatasetSetBase64(DatasetRef ds,
                      bool base64);

#ifdef __cplusplus
}
#endif

#endif  // DATASET_H

