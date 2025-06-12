#pragma once
#ifndef DEPENDENTVARIABLE_H
#define DEPENDENTVARIABLE_H
#include "RMNLibrary.h"
/**
 * @file DependentVariable.h
 * @brief Public API for DependentVariable
 */
/**
 * @defgroup DependentVariable DependentVariable
 * @brief Object model for a dependent variable.
 * @{
 */
/** Opaque handle to a DependentVariable instance. */
typedef struct impl_DependentVariable *DependentVariableRef;
/**
 * @brief Returns the unique OCTypeID for DependentVariable.
 */
OCTypeID DependentVariableGetTypeID(void);
/**
 * @brief Create a new DependentVariable.
 *
 * @param name                    name of the dependent variable
 * @param components              array of SIScalarRef values (component data)
 * @param componentLabels         parallel array of OCStringRef labels
 * @param quantityName            SI quantity name (e.g. "meter", "second")
 * @param quantityType            SI quantity type (e.g. "length", "time")
 * @param description             human-readable description
 * @param sparseDimensionIndexes  an OCIndexSetRef indicating which dims are sparse
 * @param sparseGridVertexes      serialized representation of the sparse grid
 * @param metadata                an (optional) mutable dictionary of user metadata
 * @param dataset                 back-pointer to the parent DatasetRef
 */
DependentVariableRef
DependentVariableCreate(
    OCStringRef name,
    OCArrayRef components,
    OCArrayRef componentLabels,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCStringRef description,
    OCIndexSetRef sparseDimensionIndexes,
    OCStringRef sparseGridVertexes,
    OCMutableDictionaryRef metadata);
/**
 * @brief Deep (immutable) copy of a DependentVariable.
 */
DependentVariableRef DependentVariableCreateCopy(DependentVariableRef orig);
/**
 * @name Accessors
 * @{
 */
OCStringRef DependentVariableGetName(DependentVariableRef dv);
OCArrayRef DependentVariableGetComponents(DependentVariableRef dv);
OCArrayRef DependentVariableGetComponentLabels(DependentVariableRef dv);
OCStringRef DependentVariableGetQuantityName(DependentVariableRef dv);
OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv);
OCStringRef DependentVariableGetDescription(DependentVariableRef dv);
OCIndexSetRef DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv);
OCStringRef DependentVariableGetSparseGridVertexes(DependentVariableRef dv);
OCDictionaryRef DependentVariableGetMetadata(DependentVariableRef dv);
OCTypeRef DependentVariableGetDataset(DependentVariableRef dv);
OCDictionaryRef DependentVariableCopyAsDictionary(DependentVariableRef dv);
DependentVariableRef DependentVariableCreateFromDictionary(OCDictionaryRef dict);
/** @} */
/** @} */  // end of DependentVariable group
#endif     /* DEPENDENTVARIABLE_H */
