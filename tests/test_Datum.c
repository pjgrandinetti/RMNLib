#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "RMNLibrary.h"

void test_Datum_NULL_cases(void) {
    // Redirect stderr to hide expected warnings
    int saved_err = dup(fileno(stderr));
    FILE *nullf = fopen("/dev/null", "w");
    if (!nullf) {
        perror("Failed to open /dev/null");
        return;
    }
    dup2(fileno(nullf), fileno(stderr));

    // NULL-case tests
    assert(DatumGetTypeID() != 0);
    assert(DatumGetComponentIndex(NULL) == -1);
    assert(DatumGetDependentVariableIndex(NULL) == -1);
    assert(DatumGetMemOffset(NULL) == -1);
    assert(DatumCoordinatesCount(NULL) == 0);
    assert(DatumGetCoordinateAtIndex(NULL, 0) == NULL);
    assert(DatumCreate(NULL, NULL, 0, 0, 0) == NULL);
    assert(DatumCopy(NULL) == NULL);
    assert(!DatumHasSameReducedDimensionalities(NULL, NULL));
    assert(DatumCreateResponse(NULL) == NULL);
    assert(DatumCreateDictionary(NULL) == NULL);
    assert(DatumCreateWithDictionary(NULL, NULL) == NULL);

    // Restore stderr
    fflush(stderr);
    dup2(saved_err, fileno(stderr));
    close(saved_err);
    fclose(nullf);

    printf("Datum NULL-case tests passed.\n");
}

void test_Datum_functional(void) {
    // Functional tests
    SIScalarRef value = SIScalarCreateWithDouble(42.0, SIUnitDimensionlessAndUnderived());
    if (!value) {
        fprintf(stderr, "Failed to create SIScalar value.\n");
        return;
    }

    OCMutableArrayRef coords = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    if (!coords) {
        fprintf(stderr, "Failed to create coordinates array.\n");
        OCRelease(value);
        return;
    }

    SIScalarRef c0 = SIScalarCreateWithDouble(1.1, SIUnitDimensionlessAndUnderived());
    SIScalarRef c1 = SIScalarCreateWithDouble(2.2, SIUnitDimensionlessAndUnderived());
    if (!c0 || !c1) {
        fprintf(stderr, "Failed to create coordinate scalars.\n");
        OCRelease(value);
        OCRelease(coords);
        return;
    }

    OCArrayAppendValue(coords, c0);
    OCArrayAppendValue(coords, c1);

    DatumRef datum = DatumCreate(value, coords, 1, 2, 3);
    if (!datum) {
        fprintf(stderr, "Failed to create Datum.\n");
        OCRelease(value);
        OCRelease(coords);
        OCRelease(c0);
        OCRelease(c1);
        return;
    }

    assert(DatumGetDependentVariableIndex(datum) == 1);
    assert(DatumGetComponentIndex(datum) == 2);
    assert(DatumGetMemOffset(datum) == 3);
    assert(DatumCoordinatesCount(datum) == 2);

    SIScalarRef fetched = DatumGetCoordinateAtIndex(datum, 1);
    assert(fetched != NULL && SIScalarGetValue(fetched).doubleValue == 2.2);

    DatumRef copy = DatumCopy(datum);
    assert(copy != NULL);
    assert(DatumHasSameReducedDimensionalities(datum, copy));

    OCDictionaryRef dict = DatumCreateDictionary(datum);
    assert(dict != NULL);

    OCStringRef error = NULL;
    DatumRef fromDict = DatumCreateWithDictionary(dict, &error);
    assert(fromDict != NULL);
    assert(error == NULL);

    // Cleanup
    OCRelease(datum);
    OCRelease(copy);
    OCRelease(fromDict);
    OCRelease(coords);
    OCRelease(value);
    OCRelease(c0);
    OCRelease(c1);
    OCRelease(dict);
    OCRelease(error);

    printf("Datum functional tests passed.\n");
    // end of test_Datum function
}
