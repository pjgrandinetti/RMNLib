#ifndef DATASET_TEST_H
#define DATASET_TEST_H
#include <stdbool.h>
#include "Dataset.h"
#ifdef __cplusplus
extern "C" {
#endif
bool test_Dataset_minimal_create(void);
bool test_DatasetCreateMinimal(void);
bool test_Dataset_mutators(void);
bool test_Dataset_copy_and_roundtrip(void);
bool test_Dataset_rigorous_roundtrip(void);
bool test_Dataset_type_contract(void);
bool test_Dataset_open_blank_csdf(void);
bool test_Dataset_open_blochDecay_base64_csdf(void);
#ifdef __cplusplus
}
#endif
#endif  // DATASET_TEST_H
