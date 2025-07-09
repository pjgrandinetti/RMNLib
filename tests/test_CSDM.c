#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "RMNLibrary.h"
#include "test_utils.h"

/// Test valid inline CSDM import (.csdf with internal data)
bool test_Dataset_import_inline(void) {
    printf("test_Dataset_import_inline...\n");
    bool ok = false;
    OCStringRef err = NULL;

    // Use known-good .csdf
    char *json = resolve_test_path("NMR/blochDecay/blochDecay.csdf");
    char *bin  = resolve_test_path("NMR/blochDecay");  // safe but unused

    DatasetRef ds = DatasetCreateWithImport(json, bin, &err);
    if (!ds) {
        fprintf(stderr, "[ERROR] import_inline failed\n");
        if (err) fprintf(stderr, "Reason: %s\n", OCStringGetCString(err));
        goto cleanup;
    }

    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) > 0);
    ok = true;

cleanup:
    OCRelease(ds);
    OCRelease(err);
    free(json);
    free(bin);
    printf("test_Dataset_import_inline %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

/// Test valid external CSDM import (.csdfe + binary blob)
bool test_Dataset_import_external(void) {
    printf("test_Dataset_import_external...\n");
    bool ok = false;
    OCStringRef err = NULL;

    char *json = resolve_test_path("BubbleNebula/bubble.csdm/Bubble.csdfe");
    char *bin  = resolve_test_path("BubbleNebula/bubble.csdm");

    DatasetRef ds = DatasetCreateWithImport(json, bin, &err);
    if (!ds) {
        fprintf(stderr, "[ERROR] import_external failed\n");
        if (err) fprintf(stderr, "Reason: %s\n", OCStringGetCString(err));
        goto cleanup;
    }

    OCArrayRef dvs = DatasetGetDependentVariables(ds);
    TEST_ASSERT(OCArrayGetCount(dvs) > 0);

    DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvs, 0);
    TEST_ASSERT(dv != NULL);
    TEST_ASSERT(DependentVariableGetComponentCount(dv) > 0);

    ok = true;

cleanup:
    OCRelease(ds);
    OCRelease(err);
    free(json);
    free(bin);
    printf("test_Dataset_import_external %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

/// Test failure case for missing JSON path
bool test_Dataset_import_invalid_path(void) {
    printf("test_Dataset_import_invalid_path...\n");
    bool ok = false;
    OCStringRef err = NULL;

    DatasetRef ds = DatasetCreateWithImport("nonexistent/path.csdf", ".", &err);
    TEST_ASSERT(ds == NULL);
    TEST_ASSERT(err != NULL);

    const char *msg = OCStringGetCString(err);
    TEST_ASSERT(strstr(msg, "cannot open JSON file") != NULL);

    ok = true;

cleanup:
    OCRelease(err);
    printf("test_Dataset_import_invalid_path %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
