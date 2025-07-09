#ifndef TEST_CSDM_H
#define TEST_CSDM_H

#pragma once
#include <stdbool.h>

// Test importing inline (non-external) CSDM files
bool test_Dataset_import_inline(void);

// Test importing external CSDM files with binary component data
bool test_Dataset_import_external(void);

// Test failure case for invalid JSON path
bool test_Dataset_import_invalid_path(void);

#endif /* TEST_CSDM_H */