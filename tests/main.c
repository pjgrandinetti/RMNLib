#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "RMNLibrary.h"
#include "test_Datum.h"

int main(void) {
    printf("=== Running RMNDatum Tests ===\n");

    // Seed random number generator for potential future tests
    srand((unsigned int)time(NULL));

    // Run tests
    test_Datum_NULL_cases();
    test_Datum_functional();

    printf("=== All RMNDatum tests passed ===\n");
    return EXIT_SUCCESS;
}
