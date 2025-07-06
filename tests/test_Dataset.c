#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "RMNLibrary.h"
#include "test_utils.h"

// Helper
static DependentVariableRef _make_mock_dv(void) {
    return DependentVariableCreateDefault(STR("scalar"),
                                          kSINumberFloat64Type,
                                          1,
                                          NULL);
}

bool test_Dataset_minimal_create(void) {
    printf("test_Dataset_minimal_create...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) {
        fprintf(stderr, "[ERROR] failed to create mock DV\n");
        goto cleanup1;
    }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    // create with only DVs, other fields default
    DatasetRef ds = DatasetCreate(NULL, NULL, dvs,
                                  NULL, NULL, NULL,
                                  NULL, NULL, NULL);
    if (!ds) {
        fprintf(stderr, "[ERROR] DatasetCreate(minimal) returned NULL\n");
        goto cleanup1;
    }

    // ——— Minimal default‐state sanity checks ———
    //   * Exactly one dependent‐variable
    //   * All other collections you didn't explicitly set are empty
    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) == 1);
    TEST_ASSERT(OCArrayGetCount(DatasetGetDimensions(       ds)) == 0);
    TEST_ASSERT(OCArrayGetCount(DatasetGetTags(             ds)) == 0);
    TEST_ASSERT(OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) == 0);
    TEST_ASSERT(OCDictionaryGetCount(DatasetGetMetaData(ds)) == 0);

    // type and retain behavior (unchanged)
    if (OCGetTypeID(ds) != DatasetGetTypeID()) {
        fprintf(stderr, "[ERROR] minimal_create: typeID mismatch\n");
        goto cleanup;
    }
    int cnt = OCTypeGetRetainCount(ds);
    OCRetain(ds);
    if (OCTypeGetRetainCount(ds) != cnt + 1) {
        fprintf(stderr, "[ERROR] minimal_create: retain count did not increment\n");
        goto cleanup;
    }
    OCRelease(ds);

    ok = true;

cleanup:
    OCRelease(ds);
cleanup1:
    OCRelease(dvs);
    printf("test_Dataset_minimal_create %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_mutators(void) {
    printf("test_Dataset_mutators...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) { fprintf(stderr, "[ERROR] mutators: mock DV failure\n"); goto cleanup0; }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    DatasetRef ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ds) { fprintf(stderr, "[ERROR] mutators: DatasetCreate failed\n"); goto cleanup0; }

    OCMutableArrayRef dims = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dims, STR("D0"));
    if (!DatasetSetDimensions(ds, dims) || OCArrayGetCount(DatasetGetDimensions(ds)) != 1) {
        fprintf(stderr, "[ERROR] mutators: SetDimensions failed\n");
        goto cleanup1;
    }

    OCMutableIndexArrayRef order = OCIndexArrayCreateMutable(0);
    OCIndexArrayAppendValue(order, 0);
    if (!DatasetSetDimensionPrecedence(ds, order) ||
        OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) != 1) {
        fprintf(stderr, "[ERROR] mutators: SetDimensionPrecedence failed\n");
        goto cleanup1;
    }

    OCMutableArrayRef newDVs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!DatasetSetDependentVariables(ds, newDVs) ||
        OCArrayGetCount(DatasetGetDependentVariables(ds)) != 0) {
        fprintf(stderr, "[ERROR] mutators: SetDependentVariables failed\n");
        goto cleanup1;
    }

    OCMutableArrayRef tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(tags, STR("foo"));
    OCArrayAppendValue(tags, STR("bar"));
    if (!DatasetSetTags(ds, tags) || OCArrayGetCount(DatasetGetTags(ds)) != 2) {
        fprintf(stderr, "[ERROR] mutators: SetTags failed\n");
        goto cleanup1;
    }

    if (!DatasetSetDescription(ds, STR("hello")) ||
        !OCStringEqual(DatasetGetDescription(ds), STR("hello")) ||
        !DatasetSetTitle(ds, STR("world")) ||
        !OCStringEqual(DatasetGetTitle(ds), STR("world"))) {
        fprintf(stderr, "[ERROR] mutators: title/description failed\n");
        goto cleanup1;
    }

    OCDictionaryRef md = OCDictionaryCreateMutable(0);
    OCDictionarySetValue((OCMutableDictionaryRef)md, STR("k"), STR("v"));
    if (!DatasetSetMetaData(ds, md) || OCDictionaryGetCount(DatasetGetMetaData(ds)) != 1) {
        fprintf(stderr, "[ERROR] mutators: SetMetaData failed\n");
        goto cleanup1;
    }

    if (!DatasetSetFocus(ds, NULL) || DatasetGetFocus(ds) != NULL ||
        !DatasetSetPreviousFocus(ds, NULL) || DatasetGetPreviousFocus(ds) != NULL) {
        fprintf(stderr, "[ERROR] mutators: focus handling failed\n");
        goto cleanup1;
    }

    ok = true;

cleanup1:
    OCRelease(dims);
    OCRelease(order);
    OCRelease(newDVs);
    OCRelease(tags);
    OCRelease(md);
    OCRelease(ds);
cleanup0:
    OCRelease(dvs);

    printf("test_Dataset_mutators %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_type_contract(void) {
    printf("test_Dataset_type_contract...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) goto cleanup0;
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    DatasetRef ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ds) goto cleanup0;

    if (OCGetTypeID(ds) != DatasetGetTypeID()) {
        fprintf(stderr, "[ERROR] type_contract: OCGetTypeID mismatch\n");
        goto cleanup1;
    }

    int cnt = OCTypeGetRetainCount(ds);
    OCRetain(ds);
    if (OCTypeGetRetainCount(ds) != cnt + 1) {
        fprintf(stderr, "[ERROR] type_contract: retain count did not increment\n");
        goto cleanup1;
    }
    OCRelease(ds);

    ok = true;

cleanup1:
    OCRelease(ds);
cleanup0:
    OCRelease(dvs);

    printf("test_Dataset_type_contract %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_copy_and_roundtrip(void) {
    printf("test_Dataset_copy_and_roundtrip...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) goto cleanup0;
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    DatasetRef ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ds) goto cleanup0;

    DatasetRef copy = DatasetCreateCopy(ds);
    if (!copy || !OCTypeEqual(ds, copy)) {
        fprintf(stderr, "[ERROR] copy_roundtrip: DatasetCreateCopy failed or unequal\n");
        goto cleanup1;
    }

    OCDictionaryRef dict = DatasetCopyAsDictionary(ds);
    if (!dict) goto cleanup2;

    DatasetRef rt = DatasetCreateFromDictionary(dict, NULL);
    if (!rt || !OCTypeEqual(ds, rt)) {
        fprintf(stderr, "[ERROR] copy_roundtrip: roundtrip failed or unequal\n");
        goto cleanup3;
    }

    ok = true;

cleanup3:
    OCRelease(rt);
cleanup2:
    OCRelease(dict);
cleanup1:
    OCRelease(copy);
cleanup0:
    OCRelease(ds);
    OCRelease(dvs);

    printf("test_Dataset_copy_and_roundtrip %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
