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
    printf("=== Running RMNDatum Tests ===\n");
    // Seed random number generator for potential future tests
    srand((unsigned int)time(NULL));
    // Run tests
    test_Datum_NULL_cases();
    test_Datum_functional();
    printf("=== All RMNDatum tests passed ===\n");


    fprintf(stderr, "Starting Dimension test suite...\n");
    int failures = 0;
    if (!test_Dimension_base()) failures++;
    if (!test_LabeledDimension_basic()) failures++;
    // Add further calls as new tests are added:
    // if (!test_SIDimension_basic())    failures++;
    // if (!test_SIMonotonic_basic())    failures++;
    // if (!test_SILinear_basic())       failures++;
    if (failures) {
        fprintf(stderr, "\n%d test(s) failed.\n", failures);
        return EXIT_FAILURE;
    }
    fprintf(stderr, "\nAll dimension tests passed successfully!\n");
    return EXIT_SUCCESS;
}
