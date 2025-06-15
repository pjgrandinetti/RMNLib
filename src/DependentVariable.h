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

/** Text or binary export formats. */
typedef enum formatType {
    kDependentVariableText,
    kDependentVariableBinary
} formatType;

/**
 * @name Type & Copying
 * @{
 */

/** Returns the unique OCTypeID for DependentVariable. */
OCTypeID DependentVariableGetTypeID(void);

/** Deep (immutable) copy. */
DependentVariableRef DependentVariableCreateCopy(DependentVariableRef orig);

/** Like CreateCopy, but result is guaranteed complex-typed. */
DependentVariableRef DependentVariableCreateComplexCopy(DependentVariableRef src,
                                                       OCTypeRef           owner);
/** @} */

/**
 * @name Creation
 * @{
 */
DependentVariableRef
DependentVariableCreate(
    OCStringRef    name,
    OCStringRef    description,
    SIUnitRef      unit,
    OCStringRef    quantityName,
    OCStringRef    quantityType,
    SINumberType   elementType,
    OCArrayRef     componentLabels,
    OCArrayRef     components,
    OCTypeRef      owner);

DependentVariableRef
DependentVariableCreateWithComponentsNoCopy(
    OCStringRef    name,
    OCStringRef    description,
    SIUnitRef      unit,
    OCStringRef    quantityName,
    OCStringRef    quantityType,
    SINumberType   elementType,
    OCArrayRef     componentLabels,
    OCArrayRef     components,
    OCTypeRef      owner);

DependentVariableRef
DependentVariableCreateWithSize(
    OCStringRef    name,
    OCStringRef    description,
    SIUnitRef      unit,
    OCStringRef    quantityName,
    OCStringRef    quantityType,
    SINumberType   elementType,
    OCArrayRef     componentLabels,
    OCIndex        size,
    OCTypeRef      owner);

DependentVariableRef
DependentVariableCreateDefault(
    OCStringRef    quantityType,
    SINumberType   elementType,
    OCIndex        size,
    OCTypeRef      owner);

DependentVariableRef
DependentVariableCreateWithComponent(
    OCStringRef    name,
    OCStringRef    description,
    SIUnitRef      unit,
    OCStringRef    quantityName,
    SINumberType   elementType,
    OCArrayRef     componentLabels,
    OCDataRef      component,
    OCTypeRef      owner);
/** @} */

/**
 * @name Serialization
 * @{
 */
/** Serialize to dictionary (for deep-copy & tests). */
OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv);

/** Reconstruct from dictionary. */
DependentVariableRef
DependentVariableCreateFromDictionary(OCDictionaryRef dict);
/** @} */

/**
 * @name Type Queries
 * @{
 */
bool DependentVariableIsScalarType(DependentVariableRef dv);
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsMatrixType(DependentVariableRef dv, OCIndex *outRows, OCIndex *outCols);
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv, OCIndex *outN);

/** Expected component count from a quantity-type string. */
OCIndex
DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType);

/** All valid `"scalar"|"vector_N"|…` choices for current component count. */
OCMutableArrayRef
DependentVariableCreateQuantityTypesArray(DependentVariableRef dv);
/** @} */

/**
 * @name Basic Accessors
 * @{
 */
OCStringRef        DependentVariableGetName(DependentVariableRef dv);
bool               DependentVariableSetName(DependentVariableRef dv, OCStringRef newName);

OCStringRef        DependentVariableGetDescription(DependentVariableRef dv);
bool               DependentVariableSetDescription(DependentVariableRef dv, OCStringRef newDesc);

OCStringRef        DependentVariableGetQuantityName(DependentVariableRef dv);
bool               DependentVariableSetQuantityName(DependentVariableRef dv, OCStringRef quantityName);

OCStringRef        DependentVariableGetQuantityType(DependentVariableRef dv);
bool               DependentVariableSetQuantityType(DependentVariableRef dv, OCStringRef quantityType);

OCIndexSetRef      DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv);
bool               DependentVariableSetSparseDimensionIndexes(DependentVariableRef dv, OCIndexSetRef idxSet);

OCStringRef        DependentVariableGetSparseGridVertexes(DependentVariableRef dv);
bool               DependentVariableSetSparseGridVertexes(DependentVariableRef dv, OCStringRef verts);

OCDictionaryRef    DependentVariableGetMetaData(DependentVariableRef dv);
bool               DependentVariableSetMetaData(DependentVariableRef dv, OCDictionaryRef dict);

OCTypeRef          DependentVariableGetOwner(DependentVariableRef dv);
/** @} */

/**
 * @name Component-array Accessors
 * @{
 */
OCIndex           DependentVariableGetComponentCount(DependentVariableRef dv);
OCMutableArrayRef DependentVariableGetComponents(DependentVariableRef dv);
OCMutableArrayRef DependentVariableCopyComponents(DependentVariableRef dv);

OCDataRef         DependentVariableGetComponentAtIndex(DependentVariableRef dv, OCIndex idx);
bool              DependentVariableSetComponentAtIndex(DependentVariableRef dv,
                                                        OCDataRef newBuf,
                                                        OCIndex idx);

bool              DependentVariableInsertComponentAtIndex(DependentVariableRef dv,
                                                           OCDataRef component,
                                                           OCIndex idx);
bool              DependentVariableRemoveComponentAtIndex(DependentVariableRef dv,
                                                           OCIndex idx);
/** @} */

/**
 * @name Size & Element-type
 * @{
 */
/** How many elements in each component buffer? */
OCIndex           DependentVariableGetSize(DependentVariableRef dv);

/** Resize all buffers (zero-filling any new tail). */
bool              DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize);

SINumberType      DependentVariableGetElementType(DependentVariableRef dv);
bool              DependentVariableSetElementType(DependentVariableRef dv, SINumberType newType);
/** @} */

/**
 * @name Per-component Labels
 * @{
 */
OCStringRef       DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv, OCIndex idx);

OCStringRef       DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv, OCIndex idx);
bool              DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv,
                                                             OCStringRef newLabel,
                                                             OCIndex idx);
/** @} */

/**
 * @name Low-level value accessors
 * @{
 */
float         DependentVariableGetFloatValueAtMemOffset(DependentVariableRef dv,
                                                         OCIndex compIdx,
                                                         OCIndex memOffset);
double        DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv,
                                                          OCIndex compIdx,
                                                          OCIndex memOffset);
float complex DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv,
                                                               OCIndex compIdx,
                                                               OCIndex memOffset);
double complex DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv,
                                                                 OCIndex compIdx,
                                                                 OCIndex memOffset);

double DependentVariableGetDoubleValueAtMemOffsetForPart(DependentVariableRef dv,
                                                         OCIndex compIdx,
                                                         OCIndex memOffset,
                                                         complexPart part);
float  DependentVariableGetFloatValueAtMemOffsetForPart(DependentVariableRef dv,
                                                        OCIndex compIdx,
                                                        OCIndex memOffset,
                                                        complexPart part);

SIScalarRef DependentVariableCreateValueFromMemOffset(DependentVariableRef dv,
                                                      OCIndex compIdx,
                                                      OCIndex memOffset);

bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv,
                                          OCIndex compIdx,
                                          OCIndex memOffset,
                                          SIScalarRef value,
                                          OCStringRef *error);
/** @} */

/** @} */  // end of DependentVariable group

#endif /* DEPENDENTVARIABLE_H */
