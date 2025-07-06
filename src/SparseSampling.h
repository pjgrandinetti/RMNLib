/**
 * @file SparseSampling.h
 * @brief OCType for representing sparse sampling of multi-dimensional grids.
 *
 * The SparseSampling OCType encapsulates:
 *   - A set of fixed dimension indexes.
 *   - A list of sparse grid vertices (each an OCIndexPairSetRef).
 *   - Underlying unsigned integer type for indexing.
 *   - Encoding of the sparse_grid_vertexes array ("none" or "base64").
 *   - Optional description and metadata.
 *
 * This type supports:
 *   - Serialization to/from OCDictionary.
 *   - JSON serialization.
 *   - Deep-copy, equality.
 *
 * .. seealso::
 *    :c:func:`DependentVariable`
 */
#ifndef SPARSE_SAMPLING_H
#define SPARSE_SAMPLING_H

#include "RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Valid values for the encoding of the sparse_grid_vertexes array. */
#define kSparseSamplingEncodingValueNone   "none"
#define kSparseSamplingEncodingValueBase64 "base64"

/**
 * @brief Retrieve the OCTypeID for SparseSampling, registering the type on first use.
 *
 * @returns Unique OCTypeID for SparseSampling.
 */
OCTypeID
SparseSamplingGetTypeID(void);

/**
 * @brief Validate that a SparseSampling is well-formed.
 *
 * Checks that
 *   - `unsignedIntegerType` is one of the allowed unsigned types,
 *   - `encoding` is either "none" or "base64",
 *   - `dimensionIndexes` is non-NULL,
 *   - every element of `sparseGridVertexes` is an OCIndexPairSetRef.
 *
 * @param ss        The SparseSamplingRef to validate.
 * @param outError  If non-NULL, on failure will be set to an OCStringRef
 *                  describing the first validation error (must be released).
 * @returns         true if `ss` passes all checks, false otherwise.
 */
bool
validateSparseSampling(SparseSamplingRef ss,
                       OCStringRef      *outError);
                       
/**
 * @brief Create a new SparseSampling object.
 *
 * @param dimensionIndexes    An OCIndexSetRef of fixed dimension indexes.
 *                            If NULL, initializes to empty set.
 * @param sparseGridVertexes  An OCArrayRef of OCIndexPairSetRef vertices.
 *                            If NULL, initializes to empty array.
 * @param unsignedIntegerType The numeric type used for indexing. Must be one of:
 *                            kOCNumberUInt8Type, kOCNumberUInt16Type,
 *                            kOCNumberUInt32Type, or kOCNumberUInt64Type.
 * @param encoding            Encoding applied to `sparse_grid_vertexes`. Must be
 *                            either kSparseSamplingEncodingValueNone or
 *                            kSparseSamplingEncodingValueBase64.
 * @param description         Optional OCStringRef description. May be NULL.
 * @param metadata            Optional OCDictionaryRef metadata. May be NULL.
 * @param outError            Optional output pointer for error string on failure.
 *
 * @returns New SparseSamplingRef on success (must be released), NULL on error.
 */
SparseSamplingRef
SparseSamplingCreate(OCIndexSetRef    dimensionIndexes,
                     OCArrayRef       sparseGridVertexes,
                     OCNumberType     unsignedIntegerType,
                     OCStringRef      encoding,
                     OCStringRef      description,
                     OCDictionaryRef  metadata,
                     OCStringRef     *outError);

/**
 * @brief Serialize a SparseSampling to an OCDictionary.
 *
 * @param ss  The SparseSamplingRef to serialize.
 * @returns   An OCDictionaryRef representing the object (caller must release).
 */
OCDictionaryRef
SparseSamplingCopyAsDictionary(SparseSamplingRef ss);

/**
 * @brief Create a SparseSampling object from an OCDictionary.
 *
 * @param dict      Source dictionary to parse.
 * @param outError  Optional output pointer for error string on failure.
 * @returns         New SparseSamplingRef on success (must be released), NULL on error.
 */
SparseSamplingRef
SparseSamplingCreateFromDictionary(OCDictionaryRef dict,
                                   OCStringRef    *outError);

/**
 * @brief Deserialize a SparseSampling from a cJSON object.
 *
 * @param json      cJSON object representing sparse_sampling.
 * @param outError  Optional pointer to receive an error string on failure.
 * @returns         New SparseSamplingRef (caller must release), or NULL on error.
 */
SparseSamplingRef
SparseSamplingCreateFromJSON(cJSON      *json,
                             OCStringRef *outError);

/**
 * @brief Validate that a SparseSampling is well-formed.
 *
 * Checks dimension indexes, vertex arrays, numeric types and encoding.
 *
 * @param ss        The SparseSampling to validate.
 * @param outError  Optional pointer to receive an error string on failure.
 * @returns         true if valid, false otherwise.
 */
bool
SparseSamplingValidate(SparseSamplingRef ss,
                       OCStringRef      *outError);

/**
 * @name SparseSampling Accessors
 * @{
 */

/**
 * @brief Get the set of fixed dimension indexes.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCIndexSetRef of indexes (do not release).
 */
OCIndexSetRef
SparseSamplingGetDimensionIndexes(SparseSamplingRef ss);

/**
 * @brief Set the fixed dimension indexes.
 *
 * @param ss     SparseSamplingRef object.
 * @param idxSet New OCIndexSetRef of indexes. May be NULL to clear.
 * @returns      true on success, false on NULL ss or allocation failure.
 */
bool
SparseSamplingSetDimensionIndexes(SparseSamplingRef ss,
                                  OCIndexSetRef      idxSet);

/**
 * @brief Get the array of sparse grid vertices.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCArrayRef of OCIndexPairSetRef vertices (do not release).
 */
OCArrayRef
SparseSamplingGetSparseGridVertexes(SparseSamplingRef ss);

/**
 * @brief Set the array of sparse grid vertices.
 *
 * @param ss    SparseSamplingRef object.
 * @param verts New OCArrayRef of OCIndexPairSetRef. May be NULL to clear.
 * @returns     true on success, false on NULL ss or allocation failure.
 */
bool
SparseSamplingSetSparseGridVertexes(SparseSamplingRef ss,
                                    OCArrayRef         verts);

/**
 * @brief Get the unsigned integer type used for indexing.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCNumberType enumerator.
 */
OCNumberType
SparseSamplingGetUnsignedIntegerType(SparseSamplingRef ss);

/**
 * @brief Set the unsigned integer type for indexing.
 *
 * Only unsigned types are allowed (UInt8, UInt16, UInt32, UInt64).
 *
 * @param ss    SparseSamplingRef object.
 * @param type  OCNumberType to set.
 * @returns     true on success, false on NULL ss or invalid type.
 */
bool
SparseSamplingSetUnsignedIntegerType(SparseSamplingRef ss,
                                     OCNumberType       type);

/**
 * @brief Get the encoding for sparse_grid_vertexes.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCStringRef encoding value (do not release).
 */
OCStringRef
SparseSamplingGetEncoding(SparseSamplingRef ss);

/**
 * @brief Set the encoding for sparse_grid_vertexes.
 *
 * Must be one of kSparseSamplingEncodingValueNone or kSparseSamplingEncodingValueBase64.
 *
 * @param ss        SparseSamplingRef object.
 * @param encoding  OCStringRef encoding to set.
 * @returns         true on success, false on NULL ss or invalid encoding.
 */
bool
SparseSamplingSetEncoding(SparseSamplingRef ss,
                          OCStringRef      encoding);

/**
 * @brief Get the human-readable description.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCStringRef description (do not release). May be empty string.
 */
OCStringRef
SparseSamplingGetDescription(SparseSamplingRef ss);

/**
 * @brief Set the human-readable description.
 *
 * @param ss   SparseSamplingRef object.
 * @param desc New OCStringRef description. May be NULL to clear.
 * @returns    true on success, false on NULL ss or allocation failure.
 */
bool
SparseSamplingSetDescription(SparseSamplingRef ss,
                             OCStringRef      desc);

/**
 * @brief Get the metadata dictionary.
 *
 * @param ss  SparseSamplingRef object.
 * @returns   OCDictionaryRef metadata (do not release). May be empty.
 */
OCDictionaryRef
SparseSamplingGetMetaData(SparseSamplingRef ss);

/**
 * @brief Set the metadata dictionary.
 *
 * @param ss        SparseSamplingRef object.
 * @param metadata  New OCDictionaryRef metadata. May be NULL to clear.
 * @returns         true on success, false on NULL ss or allocation failure.
 */
bool
SparseSamplingSetMetaData(SparseSamplingRef ss,
                          OCDictionaryRef   metadata);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  // SPARSE_SAMPLING_H
