/**
 * @file Dimension_private.h
 * @brief Private header for Dimension implementation modules
 *
 * This header contains the opaque struct definitions and internal declarations
 * shared across the modularized Dimension implementation files.
 *
 * INHERITANCE HIERARCHY:
 * =====================
 * Dimension (base type)
 * ├── LabeledDimension (inherits from Dimension)
 * └── SIDimension (inherits from Dimension)
 *     ├── SIMonotonicDimension (inherits from SIDimension)
 *     └── SILinearDimension (inherits from SIDimension)
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
// SERIALIZATION CONSTANTS
// ============================================================================
// Base Dimension serialization keys
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
// ============================================================================
// TYPE HIERARCHY: OPAQUE STRUCT DEFINITIONS
// ============================================================================
/**
 * @brief BASE TYPE: Dimension (abstract base)
 *
 * The root of the dimension type hierarchy. Contains all common fields
 * shared by all dimension types.
 */
struct impl_Dimension {
    OCBase base;                      // OCTypes base object
    OCStringRef label;                // Human-readable label
    OCStringRef description;          // Extended description
    OCMutableDictionaryRef metadata;  // Application-specific metadata
};
// ----------------------------------------------------------------------------
// FIRST-LEVEL INHERITANCE: Types that inherit directly from Dimension
// ----------------------------------------------------------------------------
/**
 * @brief LabeledDimension (inherits from Dimension)
 *
 * Represents a discrete dimension with string labels for each coordinate.
 * Used for categorical or nominal data.
 */
struct impl_LabeledDimension {
    struct impl_Dimension _super;        // Inherit all base Dimension fields
    OCMutableArrayRef coordinateLabels;  // Array of OCStringRef labels
};
/**
 * @brief SIDimension (inherits from Dimension)
 *
 * Represents a quantitative dimension with SI units and numeric coordinates.
 * Base class for all scientifically measurable dimensions.
 */
struct impl_SIDimension {
    struct impl_Dimension _super;  // Inherit all base Dimension fields
    OCStringRef quantityName;      // Physical quantity name (e.g., "time", "frequency")
    SIScalarRef offset;            // Coordinate system offset
    SIScalarRef origin;            // Physical origin point
    SIScalarRef period;            // Period for periodic dimensions
    dimensionScaling scaling;      // Scaling type (linear, logarithmic, etc.)
};
// ----------------------------------------------------------------------------
// SECOND-LEVEL INHERITANCE: Types that inherit from SIDimension
// ----------------------------------------------------------------------------
/**
 * @brief SIMonotonicDimension (inherits from SIDimension)
 *
 * Represents a quantitative dimension with monotonically ordered coordinates.
 * Used for irregular but ordered sampling (e.g., non-uniform time points).
 */
struct impl_SIMonotonicDimension {
    struct impl_SIDimension _super;  // Inherit all Dimension + SIDimension fields
    SIDimensionRef reciprocal;       // Optional reciprocal dimension
    OCMutableArrayRef coordinates;   // Array of SIScalarRef (≥2 entries, monotonic)
};
/**
 * @brief SILinearDimension (inherits from SIDimension)
 *
 * Represents a quantitative dimension with linearly spaced coordinates.
 * Used for regular sampling (e.g., uniform time intervals, spectral grids).
 */
struct impl_SILinearDimension {
    struct impl_SIDimension _super;  // Inherit all Dimension + SIDimension fields
    SIDimensionRef reciprocal;       // Optional reciprocal dimension
    OCIndex count;                   // Number of points (≥2)
    SIScalarRef increment;           // Spacing between points
    bool fft;                        // FFT processing flag
};
// ============================================================================
// FUNCTION DECLARATIONS - ORGANIZED BY INHERITANCE HIERARCHY
// ============================================================================
// ----------------------------------------------------------------------------
// BASE TYPE: Dimension - Core Functions
// ----------------------------------------------------------------------------
// Shared base initialization
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
// Base Dimension - Private accessor functions (defined in Dimension_accessors.c)
OCStringRef DimensionGetLabel(DimensionRef dim);
OCStringRef DimensionGetDescription(DimensionRef dim);
OCMutableDictionaryRef DimensionGetApplicationMetaData(DimensionRef dim);
// ----------------------------------------------------------------------------
// FIRST-LEVEL INHERITANCE: LabeledDimension
// ----------------------------------------------------------------------------
// LabeledDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_LabeledDimensionEqual(const void *a, const void *b);
void impl_LabeledDimensionFinalize(const void *obj);
OCStringRef impl_LabeledDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_LabeledDimensionCreateJSON(const void *obj);
void *impl_LabeledDimensionDeepCopy(const void *obj);
// LabeledDimension - Internal allocation (defined in Dimension_core.c)
LabeledDimensionRef LabeledDimensionAllocate(void);
// LabeledDimension - Private accessor functions (defined in Dimension_accessors.c)
OCArrayRef LabeledDimensionGetCoordinateLabels(LabeledDimensionRef dim);
// ----------------------------------------------------------------------------
// FIRST-LEVEL INHERITANCE: SIDimension
// ----------------------------------------------------------------------------
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
    dimensionScaling scaling);
bool impl_validateOrDefaultScalar(
    const char *paramName,
    SIScalarRef *scalarPtr,
    SIUnitRef unit,
    SIDimensionalityRef dimensionality,
    OCStringRef *outError);
// SIDimension - Internal allocation (defined in Dimension_core.c)
SIDimensionRef SIDimensionAllocate(void);
// SIDimension JSON helper functions (declared non-static for cross-module access)
OCDictionaryRef SIDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
// SIDimension - Private accessor functions (defined in Dimension_accessors.c)
OCStringRef SIDimensionGetQuantityName(SIDimensionRef dim);
SIScalarRef SIDimensionGetCoordinatesOffset(SIDimensionRef dim);
SIScalarRef SIDimensionGetOriginOffset(SIDimensionRef dim);
SIScalarRef SIDimensionGetPeriod(SIDimensionRef dim);
dimensionScaling SIDimensionGetScaling(SIDimensionRef dim);
// Base Dimension - Private accessor functions (defined in Dimension_accessors.c)
OCStringRef DimensionGetLabel(DimensionRef dim);
OCStringRef DimensionGetDescription(DimensionRef dim);
// SIDimension helper functions (defined in Dimension_core.c)
bool impl_SIDimensionIsReciprocalOf(SIDimensionRef src, SIDimensionRef rec, OCStringRef *outError);
// ----------------------------------------------------------------------------
// SECOND-LEVEL INHERITANCE: SIMonotonicDimension (inherits from SIDimension)
// ----------------------------------------------------------------------------
// SIMonotonicDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_SIMonotonicDimensionEqual(const void *a, const void *b);
void impl_SIMonotonicDimensionFinalize(const void *obj);
OCStringRef impl_SIMonotonicDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_SIMonotonicDimensionCreateJSON(const void *obj);
void *impl_SIMonotonicDimensionDeepCopy(const void *obj);
// SIMonotonicDimension - Internal allocation (defined in Dimension_core.c)
SIMonotonicDimensionRef SIMonotonicDimensionAllocate(void);
OCDictionaryRef SIMonotonicDimensionDictionaryCreateFromJSON(cJSON *json, OCStringRef *outError);
// SIMonotonicDimension - Private accessor functions (defined in Dimension_accessors.c)
OCArrayRef SIMonotonicDimensionGetCoordinates(SIMonotonicDimensionRef dim);
SIDimensionRef SIMonotonicDimensionGetReciprocal(SIMonotonicDimensionRef dim);
// ----------------------------------------------------------------------------
// SECOND-LEVEL INHERITANCE: SILinearDimension (inherits from SIDimension)
// ----------------------------------------------------------------------------
// SILinearDimension core lifecycle functions (defined in Dimension_core.c)
bool impl_SILinearDimensionEqual(const void *a, const void *b);
void impl_SILinearDimensionFinalize(const void *obj);
OCStringRef impl_SILinearDimensionCopyFormattingDesc(OCTypeRef cf);
cJSON *impl_SILinearDimensionCreateJSON(const void *obj);
void *impl_SILinearDimensionDeepCopy(const void *obj);
// SILinearDimension - Internal allocation (defined in Dimension_core.c)
SILinearDimensionRef SILinearDimensionAllocate(void);
// SILinearDimension - Private accessor functions (defined in Dimension_accessors.c)
OCIndex SILinearDimensionGetCount(SILinearDimensionRef dim);
SIScalarRef SILinearDimensionGetIncrement(SILinearDimensionRef dim);
SIDimensionRef SILinearDimensionGetReciprocal(SILinearDimensionRef dim);
#ifdef __cplusplus
}
#endif
#endif /* DIMENSION_PRIVATE_H */
