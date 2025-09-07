#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_CSDM.h"
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
    fprintf(stderr, "\n=== Running CSDM Import and Roundtrip Test Only ===\n");
    // Set up CSDM test root if not already set
    if (!getenv("CSDM_TEST_ROOT")) {
        cross_platform_setenv("CSDM_TEST_ROOT",
                              "/Users/philip/Github/Software/OCTypes-SITypes/RMNLib/tests/CSDM-TestFiles-1.0",
                              1);
        fprintf(stderr, "[INFO] Defaulted CSDM_TEST_ROOT to hardcoded path.\n");
    }
    fprintf(stderr, "[INFO] CSDM_TEST_ROOT = %s\n",
            getenv("CSDM_TEST_ROOT"));
    // Run only the roundtrip test
    if (!test_Dataset_import_and_roundtrip()) failures++;
    if (failures > 0) {
        fprintf(stderr, "\nCSDP import and roundtrip test failed.\n");
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nCSDM import and roundtrip test passed successfully!\n");
    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
