#pragma once
#ifndef RMNDATUM_H
#define RMNDATUM_H

#include "RMNLibrary.h"

/**
 * @file RMNDatum.h
 * @brief Public API for RMNDatum, a data structure for managing scalar quantities with coordinates.
 */

/**
 * @defgroup RMNDatum RMNDatum
 * @brief Object model for data points with scalar value and coordinates.
 * @{
 */

/**
 * @typedef RMNDatumRef
 * @brief A reference to an RMNDatum object.
 */
typedef struct __RMNDatum *RMNDatumRef;

/**
 * @brief Get the OCTypeID for RMNDatum objects.
 * @return The type ID for RMNDatum.
 * @ingroup RMNDatum
 */
OCTypeID RMNDatumGetTypeID(void);

/**
 * @brief Create a new RMNDatum object.
 * @param theScalar The scalar value.
 * @param coordinates An OCArray of coordinate scalars.
 * @param dependentVariableIndex Index of the dependent variable.
 * @param componentIndex Index of the component.
 * @param memOffset Memory offset for internal indexing.
 * @return A new RMNDatumRef, or NULL on failure.
 * @ingroup RMNDatum
 */
RMNDatumRef RMNDatumCreate(SIScalarRef theScalar,
                           OCArrayRef coordinates,
                           int dependentVariableIndex,
                           int componentIndex,
                           int memOffset);

/**
 * @brief Create a copy of an RMNDatum.
 * @param theDatum The RMNDatum to copy.
 * @return A deep copy of the datum, or NULL on failure.
 * @ingroup RMNDatum
 */
RMNDatumRef RMNDatumCopy(RMNDatumRef theDatum);

/**
 * @brief Compare whether two datums share the same reduced dimensionalities.
 * @param input1 First datum.
 * @param input2 Second datum.
 * @return true if reduced dimensionalities match, false otherwise.
 * @ingroup RMNDatum
 */
bool RMNDatumHasSameReducedDimensionalities(RMNDatumRef input1, RMNDatumRef input2);

/**
 * @brief Get the component index of a datum.
 * @param theDatum The datum to query.
 * @return The component index, or -1 if NULL.
 * @ingroup RMNDatum
 */
int RMNDatumGetComponentIndex(RMNDatumRef theDatum);

/**
 * @brief Set the component index.
 * @param theDatum The datum to modify.
 * @param componentIndex New component index.
 * @ingroup RMNDatum
 */
void RMNDatumSetComponentIndex(RMNDatumRef theDatum, int componentIndex);

/**
 * @brief Get the dependent variable index.
 * @param theDatum The datum to query.
 * @return Index of the dependent variable, or -1 if NULL.
 * @ingroup RMNDatum
 */
int RMNDatumGetDependentVariableIndex(RMNDatumRef theDatum);

/**
 * @brief Set the dependent variable index.
 * @param theDatum The datum to modify.
 * @param dependentVariableIndex New index.
 * @ingroup RMNDatum
 */
void RMNDatumSetDependentVariableIndex(RMNDatumRef theDatum, int dependentVariableIndex);

/**
 * @brief Get the memory offset.
 * @param theDatum The datum to query.
 * @return The offset value, or -1 if NULL.
 * @ingroup RMNDatum
 */
int RMNDatumGetMemOffset(RMNDatumRef theDatum);

/**
 * @brief Set the memory offset.
 * @param theDatum The datum to modify.
 * @param memOffset New offset value.
 * @ingroup RMNDatum
 */
void RMNDatumSetMemOffset(RMNDatumRef theDatum, int memOffset);

/**
 * @brief Get a coordinate at a given index.
 * @param theDatum The datum to query.
 * @param index Zero-based index.
 * @return The scalar coordinate, or NULL on failure.
 * @ingroup RMNDatum
 */
SIScalarRef RMNDatumGetCoordinateAtIndex(RMNDatumRef theDatum, int index);

/**
 * @brief Create a scalar response based on the datum’s configuration.
 * @param theDatum The datum to process.
 * @return A new SIScalarRef, or NULL on failure.
 * @ingroup RMNDatum
 */
SIScalarRef RMNDatumCreateResponse(RMNDatumRef theDatum);

/**
 * @brief Get the number of coordinates.
 * @param theDatum The datum to query.
 * @return Count of coordinates, or 0 on error.
 * @ingroup RMNDatum
 */
int RMNDatumCoordinatesCount(RMNDatumRef theDatum);

/**
 * @brief Create a dictionary representation of the datum.
 * @param theDatum The RMNDatum to serialize.
 * @return An OCDictionaryRef containing key-value pairs.
 * @ingroup RMNDatum
 */
OCDictionaryRef RMNDatumCreateDictionary(RMNDatumRef theDatum);

/**
 * @brief Create a new RMNDatum from a dictionary.
 * @param dictionary The dictionary to parse.
 * @param error Optional: Receives a descriptive error string.
 * @return A new RMNDatumRef or NULL on error.
 * @ingroup RMNDatum
 */
RMNDatumRef RMNDatumCreateWithDictionary(OCDictionaryRef dictionary, OCStringRef *error);

/** @} */  // end of RMNDatum group

#endif /* RMNDATUM_H */
