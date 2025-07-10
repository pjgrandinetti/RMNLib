#ifndef DEPENDENTVARIABLE_H
#define DEPENDENTVARIABLE_H
#include "RMNLibrary.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL */
#ifndef DEPENDENT_VARIABLE_KEYS_H
#define DEPENDENT_VARIABLE_KEYS_H

#define kDependentVariableUnitKey                    "unit"
#define kDependentVariableNumericTypeKey             "numeric_type"
#define kDependentVariableNameKey                    "name"
#define kDependentVariableDescriptionKey             "description"
#define kDependentVariableMetaDataKey                "metadata"
#define kDependentVariableQuantityNameKey            "quantity_name"
#define kDependentVariableQuantityTypeKey            "quantity_type"
#define kDependentVariableTypeKey                    "type"
#define kDependentVariableEncodingKey                "encoding"
#define kDependentVariableEncodingValueBase64        "base64"
#define kDependentVariableEncodingValueNone          "none"
#define kDependentVariableComponentsURLKey           "components_url"
#define kDependentVariableComponentsKey              "components"
#define kDependentVariableComponentTypeValueInternal "internal"
#define kDependentVariableComponentTypeValueExternal "external"
#define kDependentVariableComponentLabelsKey         "component_labels"
#define kDependentVariableSparseSamplingKey          "sparse_sampling"

#endif // DEPENDENT_VARIABLE_KEYS_H
/** @endcond */

/**
 * @file DependentVariable.h
 * @brief Public API for DependentVariable: an N-D dataset variable with
 *        support for internal/external storage, multiple components,
 *        sparse sampling, and JSON serialization.
 *
 * A DependentVariable encapsulates:
 *  - a physical unit (SIUnitRef) and numerical type (double, float, complex, etc.)
 *  - one or more binary component buffers (OCDataRef)
 *  - semantic metadata (name, description, quantityName, quantityType)
 *  - optional sparse‐grid sampling support
 *  - dictionary- and JSON-based round-trip serialization
 */
/**
 * @defgroup DependentVariable DependentVariable
 * @brief Object model and operations for a dataset’s dependent variable.
 * @{
 */
/** Opaque handle for a DependentVariable. */
typedef struct impl_DependentVariable *DependentVariableRef;
/**
 * @name Type & Copying
 * @{
 */
/**
 * @brief Retrieve the unique OCTypeID for DependentVariable.
 * @return Type identifier.
 */
OCTypeID
DependentVariableGetTypeID(void);
/**
 * @brief Create a deep (immutable) copy of an existing DependentVariable.
 * @param orig Source object.
 * @return New DependentVariableRef, or NULL on failure.
 */
DependentVariableRef
DependentVariableCreateCopy(DependentVariableRef orig);
/**
 * @brief Like CreateCopy, but ensures the result is complex-typed.
 * @param src   Source object.
 * @param owner Optional back-pointer owner.
 * @return New complex-typed DependentVariableRef.
 */
DependentVariableRef
DependentVariableCreateComplexCopy(DependentVariableRef src,
                                   OCTypeRef owner);
/** @} end of Type & Copying */
/**
 * @name Creation
 * @{
 */
/**
 * @brief Construct an “internal” DependentVariable (data copied).
 * @param name            Optional human-readable name.
 * @param description     Optional longer description.
 * @param unit            SI unit (NULL→dimensionless).
 * @param quantityName    Logical quantity name (e.g. “temperature”).
 * @param quantityType    Semantic type (“scalar”, “vector_N”, etc.).
 * @param elementType     Numeric storage type (kOCNumberFloat64Type, etc.).
 * @param componentLabels Optional array of OCStringRef labels.
 * @param components      Array of OCDataRef buffers.
 * @param outError        Optional pointer for error message.
 * @return New DependentVariableRef or NULL on validation/alloc error.
 */
DependentVariableRef
DependentVariableCreate(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError);
/**
 * @brief Same as Create, but does *not* copy the provided data buffers.
 */
DependentVariableRef
DependentVariableCreateWithComponentsNoCopy(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCArrayRef components,
    OCStringRef *outError);
/**
 * @brief Pre-allocate a new “internal” DependentVariable of given size (zero-filled).
 * @param size Number of elements per component.
 */
DependentVariableRef
DependentVariableCreateWithSize(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCIndex size,
    OCStringRef *outError);
/**
 * @brief Minimal defaults constructor: scalar/pixel/vector by quantityType + size.
 */
DependentVariableRef
DependentVariableCreateDefault(
    OCStringRef quantityType,
    OCNumberType elementType,
    OCIndex size,
    OCStringRef *outError);
/**
 * @brief Single-component convenience constructor.
 */
DependentVariableRef DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCStringRef *outError);
DependentVariableRef DependentVariableCreateExternal(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCStringRef componentsURL,
    OCStringRef *outError);
/**
 * @brief Deserialize from cJSON using internal dictionary logic.
 */
DependentVariableRef
DependentVariableCreateFromJSON(cJSON *json,
                                OCStringRef *outError);
/** @cond INTERNAL */
/**
 * @brief Internal helper: parse a cJSON object into the same
 *        dictionary format produced by DependentVariableCopyAsDictionary().
 * @internal
 */
OCDictionaryRef
DependentVariableDictionaryCreateFromJSON(cJSON *json,
                                         OCStringRef *outError);
/** @endcond */

/**
 * @brief Create an N-D “cross-section” slice at fixed indices.
 */
DependentVariableRef
DependentVariableCreateCrossSection(
    DependentVariableRef dv,
    OCArrayRef dimensions,
    OCIndexPairSetRef indexPairs,
    OCStringRef *outError);
/**
 * @brief Extract sparse-packed components given sparseDimensionIndexes.
 */
OCArrayRef
DependentVariableCreatePackedSparseComponentsArray(
    DependentVariableRef dv,
    OCArrayRef dimensions);
/**
 * @brief Concatenate all component buffers (with sparse packing if needed)
 *        into a single OCDataRef, suitable for CSDM external blobs.
 */
OCDataRef
DependentVariableCreateCSDMComponentsData(
    DependentVariableRef dv,
    OCArrayRef dimensions);
/** @} end of Creation */
/**
 * @name In-place Mutation
 * @{
 */
/**
 * @brief Append another DependentVariable’s data onto the end of this one.
 */
bool DependentVariableAppend(
    DependentVariableRef dv,
    DependentVariableRef appendedDV,
    OCStringRef *outError);
/** @} end of In-place Mutation */
/**
 * @name Serialization
 * @{
 */
/**
 * @brief Serialize into a deep-copyable OCDictionary (for JSON, tests).
 */
OCDictionaryRef
DependentVariableCopyAsDictionary(DependentVariableRef dv);
/**
 * @brief Reconstruct from a dictionary produced by CopyAsDictionary().
 */
DependentVariableRef
DependentVariableCreateFromDictionary(
    OCDictionaryRef dict,
    OCStringRef *outError);
/** @} end of Serialization */
/**
 * @name Basic Accessors
 * @{
 */
bool DependentVariableIsScalarType(DependentVariableRef dv);
bool DependentVariableIsVectorType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsPixelType(DependentVariableRef dv, OCIndex *outCount);
bool DependentVariableIsMatrixType(DependentVariableRef dv, OCIndex *outRows, OCIndex *outCols);
bool DependentVariableIsSymmetricMatrixType(DependentVariableRef dv, OCIndex *outN);
OCIndex DependentVariableComponentsCountFromQuantityType(OCStringRef quantityType);
OCStringRef DependentVariableGetType(DependentVariableRef dv);
bool DependentVariableSetType(DependentVariableRef dv, OCStringRef newType);
bool DependentVariableShouldSerializeExternally(DependentVariableRef dv);
OCStringRef DependentVariableGetEncoding(DependentVariableRef dv);
bool DependentVariableSetEncoding(DependentVariableRef dv, OCStringRef newEnc);
OCStringRef DependentVariableGetComponentsURL(DependentVariableRef dv);
bool DependentVariableSetComponentsURL(DependentVariableRef dv, OCStringRef url);
OCStringRef DependentVariableGetName(DependentVariableRef dv);
bool DependentVariableSetName(DependentVariableRef dv, OCStringRef newName);
OCStringRef DependentVariableGetDescription(DependentVariableRef dv);
bool DependentVariableSetDescription(DependentVariableRef dv, OCStringRef newDesc);
OCStringRef DependentVariableGetQuantityName(DependentVariableRef dv);
bool DependentVariableSetQuantityName(DependentVariableRef dv, OCStringRef quantityName);
OCStringRef DependentVariableGetQuantityType(DependentVariableRef dv);
bool DependentVariableSetQuantityType(DependentVariableRef dv, OCStringRef quantityType);
OCStringRef DependentVariableGetUnitSymbol(DependentVariableRef dv);
OCNumberType DependentVariableGetElementType(DependentVariableRef dv);
bool DependentVariableSetElementType(DependentVariableRef dv, OCNumberType newType);
/**
 * @name Sparse-sampling Accessors
 * @{
 */
/**
 * @brief Get the full SparseSampling metadata for this variable.
 * @param dv    The DependentVariable.
 * @return      A retained SparseSamplingRef, or NULL if none set.
 */
SparseSamplingRef
DependentVariableGetSparseSampling(DependentVariableRef dv);
/**
 * @brief Replace this variable’s SparseSampling metadata.
 * @param dv    The DependentVariable.
 * @param ss    New SparseSampling; may be NULL to clear out.
 * @return      true on success, false on failure.
 */
bool DependentVariableSetSparseSampling(DependentVariableRef dv,
                                        SparseSamplingRef ss);
/** @} end of Sparse-sampling Accessors */
OCDictionaryRef DependentVariableGetMetaData(DependentVariableRef dv);
bool DependentVariableSetMetaData(DependentVariableRef dv, OCDictionaryRef dict);
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv);
bool DependentVariableSetOwner(DependentVariableRef dv, OCTypeRef owner);
/** @} end of Basic Accessors */
/**
 * @name Component-array Accessors
 * @{
 */
OCIndex DependentVariableGetComponentCount(DependentVariableRef dv);
OCMutableArrayRef DependentVariableGetComponents(DependentVariableRef dv);
bool DependentVariableSetComponents(DependentVariableRef dv, OCArrayRef newComponents);
OCMutableArrayRef DependentVariableCopyComponents(DependentVariableRef dv);
OCDataRef DependentVariableGetComponentAtIndex(DependentVariableRef dv, OCIndex idx);
bool DependentVariableSetComponentAtIndex(DependentVariableRef dv, OCDataRef newBuf, OCIndex idx);
bool DependentVariableInsertComponentAtIndex(DependentVariableRef dv, OCDataRef component, OCIndex idx);
bool DependentVariableRemoveComponentAtIndex(DependentVariableRef dv, OCIndex idx);
/** @} end of Component-array Accessors */
/**
 * @name Size & Element Type
 * @{
 */
OCIndex DependentVariableGetSize(DependentVariableRef dv);
bool DependentVariableSetSize(DependentVariableRef dv, OCIndex newSize);
OCNumberType DependentVariableGetElementType(DependentVariableRef dv);
bool DependentVariableSetElementType(DependentVariableRef dv, OCNumberType newType);
/** @} end of Size & Element Type */
/**
 * @name Per-component Labels
 * @{
 */
OCArrayRef DependentVariableGetComponentLabels(DependentVariableRef dv);
bool DependentVariableSetComponentLabels(DependentVariableRef dv, OCArrayRef labels);
OCStringRef DependentVariableCreateComponentLabelForIndex(DependentVariableRef dv, OCIndex idx);
OCStringRef DependentVariableGetComponentLabelAtIndex(DependentVariableRef dv, OCIndex idx);
bool DependentVariableSetComponentLabelAtIndex(DependentVariableRef dv, OCStringRef newLabel, OCIndex idx);
/** @} end of Per-component Labels */
/**
 * @name Low-level Value Accessors
 * @{
 */
float DependentVariableGetFloatValueAtMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset);
double DependentVariableGetDoubleValueAtMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset);
float complex DependentVariableGetFloatComplexValueAtMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset);
double complex DependentVariableGetDoubleComplexValueAtMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset);
double DependentVariableGetDoubleValueAtMemOffsetForPart(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset, complexPart part);
float DependentVariableGetFloatValueAtMemOffsetForPart(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset, complexPart part);
SIScalarRef DependentVariableCreateValueFromMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset);
bool DependentVariableSetValueAtMemOffset(DependentVariableRef dv, OCIndex compIdx, OCIndex memOffset, SIScalarRef value, OCStringRef *error);
/** @} end of Low-level Value Accessors */
/** @} end of DependentVariable group */
#ifdef __cplusplus
}
#endif
#endif  // DEPENDENTVARIABLE_H
