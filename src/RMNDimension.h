#pragma once
#ifndef RMNDIMENSIONS_H
#define RMNDIMENSIONS_H

#include "RMNLibrary.h"

/**
 * @file RMNDimensions.h
 * @brief Public interface for RMNDimension hierarchy (labeled, quantitative, monotonic, linear).
 *
 * This header declares opaque types and their associated functions for
 * creating and manipulating various dimension objects. Documentation is
 * Doxygen‐compatible for integration with Read the Docs via Sphinx and Breathe.
 */


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup RMNDimension RMNDimension
 * @brief Abstract base type and derived dimension types (labeled, quantitative, monotonic, linear).
 * @{
 */

// -----------------------------------------------------------------------------
// MARK: - Shared Types & Enums
// -----------------------------------------------------------------------------

/**
 * @enum dimensionScaling
 * @brief Scaling modes for quantitative dimensions.
 * @ingroup RMNDimension
 */
typedef enum dimensionScaling {
    kDimensionScalingNone,  /**< No scaling applied. */
    kDimensionScalingNMR    /**< NMR‐specific scaling applied. */
} dimensionScaling;

/**
 * Opaque reference to an RMNDimension object (must include struct definition before use).
 */
typedef struct __RMNDimension * RMNDimensionRef;

/** Opaque type for a labeled dimension (categorical strings). */
typedef struct __RMNLabeledDimension * RMNLabeledDimensionRef;

/** Opaque type for a quantitative dimension (numeric offsets, periods, scaling). */
typedef struct __RMNQuantitativeDimension * RMNQuantitativeDimensionRef;

/** Opaque type for a monotonic dimension (ordered coordinates). */
typedef struct __RMNMonotonicDimension * RMNMonotonicDimensionRef;

/** Opaque type for a linear dimension (uniformly spaced). */
typedef struct __RMNLinearDimension * RMNLinearDimensionRef;

// -----------------------------------------------------------------------------
// MARK: - RMNDimension (Abstract Base Type)
// -----------------------------------------------------------------------------

/**
 * @brief Returns the registered OCTypeID for RMNDimension.
 * @ingroup RMNDimension
 * @return The OCTypeID corresponding to RMNDimension.
 */
OCTypeID RMNDimensionGetTypeID(void);

/**
 * @brief Get the “label” string of a base dimension.
 * @ingroup RMNDimension
 * @param dim  An RMNDimensionRef (or subtype) instance.
 * @return     The OCStringRef label, or NULL if dim is NULL.
 */
OCStringRef      OCDimensionGetLabel(RMNDimensionRef dim);

/**
 * @brief Set the “label” string of a base dimension.
 * @ingroup RMNDimension
 * @param dim    An RMNDimensionRef (or subtype) instance.
 * @param label  The new OCStringRef label (retain‐copied).  May be NULL.
 */
bool             OCDimensionSetLabel(RMNDimensionRef dim, OCStringRef label);

/**
 * @brief Get the “description” string of a base dimension.
 * @ingroup RMNDimension
 * @param dim  An RMNDimensionRef (or subtype) instance.
 * @return     The OCStringRef description, or NULL if dim is NULL.
 */
OCStringRef      OCDimensionGetDescription(RMNDimensionRef dim);

/**
 * @brief Set the “description” string of a base dimension.
 * @ingroup RMNDimension
 * @param dim   An RMNDimensionRef (or subtype) instance.
 * @param desc  The new OCStringRef description (retain‐copied).  May be NULL.
 */
bool             OCDimensionSetDescription(RMNDimensionRef dim, OCStringRef desc);

/**
 * @brief Get the “metaData” dictionary associated with a dimension.
 * @ingroup RMNDimension
 * @param dim  An RMNDimensionRef (or subtype) instance.
 * @return     The OCDictionaryRef metaData, or NULL if dim is NULL or no dictionary.
 */
OCDictionaryRef  OCDimensionGetMetaData(RMNDimensionRef dim);

/**
 * @brief Set the “metaData” dictionary of a dimension.
 * @ingroup RMNDimension
 * @param dim   An RMNDimensionRef (or subtype) instance.
 * @param dict  The new OCDictionaryRef (retain‐copied).  May be NULL.
 */
bool             OCDimensionSetMetaData(RMNDimensionRef dim, OCDictionaryRef dict);


/// Deep copy any RMNDimension-derived object.
/// Returns a new object of the same type, or NULL on failure.
RMNDimensionRef RMNDimensionCreateDeepCopy(RMNDimensionRef original);

// -----------------------------------------------------------------------------
// MARK: - RMNLabeledDimension
// -----------------------------------------------------------------------------

/**
 * @defgroup RMNLabeledDimension RMNLabeledDimension
 * @brief Labeled (categorical) dimension type.
 * @ingroup RMNDimension
 * @{
 */

/**
 * @brief Returns the registered OCTypeID for RMNLabeledDimension.
 * @ingroup RMNLabeledDimension
 * @return The OCTypeID corresponding to RMNLabeledDimension.
 */
OCTypeID RMNLabeledDimensionGetTypeID(void);

/**
 * @brief Create a labeled (categorical) dimension from an array of labels.
 * @ingroup RMNLabeledDimension
 * @param inputLabels  An OCArrayRef of OCStringRefs (must have ≥2 entries).
 * @return An RMNLabeledDimensionRef on success (label field empty string by default),
 *         or NULL on failure (invalid input).
 */
RMNLabeledDimensionRef
    RMNLabeledDimensionCreateWithLabels(OCArrayRef inputLabels);

/**
 * @brief Get the array of labels for a labeled dimension.
 * @ingroup RMNLabeledDimension
 * @param dim  An RMNLabeledDimensionRef.
 * @return     The OCArrayRef of labels (immutable), or NULL if dim is NULL.
 */
OCArrayRef  RMNLabeledDimensionGetLabels(RMNLabeledDimensionRef dim);

/**
 * @brief Replace the labels array of a labeled dimension.
 * @ingroup RMNLabeledDimension
 * @param dim     An RMNLabeledDimensionRef.
 * @param labels  A new OCArrayRef of OCStringRefs (must have ≥2 entries).
 * @return true on success, false if dim or labels is NULL.
 */
bool        RMNLabeledDimensionSetLabels(RMNLabeledDimensionRef dim,
                                         OCArrayRef labels);

/**
 * @brief Get a single label string at the given index.
 * @ingroup RMNLabeledDimension
 * @param dim    An RMNLabeledDimensionRef.
 * @param index  Index of the label (0 ≤ index < count).
 * @return       The OCStringRef at that index, or NULL on invalid dim/index.
 */
OCStringRef RMNLabeledDimensionGetLabelAtIndex(RMNLabeledDimensionRef dim,
                                               OCIndex index);

/**
 * @brief Replace a single label at the specified index.
 * @ingroup RMNLabeledDimension
 * @param dim    An RMNLabeledDimensionRef.
 * @param index  Index of label to replace (0 ≤ index < count).
 * @param label  New OCStringRef label (retain‐copied).  Must be non‐NULL.
 * @return true on success, false on invalid arguments.
 */
bool        RMNLabeledDimensionSetLabelAtIndex(RMNLabeledDimensionRef dim,
                                               OCIndex index,
                                               OCStringRef label);

/** @} */ // end of RMNLabeledDimension group

// -----------------------------------------------------------------------------
// MARK: - RMNQuantitativeDimension
// -----------------------------------------------------------------------------

/**
 * @defgroup RMNQuantitativeDimension RMNQuantitativeDimension
 * @brief Quantitative (numeric) dimension type.
 * @ingroup RMNDimension
 * @{
 */

/**
 * @brief Returns the registered OCTypeID for RMNQuantitativeDimension.
 * @ingroup RMNQuantitativeDimension
 * @return The OCTypeID corresponding to RMNQuantitativeDimension.
 */
OCTypeID RMNQuantitativeDimensionGetTypeID(void);

/**
 * @brief Creates a fully initialized quantitative dimension with specified metadata and scalar values.
 *
 * This factory function validates that the referenceOffset, originOffset, and period all share the same
 * reduced dimensionality, and that the quantityName (if provided) is compatible with that dimensionality.
 * If quantityName is NULL or incompatible, it will be inferred from the reference offset’s unit.
 *
 * All provided OCStringRef and SIScalarRef values are retained (not copied), except metaData,
 * which is deep-copied if provided.
 *
 * @param label Optional label string (may be NULL for empty).
 * @param description Optional description string (may be NULL for empty).
 * @param metaData Optional metadata dictionary (copied if provided).
 * @param quantityName Optional quantity name string (validated against dimensionality or inferred).
 * @param referenceOffset Required real-valued scalar with unit and dimensionality.
 * @param originOffset Required real-valued scalar with same dimensionality.
 * @param period Required real-valued scalar with same dimensionality.
 * @param periodic True if the dimension is periodic.
 * @param scaling The scaling type (e.g., linear, logarithmic).
 *
 * @return A new RMNQuantitativeDimensionRef on success, or NULL on failure due to invalid input or allocation.
 *
 * @ingroup RMNQuantitativeDimension
 */
RMNQuantitativeDimensionRef RMNQuantitativeDimensionCreate(
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metaData,
    OCStringRef quantityName,
    SIScalarRef referenceOffset,
    SIScalarRef originOffset,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling);


/**
 * @brief Get the quantity name of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return     The OCStringRef quantityName, or NULL if not set or dim is NULL.
 */
OCStringRef RMNQuantitativeDimensionGetQuantityName(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the quantity name of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim   An RMNQuantitativeDimensionRef.
 * @param name  The new OCStringRef quantityName (retain‐copied).  May be NULL.
 */
bool         RMNQuantitativeDimensionSetQuantityName(RMNQuantitativeDimensionRef dim,
                                                     OCStringRef name);

/**
 * @brief Get the reference offset SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return     The SIScalarRef referenceOffset, or NULL if not set or dim is NULL.
 */
SIScalarRef RMNQuantitativeDimensionGetReferenceOffset(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the reference offset SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @param val  The new SIScalarRef (retain‐copied).  May be NULL.
 */
bool         RMNQuantitativeDimensionSetReferenceOffset(RMNQuantitativeDimensionRef dim,
                                                        SIScalarRef val);

/**
 * @brief Get the origin offset SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return     The SIScalarRef originOffset, or NULL if not set or dim is NULL.
 */
SIScalarRef RMNQuantitativeDimensionGetOriginOffset(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the origin offset SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @param val  The new SIScalarRef (retain‐copied).  May be NULL.
 */
bool         RMNQuantitativeDimensionSetOriginOffset(RMNQuantitativeDimensionRef dim,
                                                     SIScalarRef val);

/**
 * @brief Get the period SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return     The SIScalarRef period, or NULL if not set or dim is NULL.
 */
SIScalarRef RMNQuantitativeDimensionGetPeriod(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the period SIScalar of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @param val  The new SIScalarRef (retain‐copied).  May be NULL.
 */
bool         RMNQuantitativeDimensionSetPeriod(RMNQuantitativeDimensionRef dim,
                                               SIScalarRef val);

/**
 * @brief Query whether the dimension is periodic.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return true if periodic, false otherwise or if dim is NULL.
 */
bool         RMNQuantitativeDimensionIsPeriodic(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the periodic flag on a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim   An RMNQuantitativeDimensionRef.
 * @param flag  true to mark periodic, false to clear.
 */
bool         RMNQuantitativeDimensionSetPeriodic(RMNQuantitativeDimensionRef dim,
                                                 bool flag);

/**
 * @brief Get the scaling mode of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return The dimensionScaling value, or kDimensionScalingNone if dim is NULL.
 */
dimensionScaling
             RMNQuantitativeDimensionGetScaling(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the scaling mode of a quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim      An RMNQuantitativeDimensionRef.
 * @param scaling  The new dimensionScaling value.
 */
bool         RMNQuantitativeDimensionSetScaling(RMNQuantitativeDimensionRef dim,
                                                dimensionScaling scaling);

/**
 * @brief Get the reciprocal (inverse) quantitative dimension, if any.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @return     The RMNQuantitativeDimensionRef reciprocal, or NULL if none or dim is NULL.
 */
RMNQuantitativeDimensionRef
             RMNQuantitativeDimensionGetReciprocal(RMNQuantitativeDimensionRef dim);

/**
 * @brief Set the reciprocal (inverse) quantitative dimension.
 * @ingroup RMNQuantitativeDimension
 * @param dim  An RMNQuantitativeDimensionRef.
 * @param r    The RMNQuantitativeDimensionRef to use as reciprocal.  May be NULL.
 */
bool         RMNQuantitativeDimensionSetReciprocal(RMNQuantitativeDimensionRef dim,
                                                   RMNQuantitativeDimensionRef r);

/** @} */ // end of RMNQuantitativeDimension group

// -----------------------------------------------------------------------------
// MARK: - RMNMonotonicDimension
// -----------------------------------------------------------------------------

/**
 * @defgroup RMNMonotonicDimension RMNMonotonicDimension
 * @brief Monotonic (ordered coordinate) dimension type.
 * @ingroup RMNDimension
 * @{
 */

/**
 * @brief Returns the registered OCTypeID for RMNMonotonicDimension.
 * @ingroup RMNMonotonicDimension
 * @return The OCTypeID corresponding to RMNMonotonicDimension.
 */
OCTypeID RMNMonotonicDimensionGetTypeID(void);

/**
 * @brief Create a monotonic dimension from an array of coordinates.
 * @ingroup RMNMonotonicDimension
 * @param coordinates  An OCArrayRef of SIScalarRefs (≥2 elements, sorted or unsorted).
 * @return An RMNMonotonicDimensionRef with coordinates copied, or NULL on invalid input.
 */
RMNMonotonicDimensionRef
    RMNMonotonicDimensionCreateWithCoordinates(OCArrayRef coordinates);

/**
 * @brief Get the coordinates array of a monotonic dimension.
 * @ingroup RMNMonotonicDimension
 * @param dim  An RMNMonotonicDimensionRef.
 * @return     The OCArrayRef of SIScalarRefs, or NULL if dim is NULL.
 */
OCArrayRef  RMNMonotonicDimensionGetCoordinates(RMNMonotonicDimensionRef dim);

/**
 * @brief Replace the coordinates array of a monotonic dimension.
 * @ingroup RMNMonotonicDimension
 * @param dim         An RMNMonotonicDimensionRef.
 * @param coordinates A new OCArrayRef of SIScalarRefs (≥2 elements).
 * @return true on success, false on invalid arguments.
 */
bool        RMNMonotonicDimensionSetCoordinates(RMNMonotonicDimensionRef dim,
                                                OCArrayRef coordinates);

/** @} */ // end of RMNMonotonicDimension group

// -----------------------------------------------------------------------------
// MARK: - RMNLinearDimension
// -----------------------------------------------------------------------------

/**
 * @defgroup RMNLinearDimension RMNLinearDimension
 * @brief Linear (uniformly spaced) dimension type.
 * @ingroup RMNDimension
 * @{
 */

/**
 * @brief Returns the registered OCTypeID for RMNLinearDimension.
 * @ingroup RMNLinearDimension
 * @return The OCTypeID corresponding to RMNLinearDimension.
 */
OCTypeID RMNLinearDimensionGetTypeID(void);

/**
 * @brief Create a linear dimension representing uniform spacing.
 * @ingroup RMNLinearDimension
 * @param count            Number of points (must be >1).
 * @param increment        SIScalarRef step size (retain‐copied; must be non‐NULL).
 * @param origin           SIScalarRef origin offset (retain‐copied; may be NULL).
 * @param referenceOffset  SIScalarRef reference offset (retain‐copied; may be NULL).
 * @param period           SIScalarRef period for wraparound (retain‐copied; may be NULL).
 * @param quantityName     OCStringRef quantity name (retain‐copied; may be NULL).
 * @return RMNLinearDimensionRef on success, or NULL on invalid inputs/allocation failure.
 */
RMNLinearDimensionRef
    RMNLinearDimensionCreate( OCIndex        count,
                              SIScalarRef    increment,
                              SIScalarRef    origin,
                              SIScalarRef    referenceOffset,
                              SIScalarRef    period,
                              OCStringRef    quantityName );

/**
 * @brief Get the count (number of uniformly spaced points) of a linear dimension.
 * @ingroup RMNLinearDimension
 * @param dim  An RMNLinearDimensionRef.
 * @return     The OCIndex count, or 0 if dim is NULL.
 */
OCIndex     RMNLinearDimensionGetCount(RMNLinearDimensionRef dim);

/**
 * @brief Get the step increment SIScalar of a linear dimension.
 * @ingroup RMNLinearDimension
 * @param dim  An RMNLinearDimensionRef.
 * @return     The SIScalarRef increment, or NULL if dim is NULL.
 */
SIScalarRef RMNLinearDimensionGetIncrement(RMNLinearDimensionRef dim);

/**
 * @brief Get the origin offset SIScalar of a linear dimension.
 * @ingroup RMNLinearDimension
 * @param dim  An RMNLinearDimensionRef.
 * @return     The SIScalarRef origin offset, or NULL if dim is NULL.
 */
SIScalarRef RMNLinearDimensionGetOrigin(RMNLinearDimensionRef dim);

/**
 * @brief Get the reference offset SIScalar of a linear dimension.
 * @ingroup RMNLinearDimension
 * @param dim  An RMNLinearDimensionRef.
 * @return     The SIScalarRef reference offset, or NULL if dim is NULL.
 */
SIScalarRef RMNLinearDimensionGetReferenceOffset(RMNLinearDimensionRef dim);

/**
 * @brief Test whether an OCTypeRef is an RMNLinearDimension.
 * @ingroup RMNLinearDimension
 * @param obj  An OCTypeRef (possibly any type).
 * @return true if obj’s type ID equals RMNLinearDimensionGetTypeID(), false otherwise.
 */
bool        RMNIsLinearDimension(OCTypeRef obj);

/** @} */ // end of RMNLinearDimension group

/** @} */ // end of RMNDimension overarching group

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // RMNDIMENSIONS_H