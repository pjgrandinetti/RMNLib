#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_CSDM.h"
#include "test_Image.h"
#include "test_JCAMP.h"
#include "test_Tecmag.h"

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

    fprintf(stderr, "\n=== Running Import Tests Only ===\n");

    fprintf(stderr, "\n=== Running CSDM Import Tests ===\n");
    if (!getenv("CSDM_TEST_ROOT")) {
        cross_platform_setenv("CSDM_TEST_ROOT",
                              "/Users/philip/Github/Software/OCTypes-SITypes/RMNLib/tests/CSDM-TestFiles-1.0",
                              1);
        fprintf(stderr, "[INFO] Defaulted CSDM_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] CSDM_TEST_ROOT = %s\n",
            getenv("CSDM_TEST_ROOT"));
    if (!test_Dataset_import_and_roundtrip()) failures++;

    fprintf(stderr, "\n=== Running JCAMP Import Tests ===\n");
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

    fprintf(stderr, "\n=== Running Image Import Tests ===\n");
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

    fprintf(stderr, "\n=== Running Tecmag Import Tests ===\n");
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
        fprintf(stderr, "\n%d import test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nAll import tests passed successfully!\n");
    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
