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
 * @brief Create a basic Dimension instance.
 * @param label       Human-readable label for this dimension.
 * @param description Optional description (can be NULL).
 * @param metadata    Optional application metadata dictionary (can be NULL).
 * @param outError    On failure, receives a descriptive OCStringRef.
 * @return A new DimensionRef on success, NULL on failure.
 */
DimensionRef DimensionCreate(OCStringRef label,
                             OCStringRef description,
                             OCDictionaryRef metadata,
                             OCStringRef *outError);
/**
 * @brief Retrieve a human-readable label for this dimension.
 * @param dim The Dimension instance.
 * @return Its label, or an empty string if unset.
 */
OCStringRef DimensionCopyLabel(DimensionRef dim);
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
OCStringRef DimensionCopyDescription(DimensionRef dim);
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
OCMutableDictionaryRef DimensionGetApplicationMetaData(DimensionRef dim);
/**
 * @brief Replace this dimension’s metadata.
 * @param dim      The Dimension instance.
 * @param dict     New metadata dictionary.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool DimensionSetApplicationMetaData(DimensionRef dim,
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
LabeledDimensionRef LabeledDimensionCreate(OCStringRef label,
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
OCArrayRef LabeledDimensionCopyCoordinateLabels(LabeledDimensionRef dim);
/**
 * @brief Replace the set of coordinate labels.
 * @param dim      The LabeledDimension.
 * @param labels   New array of OCStringRef labels (≥2).
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return true on success.
 */
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                         OCArrayRef coordinateLabels,
                                         OCStringRef *outError);
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
LabeledDimensionRef LabeledDimensionCreateFromDictionary(OCDictionaryRef dict,
                                                         OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
LabeledDimensionRef LabeledDimensionCreateFromJSON(cJSON *json,
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
                                 dimensionScaling scaling,
                                 OCStringRef *outError);
/**
 * @brief Create an SI-quantitative dimension with only quantityName set; all other params default.
 *        label, description, metadata are NULL; offset, origin, period are NULL; periodic is false; scaling is kDimensionScalingNone.
 * @param quantityName Name of the physical quantity (e.g. "time").
 * @param outError     On failure, receives a descriptive OCStringRef.
 * @return New SIDimensionRef, or NULL.
 */
SIDimensionRef SIDimensionCreateWithQuantity(OCStringRef quantityName, OCStringRef *outError);
/**
 * @brief Get the physical quantity name.
 */
OCStringRef SIDimensionCopyQuantityName(SIDimensionRef dim);
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
SIScalarRef SIDimensionCopyCoordinatesOffset(SIDimensionRef dim);
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
SIScalarRef SIDimensionCopyOriginOffset(SIDimensionRef dim);
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
SIScalarRef SIDimensionCopyPeriod(SIDimensionRef dim);
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
SIDimensionRef SIDimensionCreateFromJSON(cJSON *json,
                                         OCStringRef *outError);
SIDimensionRef SIDimensionCreateCopy(SIDimensionRef dim);
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
 * @param scaling     dimensionScaling.
 * @param coordinates Array of SIScalarRef at each grid point (≥2).
 * @param reciprocal  Reciprocal SIDimension (for FFT, etc).
 * @param outError    On failure, receives a descriptive OCStringRef.
 * @return New SIMonotonicDimensionRef, or NULL.
 */
SIMonotonicDimensionRef SIMonotonicDimensionCreate(OCStringRef label,
                                                   OCStringRef description,
                                                   OCDictionaryRef metadata,
                                                   OCStringRef quantityName,
                                                   SIScalarRef offset,
                                                   SIScalarRef origin,
                                                   SIScalarRef period,
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
 * - scaling: kDimensionScalingNone
 *
 * @param quantityName Physical quantity name.
 * @param coordinates Array of SIScalarRef at each grid point (≥2).
 * @param reciprocal Reciprocal SIDimension (for FFT, etc), or NULL.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return New SIMonotonicDimensionRef, or NULL.
 */
SIMonotonicDimensionRef SIMonotonicDimensionCreateMinimal(OCStringRef quantityName,
                                                          OCArrayRef coordinates,
                                                          SIDimensionRef reciprocal,
                                                          OCStringRef *outError);
/**
 * @brief Replace the coordinate array.
 * @param dim    The SIMonotonicDimension.
 * @param coords New array (≥2).
 * @return true on success.
 */
bool SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim,
                                        OCArrayRef coords,
                                        OCStringRef *outError);
/**
 * @brief Create a copy of the coordinate array.
 * @param dim The SIMonotonicDimension.
 * @return OCMutableArrayRef containing SIScalarRef coordinates, or NULL on failure.
 *         Caller must release.
 */
OCArrayRef SIMonotonicDimensionCopyCoordinates(SIMonotonicDimensionRef dim);
/**
 * @brief Create absolute coordinate array using CSDM convention.
 *
 * Creates absolute coordinates using the formula:
 * X^abs_k = X_k + o_k × 1
 *
 * Where:
 * - X_k = regular coordinates from SIMonotonicDimensionGetCoordinates()
 * - o_k = origin_offset value
 *
 * @param dim The SIMonotonicDimension.
 * @return OCArrayRef containing SIScalarRef absolute coordinates, or NULL on failure.
 */
OCArrayRef SIMonotonicDimensionCreateAbsoluteCoordinates(SIMonotonicDimensionRef dim);
/**
 * @brief Get reciprocal dimension.
 */
SIDimensionRef SIMonotonicDimensionCopyReciprocal(SIMonotonicDimensionRef dim);
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
SIMonotonicDimensionRef SIMonotonicDimensionCreateFromDictionary(OCDictionaryRef dict,
                                                                 OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SIMonotonicDimensionRef SIMonotonicDimensionCreateFromJSON(cJSON *json,
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
 * @param scaling      dimensionScaling.
 * @param count        Number of points (≥2).
 * @param increment    SIScalar step between points.
 * @param fft          True if used for FFT.
 * @param reciprocal   Reciprocal dimension.
 * @param outError     On failure, receives a descriptive OCStringRef.
 * @return New SILinearDimensionRef, or NULL.
 */
SILinearDimensionRef SILinearDimensionCreate(OCStringRef label,
                                             OCStringRef description,
                                             OCDictionaryRef metadata,
                                             OCStringRef quantityName,
                                             SIScalarRef offset,
                                             SIScalarRef origin,
                                             SIScalarRef period,
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
SILinearDimensionRef SILinearDimensionCreateMinimal(
    OCStringRef quantityName,
    OCIndex count,
    SIScalarRef increment,
    SIDimensionRef reciprocal,
    OCStringRef *outError);
/**
 * @brief Get the total point count.
 * @param dim   The SILinearDimension.
 * @return Point count.
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
SIScalarRef SILinearDimensionCopyIncrement(SILinearDimensionRef dim);
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
SIScalarRef SILinearDimensionCreateReciprocalIncrement(SILinearDimensionRef dim);
/**
 * @brief Create coordinate array following CSDM convention.
 *
 * Creates coordinates using the formula:
 * X_k = Δx_k × (J_k - Z_k) + b_k × 1
 *
 * Where:
 * - Δx_k = increment
 * - b_k = coordinates_offset
 * - J_k = [0, 1, 2, ..., count-1] (index array)
 * - Z_k = 0 if complex_fft is false, T_k/2 if complex_fft is true
 * - T_k = count for even count, count-1 for odd count
 *
 * @param dim The SILinearDimension.
 * @return OCArrayRef containing SIScalarRef coordinates, or NULL on failure.
 */
OCArrayRef SILinearDimensionCreateCoordinates(SILinearDimensionRef dim);
/**
 * @brief Create absolute coordinate array using CSDM convention.
 *
 * Creates absolute coordinates using the formula:
 * X^abs_k = X_k + o_k × 1
 *
 * Where:
 * - X_k = regular coordinates from SILinearDimensionCreateCoordinates()
 * - o_k = origin_offset value
 *
 * @param dim The SILinearDimension.
 * @return OCArrayRef containing SIScalarRef absolute coordinates, or NULL on failure.
 */
OCArrayRef SILinearDimensionCreateAbsoluteCoordinates(SILinearDimensionRef dim);
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
SIDimensionRef SILinearDimensionCopyReciprocal(SILinearDimensionRef dim);
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
 * @brief Set period to window size (increment × count).
 */
bool SILinearDimensionSetPeriodToWindow(SILinearDimensionRef dim);
/**
 * @brief Set period to infinity (non-periodic).
 */
bool SIDimensionSetPeriodToInfinity(SILinearDimensionRef dim);
/**
 * @brief Dictionary serializer for SILinearDimension.
 */
OCDictionaryRef SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim);
/**
 * @brief Recreate from a dictionary.
 */
SILinearDimensionRef SILinearDimensionCreateFromDictionary(OCDictionaryRef dict,
                                                           OCStringRef *outError);
/**
 * @brief Recreate from JSON.
 */
SILinearDimensionRef SILinearDimensionCreateFromJSON(cJSON *json,
                                                     OCStringRef *outError);
/** @} */
/*==============================================================================
  Operations
==============================================================================*/
/**
 * @name Dimension Operations
 * @{
 */
/**
 * @brief Multiply an SILinearDimension by a scalar, updating all dimension properties accordingly.
 *
 * This operation scales the dimension's increment, offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar. The dimension
 * is modified in-place.
 *
 * **Properties affected:**
 * - `increment`: Multiplied by the scalar
 * - `offset` and `origin`: Multiplied by the scalar
 * - `period` (if periodic): Multiplied by the scalar
 * - `quantityName`: Updated based on the new unit
 * - `reciprocal` (if exists): Multiplied by the inverse scalar
 *
 * @param dim The SILinearDimension to multiply (must be mutable)
 * @param scalar The scalar to multiply by (cannot be zero)
 * @param outError Optional error output parameter
 * @return true if successful, false on error
 */
bool SILinearDimensionMultiplyByScalar(SILinearDimensionRef dim,
                                       SIScalarRef scalar,
                                       OCStringRef *outError);
/**
 * @brief Multiply an SIMonotonicDimension by a scalar, updating all dimension properties accordingly.
 *
 * This operation scales all coordinates, offset, origin, and period by the scalar,
 * and updates the reciprocal dimension properties with the inverse scalar. The dimension
 * is modified in-place.
 *
 * **Properties affected:**
 * - All `coordinates`: Each coordinate multiplied by the scalar
 * - `offset` and `origin`: Multiplied by the scalar
 * - `period` (if periodic): Multiplied by the scalar
 * - `quantityName`: Updated based on the new unit
 * - `reciprocal` (if exists): Multiplied by the inverse scalar
 *
 * @param dim The SIMonotonicDimension to multiply (must be mutable)
 * @param scalar The scalar to multiply by (cannot be zero)
 * @param outError Optional error output parameter
 * @return true if successful, false on error
 */
bool SIMonotonicDimensionMultiplyByScalar(SIMonotonicDimensionRef dim,
                                          SIScalarRef scalar,
                                          OCStringRef *outError);
/**
 * @brief Create a new SIMonotonicDimension by multiplying by a scalar.
 *
 * This creates a copy of the original dimension with all coordinates and properties scaled by the scalar.
 *
 * @param dim The SIMonotonicDimension to multiply
 * @param scalar The scalar to multiply by (cannot be zero)
 * @param outError Optional error output parameter
 * @return New SIMonotonicDimensionRef with scaled properties, or NULL on error
 */
SIMonotonicDimensionRef SIMonotonicDimensionCreateByMultiplyingByScalar(SIMonotonicDimensionRef dim,
                                                                        SIScalarRef scalar,
                                                                        OCStringRef *outError);
/**
 * @brief Create a new SILinearDimension by multiplying by a scalar.
 * @param dim The SILinearDimension to multiply
 * @param scalar The scalar to multiply by
 * @param outError Optional error output parameter
 * @return New SILinearDimensionRef with scaled properties, or NULL on error
 */
SILinearDimensionRef SILinearDimensionCreateByMultiplyingByScalar(SILinearDimensionRef dim,
                                                                  SIScalarRef scalar,
                                                                  OCStringRef *outError);
/**
 * @brief Create an inverse SILinearDimension.
 * @param dim The original SILinearDimension
 * @param error Optional error output parameter
 * @return New SILinearDimensionRef representing the inverse, or NULL on error
 */
SILinearDimensionRef SILinearDimensionCreateInverse(SILinearDimensionRef dim, OCStringRef *error);
/** @} */
/*==============================================================================
  Utilities
==============================================================================*/
/**
 * @brief Determine whether a dimension represents quantitative or qualitative data.
 *
 * Quantitative dimensions contain numeric, measurable data with physical units
 * or mathematical relationships between data points. Qualitative dimensions
 * contain categorical, discrete labels without inherent numeric meaning.
 *
 * **Classification Rules:**
 * - **LabeledDimension:** Always qualitative (false) - represents discrete categories
 * - **SIDimension:** Always quantitative (true) - has physical units and scaling
 * - **SIMonotonicDimension:** Always quantitative (true) - ordered numeric coordinates
 * - **SILinearDimension:** Always quantitative (true) - evenly spaced numeric values
 * - **Base Dimension:** Qualitative (false) - abstract placeholder
 *
 *
 * @param dim The Dimension instance to classify.
 * @return true if the dimension represents quantitative (numeric) data,
 *         false if qualitative (categorical) data or if dim is NULL.
 */
bool DimensionIsQuantitative(DimensionRef dim);
/**
 * @brief Serialize a Dimension (any subclass) to a dictionary.
 *
 * Includes all base fields plus a "type" discriminator for dispatch.
 * @param dim The Dimension instance.
 * @return A new OCDictionaryRef, or NULL on error. Caller must release.
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);
/**
 * @brief Serialize a Dimension (any subclass) to JSON.
 *
 * Includes all base fields plus a "type" discriminator for dispatch.
 * @param dim The Dimension instance.
 * @param typed Whether to include OCTypes metadata wrapping.
 * @param outError On failure, receives a descriptive OCStringRef.
 * @return A new cJSON object, or NULL on error. Caller must release with cJSON_Delete.
 */
cJSON *DimensionCopyAsJSON(DimensionRef dim, bool typed, OCStringRef *outError);
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
 * @brief Create a human-readable label for a specific coordinate index.
 *
 * e.g. "Phase-3", "Time-3/s", "Frequency-5/Hz"
 * @param dim   The Dimension instance.
 * @param index Zero-based coordinate index.
 * @return New OCStringRef (caller must release), or NULL.
 */
OCStringRef DimensionCreateAxisLabel(DimensionRef dim, OCIndex index);
/** @} */
OCIndex DimensionGetCount(DimensionRef dim);
OCStringRef DimensionGetType(DimensionRef dim);
bool SIDimensionIsPeriodic(SIDimensionRef dim);
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim);
/**
 * @brief Get the quantity name for this SI dimension.
 * @param dim The SIDimension instance.
 * @return The quantity name string.
 */
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim);
bool SILinearDimensionGetComplexFFT(SILinearDimensionRef dim);
OCTypeRef DimensionCopyCoordinateAtIndex(DimensionRef dim, double index);
OCTypeRef DimensionCreateInterpolatedCoordinateAtIndex(SILinearDimensionRef dim, double dIndex);
/**
 * @brief Compute the total number of samples implied by an array of DimensionRef.
 *
 * For each dimension in `dimensions`, multiplies together:
 *  - SILinearDimension → count
 *  - SIMonotonicDimension → number of coordinates
 *  - LabeledDimension → number of labels
 *  - all other Dimension subclasses → 1
 *
 * @param dimensions  An OCArrayRef of DimensionRef
 * @return            The product of all per-dimension sample counts (or 1 if NULL).
 */
OCIndex RMNCalculateSizeFromDimensions(OCArrayRef dimensions);
/**
 * @brief Like RMNCalculateSizeFromDimensions, but skip any dimensions
 *        whose index appears in `ignored`.
 *
 * @param dimensions  An OCArrayRef of DimensionRef
 * @param ignored     An OCIndexSetRef of dimension‐indices to skip
 * @return            The product of sample counts over the non-ignored dimensions.
 */
OCIndex RMNCalculateSizeFromDimensionsIgnoring(OCArrayRef dimensions, OCIndexSetRef ignored);
/**
 * @brief Convert a multi‐dimensional index vector into a single memory offset.
 *
 * Wraps each index into [0..npts−1] before computing:
 *   offset = Σ_{k=0..D−1} ( index[k] * ∏_{j<k} npts[j] )
 *
 * @param dimensions   An OCArrayRef of DimensionRef
 * @param indexes      Array of length D giving each coordinate (may be out of range)
 * @return             The flat memory offset, or (OCIndex)−1 on error.
 */
OCIndex RMNGridMemOffsetFromIndexes(OCArrayRef dimensions, const OCIndex indexes[]);
/**
 * @brief Recover a single coordinate along one dimension from a flat offset.
 *
 * coord = (offset / ∏_{j<dim} npts[j]) % npts[dim]
 *
 * @param dimensions      An OCArrayRef of DimensionRef
 * @param memOffset       The flat offset
 * @param dimensionIndex  Which dimension to extract
 * @return                The wrapped coordinate, or (OCIndex)−1 on error.
 */
OCIndex
RMNGridCoordinateIndexFromMemOffset(OCArrayRef dimensions, OCIndex memOffset, OCIndex dimensionIndex);
/**
 * @brief Compute the stride (flat‐index increment) along a given dimension.
 *
 * stride = ∏_{j<dimensionIndex} npts[j], or 1 if dimensionIndex == 0.
 *
 * @param npts              Array of per-dimension sizes (length D)
 * @param dimensionsCount   Number of dimensions (D)
 * @param dimensionIndex    Which dimension’s stride to compute
 * @return                  The linear stride for that dimension.
 */
OCIndex
strideAlongDimensionIndex(const OCIndex *npts, OCIndex dimensionsCount, OCIndex dimensionIndex);
/**
 * @brief Convert a full index‐vector into a flat offset, wrapping out‐of‐range indexes.
 *
 * On exit, each indexes[i] has been reduced modulo npts[i].
 * offset = Σ_{k=0..D−1} ( indexes[k] * ∏_{j<k} npts[j] )
 *
 * @param indexes           In/out array of length D; on entry may be arbitrary,
 *                          on exit each entry is wrapped into valid range.
 * @param dimensionsCount   Number of dimensions (D)
 * @param npts              Per-dimension sizes (length D)
 * @return                  The flat offset.
 */
OCIndex
memOffsetFromIndexes(OCIndex *indexes, OCIndex dimensionsCount, const OCIndex *npts);
/**
 * @brief Convert a flat offset into a full index‐vector.
 *
 * index[k] = (offset / ∏_{j<k} npts[j]) % npts[k]
 *
 * @param memOffset         The flat offset.
 * @param indexes           Output array of length D.
 * @param dimensionsCount   Number of dimensions (D).
 * @param npts              Per-dimension sizes (length D).
 */
void setIndexesForMemOffset(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts);
/**
 * @brief Like setIndexesForMemOffset, but skip one dimension.
 *
 * Fills `indexes[idim]` only for idim ≠ ignoredDimension; the array must be pre-initialized.
 *
 * @param memOffset           The flat offset.
 * @param indexes             Output array of length D.
 * @param dimensionsCount     Number of dimensions (D).
 * @param npts                Per-dimension sizes (length D).
 * @param ignoredDimension    The one dimension index to skip.
 */
void setIndexesForReducedMemOffsetIgnoringDimension(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts, OCIndex ignoredDimension);
/**
 * @brief Like setIndexesForMemOffset, but skip any dimensions in a set.
 *
 * Fills `indexes[idim]` for all idim ∉ dimensionIndexSet; the array must be pre-initialized.
 *
 * @param memOffset            The flat offset.
 * @param indexes              Output array of length D.
 * @param dimensionsCount      Number of dimensions (D).
 * @param npts                 Per-dimension sizes (length D).
 * @param dimensionIndexSet    Set of dimension‐indices to skip.
 */
void setIndexesForReducedMemOffsetIgnoringDimensions(OCIndex memOffset, OCIndex indexes[], OCIndex dimensionsCount, const OCIndex *npts, OCIndexSetRef dimensionIndexSet);
OCMutableArrayRef DimensionCreateCoordinateIndexesFromMemOffset(OCArrayRef dimensions, OCIndex memOffset);
#ifdef __cplusplus
}
#endif
#endif /* DIMENSION_H */
