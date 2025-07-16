#ifndef DEPENDENTVARIABLE_H
#define DEPENDENTVARIABLE_H
#include "RMNLibrary.h"
#ifdef __cplusplus
extern "C" {
#endif
/** @cond INTERNAL */
#ifndef DEPENDENT_VARIABLE_KEYS_H
#define DEPENDENT_VARIABLE_KEYS_H
#define kDependentVariableUnitKey "unit"
#define kDependentVariableNumericTypeKey "numeric_type"
#define kDependentVariableNameKey "name"
#define kDependentVariableDescriptionKey "description"
#define kDependentVariableMetaDataKey "application"
#define kDependentVariableQuantityNameKey "quantity_name"
#define kDependentVariableQuantityTypeKey "quantity_type"
#define kDependentVariableTypeKey "type"
#define kDependentVariableEncodingKey "encoding"
#define kDependentVariableEncodingValueBase64 "base64"
#define kDependentVariableEncodingValueNone "none"
#define kDependentVariableEncodingValueRaw "raw"
#define kDependentVariableComponentsURLKey "components_url"
#define kDependentVariableComponentsKey "components"
#define kDependentVariableComponentTypeValueInternal "internal"
#define kDependentVariableComponentTypeValueExternal "external"
#define kDependentVariableComponentLabelsKey "component_labels"
#define kDependentVariableSparseSamplingKey "sparse_sampling"
// Internal core‐creator signature, updated to take encoding, componentsURL,
// sparseSampling and metaData explicitly:
static DependentVariableRef
impl_DependentVariableCreate(
    OCStringRef type, /* "internal" or "external" */
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCNumberType elementType,
    OCStringRef encoding, /* "none" or "base64" */
    OCStringRef componentsURL,
    OCArrayRef components,
    bool copyComponents,
    OCIndex explicitSize,
    OCArrayRef componentLabels,
    SparseSamplingRef sparseSampling,
    bool copySparseSampling,
    OCDictionaryRef metaData,
    OCStringRef *outError);
#endif  // DEPENDENT_VARIABLE_KEYS_H
/** @endcond */
/**
 * @file DependentVariable.h
 * @brief Public API for DependentVariable: an N-D dataset variable with
 *        support for internal/external storage, multiple components,
 *        sparse sampling, and JSON serialization.
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
 * @param name            Optional human-readable name.
 * @param description     Optional longer description.
 * @param unit            SI unit (NULL→dimensionless).
 * @param quantityName    Logical quantity name.
 * @param quantityType    Semantic type.
 * @param elementType     Numeric storage type.
 * @param componentLabels Optional labels.
 * @param size            Number of elements per component.
 * @param outError        Optional pointer for error message.
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
DependentVariableRef
DependentVariableCreateWithComponent(
    OCStringRef name,
    OCStringRef description,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCNumberType elementType,
    OCArrayRef componentLabels,
    OCDataRef component,
    OCStringRef *outError);
/**
 * @brief Construct an “external” DependentVariable (blobs fetched from URL).
 */
DependentVariableRef
DependentVariableCreateExternal(
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
DependentVariableCreateFromJSON(
    cJSON *json,
    OCStringRef *outError);
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
/**
 * @brief Create a dictionary from cJSON for DependentVariable deserialization.
 */
OCDictionaryRef
DependentVariableDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
/**
 * @brief Create a packed binary data blob for CSDM serialization.
 */
OCDataRef
DependentVariableCreateCSDMComponentsData(DependentVariableRef dv,
                                          OCArrayRef dimensions);
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
/** @} end of Basic Accessors */
/**
 * @name Sparse-sampling Accessors
 * @{
 */
SparseSamplingRef
DependentVariableGetSparseSampling(DependentVariableRef dv);
bool DependentVariableSetSparseSampling(DependentVariableRef dv,
                                        SparseSamplingRef ss);
/** @} end of Sparse-sampling Accessors */
OCDictionaryRef DependentVariableGetMetaData(DependentVariableRef dv);
bool DependentVariableSetMetaData(DependentVariableRef dv, OCDictionaryRef dict);
OCTypeRef DependentVariableGetOwner(DependentVariableRef dv);
bool DependentVariableSetOwner(DependentVariableRef dv, OCTypeRef owner);
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
/**
 * @brief Convert all component data in a dependent variable to a new unit.
 *        Integer‐typed dependent variables cannot be converted and will error.
 * @param dv      The dependent variable to convert.
 * @param unit    The target unit (must have the same reduced dimensionality).
 * @param error   On failure, receives an OCStringRef describing the problem; may be NULL.
 *                   Caller should release *error if non-NULL.
 * @return true on success, false on error.
 * @ingroup RMNLib
 */
bool DependentVariableConvertToUnit(DependentVariableRef dv,
                                    SIUnitRef unit,
                                    OCStringRef *error);
/**
 * @brief Efficiently set all data values in a dependent variable (or a single component) to zero.
 *        Uses a single memset per component for maximum performance.
 * @param dv             The dependent variable whose data will be zeroed.
 * @param componentIndex Index of the component to zero; use -1 to zero all components.
 * @return true on success, false on error (e.g. no components or index out of range).
 * @ingroup RMNLib
 */
bool DependentVariableSetValuesToZero(DependentVariableRef dv,
                                      int64_t componentIndex);
/**
 * @brief Replace each value in a dependent variable (or a single component) by its absolute value.
 *        Signed integers become their magnitude; floats use fabs/fabsf; complex values
 *        are replaced by their magnitude (imaginary part dropped) and the elementType is
 *        updated to the corresponding real type.
 * @param dv             The dependent variable to process.
 * @param componentIndex Index of the component to process; use –1 to process all components.
 * @return true on success, false on error (e.g. no components or index out of range).
 * @ingroup RMNLib
 */
/**
 * @brief Zeroes out a specific component (real, imaginary, magnitude, or argument)
 *        of a DependentVariable’s data over a given index range.
 *
 * @param dv               The DependentVariable to modify.
 * @param componentIndex   Index of the component to operate on (0-based).
 *                         If negative, all components are processed.
 * @param range            The code-point range [location, location+length)
 *                         within each component to zero.
 * @param part             Which part of the value to zero:
 *                         - kRealPart       : zero the real part
 *                         - kImaginaryPart  : zero the imaginary part
 *                         - kMagnitudePart  : zero both parts
 *                         - kArgumentPart   : replace with magnitude only
 *
 * @return true if the operation succeeded on the specified component(s) and range;
 *         false if the input is invalid or the element type is unsupported.
 *
 * @ingroup DependentVariable
 *
 * @code
 * // Zero the real portion of component 2 between indices 100 and 199:
 * OCRange r = { .location = 100, .length = 100 };
 * bool ok = DependentVariableZeroPartInRange(myDV, 2, r, kRealPart);
 * @endcode
 */
bool DependentVariableZeroPartInRange(DependentVariableRef dv,
                                      OCIndex componentIndex,
                                      OCRange range,
                                      complexPart part);
bool DependentVariableTakeAbsoluteValue(DependentVariableRef dv,
                                        int64_t componentIndex);
/**
 * @brief Multiply each value in a dependent variable (or a single component) by a dimensionless complex constant.
 *        Uses BLAS level-1 routines for optimized real and complex scaling; falls back to simple loops for integer types.
 * @param dv             The dependent variable whose data will be modified.
 * @param componentIndex Index of the component to process; use -1 to process all components.
 * @param constant       The dimensionless complex constant to multiply each element by.
 * @return true on success, false on error (e.g. no components or index out of range).
 * @ingroup RMNLib
 */
bool DependentVariableMultiplyValuesByDimensionlessComplexConstant(DependentVariableRef dv,
                                                                   int64_t componentIndex,
                                                                   double complex constant);
/**
 * @brief Extracts a specific complex component (real, imaginary, magnitude, or argument)
 *        from a DependentVariable’s data, replacing each value accordingly.
 *
 * @param dv               The DependentVariable to modify.
 * @param componentIndex   Index of the component to operate on (0-based).
 *                         If negative, the operation applies to all components in sequence.
 * @param part             Which part of each element to retain:
 *                         - kSIRealPart      : keep real part, zero imaginary
 *                         - kSIImaginaryPart : keep imaginary part, zero real
 *                         - kSIMagnitudePart : replace with magnitude (abs)
 *                         - kSIArgumentPart  : replace with argument (phase)
 *
 * @return true if the data was successfully transformed; false if inputs are invalid
 *         or the variable’s numeric type does not support the requested component.
 *
 * @ingroup DependentVariable
 *
 * @code
 * // Convert component 1 of myDV to its magnitude values:
 * bool ok = DependentVariableTakeComplexPart(myDV, 1, kSIMagnitudePart);
 * @endcode
 */
bool DependentVariableTakeComplexPart(DependentVariableRef dv,
                                      OCIndex componentIndex,
                                      complexPart part);
/**
 * @brief Conjugates the complex values of one or all components in a DependentVariable.
 *
 * For each complex element in the specified component (or all components if
 * componentIndex is negative), this function replaces z = x + i·y with its
 * complex conjugate \f$\bar{z} = x - i·y\f$ by negating the imaginary part in-place.
 * Real-valued components are left unchanged.
 *
 * @param dv
 *   A valid DependentVariableRef whose data you wish to conjugate.
 *   Must not be NULL.
 *
 * @param componentIndex
 *   Index of the component to conjugate.
 *   - If \c componentIndex is in \f$[0,\,n-1)\f$, only that component is processed.
 *   - If \c componentIndex is negative, all components are processed.
 *   - If \c componentIndex ≥ number of components, the function returns \c false.
 *
 * @return
 *   \c true on success (even if there was nothing to do for real-only data),
 *   \c false if \c dv is NULL, \c componentIndex is out of range, or
 *   the data type is not supported.
 */
bool DependentVariableConjugate(DependentVariableRef dv,
                                OCIndex componentIndex);
/**
 * Multiply each element in one (or all) components of a DependentVariable
 * by a dimensionless real constant.
 *
 * This function supports signed and unsigned integer types (8/16/32/64-bit),
 * single- and double-precision real types, and single- and double-precision
 * complex types.  For integer types, each element is cast to double, scaled,
 * then cast back.  Real and complex types are scaled via the corresponding
 * CBLAS “scal” routines for maximum performance.
 *
 * @param dv
 *   The DependentVariable whose data will be modified in place.
 *   Must be non-NULL and have at least one component.
 *
 * @param componentIndex
 *   The index of the component to scale.  If >= 0, only that component is affected.
 *   If negative, all components in dv are scaled.
 *
 * @param constant
 *   The real, dimensionless scalar by which to multiply each element.
 *
 * @return
 *   true if dv was valid, componentIndex in range, and scaling completed;
 *   false otherwise (e.g. dv == NULL, no components, or unsupported element type).
 */
bool DependentVariableMultiplyValuesByDimensionlessRealConstant(DependentVariableRef dv,
                                                                OCIndex componentIndex,
                                                                double constant);
/** @} end of DependentVariable group */
#ifdef __cplusplus
}
#endif
#endif  // DEPENDENTVARIABLE_H
