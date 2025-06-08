#pragma once
#ifndef DIMENSIONS_H
#define DIMENSIONS_H
#include "RMNLibrary.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file Dimensions.h
 * @brief Public interface for Dimension types: base, LabeledDimension, SIDimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html
 */
// ----------------------------------------------------------------------------
// MARK: - Shared Types & Enums
// ----------------------------------------------------------------------------
/**
 * @enum dimensionScaling
 * @brief Scaling modes for quantitative dimensions.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#dimensionScaling
 */
typedef enum dimensionScaling {
    kDimensionScalingNone, /**< No scaling applied. */
    kDimensionScalingNMR   /**< NMR-specific scaling applied. */
} dimensionScaling;
/** Opaque base type for all dimensions. */
typedef struct __Dimension *DimensionRef;
/** Opaque type for a labeled (categorical) dimension. */
typedef struct __LabeledDimension *LabeledDimensionRef;
/** Opaque type for a quantitative (SI) dimension. */
typedef struct __SIDimension *SIDimensionRef;
// ----------------------------------------------------------------------------
// MARK: - Dimension (Abstract Base)
// ----------------------------------------------------------------------------
/**
 * @brief Returns the OCTypeID for Dimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionGetTypeID
 */
OCTypeID DimensionGetTypeID(void);
/**
 * @brief Deep-copies any Dimension-derived object.
 * @param original  The source DimensionRef.
 * @return          A fresh DimensionRef of the same concrete type, or NULL.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionDeepCopy
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);
/**
 * @brief Reconstruct a Dimension (of the correct subtype) from its dictionary.
 * @param dict  An OCDictionaryRef produced by DimensionCopyAsDictionary.
 * @return      A new DimensionRef, or NULL on failure.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionCreateFromDictionary
 */
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict);
/**
 * @brief Get the “label” string of a Dimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionGetLabel
 */
OCStringRef DimensionGetLabel(DimensionRef dim);
/**
 * @brief Set the “label” of a Dimension (deep-copied).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionSetLabel
 */
bool DimensionSetLabel(DimensionRef dim, OCStringRef label);
/**
 * @brief Get the “description” string of a Dimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionGetDescription
 */
OCStringRef DimensionGetDescription(DimensionRef dim);
/**
 * @brief Set the “description” of a Dimension (deep-copied).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionSetDescription
 */
bool DimensionSetDescription(DimensionRef dim, OCStringRef desc);
/**
 * @brief Get the metadata dictionary of a Dimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionGetMetadata
 */
OCDictionaryRef DimensionGetMetadata(DimensionRef dim);
/**
 * @brief Set the metadata dictionary of a Dimension (deep-copied or empty).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#DimensionSetMetadata
 */
bool DimensionSetMetadata(DimensionRef dim, OCDictionaryRef dict);
// ----------------------------------------------------------------------------
// MARK: - LabeledDimension
// ----------------------------------------------------------------------------
/**
 * @brief Returns the OCTypeID for LabeledDimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionGetTypeID
 */
OCTypeID LabeledDimensionGetTypeID(void);
/**
 * @brief Create a labeled dimension with full customization.
 * @param label       Optional label string (NULL ⇒ empty).
 * @param description Optional description string (NULL ⇒ empty).
 * @param metaData    Optional metadata (NULL ⇒ empty).
 * @param labels      OCArrayRef of OCStringRef (must have ≥2 entries).
 * @return            A new LabeledDimensionRef, or NULL.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionCreate
 */
LabeledDimensionRef
LabeledDimensionCreate(OCStringRef label,
                       OCStringRef description,
                       OCDictionaryRef metaData,
                       OCArrayRef labels);
/**
 * @brief Create a labeled dimension from an array of labels only.
 * @param labels  OCArrayRef of OCStringRef (must have ≥2 entries).
 * @return        A new LabeledDimensionRef (defaults empty label/description).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionCreateWithCoordinateLabels
 */
LabeledDimensionRef
LabeledDimensionCreateWithCoordinateLabels(OCArrayRef labels);
/**
 * @brief Get the coordinate-labels array.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionGetCoordinateLabels
 */
OCArrayRef
LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
/**
 * @brief Replace the coordinate-labels array (deep-copied).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionSetCoordinateLabels
 */
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                         OCArrayRef labels);
/**
 * @brief Get a single label by index.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionGetCoordinateLabelAtIndex
 */
OCStringRef
LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                          OCIndex index);
/**
 * @brief Replace a single label at the given index.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionSetCoordinateLabelAtIndex
 */
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                               OCIndex index,
                                               OCStringRef label);
/**
 * @brief Serialize a LabeledDimension into a dictionary.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionCopyAsDictionary
 */
OCDictionaryRef
LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim);
/**
 * @brief Reconstruct a LabeledDimension from its dictionary.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#LabeledDimensionCreateFromDictionary
 */
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict);
// ----------------------------------------------------------------------------
// MARK: - SIDimension (Quantitative SI)
// ----------------------------------------------------------------------------
/**
 * @brief Returns the OCTypeID for SIDimension.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetTypeID
 */
OCTypeID SIDimensionGetTypeID(void);
/**
 * @brief Create a fully-initialized SI dimension.
 *
 * Validates that offsets & periods share the same dimensionality, and
 * that quantityName matches or is inferred.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionCreate
 */
SIDimensionRef
SIDimensionCreate(OCStringRef label,
                  OCStringRef description,
                  OCDictionaryRef metaData,
                  OCStringRef quantityName,
                  SIScalarRef coordinatesOffset,
                  SIScalarRef originOffset,
                  SIScalarRef period,
                  bool periodic,
                  dimensionScaling scaling);
/**
 * @brief Get the quantity name.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetQuantity
 */
OCStringRef
SIDimensionGetQuantityName(SIDimensionRef dim);
/**
 * @brief Set the quantity name (validates dimensionality).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetQuantity
 */
bool SIDimensionSetQuantityName(SIDimensionRef dim,
                                OCStringRef name);
/**
 * @brief Get the coordinates-offset scalar.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetOffset
 */
SIScalarRef
SIDimensionGetOffset(SIDimensionRef dim);
/**
 * @brief Set the coordinates-offset (deep-copied, validates).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetOffset
 */
bool SIDimensionSetOffset(SIDimensionRef dim,
                          SIScalarRef val);
/**
 * @brief Get the origin-offset scalar.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetOrigin
 */
SIScalarRef
SIDimensionGetOrigin(SIDimensionRef dim);
/**
 * @brief Set the origin-offset (deep-copied, validates).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetOrigin
 */
bool SIDimensionSetOrigin(SIDimensionRef dim,
                          SIScalarRef val);
/**
 * @brief Get the period scalar (stored even if periodic=false).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetPeriod
 */
SIScalarRef
SIDimensionGetPeriod(SIDimensionRef dim);
/**
 * @brief Set the period (deep-copied, enables periodicity).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetPeriod
 */
bool SIDimensionSetPeriod(SIDimensionRef dim,
                          SIScalarRef val);
/**
 * @brief Query if the dimension is periodic.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionIsPeriodic
 */
bool SIDimensionIsPeriodic(SIDimensionRef dim);
/**
 * @brief Set or clear the periodic flag (requires period if enabling).
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetPeriodic
 */
bool SIDimensionSetPeriodic(SIDimensionRef dim,
                            bool flag);
/**
 * @brief Get the scaling mode.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionGetScaling
 */
dimensionScaling
SIDimensionGetScaling(SIDimensionRef dim);
/**
 * @brief Set the scaling mode.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionSetScaling
 */
bool SIDimensionSetScaling(SIDimensionRef dim,
                           dimensionScaling scaling);
/**
 * @brief Serialize an SIDimension into a dictionary.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionCopyAsDictionary
 */
OCDictionaryRef
SIDimensionCopyAsDictionary(SIDimensionRef dim);
/**
 * @brief Reconstruct an SIDimension from its dictionary.
 * @see https://rmnlibrary.readthedocs.io/en/latest/api/Dimensions.html#SIDimensionCreateFromDictionary
 */
SIDimensionRef
SIDimensionCreateFromDictionary(OCDictionaryRef dict);
#ifdef __cplusplus
}
#endif
#endif /* DIMENSIONS_H */
