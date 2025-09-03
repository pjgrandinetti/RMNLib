#include "RMNLibrary.h"
#include "core/SparseSampling_private.h"
// GCC compatibility: Define __has_feature for non-Clang compilers
#ifndef __has_feature
#define __has_feature(x) 0
#endif
bool RMNLibSetDescription(OCTypeRef theType, OCStringRef description, OCStringRef *outError) {
    if (outError) *outError = NULL;
    
    if (!theType || !description) {
        if (outError) *outError = STR("Invalid parameters: theType and description cannot be NULL");
        return false;
    }
    
    OCTypeID typeID = OCGetTypeID(theType);
    if (typeID == DimensionGetTypeID() || typeID == LabeledDimensionGetTypeID() || typeID == SIDimensionGetTypeID() || typeID == SIMonotonicDimensionGetTypeID() || typeID == SILinearDimensionGetTypeID()) {
        return DimensionSetDescription((DimensionRef)theType, description, outError);
    }
    else if (typeID == DependentVariableGetTypeID()) {
        return DependentVariableSetDescription((DependentVariableRef)theType, description);
    }
    else if (typeID == SparseSamplingGetTypeID()) {
        return SparseSamplingSetDescription((SparseSamplingRef)theType, description);
    }
    else if (typeID == DatasetGetTypeID()) {
        return DatasetSetDescription((DatasetRef)theType, description);
    }
    else if (typeID == DatumGetTypeID()) {
        // Datum doesn't have a description field
        if (outError) *outError = STR("Datum type does not support description field");
        return false;
    }
    else if (typeID == GeographicCoordinateGetTypeID()) {
        // GeographicCoordinate doesn't have a description field
        if (outError) *outError = STR("GeographicCoordinate type does not support description field");
        return false;
    }
    
    // Unknown type
    if (outError) *outError = STR("Unknown or unsupported type");
    return false;
}

OCStringRef RMNLibGetDescription(OCTypeRef theType, OCStringRef *outError) {
    if (outError) *outError = NULL;
    
    if (!theType) {
        if (outError) *outError = STR("Invalid parameter: theType cannot be NULL");
        return NULL;
    }
    
    OCTypeID typeID = OCGetTypeID(theType);
    if (typeID == DimensionGetTypeID() || typeID == LabeledDimensionGetTypeID() || typeID == SIDimensionGetTypeID() || typeID == SIMonotonicDimensionGetTypeID() || typeID == SILinearDimensionGetTypeID()) {
        return DimensionCopyDescription((DimensionRef)theType);
    }
    else if (typeID == DependentVariableGetTypeID()) {
        return DependentVariableCopyDescription((DependentVariableRef)theType);
    }
    else if (typeID == SparseSamplingGetTypeID()) {
        return SparseSamplingGetDescription((SparseSamplingRef)theType);
    }
    else if (typeID == DatasetGetTypeID()) {
        return DatasetGetDescription((DatasetRef)theType);
    }
    else if (typeID == DatumGetTypeID()) {
        // Datum doesn't have a description field
        if (outError) *outError = STR("Datum type does not support description field");
        return NULL;
    }
    else if (typeID == GeographicCoordinateGetTypeID()) {
        // GeographicCoordinate doesn't have a description field
        if (outError) *outError = STR("GeographicCoordinate type does not support description field");
        return NULL;
    }
    
    // Unknown type
    if (outError) *outError = STR("Unknown or unsupported type");
    return NULL;
}

bool RMNLibSetApplicationMetaData(OCTypeRef theType, OCDictionaryRef metadata, OCStringRef *outError) {
    if (outError) *outError = NULL;
    
    if (!theType) {
        if (outError) *outError = STR("Invalid parameter: theType cannot be NULL");
        return false;
    }
    
    OCTypeID typeID = OCGetTypeID(theType);
    if (typeID == DimensionGetTypeID() || typeID == LabeledDimensionGetTypeID() || typeID == SIDimensionGetTypeID() || typeID == SIMonotonicDimensionGetTypeID() || typeID == SILinearDimensionGetTypeID()) {
        return DimensionSetApplicationMetaData((DimensionRef)theType, metadata, outError);
    }
    else if (typeID == DependentVariableGetTypeID()) {
        return DependentVariableSetApplicationMetaData((DependentVariableRef)theType, metadata);
    }
    else if (typeID == SparseSamplingGetTypeID()) {
        return SparseSamplingSetApplicationMetaData((SparseSamplingRef)theType, metadata);
    }
    else if (typeID == DatasetGetTypeID()) {
        return DatasetSetApplicationMetaData((DatasetRef)theType, metadata);
    }
    else if (typeID == GeographicCoordinateGetTypeID()) {
        return GeographicCoordinateSetApplicationMetaData((GeographicCoordinateRef)theType, metadata);
    }
    else if (typeID == DatumGetTypeID()) {
        // Datum doesn't have an application metadata field
        if (outError) *outError = STR("Datum type does not support application metadata field");
        return false;
    }
    
    // Unknown type
    if (outError) *outError = STR("Unknown or unsupported type");
    return false;
}

OCDictionaryRef RMNLibGetApplicationMetaData(OCTypeRef theType, OCStringRef *outError) {
    if (outError) *outError = NULL;
    
    if (!theType) {
        if (outError) *outError = STR("Invalid parameter: theType cannot be NULL");
        return NULL;
    }
    
    OCTypeID typeID = OCGetTypeID(theType);
    if (typeID == DimensionGetTypeID() || typeID == LabeledDimensionGetTypeID() || typeID == SIDimensionGetTypeID() || typeID == SIMonotonicDimensionGetTypeID() || typeID == SILinearDimensionGetTypeID()) {
        return DimensionGetApplicationMetaData((DimensionRef)theType);
    }
    else if (typeID == DependentVariableGetTypeID()) {
        return DependentVariableGetApplicationMetaData((DependentVariableRef)theType);
    }
    else if (typeID == SparseSamplingGetTypeID()) {
        return SparseSamplingGetApplicationMetaData((SparseSamplingRef)theType);
    }
    else if (typeID == DatasetGetTypeID()) {
        return DatasetGetApplicationMetaData((DatasetRef)theType);
    }
    else if (typeID == GeographicCoordinateGetTypeID()) {
        return GeographicCoordinateGetApplicationMetaData((GeographicCoordinateRef)theType);
    }
    else if (typeID == DatumGetTypeID()) {
        // Datum doesn't have an application metadata field
        if (outError) *outError = STR("Datum type does not support application metadata field");
        return NULL;
    }
    
    // Unknown type
    if (outError) *outError = STR("Unknown or unsupported type");
    return NULL;
}

static bool rmnLibShutdownCalled = false;
void RMNLibTypesShutdown(void) {
    if (rmnLibShutdownCalled) return;
    rmnLibShutdownCalled = true;
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(DimensionGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(LabeledDimensionGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(SIDimensionGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(SIMonotonicDimensionGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(SILinearDimensionGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(SparseSamplingGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(DependentVariableGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(GeographicCoordinateGetTypeID());
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
    OCReportLeaksForTypeDetailed(DatasetGetTypeID());
#endif
    SITypesShutdown();
}
// If you want automatic teardown when the library is unloaded:
// __attribute__((destructor(100)))
// static void __RMNLibCleanup(void) {
//     RMNLibTypesShutdown();
// }
