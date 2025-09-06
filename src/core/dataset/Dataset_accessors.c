/**
 * @file Dataset_accessors.c
 * @brief Accessor functions for Dataset properties
 *
 * This module contains all getter and setter functions for Dataset properties,
 * providing controlled access to the internal Dataset structure.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../RMNLibrary.h"
#include "Dataset_private.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
#pragma region Dimensions and Dimension Precedence
// ============================================================================

OCMutableArrayRef DatasetGetDimensions(DatasetRef ds) {
    return ds ? ds->dimensions : NULL;
}

bool DatasetSetDimensions(DatasetRef ds, OCMutableArrayRef dims) {
    if (!ds) return false;
    OCRelease(ds->dimensions);
    // Convert NULL to empty array to avoid NULL ivars
    ds->dimensions = dims ? (OCMutableArrayRef)OCRetain(dims)
                          : OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return true;
}

OCMutableIndexArrayRef DatasetGetDimensionPrecedence(DatasetRef ds) {
    return ds ? ds->dimensionPrecedence : NULL;
}

bool DatasetSetDimensionPrecedence(DatasetRef ds, OCMutableIndexArrayRef order) {
    if (!ds) return false;
    OCRelease(ds->dimensionPrecedence);
    // Convert NULL to empty index array to avoid NULL ivars
    ds->dimensionPrecedence = order ? (OCMutableIndexArrayRef)OCRetain(order)
                                   : OCIndexArrayCreateMutable(0);
    return true;
}

// ============================================================================
#pragma region Dependent Variables
// ============================================================================

OCMutableArrayRef DatasetGetDependentVariables(DatasetRef ds) {
    return ds ? ds->dependentVariables : NULL;
}

bool DatasetSetDependentVariables(DatasetRef ds, OCMutableArrayRef dvs) {
    if (!ds) return false;
    OCRelease(ds->dependentVariables);
    // Convert NULL to empty array to avoid NULL ivars
    ds->dependentVariables = dvs ? (OCMutableArrayRef)OCRetain(dvs)
                                 : OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);

    // Set owner for all dependent variables
    OCIndex count = OCArrayGetCount(ds->dependentVariables);
    for (OCIndex i = 0; i < count; ++i) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(ds->dependentVariables, i);
        if (dv) {
            DependentVariableSetOwner(dv, (OCTypeRef)ds);
        }
    }
    return true;
}

// ============================================================================
#pragma region Tags
// ============================================================================

OCMutableArrayRef DatasetGetTags(DatasetRef ds) {
    return ds ? ds->tags : NULL;
}

bool DatasetSetTags(DatasetRef ds, OCMutableArrayRef tags) {
    if (!ds) return false;
    OCRelease(ds->tags);
    // Convert NULL to empty array to avoid NULL ivars
    ds->tags = tags ? (OCMutableArrayRef)OCRetain(tags)
                    : OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    return true;
}

// ============================================================================
#pragma region Description and Title
// ============================================================================

OCStringRef DatasetGetDescription(DatasetRef ds) {
    return ds ? ds->description : NULL;
}

bool DatasetSetDescription(DatasetRef ds, OCStringRef desc) {
    if (!ds) return false;
    OCRelease(ds->description);
    // Convert NULL to empty string to avoid NULL ivars
    ds->description = desc ? OCStringCreateCopy(desc) : STR("");
    return true;
}

OCStringRef DatasetGetTitle(DatasetRef ds) {
    return ds ? ds->title : NULL;
}

bool DatasetSetTitle(DatasetRef ds, OCStringRef title) {
    if (!ds) return false;
    OCRelease(ds->title);
    // Convert NULL to empty string to avoid NULL ivars
    ds->title = title ? OCStringCreateCopy(title) : STR("");
    return true;
}

// ============================================================================
#pragma region Focus
// ============================================================================

DatumRef DatasetGetFocus(DatasetRef ds) {
    return ds ? ds->focus : NULL;
}

bool DatasetSetFocus(DatasetRef ds, DatumRef focus) {
    if (!ds) return false;
    OCRelease(ds->focus);
    ds->focus = focus ? (DatumRef)OCRetain(focus) : NULL;
    return true;
}

DatumRef DatasetGetPreviousFocus(DatasetRef ds) {
    return ds ? ds->previousFocus : NULL;
}

bool DatasetSetPreviousFocus(DatasetRef ds, DatumRef previousFocus) {
    if (!ds) return false;
    OCRelease(ds->previousFocus);
    ds->previousFocus = previousFocus ? (DatumRef)OCRetain(previousFocus) : NULL;
    return true;
}

// ============================================================================
#pragma region Application Metadata
// ============================================================================

OCDictionaryRef DatasetGetApplicationMetaData(DatasetRef ds) {
    return ds ? ds->application : NULL;
}

bool DatasetSetApplicationMetaData(DatasetRef ds, OCDictionaryRef md) {
    if (!ds) return false;
    OCRelease(ds->application);
    // Convert NULL to empty dictionary to avoid NULL ivars
    ds->application = md ? (OCMutableDictionaryRef)OCRetain(md)
                         : OCDictionaryCreateMutable(0);
    return true;
}

// ============================================================================
#pragma region CSDM-1.0 Fields
// ============================================================================

OCStringRef DatasetGetVersion(DatasetRef ds) {
    return ds ? ds->version : NULL;
}

bool DatasetSetVersion(DatasetRef ds, OCStringRef v) {
    if (!ds) return false;
    OCRelease(ds->version);
    // Convert NULL to default version to avoid NULL ivars
    ds->version = v ? OCStringCreateCopy(v) : STR("1.0");
    return ds->version != NULL;
}

OCStringRef DatasetGetTimestamp(DatasetRef ds) {
    return ds ? ds->timestamp : NULL;
}

bool DatasetSetTimestamp(DatasetRef ds, OCStringRef ts) {
    if (!ds) return false;
    OCRelease(ds->timestamp);
    // Convert NULL to current timestamp to avoid NULL ivars
    ds->timestamp = ts ? OCStringCreateCopy(ts) : OCCreateISO8601Timestamp();
    return ds->timestamp != NULL;
}

GeographicCoordinateRef DatasetGetGeographicCoordinate(DatasetRef ds) {
    return ds ? ds->geographicCoordinate : NULL;
}

bool DatasetSetGeographicCoordinate(DatasetRef ds, GeographicCoordinateRef gc) {
    if (!ds) return false;
    OCRelease(ds->geographicCoordinate);
    ds->geographicCoordinate = gc ? (GeographicCoordinateRef)OCRetain(gc) : NULL;
    return true;
}

bool DatasetGetReadOnly(DatasetRef ds) {
    return ds ? ds->readOnly : false;
}

bool DatasetSetReadOnly(DatasetRef ds, bool readOnly) {
    if (!ds) return false;
    ds->readOnly = readOnly;
    return true;
}

#ifdef __cplusplus
}
#endif
