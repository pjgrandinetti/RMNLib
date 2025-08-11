/**
 * @file Dimension_private.h
 * @brief Private header for Dimension implementation modules
 *
 * This header contains the opaque struct definitions and internal declarations
 * shared across the modularized Dimension implementation files.
 *
 * This file should ONLY be included by Dimension implementation files:
 * - Dimension_core.c
 * - Dimension_accessors.c
 * - Dimension_data.c
 * - Dimension_operations.c
 * - Dimension_validation.c
 * - Dimension_utilities.c
 */
#ifndef DIMENSION_PRIVATE_H
#define DIMENSION_PRIVATE_H
// Include the public header for type declarations
#include "Dimension.h"
#ifdef __cplusplus
extern "C" {
#endif
// ============================================================================
// Constants for dimension serialization keys
// ============================================================================
#define kDimensionLabelKey "label"
#define kDimensionDescriptionKey "description"
#define kDimensionApplicationKey "application"
// LabeledDimension serialization keys
#define kLabeledDimensionCoordinateLabelsKey "labels"
// SIDimension serialization keys
#define kSIDimensionQuantityNameKey "quantity_name"
#define kSIDimensionOffsetKey "coordinates_offset"
#define kSIDimensionOriginKey "origin_offset"
#define kSIDimensionPeriodKey "period"
#define kSIDimensionPeriodicKey "periodic"
#define kSIDimensionScalingKey "scaling"
#define kSIDimensionReciprocalKey "reciprocal"
// SIMonotonicDimension serialization keys
#define kSIMonotonicDimensionCoordinatesKey "coordinates"
// SILinearDimension serialization keys
#define kSILinearDimensionCountKey "count"
#define kSILinearDimensionIncrementKey "increment"
#define kSILinearDimensionFFTKey "complex_fft"
/**
 * @brief The opaque Dimension (abstract base) implementation structure
 *
 * This struct contains all the internal fields and state for a base Dimension object.
 * It serves as the base for all dimension type hierarchies.
 */
struct impl_Dimension {
    //  Dimension
    OCBase base;
    OCStringRef label;
    OCStringRef description;
    OCMutableDictionaryRef metadata;
};
/**
 * @brief The opaque LabeledDimension implementation structure
 *
 * LabeledDimension represents a discrete dimension with string labels for each coordinate.
 */
struct impl_LabeledDimension {
    struct impl_Dimension _super;  // <-- inherit all base fields
    OCMutableArrayRef coordinateLabels;
};
/**
 * @brief The opaque SIDimension implementation structure
 *
 * SIDimension represents a quantitative dimension with SI units and numeric coordinates.
 */
struct impl_SIDimension {
    struct impl_Dimension _super;  // inherit all base‐Dimension fields
    OCStringRef quantityName;
    SIScalarRef offset;
    SIScalarRef origin;
    SIScalarRef period;
    bool periodic;
    dimensionScaling scaling;
};
/**
 * @brief The opaque SIMonotonicDimension implementation structure
 *
 * SIMonotonicDimension represents a quantitative dimension with monotonically ordered coordinates.
 */
struct impl_SIMonotonicDimension {
    struct impl_SIDimension _super;  // ← inherit all Dimension + SI fields
    // SIMonotonicDimension‐specific:
    SIDimensionRef reciprocal;
    OCMutableArrayRef coordinates;  // array of SIScalarRef (≥2 entries)
};
/**
 * @brief The opaque SILinearDimension implementation structure
 *
 * SILinearDimension represents a quantitative dimension with linearly spaced coordinates.
 */
struct impl_SILinearDimension {
    struct impl_SIDimension _super;  // inherit Dimension + SI fields
    SIDimensionRef reciprocal;       // optional reciprocal dimension
    OCIndex count;                   // number of points (>=2)
    SIScalarRef increment;           // spacing between points
    bool fft;                        // FFT flag
};
// Forward declarations for shared internal functions
void impl_InitBaseDimensionFields(DimensionRef dim);
// Core lifecycle functions (defined in Dimension_core.c)
bool impl_DimensionEqual(const void *a, const void *b);
void impl_DimensionFinalize(const void *obj);
OCStringRef impl_DimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_DimensionCreateJSON(const void *obj);
void *impl_DimensionDeepCopy(const void *obj);
// Core object creation functions (defined in Dimension_core.c)
DimensionRef impl_DimensionAllocate(void);
DimensionRef impl_DimensionCreate(OCStringRef label,
                                  OCStringRef description,
                                  OCDictionaryRef metadata,
                                  OCStringRef *outError);
DimensionRef impl_DimensionCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
OCDictionaryRef impl_DimensionCopyAsDictionary(DimensionRef dim);
// JSON infrastructure functions (defined in Dimension_core.c)
OCDictionaryRef impl_DimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
DimensionRef impl_DimensionCreateFromJSON(cJSON *json, OCStringRef *outError);
// LabeledDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_LabeledDimensionEqual(const void *a, const void *b);
void impl_LabeledDimensionFinalize(const void *obj);
OCStringRef impl_LabeledDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_LabeledDimensionCreateJSON(const void *obj);
void *impl_LabeledDimensionDeepCopy(const void *obj);
// SIDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_SIDimensionEqual(const void *a, const void *b);
void impl_SIDimensionFinalize(const void *obj);
OCStringRef impl_SIDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_SIDimensionCreateJSON(const void *obj);
void *impl_SIDimensionDeepCopy(const void *obj);
void impl_InitSIDimensionFields(SIDimensionRef dim);
// SIDimension validation functions (defined in Dimension_core.c)
bool impl_InitSIDimensionFieldsFromArgs(
    SIDimensionRef dim,
    OCStringRef label,
    OCStringRef description,
    OCDictionaryRef metadata,
    OCStringRef quantityName,
    SIScalarRef offset,
    SIScalarRef origin,
    SIScalarRef period,
    bool periodic,
    dimensionScaling scaling);
bool impl_validateOrDefaultScalar(
    const char *paramName,
    SIScalarRef *scalarPtr,
    SIUnitRef unit,
    SIDimensionalityRef dimensionality,
    OCStringRef *outError);
// SIMonotonicDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_SIMonotonicDimensionEqual(const void *a, const void *b);
void impl_SIMonotonicDimensionFinalize(const void *obj);
OCStringRef impl_SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_SIMonotonicDimensionCreateJSON(const void *obj);
void *impl_SIMonotonicDimensionDeepCopy(const void *obj);
// SILinearDimension core functions
bool impl_SILinearDimensionEqual(const void *a, const void *b);
void impl_SILinearDimensionFinalize(const void *obj);
OCStringRef impl_SILinearDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_SILinearDimensionCreateJSON(const void *obj);
void *impl_SILinearDimensionDeepCopy(const void *obj);
// JSON helper functions (declared non-static for cross-module access)
OCDictionaryRef SIDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
// Internal function declarations (defined in Dimension_core.c)
// These are purely internal functions not exposed in public API
// LabeledDimension - internal only
LabeledDimensionRef LabeledDimensionAllocate(void);
// SIDimension - internal only
SIDimensionRef SIDimensionAllocate(void);
// SIMonotonicDimension - internal only
SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void);
OCDictionaryRef SIMonotonicDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
// SILinearDimension - internal only
SILinearDimensionRef SILinearDimensionAllocate(void);
// Helper functions (defined in Dimension_core.c)
bool impl_SIDimensionIsReciprocalOf(SIDimensionRef src, SIDimensionRef rec, OCStringRef *outError);
// ============================================================================
// Private Get accessor functions (defined in Dimension_accessors.c)
// These return direct references to internal data and should NOT be in public API
// ============================================================================
// Dimension (Base Class) - Private Get functions
OCStringRef DimensionGetLabel(DimensionRef dim);
OCStringRef DimensionGetDescription(DimensionRef dim);
OCMutableDictionaryRef DimensionGetApplicationMetaData(DimensionRef dim);
// LabeledDimension - Private Get functions
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
// SIDimension - Private Get functions
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim);
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim);
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim);
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim);
// SIMonotonicDimension - Private Get functions
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim);
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim);
// SILinearDimension - Private Get functions
OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim);
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim);
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim);
#ifdef __cplusplus
}
#endif
#endif /* DIMENSION_PRIVATE_H */
