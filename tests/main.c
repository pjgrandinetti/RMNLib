#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_CSDM.h"
#include "test_JCAMP.h"
#include "test_Tecmag.h"
#include "test_Image.h"
#include "test_Dataset.h"
#include "test_Datum.h"
#include "test_DependentVariable.h"
#include "test_Dimension.h"
#include "test_SparseSampling.h"
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
    if (!test_CreateDimensionLongLabel()) failures++;
    if (!test_Dimension_base()) failures++;
    if (!test_LabeledDimension()) failures++;
    if (!test_SIDimension()) failures++;
    if (!test_SIMonotonic_and_SILinearDimension()) failures++;
    if (!test_minimal_monotonic()) failures++;
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
    if (!test_DatasetCreateMinimal()) failures++;
    if (!test_Dataset_mutators()) failures++;
    if (!test_Dataset_type_contract()) failures++;
    if (!test_Dataset_copy_and_roundtrip()) failures++;
    fprintf(stderr, "\n=== Running CSDM Tests ===\n");
    if (!getenv("CSDM_TEST_ROOT")) {
        cross_platform_setenv("CSDM_TEST_ROOT",
               "/Users/philip/Github/Software/OCTypes-SITypes/RMNLib/tests/CSDM-TestFiles-1.0",
               1);
        fprintf(stderr, "[INFO] Defaulted CSDM_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] CSDM_TEST_ROOT = %s\n",
            getenv("CSDM_TEST_ROOT"));
    if (!test_Dataset_import_and_roundtrip()) failures++;

    fprintf(stderr, "\n=== Running JCAMP Tests ===\n");
    if (!getenv("JCAMP_TEST_ROOT")) {
        cross_platform_setenv("JCAMP_TEST_ROOT",
               "tests/JCAMP",
               1);
        fprintf(stderr, "[INFO] Defaulted JCAMP_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] JCAMP_TEST_ROOT = %s\n",
            getenv("JCAMP_TEST_ROOT"));
    // if (!test_JCAMP_single_file()) failures++;
    if (!test_JCAMP_import_all()) failures++;

    fprintf(stderr, "\n=== Running Image Tests ===\n");
    if (!getenv("IMAGE_TEST_ROOT")) {
        cross_platform_setenv("IMAGE_TEST_ROOT",
               "tests/Images",
               1);
        fprintf(stderr, "[INFO] Defaulted IMAGE_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] IMAGE_TEST_ROOT = %s\n",
            getenv("IMAGE_TEST_ROOT"));
    if (!test_Image_dimensions()) failures++;
    if (!test_Image_memory_management()) failures++;
    if (!test_Image_single_file()) failures++;
    if (!test_Image_grayscale()) failures++;
    if (!test_Image_rgb()) failures++;
    if (!test_Image_multiple_images()) failures++;
    if (!test_Image_import_all()) failures++;

    fprintf(stderr, "\n=== Running Tecmag Tests ===\n");
    if (!getenv("TECMAG_TEST_ROOT")) {
        cross_platform_setenv("TECMAG_TEST_ROOT",
               "tests/Tecmag",
               1);
        fprintf(stderr, "[INFO] Defaulted TECMAG_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] TECMAG_TEST_ROOT = %s\n",
            getenv("TECMAG_TEST_ROOT"));
    // if (!test_Tecmag_single_file()) failures++;
    if (!test_Tecmag_import_all()) failures++;
    if (failures > 0) {
        fprintf(stderr, "\n%d test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nAll tests passed successfully!\n");
    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
