#pragma once
#ifndef DIMENSIONS_H
#define DIMENSIONS_H

#include "RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file Dimensions.h
 * @brief Public interface for Dimension types: base, LabeledDimension, SIDimension,
 *        SIMonotonicDimension, and SILinearDimension.
 */

// ----------------------------------------------------------------------------
// MARK: - Shared Types & Enums
// ----------------------------------------------------------------------------

typedef enum dimensionScaling {
    kDimensionScalingNone, /**< No scaling applied. */
    kDimensionScalingNMR   /**< NMR-specific scaling applied. */
} dimensionScaling;

/** Opaque handle for the abstract base Dimension. */
typedef struct impl_Dimension           *DimensionRef;

/** Opaque type for a labeled (categorical) dimension. */
typedef struct impl_LabeledDimension *LabeledDimensionRef;

/** Opaque type for a quantitative (SI) dimension. */
typedef struct impl_SIDimension *SIDimensionRef;

/** Opaque type for a monotonic SI dimension (ordered scalar samples). */
typedef struct impl_SIMonotonicDimension *SIMonotonicDimensionRef;

/** Opaque type for a regularly-spaced SI dimension (uniform grid / FFT). */
typedef struct impl_SILinearDimension *SILinearDimensionRef;


// ----------------------------------------------------------------------------
// MARK: - Dimension (Abstract Base)
// ----------------------------------------------------------------------------

OCTypeID        DimensionGetTypeID(void);
OCStringRef     DimensionGetLabel(DimensionRef dim);
bool            DimensionSetLabel(DimensionRef dim, OCStringRef label);
OCStringRef     DimensionGetDescription(DimensionRef dim);
bool            DimensionSetDescription(DimensionRef dim, OCStringRef desc);
OCDictionaryRef DimensionGetMetadata(DimensionRef dim);
bool            DimensionSetMetadata(DimensionRef dim, OCDictionaryRef dict);
DimensionRef    DimensionDeepCopy(DimensionRef original);


// ----------------------------------------------------------------------------
// MARK: - LabeledDimension
// ----------------------------------------------------------------------------

OCTypeID                LabeledDimensionGetTypeID(void);
LabeledDimensionRef     LabeledDimensionCreate(OCStringRef label,
                                              OCStringRef description,
                                              OCDictionaryRef metadata,
                                              OCArrayRef coordinateLabels);
LabeledDimensionRef     LabeledDimensionCreateWithCoordinateLabels(OCArrayRef labels);
OCArrayRef              LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
bool                    LabeledDimensionSetCoordinateLabels(LabeledDimensionRef dim,
                                                            OCArrayRef labels);
OCStringRef             LabeledDimensionGetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                                                   OCIndex index);
bool                    LabeledDimensionSetCoordinateLabelAtIndex(LabeledDimensionRef dim,
                                                                   OCIndex index,
                                                                   OCStringRef label);
OCDictionaryRef         LabeledDimensionCopyAsDictionary(LabeledDimensionRef dim);
LabeledDimensionRef     LabeledDimensionCreateFromDictionary(OCDictionaryRef dict);


// ----------------------------------------------------------------------------
// MARK: - SIDimension (Quantitative SI)
// ----------------------------------------------------------------------------

OCTypeID        SIDimensionGetTypeID(void);
SIDimensionRef  SIDimensionCreate(OCStringRef label,
                                  OCStringRef description,
                                  OCDictionaryRef metadata,
                                  OCStringRef quantityName,
                                  SIScalarRef offset,
                                  SIScalarRef origin,
                                  SIScalarRef period,
                                  bool periodic,
                                  dimensionScaling scaling);
OCStringRef     SIDimensionGetQuantity(SIDimensionRef dim);
bool            SIDimensionSetQuantity(SIDimensionRef dim,
                                       OCStringRef name);
SIScalarRef     SIDimensionGetOffset(SIDimensionRef dim);
bool            SIDimensionSetOffset(SIDimensionRef dim,
                                     SIScalarRef val);
SIScalarRef     SIDimensionGetOrigin(SIDimensionRef dim);
bool            SIDimensionSetOrigin(SIDimensionRef dim,
                                     SIScalarRef val);
SIScalarRef     SIDimensionGetPeriod(SIDimensionRef dim);
bool            SIDimensionSetPeriod(SIDimensionRef dim,
                                     SIScalarRef val);
bool            SIDimensionIsPeriodic(SIDimensionRef dim);
bool            SIDimensionSetPeriodic(SIDimensionRef dim,
                                       bool flag);
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim);
bool            SIDimensionSetScaling(SIDimensionRef dim,
                                      dimensionScaling scaling);
OCDictionaryRef SIDimensionCopyAsDictionary(SIDimensionRef dim);
SIDimensionRef  SIDimensionCreateFromDictionary(OCDictionaryRef dict);


// ----------------------------------------------------------------------------
// MARK: - SIMonotonicDimension
// ----------------------------------------------------------------------------

OCTypeID                    SIMonotonicDimensionGetTypeID(void);
SIMonotonicDimensionRef     SIMonotonicDimensionCreate(
    OCStringRef     label,
    OCStringRef     description,
    OCDictionaryRef metadata,
    OCStringRef     quantity,
    SIScalarRef     offset,
    SIScalarRef     origin,
    SIScalarRef     period,
    bool            periodic,
    dimensionScaling scaling,
    OCArrayRef      coordinates,
    SIDimensionRef  reciprocal);
OCArrayRef                  SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim);
bool                        SIMonotonicDimensionSetCoordinates(SIMonotonicDimensionRef dim,
                                                               OCArrayRef coords);
SIDimensionRef              SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim);
bool                        SIMonotonicDimensionSetReciprocal(SIMonotonicDimensionRef dim,
                                                              SIDimensionRef rec);
OCDictionaryRef             SIMonotonicDimensionCopyAsDictionary(SIMonotonicDimensionRef dim);
SIMonotonicDimensionRef     SIMonotonicDimensionCreateFromDictionary(OCDictionaryRef dict);


// ----------------------------------------------------------------------------
// MARK: - SILinearDimension
// ----------------------------------------------------------------------------

OCTypeID                SILinearDimensionGetTypeID(void);
SILinearDimensionRef    SILinearDimensionCreate(
    OCStringRef     label,
    OCStringRef     description,
    OCDictionaryRef metadata,
    OCStringRef     quantity,
    SIScalarRef     offset,
    SIScalarRef     origin,
    SIScalarRef     period,
    bool            periodic,
    dimensionScaling scaling,
    OCIndex         count,
    SIScalarRef     increment,
    bool            fft,
    SIDimensionRef  reciprocal);
OCIndex                 SILinearDimensionGetCount(SILinearDimensionRef dim);
bool                    SILinearDimensionSetCount(SILinearDimensionRef dim,
                                                 OCIndex count);
SIScalarRef             SILinearDimensionGetIncrement(SILinearDimensionRef dim);
bool                    SILinearDimensionSetIncrement(SILinearDimensionRef dim,
                                                      SIScalarRef inc);
SIScalarRef             SILinearDimensionGetReciprocalIncrement(SILinearDimensionRef dim);
bool                    SILinearDimensionIsFFT(SILinearDimensionRef dim);
bool                    SILinearDimensionSetFFT(SILinearDimensionRef dim,
                                                bool fft);
SIDimensionRef          SILinearDimensionGetReciprocal(SILinearDimensionRef dim);
bool                    SILinearDimensionSetReciprocal(SILinearDimensionRef dim,
                                                      SIDimensionRef rec);
OCDictionaryRef         SILinearDimensionCopyAsDictionary(SILinearDimensionRef dim);
SILinearDimensionRef    SILinearDimensionCreateFromDictionary(OCDictionaryRef dict);


OCIndex DimensionGetCount(DimensionRef dim);

#ifdef __cplusplus
}
#endif

#endif /* DIMENSIONS_H */

