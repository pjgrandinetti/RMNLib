
#include "RMNLibrary.h"

static OCTypeID kRMNDatumID = _kOCNotATypeID;

// SIScalar Opaque Type
struct __RMNDatum {
    OCBase _base;

    CFIndex             dependentVariableIndex;
    CFIndex             componentIndex;
    CFIndex             memOffset;
    CFArrayRef          coordinates;

};
