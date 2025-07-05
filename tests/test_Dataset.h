#ifndef DATASET_TEST_H
#define DATASET_TEST_H

#include "Dataset.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Verify that creating a Dataset with minimal inputs
 *        (NULL dims, NULL precedence, one DV, NULL tags, NULL strings,
 *         NULL focus, NULL previousFocus, NULL ops, NULL meta)
 *        yields empty/default fields exactly as in impl_InitDatasetFields,
 *        and that the OCType contract holds.
 */
bool test_Dataset_minimal_create(void);

/**
 * @brief Verify all of the setters and getters on a Dataset:
 *        dimensions, precedence, dependentVariables, tags,
 *        description, title, metadata, operations,
 *        focus/previousFocus, and the base64 flag.
 */
bool test_Dataset_mutators(void);

/**
 * @brief Verify deep‐copy & dictionary‐round‐trip:
 *        DatasetCreateCopy and DatasetCopyAsDictionary/DatasetCreateFromDictionary,
 *        each yields a deep‐equal object.
 */
bool test_Dataset_copy_and_roundtrip(void);

/**
 * @brief Verify the OCType contract for Dataset:
 *        OCGetTypeID == DatasetGetTypeID, and retain/release adjust the retainCount.
 */
bool test_Dataset_type_contract(void);

#ifdef __cplusplus
}
#endif

#endif // DATASET_TEST_H