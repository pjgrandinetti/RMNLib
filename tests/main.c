#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "RMNLibrary.h"
#include "test_Datum.h"
#include "test_Dimension.h"
#include "test_utils.h"

int main(void) {
    int failures = 0;

    // --- Datum tests ---
    printf("=== Running Datum Tests ===\n");
    if (!test_Datum_NULL_cases()) failures++;
    if (!test_Datum_functional()) failures++;

    // --- Dimension tests ---
    fprintf(stderr, "\n=== Running Dimension Tests ===\n");
    if (!test_Dimension_base()) failures++;
    // if (!test_LabeledDimension_basic()) failures++;

    // --- Summary ---
    if (failures > 0) {
        fprintf(stderr,
                "\n%d test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "\nAll tests passed successfully!\n");
    return EXIT_SUCCESS;
}
