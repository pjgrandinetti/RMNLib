#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_utils.h"
bool test_Datum_NULL_cases(void) {
    bool ok = true;
    int saved_err = dup(fileno(stderr));
    if (saved_err < 0) return false;
#ifdef _WIN32
    FILE *nullf = fopen("NUL", "w");
#else
    FILE *nullf = fopen("/dev/null", "w");
#endif
    if (!nullf) {
#ifdef _WIN32
        perror("Failed to open NUL");
#else
        perror("Failed to open /dev/null");
#endif
        close(saved_err);
        return false;
    }
    dup2(fileno(nullf), fileno(stderr));
    if (DatumGetTypeID() == 0) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetComponentIndex(NULL) != kOCNotFound) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetDependentVariableIndex(NULL) != kOCNotFound) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetMemOffset(NULL) != kOCNotFound) {
        ok = false;
        goto cleanup;
    }
    if (DatumCoordinatesCount(NULL) != 0) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetCoordinateAtIndex(NULL, 0) != NULL) {
        ok = false;
        goto cleanup;
    }
    if (DatumCreate(NULL, NULL, 0, 0, 0) != NULL) {
        ok = false;
        goto cleanup;
    }
    if (DatumCopy(NULL) != NULL) {
        ok = false;
        goto cleanup;
    }
    if (DatumHasSameReducedDimensionalities(NULL, NULL)) {
        ok = false;
        goto cleanup;
    }
    if (DatumCreateResponse(NULL) != NULL) {
        ok = false;
        goto cleanup;
    }
    if (DatumCopyAsDictionary(NULL) != NULL) {
        ok = false;
        goto cleanup;
    }
    if (DatumCreateFromDictionary(NULL, NULL) != NULL) {
        ok = false;
        goto cleanup;
    }
cleanup:
    fflush(stderr);
    dup2(saved_err, fileno(stderr));
    close(saved_err);
    fclose(nullf);
    printf("Datum NULL-case tests %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
bool test_Datum_functional(void) {
    bool ok = true;
    SIScalarRef value = NULL;
    OCMutableArrayRef coords = NULL;
    SIScalarRef c0 = NULL;
    SIScalarRef c1 = NULL;
    DatumRef datum = NULL;
    DatumRef copy = NULL;
    OCDictionaryRef dict = NULL;
    OCStringRef error = NULL;
    DatumRef fromDict = NULL;
    // Create value scalar
    value = SIScalarCreateWithDouble(42.0, SIUnitDimensionlessAndUnderived());
    if (!value) {
        ok = false;
        goto cleanup;
    }
    // Create coordinates array
    coords = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    c0 = SIScalarCreateWithDouble(1.1, SIUnitDimensionlessAndUnderived());
    c1 = SIScalarCreateWithDouble(2.2, SIUnitDimensionlessAndUnderived());
    if (!coords || !c0 || !c1) {
        ok = false;
        goto cleanup;
    }
    OCArrayAppendValue(coords, c0);
    OCArrayAppendValue(coords, c1);
    // Create Datum
    datum = DatumCreate(value, coords, 1, 2, 3);
    if (!datum) {
        ok = false;
        goto cleanup;
    }
    // Basic property checks
    if (DatumGetDependentVariableIndex(datum) != 1) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetComponentIndex(datum) != 2) {
        ok = false;
        goto cleanup;
    }
    if (DatumGetMemOffset(datum) != 3) {
        ok = false;
        goto cleanup;
    }
    if (DatumCoordinatesCount(datum) != 2) {
        ok = false;
        goto cleanup;
    }
    SIScalarRef fetched = DatumGetCoordinateAtIndex(datum, 1);
    if (!fetched) {
        ok = false;
        goto cleanup;
    }
    if (SIScalarGetValue(fetched).doubleValue != 2.2) {
        fprintf(stderr, "[ERROR] Coordinate value mismatch: got %.3f\n",
                SIScalarGetValue(fetched).doubleValue);
        ok = false;
        goto cleanup;
    }
    // Deep copy check
    copy = DatumCopy(datum);
    if (!copy || !DatumHasSameReducedDimensionalities(datum, copy)) {
        fprintf(stderr, "[ERROR] Copy or dimensionality comparison failed\n");
        ok = false;
        goto cleanup;
    }
    // Dictionary round-trip
    dict = DatumCopyAsDictionary(datum);
    if (!dict) {
        ok = false;
        goto cleanup;
    }
    fromDict = DatumCreateFromDictionary(dict, &error);
    if (!fromDict) {
        fprintf(stderr, "[ERROR] DatumCreateFromDictionary failed\n");
        ok = false;
        goto cleanup;
    }
    if (error) {
        fprintf(stderr, "[ERROR] Round-trip returned error: %s\n", OCStringGetCString(error));
        ok = false;
        goto cleanup;
    }
    // Field checks after round-trip
    if (DatumGetDependentVariableIndex(fromDict) != 1 ||
        DatumGetComponentIndex(fromDict) != 2 ||
        DatumGetMemOffset(fromDict) != 3 ||
        DatumCoordinatesCount(fromDict) != 2) {
        fprintf(stderr, "[ERROR] Round-trip field mismatch\n");
        ok = false;
    }
cleanup:
    if (datum)    OCRelease(datum);
    if (copy)     OCRelease(copy);
    if (fromDict) OCRelease(fromDict);
    if (dict)     OCRelease(dict);
    if (coords)   OCRelease(coords);
    if (value)    OCRelease(value);
    if (c0)       OCRelease(c0);
    if (c1)       OCRelease(c1);
    if (error)    OCRelease(error);    
    printf("Datum functional tests %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
