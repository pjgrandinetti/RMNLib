/**
 * @file DependentVariable_private.h
 * @brief Private header for DependentVariable implementation modules
 *
 * This header contains the opaque struct definition and internal declarations
 * shared across the modularized DependentVariable implementation files.
 *
 * This file should ONLY be included by DependentVariable implementation files:
 * - DependentVariable_core.c
 * - DependentVariable_accessors.c
 * - DependentVariable_components.c
 * - DependentVariable_operations.c
 * - DependentVariable_dimensions.c
 */
#ifndef DEPENDENTVARIABLE_PRIVATE_H
#define DEPENDENTVARIABLE_PRIVATE_H
// Include the public header for type declarations
#include "DependentVariable.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief The opaque DependentVariable implementation structure
 *
 * This struct contains all the internal fields and state for a DependentVariable object.
 * It must remain consistent across all implementation modules.
 */
struct impl_DependentVariable {
    OCBase base;
    // SIQuantity Type attributes
    SIUnitRef unit;
    OCNumberType numericType;
    // Dependent Variable Type attributes
    OCStringRef name;
    OCStringRef description;
    OCMutableDictionaryRef metaData;
    OCStringRef quantityName;
    OCStringRef quantityType;
    // components...
    OCStringRef type;                   // "internal" / "external"
    OCStringRef encoding;               // "base64" / "none"
    OCStringRef componentsURL;          // for external‐only
    OCMutableArrayRef components;       // And OCArray of OCDataRef types
    OCMutableArrayRef componentLabels;  // And OCArray of OCStringRef types
    // sparse‐sampling metadata
    SparseSamplingRef sparseSampling;
    // weak back‐pointer
    OCTypeRef owner;
};
bool validateDependentVariableParameters(
    OCStringRef type,
    SIUnitRef unit,
    OCStringRef quantityName,
    OCStringRef quantityType,
    OCArrayRef componentLabels,
    OCIndex componentsCount,
    SparseSamplingRef sparseSampling);
#ifdef __cplusplus
}
#endif
#endif /* DEPENDENTVARIABLE_PRIVATE_H */
