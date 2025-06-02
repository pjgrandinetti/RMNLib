#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "RMNLibrary.h"

void test_RMNDatum_NULL_cases(void) {
    // Redirect stderr to hide expected warnings
    int saved_err = dup(fileno(stderr));
    FILE *nullf = fopen("/dev/null", "w");
    if (!nullf) {
        perror("Failed to open /dev/null");
        return;
    }
    dup2(fileno(nullf), fileno(stderr));

    // NULL-case tests
    assert(RMNDatumGetTypeID() != 0);
    assert(RMNDatumGetComponentIndex(NULL) == -1);
    assert(RMNDatumGetDependentVariableIndex(NULL) == -1);
    assert(RMNDatumGetMemOffset(NULL) == -1);
    assert(RMNDatumCoordinatesCount(NULL) == 0);
    assert(RMNDatumGetCoordinateAtIndex(NULL, 0) == NULL);
    assert(RMNDatumCreate(NULL, NULL, 0, 0, 0) == NULL);
    assert(RMNDatumCopy(NULL) == NULL);
    assert(!RMNDatumHasSameReducedDimensionalities(NULL, NULL));
    assert(RMNDatumCreateResponse(NULL) == NULL);
    assert(RMNDatumCreateDictionary(NULL) == NULL);
    assert(RMNDatumCreateWithDictionary(NULL, NULL) == NULL);

    // Restore stderr
    fflush(stderr);
    dup2(saved_err, fileno(stderr));
    close(saved_err);
    fclose(nullf);

    printf("RMNDatum NULL-case tests passed.\n");
}

void test_RMNDatum_functional(void) {
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

    RMNDatumRef datum = RMNDatumCreate(value, coords, 1, 2, 3);
    if (!datum) {
        fprintf(stderr, "Failed to create RMNDatum.\n");
        OCRelease(value);
        OCRelease(coords);
        OCRelease(c0);
        OCRelease(c1);
        return;
    }

    assert(RMNDatumGetDependentVariableIndex(datum) == 1);
    assert(RMNDatumGetComponentIndex(datum) == 2);
    assert(RMNDatumGetMemOffset(datum) == 3);
    assert(RMNDatumCoordinatesCount(datum) == 2);

    SIScalarRef fetched = RMNDatumGetCoordinateAtIndex(datum, 1);
    assert(fetched != NULL && SIScalarGetValue(fetched).doubleValue == 2.2);

    RMNDatumRef copy = RMNDatumCopy(datum);
    assert(copy != NULL);
    assert(RMNDatumHasSameReducedDimensionalities(datum, copy));

    OCDictionaryRef dict = RMNDatumCreateDictionary(datum);
    assert(dict != NULL);

    OCStringRef error = NULL;
    RMNDatumRef fromDict = RMNDatumCreateWithDictionary(dict, &error);
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

    printf("RMNDatum functional tests passed.\n");
    // end of test_RMNDatum function
}
