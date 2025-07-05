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
 * @name Type & Copying
 * @{
 */
/** Returns the unique OCTypeID for DependentVariable. */
OCTypeID
DependentVariableGetTypeID(void);
/** Deep (immutable) copy. */
DependentVariableRef
DependentVariableCreateCopy(DependentVariableRef orig);
/** Like CreateCopy, but result is guaranteed complex-typed. */
DependentVariableRef
DependentVariableCreateComplexCopy(DependentVariableRef src,
                                   OCTypeRef owner);
/** @} */
/**
 * @name Creation
 * @{
 */
/** Basic internal‐storage constructor (copies blobs). */
DependentVariableRef
DependentVariableCreate(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError);
/** Like Create, but does NOT copy the provided data buffers. */
DependentVariableRef
DependentVariableCreateWithComponentsNoCopy(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError);
/** Pre-allocate to `size` (zero-filled). */
DependentVariableRef
DependentVariableCreateWithSize(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCIndex size,
    OCStringRef *outError);
/** Minimal “defaults” constructor (scalar/pixel vectors, caller supplies only size). */
DependentVariableRef
DependentVariableCreateDefault(
    OCStringRef quantityType,
    SINumberType elementType,
    OCIndex size,
    OCStringRef *outError);
/** Single‐component convenience. */
DependentVariableRef
DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    SINumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCStringRef *outError);
/**
 * Create a “cross‐section” of a multi-DIM sampling over a specified fixed
 * `indexPairs`.  On error, returns NULL and sets `*outError` (if non-NULL).
 */
DependentVariableRef
DependentVariableCreateCrossSection(
    DependentVariableRef dv,
    OCArrayRef dimensions,
    OCIndexPairSetRef indexPairs,
    OCStringRef *outError);
/**
 * Produce an array of packed, sparse‐sampled component‐data buffers for
 * this DV (using its sparseDimensionIndexes/vertexes).
 */
OCArrayRef
DependentVariableCreatePackedSparseComponentsArray(
    DependentVariableRef dv,
    OCArrayRef dimensions);
/**
 * Concatenate all components (packing sparse data first if any) into a single
 * CSDM-style data blob.  Returns NULL on error.
 */
OCDataRef
DependentVariableCreateCSDMComponentsData(
    DependentVariableRef dv,
    OCArrayRef dimensions);
/** @} */
/**
 * @name In-place mutation
 * @{
 */
/**
 * Append `appendedDV` onto the end of `dv`.  On error returns false and
 * sets `*outError` (if non-NULL).
 */
bool DependentVariableAppend(
    DependentVariableRef dv,
    DependentVariableRef appendedDV,
    OCStringRef *outError);
/** @} */
/**
 * @name Serialization
 * @{
 */
/** Shallow→deep dictionary serializer (for JSON, round-trip & tests). */
OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv);
/**
 * Reconstruct from a dictionary produced by CopyAsDictionary(…).
 * On failure returns NULL and sets `*outError` (if non-NULL).
 */
DependentVariableRef
DependentVariableCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError);
/** @} */
/**
 * @name Basic Accessors
 * @{
 */
bool DependentVariableIsScalarType(DependentVariableRef dv);
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsMatrixType(DependentVariableRef dv, OCIndex *outRows, OCIndex *outCols);
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv, OCIndex *outN);
OCStringRef
DependentVariableGetType(DependentVariableRef dv);
bool DependentVariableSetType(DependentVariableRef dv,
                              OCStringRef newType);
OCStringRef
DependentVariableGetEncoding(DependentVariableRef dv);
bool DependentVariableSetEncoding(DependentVariableRef dv,
                                  OCStringRef newEnc);
OCStringRef
DependentVariableGetComponentsURL(DependentVariableRef dv);
bool DependentVariableSetComponentsURL(DependentVariableRef dv,
                                       OCStringRef url);
OCStringRef
DependentVariableGetName(DependentVariableRef dv);
bool DependentVariableSetName(DependentVariableRef dv,
                              OCStringRef newName);
OCStringRef
DependentVariableGetDescription(DependentVariableRef dv);
bool DependentVariableSetDescription(DependentVariableRef dv,
                                     OCStringRef newDesc);
OCStringRef
DependentVariableGetQuantityName(DependentVariableRef dv);
bool DependentVariableSetQuantityName(DependentVariableRef dv,
                                      OCStringRef quantityName);
OCIndex DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType);
OCStringRef
DependentVariableGetQuantityType(DependentVariableRef dv);
bool DependentVariableSetQuantityType(DependentVariableRef dv,
                                      OCStringRef quantityType);
OCIndexSetRef
DependentVariableGetSparseDimensionIndexes(DependentVariableRef dv);
bool DependentVariableSetSparseDimensionIndexes(
    DependentVariableRef dv,
    OCIndexSetRef idxSet);
OCArrayRef
DependentVariableGetSparseGridVertexes(DependentVariableRef dv);
bool DependentVariableSetSparseGridVertexes(
    DependentVariableRef dv,
    OCArrayRef verts);
OCDictionaryRef
DependentVariableGetMetaData(DependentVariableRef dv);
bool DependentVariableSetMetaData(DependentVariableRef dv,
                                  OCDictionaryRef dict);
                                  
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv);
bool DependentVariableSetOwner(DependentVariableRef dv, OCTypeRef owner);
/** @} */
/**
 * @name Component-array Accessors
 * @{
 */
OCIndex
DependentVariableGetComponentCount(DependentVariableRef dv);
OCMutableArrayRef
DependentVariableGetComponents(DependentVariableRef dv);
OCMutableArrayRef
DependentVariableCopyComponents(DependentVariableRef dv);
OCDataRef
DependentVariableGetComponentAtIndex(DependentVariableRef dv,
                                     OCIndex idx);
bool DependentVariableSetComponentAtIndex(DependentVariableRef dv,
                                          OCDataRef newBuf,
                                          OCIndex idx);
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv,
                                             OCDataRef component,
                                             OCIndex idx);
bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv,
                                             OCIndex idx);
/** @} */
/**
 * @name Size & Element-type
 * @{
 */
/** Number of elements per component‐buffer. */
OCIndex
DependentVariableGetSize(DependentVariableRef dv);
/** Resize all buffers (zero–fill any new tail). */
bool DependentVariableSetSize(DependentVariableRef dv,
                              OCIndex newSize);
SINumberType
DependentVariableGetElementType(DependentVariableRef dv);
bool DependentVariableSetElementType(DependentVariableRef dv,
                                     SINumberType newType);
/** @} */
/**
 * @name Per-component Labels
 * @{
 */
OCArrayRef
DependentVariableGetComponentLabels(DependentVariableRef dv);
bool DependentVariableSetComponentLabels(DependentVariableRef dv,
                                         OCArrayRef labels);
OCStringRef
DependentVariableCreateComponentLabelForIndex(
    DependentVariableRef dv,
    OCIndex idx);
OCStringRef
DependentVariableGetComponentLabelAtIndex(
    DependentVariableRef dv,
    OCIndex idx);
bool DependentVariableSetComponentLabelAtIndex(
    DependentVariableRef dv,
    OCStringRef newLabel,
    OCIndex idx);
/** @} */
/**
 * @name Low-level value accessors
 * @{
 */
float DependentVariableGetFloatValueAtMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset);
double DependentVariableGetDoubleValueAtMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset);
float complex DependentVariableGetFloatComplexValueAtMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset);
double complex DependentVariableGetDoubleComplexValueAtMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset);
double DependentVariableGetDoubleValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset,
    complexPart part);
float DependentVariableGetFloatValueAtMemOffsetForPart(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset,
    complexPart part);
SIScalarRef DependentVariableCreateValueFromMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset);
bool DependentVariableSetValueAtMemOffset(
    DependentVariableRef dv,
    OCIndex compIdx,
    OCIndex memOffset,
    SIScalarRef value,
    OCStringRef *error);
/** @} */
/** @} */  // end of DependentVariable group
#endif     /* DEPENDENTVARIABLE_H */
