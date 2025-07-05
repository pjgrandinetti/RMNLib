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
/**
 * @brief Serialize to a dictionary for JSON-roundtrip.
 * @param dim The Dimension instance.
 * @return A new OCDictionaryRef, or NULL.
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);
/**
 * @brief Reconstruct a Dimension from a dictionary.
 * @param dict     Source dictionary.
 * @param outError On error, receives an OCStringRef.
 * @return New DimensionRef, or NULL on failure.
 */
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Reconstruct a Dimension from a cJSON object.
 * @param json     cJSON object.
 * @param outError On error, receives an OCStringRef.
 * @return New DimensionRef, or NULL on failure.
 */
DimensionRef DimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
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
SIScalarRef SIDimensionGetOffset(SIDimensionRef dim);
/**
 * @brief Set offset.
 */
bool SIDimensionSetOffset(SIDimensionRef dim, SIScalarRef val);
/**
 * @brief Get origin.
 */
SIScalarRef SIDimensionGetOrigin(SIDimensionRef dim);
/**
 * @brief Set origin.
 */
bool SIDimensionSetOrigin(SIDimensionRef dim, SIScalarRef val);
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
bool SILinearDimensionIsFFT(SILinearDimensionRef dim);
/**
 * @brief Mark/unmark FFT usage.
 */
bool SILinearDimensionSetFFT(SILinearDimensionRef dim, bool fft);
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
 * @brief Get the number of points in any Dimension.
 * @param dim The Dimension.
 * @return Number of entries, or 0 on error.
 */
OCIndex DimensionGetCount(DimensionRef dim);
/**
 * @brief Generate a “long” label for a single coordinate.
 * @param dim   The Dimension instance.
 * @param index Zero-based coordinate index.
 * @return A full label string, e.g. “Time (s) [index 3]”.
 */
OCStringRef CreateLongDimensionLabel(DimensionRef dim, OCIndex index);
/** @} */  // end of Dimensions group
#ifdef __cplusplus
}
#endif
#endif /* DIMENSIONS_H */
