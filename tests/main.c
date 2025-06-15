#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "RMNLibrary.h"
#include "test_Datum.h"
#include "test_Dimension.h"
#include "test_DependentVariable.h"
#include "test_utils.h"

int main(void) {
    int failures = 0;

    // --- Datum Tests ---
    printf("\n=== Running Datum Tests ===\n");
    if (!test_Datum_NULL_cases())    failures++;
    if (!test_Datum_functional())     failures++;

    // --- Dimension Tests ---
    fprintf(stderr, "\n=== Running Dimension Tests ===\n");
    if (!test_Dimension_base())           failures++;
    if (!test_LabeledDimension_basic())   failures++;

    // --- DependentVariable Tests ---
    fprintf(stderr, "\n=== Running DependentVariable Tests ===\n");
    if (!test_DependentVariable_base())        failures++;
    if (!test_DependentVariable_components())  failures++;
    if (!test_DependentVariable_values())      failures++;
    if (!test_DependentVariable_typeQueries()) failures++;
    if (!test_DependentVariable_complexCopy()) failures++;  
    if (!test_DependentVariable_invalidCreate()) failures++;  

    // --- Summary & Exit ---
    if (failures > 0) {
        fprintf(stderr,
                "\n%d test%s failed.\n",
                failures, failures > 1 ? "s" : "");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "\nAll tests passed successfully!\n");

    RMNLibTypesShutdown();
    return EXIT_SUCCESS;
}
