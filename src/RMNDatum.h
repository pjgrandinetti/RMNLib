/**
 * @file RMNDatum.h
 * @brief Public API for RMNDatum, a data structure for managing scalar quantities with coordinates.
 */

#include "RMNLibrary.h"

/**
 * @typedef RMNDatumRef
 * @brief A reference to an RMNDatum object.
 */
typedef struct __RMNDatum *RMNDatumRef;

/**
 * @brief Get the type ID for RMNDatum objects.
 * @return The type ID for RMNDatum.
 */
OCTypeID RMNDatumGetTypeID(void);

/**
 * @brief Create a new RMNDatum object.
 * @param theScalar The scalar value.
 * @param coordinates An array of coordinates.
 * @param dependentVariableIndex The index of the dependent variable.
 * @param componentIndex The index of the component.
 * @param memOffset The memory offset.
 * @return A reference to the newly created RMNDatum object, or NULL on failure.
 */
RMNDatumRef RMNDatumCreate(SIScalarRef theScalar,
                           OCArrayRef coordinates,
                           int dependentVariableIndex,
                           int componentIndex,
                           int memOffset);

/**
 * @brief Create a copy of an RMNDatum object.
 * @param theDatum The RMNDatum object to copy.
 * @return A reference to the copied RMNDatum object, or NULL on failure.
 */
RMNDatumRef RMNDatumCopy(RMNDatumRef theDatum);

/**
 * @brief Check if two RMNDatum objects have the same reduced dimensionalities.
 * @param input1 The first RMNDatum object.
 * @param input2 The second RMNDatum object.
 * @return True if the reduced dimensionalities are the same, false otherwise.
 */
bool RMNDatumHasSameReducedDimensionalities(RMNDatumRef input1, RMNDatumRef input2);

/**
 * @brief Get the component index of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return The component index, or -1 if theDatum is NULL.
 */
int RMNDatumGetComponentIndex(RMNDatumRef theDatum);

/**
 * @brief Set the component index of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @param componentIndex The new component index.
 */
void RMNDatumSetComponentIndex(RMNDatumRef theDatum, int componentIndex);

/**
 * @brief Get the dependent variable index of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return The dependent variable index, or -1 if theDatum is NULL.
 */
int RMNDatumGetDependentVariableIndex(RMNDatumRef theDatum);

/**
 * @brief Set the dependent variable index of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @param dependentVariableIndex The new dependent variable index.
 */
void RMNDatumSetDependentVariableIndex(RMNDatumRef theDatum, int dependentVariableIndex);

/**
 * @brief Get the memory offset of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return The memory offset, or -1 if theDatum is NULL.
 */
int RMNDatumGetMemOffset(RMNDatumRef theDatum);

/**
 * @brief Set the memory offset of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @param memOffset The new memory offset.
 */
void RMNDatumSetMemOffset(RMNDatumRef theDatum, int memOffset);

/**
 * @brief Get a coordinate at a specific index from an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @param index The index of the coordinate.
 * @return The coordinate at the specified index, or NULL if theDatum is NULL or index is invalid.
 */
SIScalarRef RMNDatumGetCoordinateAtIndex(RMNDatumRef theDatum, int index);

/**
 * @brief Create a response scalar from an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return A new scalar response, or NULL if theDatum is NULL.
 */
SIScalarRef RMNDatumCreateResponse(RMNDatumRef theDatum);

/**
 * @brief Get the number of coordinates in an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return The number of coordinates, or 0 if theDatum is NULL.
 */
int RMNDatumCoordinatesCount(RMNDatumRef theDatum);

/**
 * @brief Create a property list representation of an RMNDatum object.
 * @param theDatum The RMNDatum object.
 * @return A dictionary representing the RMNDatum, or NULL if theDatum is NULL.
 */
OCDictionaryRef RMNDatumCreatePList(RMNDatumRef theDatum);

// Unit Test Suggestion:
// Ensure all public functions are tested for valid and invalid inputs, including null checks and edge cases.
