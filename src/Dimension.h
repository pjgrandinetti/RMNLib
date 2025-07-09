#pragma once
#ifndef DIMENSIONS_H
#define DIMENSIONS_H
#include "RMNLibrary.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file Dimensions.h
 * @brief Public interface for all Dimension types.
 *
 * This module defines the abstract base Dimension, plus
 * concrete subclasses: LabeledDimension, SIDimension,
 * SIMonotonicDimension, and SILinearDimension.  All can
 * be serialized to/from JSON or dictionaries.
 */
/**
 * @defgroup Dimensions Dimensions
 * @brief Core types for axes and coordinate spaces.
 * @{
 */
/**
 * @enum dimensionScaling
 * @brief How to scale SI dimensions.
 */
typedef enum dimensionScaling {
    kDimensionScalingNone, /**< No scaling applied. */
    kDimensionScalingNMR   /**< NMR-specific scaling applied. */
} dimensionScaling;
/*==============================================================================
  Dimension (Abstract Base)
==============================================================================*/
/**
 * @name Dimension (abstract)
 * @{
 */
/**
 * @brief Get the OCTypeID for the base Dimension class.
 */
OCTypeID DimensionGetTypeID(void);
/**
 * @brief Retrieve a human-readable label for this dimension.
 * @param dim The Dimension instance.
 * @return Its label, or an empty string if unset.
 */
OCStringRef DimensionGetLabel(DimensionRef dim);
/**
 * @brief Set or change this dimension’s label.
 * @param dim   The Dimension instance.
 * @param label New label string.
 * @return true on success.
 */
bool DimensionSetLabel(DimensionRef dim, OCStringRef label);
/**
 * @brief Get the descriptive text for this dimension.
 * @param dim The Dimension instance.
 * @return Description string.
 */
OCStringRef DimensionGetDescription(DimensionRef dim);
/**
 * @brief Set or change this dimension’s description.
 * @param dim The Dimension instance.
 * @param desc New descriptive text.
 * @return true on success.
 */
bool DimensionSetDescription(DimensionRef dim, OCStringRef desc);
/**
 * @brief Retrieve arbitrary metadata attached to this dimension.
 * @param dim The Dimension instance.
 * @return A shallow-deep‐copied OCDictionaryRef.
 */
OCDictionaryRef DimensionGetMetadata(DimensionRef dim);
/**
 * @brief Replace this dimension’s metadata.
 * @param dim  The Dimension instance.
 * @param dict New metadata dictionary.
 * @return true on success.
 */
bool DimensionSetMetadata(DimensionRef dim, OCDictionaryRef dict);
/** @} */
/*==============================================================================
  LabeledDimension
==============================================================================*/
/**
 * @name LabeledDimension
 * @{
 */
/**
 * @brief Get the OCTypeID for LabeledDimension.
 */
OCTypeID LabeledDimensionGetTypeID(void);
/**
 * @brief Create a custom LabeledDimension.
 * @param label            Name of the dimension.
 * @param description      Optional description.
 * @param metadata         Optional metadata dict.
 * @param coordinateLabels Array of strings labeling each coordinate.
 * @return New LabeledDimensionRef, or NULL.
 */
LabeledDimensionRef
LabeledDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCArrayRef coordinateLabels);
/**
 * @brief Create a LabeledDimension with only labels.
 * @param labels Array of OCStringRef coordinate labels.
 * @return New LabeledDimensionRef, or NULL.
 */
LabeledDimensionRef
LabeledDimensionCreateWithCoordinateLabels(OCArrayRef labels);
/**
 * @brief Get all coordinate labels.
 * @param dim The LabeledDimension.
 * @return OCArrayRef of OCStringRef.
 */
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
/**
 * @brief Replace the set of coordinate labels.
 * @param dim    The LabeledDimension.
 * @param labels New array of OCStringRef labels.
 * @return true on success.
 */
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim, OCArrayRef labels);
/**
 * @brief Get the label at a specific index.
 * @param dim   The LabeledDimension.
 * @param index Zero-based coordinate index.
 * @return OCStringRef label, or NULL if out of bounds.
 */
OCStringRef LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim, OCIndex index);
/**
 * @brief Set the label at a specific index.
 * @param dim   The LabeledDimension.
 * @param index Zero-based coordinate index.
 * @param label New label string.
 * @return true on success.
 */
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim, OCIndex index, OCStringRef label);
/**
 * @brief Dictionary serializer for LabeledDimension.
 */
OCDictionaryRef LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
LabeledDimensionRef
LabeledDimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
/** @} */
/*==============================================================================
  SIDimension (Quantitative SI)
==============================================================================*/
/**
 * @name SIDimension
 * @{
 */
/**
 * @brief Get the OCTypeID for SIDimension.
 */
OCTypeID SIDimensionGetTypeID(void);
/**
 * @brief Create an SI-quantitative dimension.
 * @param label        Name of the axis.
 * @param description  Optional description.
 * @param metadata     Optional metadata.
 * @param quantityName Name of the physical quantity (e.g. "time").
 * @param offset       Scale offset (SIScalarRef).
 * @param origin       Reference origin (SIScalarRef).
 * @param period       Period for wrapping (SIScalarRef).
 * @param periodic     True if periodic.
 * @param scaling      dimensionScaling enum.
 * @return New SIDimensionRef, or NULL.
 */
SIDimensionRef
SIDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling);
/**
 * @brief Get the physical quantity name.
 */
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim);
/**
 * @brief Set the physical quantity name.
 */
bool SIDimensionSetQuantityName(SIDimensionRef dim, OCStringRef name);
/**
 * @brief Get offset.
 */
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim);
/**
 * @brief Set offset.
 */
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim, SIScalarRef val);
/**
 * @brief Get origin.
 */
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim);
/**
 * @brief Set origin.
 */
bool SIDimensionSetOriginOffset(SIDimensionRef dim, SIScalarRef val);
/**
 * @brief Get period.
 */
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim);
/**
 * @brief Set period.
 */
bool SIDimensionSetPeriod(SIDimensionRef dim, SIScalarRef val);
/**
 * @brief Check if periodic.
 */
bool SIDimensionIsPeriodic(SIDimensionRef dim);
/**
 * @brief Mark periodic flag.
 */
bool SIDimensionSetPeriodic(SIDimensionRef dim, bool flag);
/**
 * @brief Get scaling type.
 */
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim);
/**
 * @brief Set scaling type.
 */
bool SIDimensionSetScaling(SIDimensionRef dim, dimensionScaling scaling);
/**
 * @brief Dictionary serializer for SIDimension.
 */
OCDictionaryRef SIDimensionCopyAsDictionary(SIDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SIDimensionRef
SIDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SIDimensionRef
SIDimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
/** @} */
/*==============================================================================
  SIMonotonicDimension
==============================================================================*/
/**
 * @name SIMonotonicDimension
 * @{
 */
/**
 * @brief Get OCTypeID for SIMonotonicDimension.
 */
OCTypeID SIMonotonicDimensionGetTypeID(void);
/**
 * @brief Create a monotonic (but not evenly-spaced) SI dimension.
 * @param label          Axis name.
 * @param description    Optional description.
 * @param metadata       Optional metadata.
 * @param quantity       Physical quantity name.
 * @param offset         SIScalar offset.
 * @param origin         SIScalar origin.
 * @param period         SIScalar period.
 * @param periodic       True if wraps around.
 * @param scaling        dimensionScaling.
 * @param coordinates    Array of SIScalarRef at each grid point.
 * @param reciprocal     Reciprocal SIDimension (for FFT, etc).
 * @return New SIMonotonicDimensionRef, or NULL.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantity,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling,
    OCArrayRef coordinates,
    SIDimensionRef reciprocal);
/**
 * @brief Get the coordinate array.
 */
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim);
/**
 * @brief Replace the coordinate array.
 */
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim, OCArrayRef coords);
/**
 * @brief Get reciprocal dimension.
 */
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim);
/**
 * @brief Set reciprocal dimension.
 */
bool SIMonotonicDimensionSetReciprocal(SIMonotonicDimensionRef dim, SIDimensionRef rec);
/**
 * @brief Dictionary serializer for SIMonotonicDimension.
 */
OCDictionaryRef SIMonotonicDimensionCopyAsDictionary(SIMonotonicDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
/** @} */
/*==============================================================================
  SILinearDimension
==============================================================================*/
/**
 * @name SILinearDimension
 * @{
 */
/**
 * @brief Get OCTypeID for SILinearDimension.
 */
OCTypeID SILinearDimensionGetTypeID(void);
/**
 * @brief Create an evenly-spaced SI dimension.
 * @param label          Axis name.
 * @param description    Optional description.
 * @param metadata       Optional metadata.
 * @param quantity       Physical quantity name.
 * @param offset         SIScalar offset.
 * @param origin         SIScalar origin.
 * @param period         SIScalar period.
 * @param periodic       True if wraps.
 * @param scaling        dimensionScaling.
 * @param count          Number of points.
 * @param increment      SIScalar step between points.
 * @param fft            True if used for FFT.
 * @param reciprocal     Reciprocal dimension.
 * @return New SILinearDimensionRef, or NULL.
 */
SILinearDimensionRef
SILinearDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantity,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling,
    OCIndex count,
    SIScalarRef increment,
    bool fft,
    SIDimensionRef reciprocal);
/**
 * @brief Get the total point count.
 */
OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim);
/**
 * @brief Set the total point count.
 */
bool SILinearDimensionSetCount(SILinearDimensionRef dim, OCIndex count);
/**
 * @brief Get the increment between points.
 */
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim);
/**
 * @brief Set the increment.
 */
bool SILinearDimensionSetIncrement(SILinearDimensionRef dim, SIScalarRef inc);
/**
 * @brief Get reciprocal increment as SIScalar.
 */
SIScalarRef SILinearDimensionGetReciprocalIncrement(SILinearDimensionRef dim);
/**
 * @brief Check whether this is marked for FFT.
 */
bool SILinearDimensionGetComplexFFT(SILinearDimensionRef dim);
/**
 * @brief Mark/unmark FFT usage.
 */
bool SILinearDimensionSetComplexFFT(SILinearDimensionRef dim, bool fft);
/**
 * @brief Get the reciprocal SIDimension.
 */
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim);
/**
 * @brief Set the reciprocal SIDimension.
 */
bool SILinearDimensionSetReciprocal(SILinearDimensionRef dim, SIDimensionRef rec);
/**
 * @brief Dictionary serializer for SILinearDimension.
 */
OCDictionaryRef SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SILinearDimensionRef
SILinearDimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SILinearDimensionRef
SILinearDimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
/** @} */
/*==============================================================================
  Utilities
==============================================================================*/
/**
 * @brief Return a short string identifier for the runtime type of the dimension.
 *
 * This string matches the value stored in the `"type"` key for serialization.
 * Possible return values include: `"labeled"`, `"monotonic"`, `"linear"`, or `"si_dimension"`.
 * If the type is unknown, returns `"dimension"`.
 *
 * @note The returned OCStringRef is a constant string; do not release it.
 *
 * @param dim The Dimension instance.
 * @return OCStringRef type identifier, or NULL if input is NULL.
 */
OCStringRef DimensionGetType(DimensionRef dim);

/**
 * @brief Serialize a Dimension (of any subclass) to a dictionary.
 *
 * The returned dictionary includes all base fields and a `"type"` discriminator
 * when needed for dispatch. It can be used with `DimensionCreateFromDictionary()`
 * for round-trip deserialization.
 *
 * @param dim The Dimension instance to serialize.
 * @return A new OCDictionaryRef, or NULL on error.
 *         Caller is responsible for releasing the returned dictionary.
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);

/**
 * @brief Reconstruct a Dimension from a dictionary representation.
 *
 * This function dispatches to the appropriate subclass constructor
 * based on the `"type"` key in the dictionary. If `"type"` is missing,
 * falls back to the abstract base `Dimension`.
 *
 * @param dict     Source dictionary.
 * @param outError On failure, receives a descriptive OCStringRef (optional).
 * @return A new DimensionRef, or NULL on failure.
 *         Caller is responsible for releasing the returned object.
 */
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);

/**
 * @brief Reconstruct a Dimension from a cJSON representation.
 *
 * Parses a cJSON object into a dictionary and delegates to
 * `DimensionCreateFromDictionary()`. Supports subclass dispatch via `"type"`.
 *
 * @param json     Input cJSON object.
 * @param outError On failure, receives a descriptive OCStringRef (optional).
 * @return A new DimensionRef, or NULL on failure.
 *         Caller is responsible for releasing the returned object.
 */
DimensionRef DimensionCreateFromJSON(cJSON *json, OCStringRef *outError);

/**
 * @brief Get the number of coordinate entries for any Dimension.
 *
 * The count is defined by the dimension type:
 * - LabeledDimension: number of labels.
 * - SIMonotonicDimension: number of coordinates.
 * - SILinearDimension: `count` field.
 * - All others: returns 1.
 *
 * @param dim The Dimension instance.
 * @return Non-negative count, or 0 if invalid.
 */
OCIndex DimensionGetCount(DimensionRef dim);

/**
 * @brief Create a human-readable label for a specific coordinate index.
 *
 * For example, a labeled dimension might return: `Phase-3`,
 * and an SI dimension might return: `Time-3/s` or `Frequency-5/Hz`.
 *
 * This is primarily used in UI contexts or data export tools.
 *
 * @param dim   The Dimension instance.
 * @param index Coordinate index (zero-based).
 * @return A new OCStringRef with the constructed label, or NULL.
 *         Caller is responsible for releasing the returned string.
 */
OCStringRef CreateLongDimensionLabel(DimensionRef dim, OCIndex index);

/** @} */  // end of Dimensions group
#ifdef __cplusplus
}
#endif
#endif /* DIMENSIONS_H */
