#include "RMNLibrary.h"

static OCTypeID kRMNDatasetID = _kOCNotATypeID;

// SIScalar Opaque Type
struct __RMNDataset {
    OCBase _base;

    // CSDM attributes
    OCMutableArrayRef       dimensions;         // array of PSDimensions, each representing a uniformly sampled dimension.
    OCMutableArrayRef       dependentVariables; // Array with dependentVariables. Each element is a PSDependentVariable
    OCMutableArrayRef       tags;
    OCStringRef             description;

    // RMN extra attributes below
    OCStringRef             title;
    RMNDatumRef              focus;
    RMNDatumRef              previousFocus;
    OCMutableArrayRef       dimensionPrecedence; // ordered array of indexes, representing dimension precedence.
    OCDictionaryRef         metaData;
    OCMutableDictionaryRef  operations;
    // ***** End Persistent Attributes
    
    bool                    base64;

};
