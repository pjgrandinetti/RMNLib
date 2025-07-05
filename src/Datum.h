#pragma once
#ifndef DATUM_H
#define DATUM_H
#include "RMNLibrary.h"
/**
 * @file Datum.h
 * @brief Public API for Datum, a data structure for managing scalar quantities with coordinates.
 */
/**
 * @defgroup Datum Datum
 * @brief Object model for data points with scalar value and coordinates.
 * @{
 */
/**
 * @typedef DatumRef
 * @brief A reference to an Datum object.
 */
typedef struct impl_Datum *DatumRef;
/**
 * @brief Get the OCTypeID for Datum objects.
 * @return The type ID for Datum.
 * @ingroup Datum
 */
OCTypeID DatumGetTypeID(void);
/**
 * @brief Create a new Datum object.
 * @param theScalar The scalar value.
 * @param coordinates An OCArray of coordinate scalars.
 * @param dependentVariableIndex Index of the dependent variable.
 * @param componentIndex Index of the component.
 * @param memOffset Memory offset for internal indexing.
 * @return A new DatumRef, or NULL on failure.
 * @ingroup Datum
 */
DatumRef DatumCreate(SIScalarRef theScalar,
                     OCArrayRef coordinates,
                     OCIndex dependentVariableIndex,
                     OCIndex componentIndex,
                     OCIndex memOffset);
/**
 * @brief Create a copy of an Datum.
 * @param theDatum The Datum to copy.
 * @return A deep copy of the datum, or NULL on failure.
 * @ingroup Datum
 */
DatumRef DatumCopy(DatumRef theDatum);
/**
 * @brief Compare whether two datums share the same reduced dimensionalities.
 * @param input1 First datum.
 * @param input2 Second datum.
 * @return true if reduced dimensionalities match, false otherwise.
 * @ingroup Datum
 */
bool DatumHasSameReducedDimensionalities(DatumRef input1, DatumRef input2);
/**
 * @brief Get the component index of a datum.
 * @param theDatum The datum to query.
 * @return The component index, or -1 if NULL.
 * @ingroup Datum
 */
OCIndex DatumGetComponentIndex(DatumRef theDatum);
/**
 * @brief Set the component index.
 * @param theDatum The datum to modify.
 * @param componentIndex New component index.
 * @ingroup Datum
 */
void DatumSetComponentIndex(DatumRef theDatum, OCIndex componentIndex);
/**
 * @brief Get the dependent variable index.
 * @param theDatum The datum to query.
 * @return Index of the dependent variable, or -1 if NULL.
 * @ingroup Datum
 */
OCIndex DatumGetDependentVariableIndex(DatumRef theDatum);
/**
 * @brief Set the dependent variable index.
 * @param theDatum The datum to modify.
 * @param dependentVariableIndex New index.
 * @ingroup Datum
 */
void DatumSetDependentVariableIndex(DatumRef theDatum, OCIndex dependentVariableIndex);
/**
 * @brief Get the memory offset.
 * @param theDatum The datum to query.
 * @return The offset value, or -1 if NULL.
 * @ingroup Datum
 */
OCIndex DatumGetMemOffset(DatumRef theDatum);
/**
 * @brief Set the memory offset.
 * @param theDatum The datum to modify.
 * @param memOffset New offset value.
 * @ingroup Datum
 */
void DatumSetMemOffset(DatumRef theDatum, OCIndex memOffset);
/**
 * @brief Get a coordinate at a given index.
 * @param theDatum The datum to query.
 * @param index Zero-based index.
 * @return The scalar coordinate, or NULL on failure.
 * @ingroup Datum
 */
SIScalarRef DatumGetCoordinateAtIndex(DatumRef theDatum, OCIndex index);
/**
 * @brief Create a scalar response based on the datum’s configuration.
 * @param theDatum The datum to process.
 * @return A new SIScalarRef, or NULL on failure.
 * @ingroup Datum
 */
SIScalarRef DatumCreateResponse(DatumRef theDatum);
/**
 * @brief Get the number of coordinates.
 * @param theDatum The datum to query.
 * @return Count of coordinates, or 0 on error.
 * @ingroup Datum
 */
OCIndex DatumCoordinatesCount(DatumRef theDatum);
/**
 * @brief Create a dictionary representation of the datum.
 * @param theDatum The Datum to serialize.
 * @return An OCDictionaryRef containing key-value pairs.
 * @ingroup Datum
 */
OCDictionaryRef DatumCopyAsDictionary(DatumRef theDatum);
/**
 * @brief Create a new Datum from a dictionary.
 * @param dictionary The dictionary to parse.
 * @param error Optional: Receives a descriptive error string.
 * @return A new DatumRef or NULL on error.
 * @ingroup Datum
 */
DatumRef DatumCreateFromDictionary(OCDictionaryRef dictionary, OCStringRef *error);
/** @} */  // end of Datum group
#endif     /* DATUM_H */
