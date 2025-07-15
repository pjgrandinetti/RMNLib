#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "RMNLibrary.h"
#include "test_CSDM.h"
#include "test_Dataset.h"
#include "test_Datum.h"
#include "test_DependentVariable.h"
#include "test_Dimension.h"
#include "test_SparseSampling.h"
#include "test_utils.h"
int main(void) {
    int failures = 0;
    printf("\n=== Running Datum Tests ===\n");
    if (!test_Datum_NULL_cases()) failures++;
    if (!test_Datum_functional()) failures++;
    fprintf(stderr, "\n=== Running Dimension Tests ===\n");
    if (!test_CreateDimensionLongLabel()) failures++;
    if (!test_Dimension_base()) failures++;
    if (!test_LabeledDimension()) failures++;
    if (!test_SIDimension()) failures++;
    if (!test_SIMonotonic_and_SILinearDimension()) failures++;
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
    fprintf(stderr, "\n=== Running Dataset Tests ===\n");
    if (!test_Dataset_minimal_create()) failures++;
    if (!test_Dataset_mutators()) failures++;
    if (!test_Dataset_type_contract()) failures++;
    if (!test_Dataset_copy_and_roundtrip()) failures++;
    fprintf(stderr, "\n=== Running CSDM Tests ===\n");
    if (!getenv("CSDM_TEST_ROOT")) {
        setenv("CSDM_TEST_ROOT",
               "/Users/philip/Github/Software/OCTypes-SITypes/RMNLib/tests/CSDM-TestFiles-1.0",
               1);
        fprintf(stderr, "[INFO] Defaulted CSDM_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] CSDM_TEST_ROOT = %s\n",
            getenv("CSDM_TEST_ROOT"));
    // Test single failing file for debugging
     if (!test_Dataset_import_all_csdm()) failures++;
    //  if (!test_Dataset_import_single_csdm()) failures++;
    if (failures > 0) {
        fprintf(stderr, "\n%d test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nAll tests passed successfully!\n");
    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
