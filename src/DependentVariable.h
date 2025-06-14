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

typedef enum formatType {
    kDependentVariableText,
    kDependentVariableBinary
} formatType;

/**
 * @name Type & Copying
 * @{
 */

/**
 * @brief Returns the unique OCTypeID for DependentVariable.
 */
OCTypeID DependentVariableGetTypeID(void);

/**
 * @brief Deep (immutable) copy of a DependentVariable.
 */
DependentVariableRef DependentVariableCreateCopy(DependentVariableRef orig);

/**
 * @brief Like CreateCopy, but guarantees the result is in a complex number type.
 */
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

/**
 * @brief Serialize to a mutable dictionary (for deep-copy & testing).
 */
OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv);

/**
 * @brief Reconstruct a DependentVariable from a dictionary.
 */
DependentVariableRef
DependentVariableCreateFromDictionary(OCDictionaryRef dict);

/** @} */

/**
 * @name Type Queries
 * @{
 */

/**
 * @brief Is this a “scalar” quantityType?
 */
bool DependentVariableIsScalarType(DependentVariableRef dv);

/**
 * @brief Is this a “vector_N” quantityType? If so, *outCount = N.
 */
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount);

/**
 * @brief Is this a “pixel_N” quantityType? If so, *outCount = N.
 */
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount);

/**
 * @brief Is this a “matrix_R_C” quantityType? If so, *outRows = R, *outCols = C.
 */
bool DependentVariableIsMatrixType(DependentVariableRef dv,
                                   OCIndex            *outRows,
                                   OCIndex            *outCols);

/**
 * @brief Is this a “symmetric_matrix_N” quantityType? If so, *outN = N.
 */
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv,
                                            OCIndex            *outN);

/**
 * @brief Given a quantityType string, return the expected number of components.
 */
OCIndex
DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType);

/**
 * @brief Build an array of all valid quantityType strings matching the current component count.
 */
OCMutableArrayRef
DependentVariableCreateQuantityTypesArray(DependentVariableRef dv);

/** @} */

/**
 * @name Basic Accessors
 * @{
 */

/** Name & description */
OCStringRef DependentVariableGetName(DependentVariableRef dv);
bool         DependentVariableSetName(DependentVariableRef dv, OCStringRef newName);

OCStringRef DependentVariableGetDescription(DependentVariableRef dv);
bool         DependentVariableSetDescription(DependentVariableRef dv, OCStringRef newDesc);

/** Quantity metadata */
OCStringRef DependentVariableGetQuantityName(DependentVariableRef dv);
bool         DependentVariableSetQuantityName(DependentVariableRef dv, OCStringRef quantityName);

OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv);
bool         DependentVariableSetQuantityType(DependentVariableRef dv, OCStringRef quantityType);

/** Sparse‐grid support */
OCIndexSetRef DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv);
bool           DependentVariableSetSparseDimensionIndexes(DependentVariableRef dv,
                                                           OCIndexSetRef        idxSet);

OCStringRef DependentVariableGetSparseGridVertexes(DependentVariableRef dv);
bool         DependentVariableSetSparseGridVertexes(DependentVariableRef dv, OCStringRef verts);

/** User‐metadata dictionary */
OCDictionaryRef DependentVariableGetMetaData(DependentVariableRef dv);
bool            DependentVariableSetMetaData(DependentVariableRef dv,
                                             OCDictionaryRef      dict);

/** Weak back‐pointer to owning dataset */
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv);

/** @} */

/**
 * @name Component‐array Accessors
 * @{
 */

/** Number of component‐buffers */
OCIndex           DependentVariableGetComponentCount(DependentVariableRef dv);

/** Direct mutable array of OCDataRef buffers */
OCMutableArrayRef DependentVariableGetComponents(DependentVariableRef dv);

/** Deep-copy of the array of buffers */
OCMutableArrayRef DependentVariableCopyComponents(DependentVariableRef dv);

/** Get/replace a single buffer (must match length) */
OCDataRef DependentVariableGetComponentAtIndex(DependentVariableRef dv, OCIndex idx);
bool      DependentVariableSetComponentAtIndex(DependentVariableRef dv,
                                               OCDataRef             newBuf,
                                               OCIndex               idx);

/** Insert/remove entire buffers (and update quantityType accordingly) */
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv,
                                             OCDataRef             component,
                                             OCIndex               idx);

bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv,
                                             OCIndex               idx);

/** @} */

/**
 * @name Size & Element‐type
 * @{
 */

/** Number of elements in each buffer */
OCIndex DependentVariableGetSize(DependentVariableRef dv);

/** Resize all buffers to this many elements (zero‐fills new tail) */
bool    DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize);

/** Float/complex type of each element */
SINumberType DependentVariableGetElementType(DependentVariableRef dv);
bool         DependentVariableSetElementType(DependentVariableRef dv,
                                              SINumberType         newType);

/** @} */

/**
 * @name Per-component Labels
 * @{
 */

/** Build a human label for one component, e.g. “velocity : x” */
OCStringRef DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv,
                                                          OCIndex               idx);

/** Direct get/set on the label array */
OCStringRef DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv,
                                                      OCIndex               idx);

bool         DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv,
                                                        OCStringRef           newLabel,
                                                        OCIndex               idx);

/** @} */

/**
 * @name Low-level value accessors
 * @{
 */

/** Read a raw element as a float/double/complex */
float         DependentVariableGetFloatValueAtMemOffset(DependentVariableRef dv,
                                                         OCIndex               compIdx,
                                                         OCIndex               memOffset);
double        DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv,
                                                          OCIndex               compIdx,
                                                          OCIndex               memOffset);
float complex DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv,
                                                               OCIndex               compIdx,
                                                               OCIndex               memOffset);
double complex DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv,
                                                                 OCIndex               compIdx,
                                                                 OCIndex               memOffset);

/** Read a real‐part, imag‐part, magnitude or phase from a complex buffer */
double DependentVariableGetDoubleValueAtMemOffsetForPart(DependentVariableRef dv,
                                                         OCIndex               compIdx,
                                                         OCIndex               memOffset,
                                                         complexPart           part);
float  DependentVariableGetFloatValueAtMemOffsetForPart(DependentVariableRef dv,
                                                        OCIndex               compIdx,
                                                        OCIndex               memOffset,
                                                        complexPart           part);

/** Wrap raw data back into an SIScalar */
SIScalarRef DependentVariableCreateValueFromMemOffset(DependentVariableRef dv,
                                                      OCIndex               compIdx,
                                                      OCIndex               memOffset);

/**
 * @brief Overwrite one element from an SIScalar
 * @param error optional out‐string on dimensional‐mismatch
 */
bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv,
                                          OCIndex               compIdx,
                                          OCIndex               memOffset,
                                          SIScalarRef           value,
                                          OCStringRef          *error);

                                          OCIndex
DependentVariableGetSize(DependentVariableRef dv);


/** @} */

/** @} */  // end of DependentVariable group

#endif /* DEPENDENTVARIABLE_H */
