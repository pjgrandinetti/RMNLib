#pragma once
#ifndef TEST_SPARSE_SAMPLING_H
#define TEST_SPARSE_SAMPLING_H

#include <stdbool.h>

// Basic functionality tests
bool test_SparseSampling_basic_create(void);
bool test_SparseSampling_validation(void);
bool test_SparseSampling_copy_and_equality(void);
bool test_SparseSampling_dictionary_roundtrip(void);

// Edge cases and error handling
bool test_SparseSampling_invalid_create(void);
bool test_SparseSampling_null_and_empty(void);

// Sparse sampling scenarios
bool test_SparseSampling_fully_sparse(void);
bool test_SparseSampling_partially_sparse(void);
bool test_SparseSampling_base64_encoding(void);

// Integration with datasets
bool test_SparseSampling_with_dataset(void);
bool test_SparseSampling_size_calculations(void);

#endif // TEST_SPARSE_SAMPLING_H
