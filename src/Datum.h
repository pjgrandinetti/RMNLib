#ifndef DATUM_H
#define DATUM_H

#include "RMNLibrary.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file Datum.h
 * @brief Public API for Datum—a scalar sample with coordinates and indexing metadata.
 *
 * A Datum wraps:
 *  - a scalar response (SIScalarRef),
 *  - an array of coordinate scalars (OCArrayRef of SIScalarRef),
 *  - three integer indices (dependent-variable, component, memory offset).
 *
 * It supports deep-copying, dictionary serialization, and schema-bound JSON I/O.
 */

/**
 * @defgroup Datum Datum
 * @brief Object model for a “data point” in an N-D dataset.
 * @{
 */

/**
 * @brief Opaque handle for a Datum instance.
 */
typedef struct impl_Datum *DatumRef;

/**
 * @brief Retrieve the OCTypeID for Datum.
 * @return A unique type identifier.
 */
OCTypeID DatumGetTypeID(void);

/**
 * @brief Create a new Datum.
 *
 * Allocates and retains a copy of the given scalar and coordinates.
 *
 * @param response                 The primary scalar value (must not be NULL).
 * @param coordinates              Optional OCArrayRef of SIScalarRef coordinates.
 *                                 Pass NULL if there are no coordinates.
 * @param dependentVariableIndex   Index of the parent DependentVariable.
 * @param componentIndex           Index of the component within that DV.
 * @param memOffset                Raw memory‐offset index (for internal use).
 * @return A new DatumRef, or NULL on allocation/failure.
 */
DatumRef
DatumCreate(
    SIScalarRef response,
    OCArrayRef  coordinates,
    OCIndex     dependentVariableIndex,
    OCIndex     componentIndex,
    OCIndex     memOffset);

/**
 * @brief Deep-copy an existing Datum.
 * @param theDatum The Datum to copy (must not be NULL).
 * @return A new DatumRef, or NULL on error.
 */
DatumRef
DatumCopy(DatumRef theDatum);

/**
 * @brief Compare whether two Datums share the same reduced (unit) dimensions.
 *
 * Both their response scalar and each coordinate scalar must have
 * identical dimensionality (e.g. both “NMR chemical shift” vs. “hertz”).
 *
 * @param input1 First Datum.
 * @param input2 Second Datum.
 * @return true if dimensionalities match; false otherwise.
 */
bool
DatumHasSameReducedDimensionalities(DatumRef input1,
                                    DatumRef input2);

/**
 * @brief Get the component index.
 * @param theDatum DatumRef to inspect.
 * @return componentIndex, or kOCNotFound if theDatum is NULL.
 */
OCIndex
DatumGetComponentIndex(DatumRef theDatum);

/**
 * @brief Set the component index.
 * @param theDatum DatumRef to modify.
 * @param componentIndex New component index.
 */
void
DatumSetComponentIndex(DatumRef theDatum,
                       OCIndex componentIndex);

/**
 * @brief Get the dependent-variable index.
 * @param theDatum DatumRef to inspect.
 * @return dependentVariableIndex, or kOCNotFound if NULL.
 */
OCIndex
DatumGetDependentVariableIndex(DatumRef theDatum);

/**
 * @brief Set the dependent-variable index.
 * @param theDatum DatumRef to modify.
 * @param dependentVariableIndex New DV index.
 */
void
DatumSetDependentVariableIndex(DatumRef theDatum,
                               OCIndex dependentVariableIndex);

/**
 * @brief Get the raw memory‐offset.
 * @param theDatum DatumRef to inspect.
 * @return memOffset, or kOCNotFound if NULL.
 */
OCIndex
DatumGetMemOffset(DatumRef theDatum);

/**
 * @brief Set the raw memory‐offset.
 * @param theDatum DatumRef to modify.
 * @param memOffset New offset value.
 */
void
DatumSetMemOffset(DatumRef theDatum,
                  OCIndex memOffset);

/**
 * @brief Retrieve a coordinate scalar by index.
 * @param theDatum DatumRef to inspect.
 * @param index    Zero-based coordinate index.
 * @return SIScalarRef at that index, or NULL on error.
 */
SIScalarRef
DatumGetCoordinateAtIndex(DatumRef theDatum,
                          OCIndex   index);

/**
 * @brief Create a standalone copy of the response scalar.
 * @param theDatum DatumRef to inspect.
 * @return New SIScalarRef copy, or NULL on error.
 */
SIScalarRef
DatumCreateResponse(DatumRef theDatum);

/**
 * @brief Number of coordinate scalars.
 * @param theDatum DatumRef to inspect.
 * @return Count of coordinates, or 0 if none/NULL.
 */
OCIndex
DatumCoordinatesCount(DatumRef theDatum);

/**
 * @brief Serialize this Datum into a deep‐copyable dictionary.
 *
 * Keys:
 *  - "dependent_variable_index" (number)
 *  - "component_index"          (number)
 *  - "mem_offset"               (number)
 *  - "response"                 (string scalar expression)
 *  - "coordinates"              (array of string scalar expressions)
 *
 * @param theDatum DatumRef to serialize.
 * @return New OCDictionaryRef; caller must OCRelease().
 */
OCDictionaryRef
DatumCopyAsDictionary(DatumRef theDatum);

/**
 * @brief Reconstruct a Datum from a dictionary produced by DatumCopyAsDictionary().
 * @param dictionary Source dictionary.
 * @param error      Optional output error string.
 * @return DatumRef or NULL on parse/validation error.
 */
DatumRef
DatumCreateFromDictionary(OCDictionaryRef dictionary,
                          OCStringRef    *error);

/**
 * @} */  // end of Datum group


/**
 * @defgroup DatumJSON JSON Serialization
 * @brief Schema‐bound JSON I/O for Datum via cJSON.
 * @{
 */

/**
 * @brief Construct a DatumRef directly from cJSON.
 *
 * Implements:
 *   dict = DatumDictionaryCreateFromJSON(json, outError);
 *   datum = DatumCreateFromDictionary(dict, outError);
 *
 * @param json     cJSON object representing a Datum.
 * @param outError Optional output error string.
 * @return DatumRef or NULL on error.
 */
DatumRef
DatumCreateFromJSON(cJSON      *json,
                    OCStringRef *outError);

/** @} */  // end of DatumJSON group

#ifdef __cplusplus
}
#endif

#endif  // DATUM_H