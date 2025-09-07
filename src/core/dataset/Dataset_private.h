/**
 * @file Dataset_private.h
 * @brief Private header for Dataset implementation modules
 *
 * This header contains the opaque struct definition and internal declarations
 * shared across the modularized Dataset implementation files.
 *
 * This file should ONLY be included by Dataset implementation files:
 * - Dataset_core.c
 * - Dataset_accessors.c
 * - Dataset_operations.c
 * - Dataset_io.c
 */
#ifndef DATASET_PRIVATE_H
#define DATASET_PRIVATE_H
// Include the public header for type declarations
#include "../Dataset.h"
#include "../dependent_variable/DependentVariable_private.h"
#ifdef __cplusplus
extern "C" {
#endif
// ============================================================================
// SERIALIZATION CONSTANTS
// ============================================================================
#define kDatasetCsdmEnvelopeKey "csdm"
#define kDatasetVersionKey "version"
#define kDatasetTimestampKey "timestamp"
#define kDatasetReadOnlyKey "read_only"
#define kDatasetGeoCoordinateKey "geographic_coordinate"
#define kDatasetTagsKey "tags"
#define kDatasetDescriptionKey "description"
#define kDatasetTitleKey "title"
#define kDatasetDimensionsKey "dimensions"
#define kDatasetDimensionPrecedenceKey "dimension_precedence"
#define kDatasetDependentVariablesKey "dependent_variables"
#define kDatasetFocusKey "focus"
#define kDatasetPreviousFocusKey "previous_focus"
#define kDatasetApplicationKey "application"
// ============================================================================
// OPAQUE STRUCT DEFINITION
// ============================================================================
/**
 * @brief The opaque Dataset implementation structure
 *
 * This struct contains all the internal fields and state for a Dataset object.
 * It must remain consistent across all implementation modules.
 */
struct impl_Dataset {
    OCBase base;
    OCStringRef version;                           // e.g. "1.0"
    OCStringRef timestamp;                         // ISO-8601 UTC
    GeographicCoordinateRef geographicCoordinate;  // or NULL if not set
    bool readOnly;                                 // default false
    OCMutableArrayRef dimensions;
    OCMutableArrayRef dependentVariables;
    OCMutableArrayRef tags;
    OCStringRef description;
    // RMN extras
    OCStringRef title;
    DatumRef focus;
    DatumRef previousFocus;
    OCMutableIndexArrayRef dimensionPrecedence;
    OCMutableDictionaryRef application;
};
// ============================================================================
// INTERNAL FUNCTION DECLARATIONS
// ============================================================================
// Core infrastructure functions (Dataset_core.c)
struct impl_Dataset *DatasetAllocate(void);
void impl_InitDatasetFields(DatasetRef ds);
bool impl_ValidateDatasetParameters(OCArrayRef dimensions,
                                    OCArrayRef dependentVariables,
                                    OCStringRef *outError);
// Core JSON functions (Dataset_core.c)
DatasetRef DatasetCreateFromJSON(cJSON *root, OCStringRef *outError);
DatumRef DatasetCreateDatumFromMemOffset(DatasetRef theDataset,
                                         OCIndex dependentVariableIndex,
                                         OCIndex componentIndex,
                                         OCIndex memOffset);
// I/O utility functions (Dataset_io.c)
bool ensure_directory(const char *dir, OCStringRef *outError);
bool ensure_parent_dirs(const char *fullpath, OCStringRef *outError);
bool derive_directory_from_path(const char *filepath, char *output, size_t output_size);
uint8_t *read_file_bytes(const char *path, size_t *out_len);
const char *parse_components_url_path(const char *url);
bool join_path(char *dest, size_t dest_size, const char *dir, char sep, const char *filename);
#ifdef __cplusplus
}
#endif
#endif  // DATASET_PRIVATE_H
