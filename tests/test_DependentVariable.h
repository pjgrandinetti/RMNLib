#pragma once
#ifndef TEST_DEPENDENT_VARIABLE_H
#define TEST_DEPENDENT_VARIABLE_H
#include <stdbool.h>
bool test_DependentVariable_base(void);
bool test_DependentVariable_internal_vs_external(void);
bool test_DependentVariable_values_and_accessors(void);
bool test_DependentVariable_type_queries(void);
bool test_DependentVariable_sparse_sampling(void);
bool test_DependentVariable_copy_and_roundtrip(void);
bool test_DependentVariable_invalid_create(void);
bool test_DependentVariable_components(void);
bool test_DependentVariable_values(void);
bool test_DependentVariable_typeQueries(void);
bool test_DependentVariable_complexCopy(void);
bool test_DependentVariable_invalidCreate(void);
bool test_DependentVariable_copy_component_labels(void);
bool test_DependentVariable_copy_component_label_at_index(void);
bool test_DependentVariable_copy_component_at_index(void);
bool test_DependentVariable_arithmetic_operations(void);
// Comprehensive arithmetic tests
bool test_DependentVariable_arithmetic_comprehensive_types(void);
bool test_DependentVariable_arithmetic_error_cases(void);
bool test_DependentVariable_arithmetic_complex(void);
bool test_DependentVariable_arithmetic_edge_cases(void);
bool test_DependentVariable_arithmetic_large_scale(void);
bool test_DependentVariable_arithmetic_integer_types(void);
#endif  // TEST_DEPENDENT_VARIABLE_H
