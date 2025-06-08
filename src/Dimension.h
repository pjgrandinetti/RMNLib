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
 *
 * Opaque types and their associated functions for creating and
 * manipulating various Dimension objects.
 */
// ----------------------------------------------------------------------------
// MARK: - Shared Types & Enums
// ----------------------------------------------------------------------------
/**
 * @enum dimensionScaling
 * @brief Scaling modes for quantitative dimensions.
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
 */
OCTypeID DimensionGetTypeID(void);
/**
 * @brief Deep-copies any Dimension-derived object.
 * @param original  The source DimensionRef.
 * @return          A fresh DimensionRef of the same concrete type, or NULL.
 */
DimensionRef DimensionCreateDeepCopy(DimensionRef original);
/**
 * @brief Serialize a Dimension into a dictionary.
 * @param dim  The DimensionRef to serialize.
 * @return     A new OCDictionaryRef representing its state, or NULL.
 */
OCDictionaryRef DimensionCopyAsDictionary(DimensionRef dim);
/**
 * @brief Reconstruct a Dimension (of the correct subtype) from its dictionary.
 * @param dict  An OCDictionaryRef produced by DimensionCopyAsDictionary.
 * @return      A new DimensionRef, or NULL on failure.
 */
DimensionRef DimensionCreateFromDictionary(OCDictionaryRef dict);
/**
 * @brief Get the “label” string of a Dimension.
 */
OCStringRef DimensionGetLabel(DimensionRef dim);
/**
 * @brief Set the “label” of a Dimension (deep-copied).
 */
bool DimensionSetLabel(DimensionRef dim, OCStringRef label);
/**
 * @brief Get the “description” string of a Dimension.
 */
OCStringRef DimensionGetDescription(DimensionRef dim);
/**
 * @brief Set the “description” of a Dimension (deep-copied).
 */
bool DimensionSetDescription(DimensionRef dim, OCStringRef desc);
/**
 * @brief Get the metadata dictionary of a Dimension.
 */
OCDictionaryRef DimensionGetMetaData(DimensionRef dim);
/**
 * @brief Set the metadata dictionary of a Dimension (deep-copied or empty).
 */
bool DimensionSetMetaData(DimensionRef dim, OCDictionaryRef dict);
// ----------------------------------------------------------------------------
// MARK: - LabeledDimension
// ----------------------------------------------------------------------------
/**
 * @brief Returns the OCTypeID for LabeledDimension.
 */
OCTypeID LabeledDimensionGetTypeID(void);
/**
 * @brief Create a labeled dimension with full customization.
 * @param label       Optional label string (NULL ⇒ empty).
 * @param description Optional description string (NULL ⇒ empty).
 * @param metaData    Optional metadata (NULL ⇒ empty).
 * @param labels      OCArrayRef of OCStringRef (must have ≥2 entries).
 * @return            A new LabeledDimensionRef, or NULL.
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
 */
LabeledDimensionRef
LabeledDimensionCreateWithCoordinateLabels(OCArrayRef labels);
/**
 * @brief Get the coordinate-labels array.
 */
OCArrayRef
LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
/**
 * @brief Replace the coordinate-labels array (deep-copied).
 */
bool LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                         OCArrayRef labels);
/**
 * @brief Get a single label by index.
 */
OCStringRef
LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                          OCIndex index);
/**
 * @brief Replace a single label at the given index.
 */
bool LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                               OCIndex index,
                                               OCStringRef label);
/**
 * @brief Serialize a LabeledDimension into a dictionary.
 */
OCDictionaryRef
LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim);
/**
 * @brief Reconstruct a LabeledDimension from its dictionary.
 */
LabeledDimensionRef
LabeledDimensionCreateFromDictionary(OCDictionaryRef dict);
// ----------------------------------------------------------------------------
// MARK: - SIDimension (Quantitative SI)
// ----------------------------------------------------------------------------
/**
 * @brief Returns the OCTypeID for SIDimension.
 */
OCTypeID SIDimensionGetTypeID(void);
/**
 * @brief Create a fully-initialized SI dimension.
 *
 * Validates that offsets & periods share the same dimensionality, and
 * that quantityName matches or is inferred.
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
 */
OCStringRef
SIDimensionGetQuantityName(SIDimensionRef dim);
/**
 * @brief Set the quantity name (validates dimensionality).
 */
bool SIDimensionSetQuantityName(SIDimensionRef dim,
                                OCStringRef name);
/**
 * @brief Get the coordinates-offset scalar.
 */
SIScalarRef
SIDimensionGetCoordinatesOffset(SIDimensionRef dim);
/**
 * @brief Set the coordinates-offset (deep-copied, validates).
 */
bool SIDimensionSetCoordinatesOffset(SIDimensionRef dim,
                                     SIScalarRef val);
/**
 * @brief Get the origin-offset scalar.
 */
SIScalarRef
SIDimensionGetOriginOffset(SIDimensionRef dim);
/**
 * @brief Set the origin-offset (deep-copied, validates).
 */
bool SIDimensionSetOriginOffset(SIDimensionRef dim,
                                SIScalarRef val);
/**
 * @brief Get the period scalar (stored even if periodic=false).
 */
SIScalarRef
SIDimensionGetPeriod(SIDimensionRef dim);
/**
 * @brief Set the period (deep-copied, enables periodicity).
 */
bool SIDimensionSetPeriod(SIDimensionRef dim,
                          SIScalarRef val);
/**
 * @brief Query if the dimension is periodic.
 */
bool SIDimensionIsPeriodic(SIDimensionRef dim);
/**
 * @brief Set or clear the periodic flag (requires period if enabling).
 */
bool SIDimensionSetPeriodic(SIDimensionRef dim,
                            bool flag);
/**
 * @brief Get the scaling mode.
 */
dimensionScaling
SIDimensionGetScaling(SIDimensionRef dim);
/**
 * @brief Set the scaling mode.
 */
bool SIDimensionSetScaling(SIDimensionRef dim,
                           dimensionScaling scaling);
/**
 * @brief Serialize an SIDimension into a dictionary.
 */
OCDictionaryRef
SIDimensionCopyAsDictionary(SIDimensionRef dim);
/**
 * @brief Reconstruct an SIDimension from its dictionary.
 */
SIDimensionRef
SIDimensionCreateFromDictionary(OCDictionaryRef dict);
#ifdef __cplusplus
}
#endif
#endif /* DIMENSIONS_H */