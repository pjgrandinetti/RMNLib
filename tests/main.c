#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_Dataset.h"
#include "test_Datum.h"
#include "test_DependentVariable.h"
#include "test_Dimension.h"
#include "test_Dimension_JSON.h"
#include "test_SparseSampling.h"
#include "test_dimension_operations.h"
#include "test_utils.h"
// Cross-platform setenv function
static int cross_platform_setenv(const char *name, const char *value, int overwrite) {
#ifdef _WIN32
    // Windows implementation
    if (!overwrite && getenv(name)) {
        return 0;  // Don't overwrite existing value
    }
    size_t len = strlen(name) + strlen(value) + 2;  // +2 for '=' and '\0'
    char *env_str = malloc(len);
    if (!env_str) return -1;
    strcpy(env_str, name);
    strcat(env_str, "=");
    strcat(env_str, value);
    int result = putenv(env_str);
    // Note: We don't free env_str because putenv on Windows requires it to remain valid
    return result;
#else
    // Unix/Linux implementation
    return setenv(name, value, overwrite);
#endif
}
int main(void) {
    int failures = 0;
    printf("\n=== Running Datum Tests ===\n");
    if (!test_Datum_NULL_cases()) failures++;
    if (!test_Datum_functional()) failures++;
    fprintf(stderr, "\n=== Running Dimension Tests ===\n");
    if (!test_DimensionCreateAxisLabel()) failures++;
    if (!test_Dimension_base()) failures++;
    if (!test_LabeledDimension()) failures++;
    if (!test_SIDimension()) failures++;
    if (!test_SIMonotonic_and_SILinearDimension()) failures++;
    if (!test_minimal_monotonic()) failures++;
    if (!test_SILinearDimensionCreateCoordinates()) failures++;
    if (!test_AbsoluteCoordinates()) failures++;
    if (!test_DimensionPeriodOperations()) failures++;
    if (!test_monotonic_large_scale_values()) failures++;
    if (!test_DimensionMetadataRoundTrip()) failures++;
    fprintf(stderr, "\n=== Running Dimension JSON Tests ===\n");
    if (!test_Dimension_JSON_roundtrip()) failures++;
    if (!test_LabeledDimension_JSON_roundtrip()) failures++;
    if (!test_SIDimension_JSON_roundtrip()) failures++;
    if (!test_SIMonotonicDimension_JSON_roundtrip()) failures++;
    if (!test_SILinearDimension_JSON_roundtrip()) failures++;
    if (!test_Dimension_JSON_typed_vs_untyped()) failures++;
    if (!test_Dimension_JSON_application_metadata_always_typed()) failures++;
    if (!test_Dimension_JSON_error_handling()) failures++;
    if (!test_Dimension_JSON_inheritance_patterns()) failures++;
    fprintf(stderr, "\n=== Running Dimension Operations Tests ===\n");
    if (!test_SILinearDimensionCreateInverse()) failures++;
    if (!test_DimensionScalarMultiplication()) failures++;
    fprintf(stderr, "\n=== Running DependentVariable Tests ===\n");
    if (!test_DependentVariable_base()) failures++;
    if (!test_DependentVariable_components()) failures++;
    if (!test_DependentVariable_values()) failures++;
    if (!test_DependentVariable_typeQueries()) failures++;
    if (!test_DependentVariable_complexCopy()) failures++;
    if (!test_DependentVariable_invalidCreate()) failures++;
    if (!test_DependentVariable_internal_vs_external()) failures++;
    if (!test_DependentVariable_values_and_accessors()) failures++;
    if (!test_DependentVariable_type_queries()) failures++;
    if (!test_DependentVariable_sparse_sampling()) failures++;
    if (!test_DependentVariable_copy_and_roundtrip()) failures++;
    if (!test_DependentVariable_invalid_create()) failures++;
    if (!test_DependentVariable_arithmetic_operations()) failures++;
    fprintf(stderr, "\n=== Running Comprehensive Arithmetic Tests ===\n");
    if (!test_DependentVariable_arithmetic_comprehensive_types()) failures++;
    if (!test_DependentVariable_arithmetic_error_cases()) failures++;
    if (!test_DependentVariable_arithmetic_complex()) failures++;
    if (!test_DependentVariable_arithmetic_edge_cases()) failures++;
    if (!test_DependentVariable_arithmetic_large_scale()) failures++;
    if (!test_DependentVariable_arithmetic_integer_types()) failures++;
    if (!test_DependentVariable_copy_component_labels()) failures++;
    if (!test_DependentVariable_copy_component_label_at_index()) failures++;
    if (!test_DependentVariable_copy_component_at_index()) failures++;
    fprintf(stderr, "\n=== Running SparseSampling Tests ===\n");
    if (!test_SparseSampling_basic_create()) failures++;
    if (!test_SparseSampling_validation()) failures++;
    if (!test_SparseSampling_copy_and_equality()) failures++;
    if (!test_SparseSampling_dictionary_roundtrip()) failures++;
    if (!test_SparseSampling_invalid_create()) failures++;
    if (!test_SparseSampling_null_and_empty()) failures++;
    if (!test_SparseSampling_fully_sparse()) failures++;
    if (!test_SparseSampling_partially_sparse()) failures++;
    if (!test_SparseSampling_base64_encoding()) failures++;
    if (!test_SparseSampling_with_dataset()) failures++;
    if (!test_SparseSampling_size_calculations()) failures++;
    if (!test_SparseSampling_json_untyped_roundtrip()) failures++;
    if (!test_SparseSampling_json_typed_roundtrip()) failures++;
    if (!test_SparseSampling_json_malformed_input()) failures++;
    if (!test_SparseSampling_json_encoding_extraction()) failures++;
    fprintf(stderr, "\n=== Running Dataset Tests ===\n");
    if (!test_Dataset_minimal_create()) failures++;
    if (!test_DatasetCreateMinimal()) failures++;
    if (!test_Dataset_mutators()) failures++;
    if (!test_Dataset_type_contract()) failures++;
    if (!test_Dataset_copy_and_roundtrip()) failures++;
    if (!test_Dataset_rigorous_roundtrip()) failures++;
    if (failures > 0) {
        fprintf(stderr, "\n%d test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nAll tests passed successfully!\n");
    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
