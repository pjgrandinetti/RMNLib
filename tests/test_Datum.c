#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "RMNLibrary.h"
#include "test_utils.h"


bool test_Datum_NULL_cases(void) {
    bool ok = true;
    int saved_err = dup(fileno(stderr));
    if (saved_err < 0) return false;
    FILE *nullf = fopen("/dev/null", "w");
    if (!nullf) {
        perror("Failed to open /dev/null");
        close(saved_err);
        return false;
    }
    dup2(fileno(nullf), fileno(stderr));

    // NULL-case tests
    TEST_ASSERT(DatumGetTypeID() != 0);
    TEST_ASSERT(DatumGetComponentIndex(NULL) == -1);
    TEST_ASSERT(DatumGetDependentVariableIndex(NULL) == -1);
    TEST_ASSERT(DatumGetMemOffset(NULL) == -1);
    TEST_ASSERT(DatumCoordinatesCount(NULL) == 0);
    TEST_ASSERT(DatumGetCoordinateAtIndex(NULL, 0) == NULL);
    TEST_ASSERT(DatumCreate(NULL, NULL, 0, 0, 0) == NULL);
    TEST_ASSERT(DatumCopy(NULL) == NULL);
    TEST_ASSERT(!DatumHasSameReducedDimensionalities(NULL, NULL));
    TEST_ASSERT(DatumCreateResponse(NULL) == NULL);
    TEST_ASSERT(DatumCreateDictionary(NULL) == NULL);
    TEST_ASSERT(DatumCreateWithDictionary(NULL, NULL) == NULL);

cleanup:
    fflush(stderr);
    dup2(saved_err, fileno(stderr));
    close(saved_err);
    fclose(nullf);

    if (ok) printf("Datum NULL-case tests passed.\n");
    return ok;
}

bool test_Datum_functional(void) {
    bool ok = true;

    // Create value scalar
    SIScalarRef value = SIScalarCreateWithDouble(42.0, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(value != NULL);

    // Create coordinates array
    OCMutableArrayRef coords = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    TEST_ASSERT(coords != NULL);

    SIScalarRef c0 = SIScalarCreateWithDouble(1.1, SIUnitDimensionlessAndUnderived());
    SIScalarRef c1 = SIScalarCreateWithDouble(2.2, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(c0 != NULL && c1 != NULL);
    OCArrayAppendValue(coords, c0);
    OCArrayAppendValue(coords, c1);

    // Create datum
    DatumRef datum = DatumCreate(value, coords, 1, 2, 3);
    TEST_ASSERT(datum != NULL);
    TEST_ASSERT(DatumGetDependentVariableIndex(datum) == 1);
    TEST_ASSERT(DatumGetComponentIndex(datum) == 2);
    TEST_ASSERT(DatumGetMemOffset(datum) == 3);
    TEST_ASSERT(DatumCoordinatesCount(datum) == 2);

    SIScalarRef fetched = DatumGetCoordinateAtIndex(datum, 1);
    TEST_ASSERT(fetched != NULL);
    TEST_ASSERT(SIScalarGetValue(fetched).doubleValue == 2.2);

    // Copy and compare
    DatumRef copy = DatumCopy(datum);
    TEST_ASSERT(copy != NULL);
    TEST_ASSERT(DatumHasSameReducedDimensionalities(datum, copy));

    // Dictionary round-trip
    OCDictionaryRef dict = DatumCreateDictionary(datum);
    TEST_ASSERT(dict != NULL);
    OCStringRef error = NULL;
    DatumRef fromDict = DatumCreateWithDictionary(dict, &error);
    TEST_ASSERT(fromDict != NULL);
    TEST_ASSERT(error == NULL);

cleanup:
    if (datum)    OCRelease(datum);
    if (copy)     OCRelease(copy);
    if (fromDict) OCRelease(fromDict);
    if (coords)   OCRelease(coords);
    if (value)    OCRelease(value);
    if (c0)       OCRelease(c0);
    if (c1)       OCRelease(c1);
    if (dict)     OCRelease(dict);
    if (error)    OCRelease(error);

    if (ok) printf("Datum functional tests passed.\n");
    return ok;
}