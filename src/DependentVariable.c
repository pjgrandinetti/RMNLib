#include "RMNLibrary.h"


static OCTypeID kDependentVariableID = kOCNotATypeID;
struct impl_DependentVariable {
    //  Dimension
    OCBase base;
    OCStringRef         name;
    OCMutableArrayRef   components;
    OCMutableArrayRef   componentLabels;
    OCStringRef         quantityName;
    OCStringRef         quantityType;
    OCStringRef         description;
    
    OCIndexSetRef       sparseDimensionIndexes;
    OCStringRef          sparseGridVertexes;
    
    // RMN extra attributes below
    OCMutableDictionaryRef  metaData;
    DatasetRef              dataset;
};
OCTypeID DependentVariableGetTypeID(void) {
    if (kDependentVariableID == kOCNotATypeID) kDependentVariableID = OCRegisterType("DependentVariable");
    return kDependentVariableID;
}

