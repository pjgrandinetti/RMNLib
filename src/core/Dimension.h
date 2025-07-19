#pragma once
#ifndef DIMENSION_H
#define DIMENSION_H
#include "../RMNLibrary.h"
#include "cJSON.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file Dimension.h
 * @brief Public interface for all Dimension types.
 *
 * This module defines the abstract base Dimension, plus
 * concrete subclasses: LabeledDimension, SIDimension,
 * SIMonotonicDimension, and SILinearDimension.  All can
 * be serialized to/from JSON or dictionaries.
 */
/**
 * @defgroup Dimension Dimension
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
 * @param dim      The Dimension instance.
 * @param label    New label string.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool DimensionSetLabel(DimensionRef dim,
                       OCStringRef label,
                       OCStringRef *outError);
/**
 * @brief Get the descriptive text for this dimension.
 * @param dim The Dimension instance.
 * @return Description string.
 */
OCStringRef DimensionGetDescription(DimensionRef dim);
/**
 * @brief Set or change this dimension’s description.
 * @param dim      The Dimension instance.
 * @param desc     New descriptive text.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool DimensionSetDescription(DimensionRef dim,
                             OCStringRef desc,
                             OCStringRef *outError);
/**
 * @brief Retrieve arbitrary metadata attached to this dimension.
 * @param dim The Dimension instance.
 * @return A shallow-deep‐copied OCDictionaryRef.
 */
OCMutableDictionaryRef DimensionGetMetadata(DimensionRef dim);
/**
 * @brief Replace this dimension’s metadata.
 * @param dim      The Dimension instance.
 * @param dict     New metadata dictionary.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool DimensionSetMetadata(DimensionRef dim,
                          OCDictionaryRef dict,
                          OCStringRef *outError);
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
 * @param coordinateLabels Array of strings labeling each coordinate (≥2).
 * @param outError         On failure, receives a descriptive OCStringRef.
 * @return New LabeledDimensionRef, or NULL.
 */
LabeledDimensionRef
LabeledDimensionCreate(OCStringRef label,
                       OCStringRef description,
                       OCDictionaryRef metadata,
                       OCArrayRef coordinateLabels,
                       OCStringRef *outError);
/**
 * @brief Create a LabeledDimension with only labels.
 * @param labels Array of OCStringRef coordinate labels (≥2).
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
 * @param dim      The LabeledDimension.
 * @param labels   New array of OCStringRef labels (≥2).
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                         OCArrayRef labels,
                                         OCStringRef *outError);
/**
 * @brief Get the label at a specific index.
 * @param dim   The LabeledDimension.
 * @param index Zero-based coordinate index.
 * @return OCStringRef label, or NULL if out of bounds.
 */
OCStringRef LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                                      OCIndex index);
/**
 * @brief Set the label at a specific index.
 * @param dim      The LabeledDimension.
 * @param index    Zero-based coordinate index.
 * @param label    New label string.
 * @return true on success.
 */
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                               OCIndex index,
                                               OCStringRef label);
/**
 * @brief Dictionary serializer for LabeledDimension.
 */
OCDictionaryRef LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict,
                                     OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
LabeledDimensionRef
LabeledDimensionCreateFromJSON(cJSON *json,
                               OCStringRef *outError);
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
 * @param outError     On failure, receives a descriptive OCStringRef.
 * @return New SIDimensionRef, or NULL.
 */
SIDimensionRef SIDimensionCreate(OCStringRef label,
                  OCStringRef description,
                  OCDictionaryRef metadata,
                  OCStringRef quantityName,
                  SIScalarRef offset,
                  SIScalarRef origin,
                  SIScalarRef period,
                  bool periodic,
                  dimensionScaling scaling,
                  OCStringRef *outError);

/**
 * @brief Create an SI-quantitative dimension with only quantityName set; all other params default.
 *        label, description, metadata are NULL; offset, origin, period are NULL; periodic is false; scaling is kDimensionScalingNone.
 * @param quantityName Name of the physical quantity (e.g. "time").
 * @param outError     On failure, receives a descriptive OCStringRef.
 * @return New SIDimensionRef, or NULL.
 */
static inline SIDimensionRef SIDimensionCreateWithQuantity(OCStringRef quantityName, OCStringRef *outError) {
    return SIDimensionCreate(
        NULL,         // label
        NULL,         // description
        NULL,         // metadata
        quantityName, // quantityName
        NULL,         // offset
        NULL,         // origin
        NULL,         // period
        false,        // periodic
        kDimensionScalingNone, // scaling
        outError      // outError
    );
}
/**
 * @brief Get the physical quantity name.
 */
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim);
/**
 * @brief Set the physical quantity name.
 * @param dim      The SIDimension.
 * @param name     New quantity name.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIDimensionSetQuantityName(SIDimensionRef dim,
                                OCStringRef name,
                                OCStringRef *outError);
/**
 * @brief Get offset.
 */
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim);
/**
 * @brief Set offset.
 * @param dim      The SIDimension.
 * @param val      New offset scalar.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim,
                                     SIScalarRef val,
                                     OCStringRef *outError);
/**
 * @brief Get origin.
 */
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim);
/**
 * @brief Set origin.
 * @param dim      The SIDimension.
 * @param val      New origin scalar.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIDimensionSetOriginOffset(SIDimensionRef dim,
                                SIScalarRef val,
                                OCStringRef *outError);
/**
 * @brief Get period.
 */
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim);
/**
 * @brief Set period.
 * @param dim      The SIDimension.
 * \param val     New period scalar.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIDimensionSetPeriod(SIDimensionRef dim,
                          SIScalarRef val,
                          OCStringRef *outError);
/**
 * @brief Check if periodic.
 */
bool SIDimensionIsPeriodic(SIDimensionRef dim);
/**
 * @brief Mark periodic flag.
 * @param dim      The SIDimension.
 * @param flag     True to enable periodicity.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIDimensionSetPeriodic(SIDimensionRef dim,
                            bool flag,
                            OCStringRef *outError);
/**
 * @brief Get scaling type.
 */
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim);
/**
 * @brief Set scaling type.
 * @param dim      The SIDimension.
 * @param scaling  New scaling enum.
 * @return true on success.
 */
bool SIDimensionSetScaling(SIDimensionRef dim,
                           dimensionScaling scaling);
/**
 * @brief Dictionary serializer for SIDimension.
 */
OCDictionaryRef SIDimensionCopyAsDictionary(SIDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SIDimensionRef
SIDimensionCreateFromDictionary(OCDictionaryRef dict,
                                OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SIDimensionRef
SIDimensionCreateFromJSON(cJSON *json,
                          OCStringRef *outError);
/**
 * @brief Validate an SIDimension instance for internal consistency.
 * @param dim      The SIDimension to check.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true if the dimension is valid.
 */
bool SIDimensionValidate(SIDimensionRef dim,
                         OCStringRef *outError);
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
 * @param label       Axis name.
 * @param description Optional description.
 * @param metadata    Optional metadata.
 * @param quantityName    Physical quantity name.
 * @param offset      SIScalar offset.
 * @param origin      SIScalar origin.
 * @param period      SIScalar period.
 * @param periodic    True if wraps around.
 * @param scaling     dimensionScaling.
 * @param coordinates Array of SIScalarRef at each grid point (≥2).
 * @param reciprocal  Reciprocal SIDimension (for FFT, etc).
 * @param outError    On failure, receives a descriptive OCStringRef.
 * @return New SIMonotonicDimensionRef, or NULL.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreate(OCStringRef label,
                           OCStringRef description,
                           OCDictionaryRef metadata,
                           OCStringRef quantityName,
                           SIScalarRef offset,
                           SIScalarRef origin,
                           SIScalarRef period,
                           bool periodic,
                           dimensionScaling scaling,
                           OCArrayRef coordinates,
                           SIDimensionRef reciprocal,
                           OCStringRef *outError);
/**
 * @brief Create a monotonic dimension with minimal parameters.
 * 
 * This is a convenience function that calls SIMonotonicDimensionCreate with
 * sensible defaults for optional parameters:
 * - label: NULL
 * - description: NULL  
 * - metadata: NULL
 * - offset: NULL (will be defaulted by the main function)
 * - origin: NULL (will be defaulted by the main function)
 * - period: NULL (will be defaulted by the main function)
 * - periodic: false
 * - scaling: kDimensionScalingNone
 * 
 * @param quantityName Physical quantity name.
 * @param coordinates Array of SIScalarRef at each grid point (≥2).
 * @param reciprocal Reciprocal SIDimension (for FFT, etc), or NULL.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return New SIMonotonicDimensionRef, or NULL.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreateMinimal(OCStringRef quantityName,
                                  OCArrayRef coordinates,
                                  SIDimensionRef reciprocal,
                                  OCStringRef *outError);
/**
 * @brief Get the coordinate array.
 */
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim);
/**
 * @brief Replace the coordinate array.
 * @param dim    The SIMonotonicDimension.
 * @param coords New array (≥2).
 * @return true on success.
 */
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim,
                                        OCArrayRef coords);
/**
 * @brief Get reciprocal dimension.
 */
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim);
/**
 * @brief Set reciprocal dimension.
 * @param dim      The SIMonotonicDimension.
 * @param rec      New reciprocal SIDimension.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SIMonotonicDimensionSetReciprocal(SIMonotonicDimensionRef dim,
                                       SIDimensionRef rec,
                                       OCStringRef *outError);
/**
 * @brief Dictionary serializer for SIMonotonicDimension.
 */
OCDictionaryRef SIMonotonicDimensionCopyAsDictionary(SIMonotonicDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreateFromDictionary(OCDictionaryRef dict,
                                         OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SIMonotonicDimensionRef
SIMonotonicDimensionCreateFromJSON(cJSON *json,
                                   OCStringRef *outError);
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
 * @param label        Axis name.
 * @param description  Optional description.
 * @param metadata     Optional metadata.
 * @param quantity     Physical quantity name.
 * @param offset       SIScalar offset.
 * @param origin       SIScalar origin.
 * @param period       SIScalar period.
 * @param periodic     True if wraps.
 * @param scaling      dimensionScaling.
 * @param count        Number of points (≥2).
 * @param increment    SIScalar step between points.
 * @param fft          True if used for FFT.
 * @param reciprocal   Reciprocal dimension.
 * @param outError     On failure, receives a descriptive OCStringRef.
 * @return New SILinearDimensionRef, or NULL.
 */
SILinearDimensionRef
SILinearDimensionCreate(OCStringRef label,
                        OCStringRef description,
                        OCDictionaryRef metadata,
                        OCStringRef quantityName,
                        SIScalarRef offset,
                        SIScalarRef origin,
                        SIScalarRef period,
                        bool periodic,
                        dimensionScaling scaling,
                        OCIndex count,
                        SIScalarRef increment,
                        bool fft,
                        SIDimensionRef reciprocal,
                        OCStringRef *outError);

/**
 * @brief Create a minimal SILinearDimension with only quantity, increment, count, and reciprocal set.
 *        All other parameters are set to NULL, false, or kDimensionScalingNone.
 * @param quantityName    Physical quantity name.
 * @param count       Number of points (≥2).
 * @param increment   SIScalar step between points.
 * @param reciprocal  Reciprocal SIDimension.
 * @param outError    On failure, receives a descriptive OCStringRef.
 * @return New SILinearDimensionRef, or NULL.
 */
static inline SILinearDimensionRef SILinearDimensionCreateMinimal(
    OCStringRef quantityName,
    OCIndex count,
    SIScalarRef increment,
    SIDimensionRef reciprocal,
    OCStringRef *outError) 
    {
    return SILinearDimensionCreate(
        NULL,         // label
        NULL,         // description
        NULL,         // metadata
        quantityName, // quantityName
        NULL,         // offset
        NULL,         // origin
        NULL,         // period
        false,        // periodic
        kDimensionScalingNone, // scaling
        count,        // count
        increment,    // increment
        false,        // fft
        reciprocal,   // reciprocal
        outError      // outError
    );
}
/**
 * @brief Get the total point count.
 */
OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim);
/**
 * @brief Set the total point count.
 * @param dim   The SILinearDimension.
 * @param count New point count (≥2).
 * @return true on success.
 */
bool SILinearDimensionSetCount(SILinearDimensionRef dim,
                               OCIndex count);
/**
 * @brief Get the increment between points.
 */
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim);
/**
 * @brief Set the increment.
 * @param dim   The SILinearDimension.
 * @param inc   New increment scalar.
 * @return true on success.
 */
bool SILinearDimensionSetIncrement(SILinearDimensionRef dim,
                                   SIScalarRef inc);
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
 * @param dim  The SILinearDimension.
 * @param fft  True to enable complex-FFT.
 * @return true on success.
 */
bool SILinearDimensionSetComplexFFT(SILinearDimensionRef dim,
                                    bool fft);
/**
 * @brief Get the reciprocal SIDimension.
 */
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim);
/**
 * @brief Set the reciprocal SIDimension.
 * @param dim      The SILinearDimension.
 * @param rec      New reciprocal SIDimension.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool SILinearDimensionSetReciprocal(SILinearDimensionRef dim,
                                    SIDimensionRef rec,
                                    OCStringRef *outError);
/**
 * @brief Dictionary serializer for SILinearDimension.
 */
OCDictionaryRef SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SILinearDimensionRef
SILinearDimensionCreateFromDictionary(OCDictionaryRef dict,
                                      OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SILinearDimensionRef
SILinearDimensionCreateFromJSON(cJSON *json,
                                OCStringRef *outError);
/** @} */
/*==============================================================================
  Utilities
==============================================================================*/
/**
 * @brief Return a short string identifier for the runtime type of the dimension.
 *
 * Possible return values: "labeled", "monotonic", "linear", "si_dimension", or "dimension".
 * @param dim The Dimension instance.
 * @return Constant OCStringRef (do not release), or NULL if input is NULL.
 */
OCStringRef DimensionGetType(DimensionRef dim);
/**
 * @brief Serialize a Dimension (any subclass) to a dictionary.
 *
 * Includes all base fields plus a "type" discriminator for dispatch.
 * @param dim The Dimension instance.
 * @return A new OCDictionaryRef, or NULL on error. Caller must release.
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);
/**
 * @brief Reconstruct a Dimension from a dictionary representation.
 *
 * Dispatches to the correct subclass based on the "type" key,
 * or falls back to the abstract base if missing.
 * @param dict     Source dictionary.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return New DimensionRef, or NULL on failure. Caller must release.
 */
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict,
                                           OCStringRef *outError);
/**
 * @brief Reconstruct a Dimension from a cJSON representation.
 *
 * Delegates to DimensionCreateFromDictionary() after parsing.
 * @param json     Input cJSON object.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return New DimensionRef, or NULL on failure. Caller must release.
 */
DimensionRef DimensionCreateFromJSON(cJSON *json,
                                     OCStringRef *outError);
/**
 * @brief Get the number of coordinate entries for any Dimension.
 *
 * - LabeledDimension: number of labels
 * - SIMonotonicDimension: number of coordinates
 * - SILinearDimension: `count` field
 * - Others: returns 1
 *
 * @param dim The Dimension instance.
 * @return Non-negative count, or 0 if invalid.
 */
OCIndex DimensionGetCount(DimensionRef dim);
/**
 * @brief Create a human-readable label for a specific coordinate index.
 *
 * e.g. "Phase-3", "Time-3/s", "Frequency-5/Hz"
 * @param dim   The Dimension instance.
 * @param index Zero-based coordinate index.
 * @return New OCStringRef (caller must release), or NULL.
 */
OCStringRef CreateDimensionLongLabel(DimensionRef dim, OCIndex index);
/** @} */
#ifdef __cplusplus
}
#endif
#endif /* DIMENSION_H */