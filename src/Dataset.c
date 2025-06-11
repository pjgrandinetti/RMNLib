#include "RMNLibrary.h"

static OCTypeID kRMNDatasetID = kOCNotATypeID;

// SIScalar Opaque Type
struct impl_Dataset {
    OCBase base;

    // CSDM attributes
    OCMutableArrayRef       dimensions;         // array of PSDimensions, each representing a uniformly sampled dimension.
    OCMutableArrayRef       dependentVariables; // Array with dependentVariables. Each element is a PSDependentVariable
    OCMutableArrayRef       tags;
    OCStringRef             description;

    // RMN extra attributes below
    OCStringRef             title;
    DatumRef             focus;
    DatumRef             previousFocus;
    OCMutableArrayRef       dimensionPrecedence; // ordered array of indexes, representing dimension precedence.
    OCDictionaryRef         metaData;
    OCMutableDictionaryRef  operations;

    // ***** End Persistent Attributes
    
    bool                    base64;

};
