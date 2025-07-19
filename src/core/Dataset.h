#ifndef DATASET_H
#define DATASET_H
#include "Datum.h"
#include "DependentVariable.h"
#include "GeographicCoordinate.h"
#include "OCArray.h"
#include "OCDictionary.h"
#include "OCIndexArray.h"
#include "OCString.h"
#include "OCType.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @file Dataset.h
 * @brief Core API for the Dataset type and its serialization.
 *
 * A Dataset bundles:
 *  - a list of dimensions (with optional custom ordering),
 *  - one or more dependent variables (each potentially externalized),
 *  - string tags, title & description,
 *  - an optional focus datum and metadata dictionary.
 *
 * You can create one in‐memory, serialize it to a deep dictionary (for testing
 * or round‐trip), and read/write full CSDF/CSDFE files with Export/Import.
 */
/**
 * @ingroup IO
 * @brief Opaque handle for a Dataset object.
 */
typedef struct impl_Dataset *DatasetRef;
/**
 * @brief Return the unique OCTypeID for Dataset.
 */
OCTypeID DatasetGetTypeID(void);
/**
 * @brief Construct a new Dataset.
 *
 * Performs basic validation:
 *  - at least one dependent variable,
 *  - each DV’s length matches the product of all dimension sizes,
 *  - dimensions themselves have valid types.
 *
 * @param dimensions          Array of DimensionRef (may be NULL ⇒ scalar DV only).
 * @param dimensionPrecedence Optional index‐ordering array (NULL ⇒ natural order).
 * @param dependentVariables  Array of DependentVariableRef (must not be empty).
 * @param tags                Array of OCStringRef tags (may be NULL).
 * @param description         Short description (may be NULL or empty).
 * @param title               Human-readable title (may be NULL or empty).
 * @param focus               Optional DatumRef focus (may be NULL).
 * @param previousFocus       Optional previous focus (may be NULL).
 * @param metaData            Arbitrary key/value metadata (may be NULL).
 * @param[out] outError       If non-NULL and creation fails, set to an OCStringRef
 *                            describing the validation or allocation error.
 * @return Newly allocated DatasetRef on success, or NULL on failure.
 */
DatasetRef DatasetCreate(
    OCArrayRef        dimensions,
    OCIndexArrayRef   dimensionPrecedence,
    OCArrayRef        dependentVariables,
    OCArrayRef        tags,
    OCStringRef       description,
    OCStringRef       title,
    DatumRef          focus,
    DatumRef          previousFocus,
    OCDictionaryRef   metaData,
    OCStringRef      *outError);

/**
 * @brief Create a minimal Dataset with only dimensions and dependent variables.
 * 
 * This is a simplified version of DatasetCreate that only requires the essential
 * components: dimensions and dependent variables. All other fields are set to
 * sensible defaults (empty strings, NULL, etc.).
 *
 * @param dimensions         Array of DimensionRef (may be NULL for scalar data).
 * @param dependentVariables Array of DependentVariableRef (must not be empty).
 * @param[out] outError      If non-NULL and creation fails, set to an OCStringRef
 *                          describing the validation or allocation error.
 * @return Newly allocated DatasetRef on success, or NULL on failure.
 */
DatasetRef DatasetCreateMinimal(
    OCArrayRef      dimensions,
    OCArrayRef      dependentVariables,
    OCStringRef    *outError);

/**
 * @brief Helper: Create an empty Dataset (all fields NULL except outError).
 * @param[out] outError If non-NULL and creation fails, set to error string.
 * @return Newly allocated DatasetRef on success, or NULL on failure.
 */
static inline DatasetRef DatasetCreateEmpty(OCStringRef *outError) {
    return DatasetCreate(
        NULL, // dimensions
        NULL, // dimensionPrecedence
        NULL, // dependentVariables
        NULL, // tags
        NULL, // description
        NULL, // title
        NULL, // focus
        NULL, // previousFocus
        NULL, // metaData
        outError
    );
}
/**
 * @brief Rebuild a Dataset from a deep‐copied dictionary.
 * @param dict     Dictionary produced by DatasetCopyAsDictionary().
 * @param outError On error, set to an OCStringRef describing the problem.
 * @return DatasetRef or NULL on parse/factory failure.
 */
DatasetRef
DatasetCreateFromDictionary(OCDictionaryRef dict, OCStringRef *outError);
/**
 * @brief Serialize a Dataset into a deep‐copyable dictionary.
 *
 * Use this for tests, round-trip, or JSON conversion via cJSON.
 *
 * @param ds DatasetRef to serialize.
 * @return An OCDictionaryRef you must OCRelease() when done.
 */
OCDictionaryRef DatasetCopyAsDictionary(DatasetRef ds);
/**
 * @brief Convenience: deep-copy via CopyAsDictionary + CreateFromDictionary.
 * @param ds Source DatasetRef (must not be NULL).
 * @return New DatasetRef copy, or NULL if any error.
 */
static inline DatasetRef DatasetCreateCopy(DatasetRef ds) {
    if (!ds) return NULL;
    OCStringRef err = NULL;
    OCDictionaryRef d = DatasetCopyAsDictionary(ds);
    DatasetRef c = DatasetCreateFromDictionary(d, &err);
    OCRelease(d);
    if (!c) OCRelease(err);
    return c;
}
/** @name Accessors & Mutators
 * @{ */
/** @brief Get mutable array of Dimensions. */
OCMutableArrayRef DatasetGetDimensions(DatasetRef ds);
/** @brief Replace the dimensions array (must match existing DVs). */
bool DatasetSetDimensions(DatasetRef ds, OCMutableArrayRef dims);
/** @brief Get mutable index array for dimension precedence. */
OCMutableIndexArrayRef DatasetGetDimensionPrecedence(DatasetRef ds);
/** @brief Replace the dimension precedence ordering. */
bool DatasetSetDimensionPrecedence(DatasetRef ds, OCMutableIndexArrayRef order);
/** @brief Get mutable array of DependentVariable. */
OCMutableArrayRef DatasetGetDependentVariables(DatasetRef ds);
/** @brief Replace the dependent-variables list. */
bool DatasetSetDependentVariables(DatasetRef ds, OCMutableArrayRef dvs);
/** @brief How many dependent variables in this Dataset. */
OCIndex DatasetGetDependentVariableCount(DatasetRef ds);
/** @brief Fetch the i-th dependent variable. */
DependentVariableRef DatasetGetDependentVariableAtIndex(DatasetRef ds, OCIndex index);
/** @brief Get/replace tags. */
OCMutableArrayRef DatasetGetTags(DatasetRef ds);
bool DatasetSetTags(DatasetRef ds, OCMutableArrayRef tags);
/** @brief Get/replace description. */
OCStringRef DatasetGetDescription(DatasetRef ds);
bool DatasetSetDescription(DatasetRef ds, OCStringRef desc);
/** @brief Get/replace title. */
OCStringRef DatasetGetTitle(DatasetRef ds);
bool DatasetSetTitle(DatasetRef ds, OCStringRef title);
/** @brief Get/replace focus Datum. */
DatumRef DatasetGetFocus(DatasetRef ds);
bool DatasetSetFocus(DatasetRef ds, DatumRef focus);
/** @brief Get/replace previous focus Datum. */
DatumRef DatasetGetPreviousFocus(DatasetRef ds);
bool DatasetSetPreviousFocus(DatasetRef ds, DatumRef previousFocus);
/** @brief Get/replace arbitrary metadata dictionary. */
OCDictionaryRef DatasetGetMetaData(DatasetRef ds);
bool DatasetSetMetaData(DatasetRef ds, OCDictionaryRef md);
/** @} */
/** @defgroup IO Disk I/O: .csdf / .csdfe
 *  Read & write full Dataset + external blobs.
 *  @{
 */
/**
 * @brief Write a Dataset to disk.
 *
 * - Serializes the Dataset to JSON and writes to `json_path` (must end in
 *   “.csdf” if no externals, or “.csdfe” if any external DVs),
 * - Writes each external DV’s raw blob under `binary_dir/…`.
 *
 * @param ds         Dataset to export.
 * @param json_path  Full path to JSON file (.csdf/.csdfe).
 * @param binary_dir Directory under which to write external‐data files.
 * @param outError   On error, set to a brief OCStringRef.
 * @return true on success, false on failure (and `*outError` set).
 */
bool DatasetExport(DatasetRef ds, const char *json_path, const char *binary_dir, OCStringRef *outError);
/**
 * @brief Read a Dataset + externals back from disk.
 *
 * - Parses the JSON file at `json_path`,
 * - Constructs the Dataset object,
 * - Loads any external blobs from `binary_dir/…`.
 *
 * @param json_path  Path to JSON (.csdf/.csdfe).
 * @param binary_dir Directory where external-data files live.
 * @param outError   On error, set to a brief OCStringRef.
 * @return Newly allocated DatasetRef, or NULL on failure.
 */
DatasetRef DatasetCreateWithImport(const char *json_path, const char *binary_dir, OCStringRef *outError);
/** @} */
/** @name CSDM-1.0 Fields
 * @{ */
/** @brief Dataset version string (always “1.0”). */
OCStringRef DatasetGetVersion(DatasetRef ds);
/** @brief Override version (rarely needed). */
bool DatasetSetVersion(DatasetRef ds, OCStringRef version);
/** @brief ISO-8601 timestamp of serialization. */
OCStringRef DatasetGetTimestamp(DatasetRef ds);
/** @brief Override timestamp (rarely needed). */
bool DatasetSetTimestamp(DatasetRef ds, OCStringRef timestamp);
/** @brief Geographic coordinate, if set. */
GeographicCoordinateRef DatasetGetGeographicCoordinate(DatasetRef ds);
/** @brief Override geographic coordinate. */
bool DatasetSetGeographicCoordinate(DatasetRef ds, GeographicCoordinateRef gc);
/** @brief Read-only flag. */
bool DatasetGetReadOnly(DatasetRef ds);
/** @brief Set or clear read-only. */
bool DatasetSetReadOnly(DatasetRef ds, bool readOnly);
/** @} */

DependentVariableRef DatasetAddEmptyDependentVariable(DatasetRef theDataset,
                                                            OCStringRef quantityType,
                                                            OCNumberType elementType,
                                                            OCIndex size);


#ifdef __cplusplus
}
#endif
#endif  // DATASET_H