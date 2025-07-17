#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_utils.h"

// Helper to create a simple DependentVariable
static DependentVariableRef _make_mock_dv(void) {
    return DependentVariableCreateDefault(STR("scalar"),
                                          kOCNumberFloat64Type,
                                          1,
                                          NULL);
}

bool test_Dataset_minimal_create(void) {
    printf("test_Dataset_minimal_create...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DatasetRef ds = NULL;

    DependentVariableRef dv = _make_mock_dv();
    if (!dv) {
        fprintf(stderr, "[ERROR] minimal_create: mock DV failed\n");
        goto cleanup;
    }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    // Create with only DVs; other params default
    ds = DatasetCreate(NULL,    // dimensions
                       NULL,    // dimensionPrecedence
                       dvs,     // dependentVariables
                       NULL,    // tags
                       NULL,    // description
                       NULL,    // title
                       NULL,    // focus
                       NULL,    // previousFocus
                       NULL,    // metaData
                       NULL);   // outError
    TEST_ASSERT(ds != NULL);

    // Sanity checks
    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) == 1);
    TEST_ASSERT(OCArrayGetCount(DatasetGetDimensions(ds)) == 0);
    TEST_ASSERT(OCArrayGetCount(DatasetGetTags(ds)) == 0);
    TEST_ASSERT(OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) == 0);
    TEST_ASSERT(OCDictionaryGetCount(DatasetGetMetaData(ds)) == 0);

    // Type ID and retain behavior
    TEST_ASSERT(OCGetTypeID(ds) == DatasetGetTypeID());
    int cnt = OCTypeGetRetainCount(ds);
    OCRetain(ds);
    TEST_ASSERT(OCTypeGetRetainCount(ds) == cnt + 1);
    OCRelease(ds);

    ok = true;

cleanup:
    OCRelease(ds);
    OCRelease(dvs);
    printf("test_Dataset_minimal_create %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_mutators(void) {
    printf("test_Dataset_mutators...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableArrayRef dims = NULL;
    OCMutableIndexArrayRef order = NULL;
    OCMutableArrayRef newDVs = NULL;
    OCMutableArrayRef tags = NULL;
    OCDictionaryRef md = NULL;
    DatasetRef ds = NULL;

    DependentVariableRef dv = _make_mock_dv();
    TEST_ASSERT(dv != NULL);
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT(ds != NULL);

    // Dimensions
    dims = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dims, STR("D0"));
    TEST_ASSERT(DatasetSetDimensions(ds, dims));
    TEST_ASSERT(OCArrayGetCount(DatasetGetDimensions(ds)) == 1);

    // Dimension precedence
    order = OCIndexArrayCreateMutable(0);
    OCIndexArrayAppendValue(order, 0);
    TEST_ASSERT(DatasetSetDimensionPrecedence(ds, order));
    TEST_ASSERT(OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) == 1);

    // Dependent variables → set to empty
    newDVs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(DatasetSetDependentVariables(ds, newDVs));
    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) == 0);

    // Tags
    tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(tags, STR("foo"));
    OCArrayAppendValue(tags, STR("bar"));
    TEST_ASSERT(DatasetSetTags(ds, tags));
    TEST_ASSERT(OCArrayGetCount(DatasetGetTags(ds)) == 2);

    // Description & title
    TEST_ASSERT(DatasetSetDescription(ds, STR("hello")));
    TEST_ASSERT(OCStringEqual(DatasetGetDescription(ds), STR("hello")));
    TEST_ASSERT(DatasetSetTitle(ds, STR("world")));
    TEST_ASSERT(OCStringEqual(DatasetGetTitle(ds), STR("world")));

    // MetaData
    md = OCDictionaryCreateMutable(0);
    OCDictionarySetValue((OCMutableDictionaryRef)md, STR("k"), STR("v"));
    TEST_ASSERT(DatasetSetMetaData(ds, md));
    TEST_ASSERT(OCDictionaryGetCount(DatasetGetMetaData(ds)) == 1);

    // Focus
    TEST_ASSERT(DatasetSetFocus(ds, NULL));
    TEST_ASSERT(DatasetGetFocus(ds) == NULL);
    TEST_ASSERT(DatasetSetPreviousFocus(ds, NULL));
    TEST_ASSERT(DatasetGetPreviousFocus(ds) == NULL);

    ok = true;

cleanup:
    OCRelease(ds);
    OCRelease(dvs);
    OCRelease(dims);
    OCRelease(order);
    OCRelease(newDVs);
    OCRelease(tags);
    OCRelease(md);
    printf("test_Dataset_mutators %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_type_contract(void) {
    printf("test_Dataset_type_contract...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DatasetRef ds = NULL;

    DependentVariableRef dv = _make_mock_dv();
    TEST_ASSERT(dv != NULL);
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT(ds != NULL);

    // Type ID
    TEST_ASSERT(OCGetTypeID(ds) == DatasetGetTypeID());

    // Retain count
    int cnt = OCTypeGetRetainCount(ds);
    OCRetain(ds);
    TEST_ASSERT(OCTypeGetRetainCount(ds) == cnt + 1);
    OCRelease(ds);

    ok = true;

cleanup:
    OCRelease(ds);
    OCRelease(dvs);
    printf("test_Dataset_type_contract %s.\n", ok ? "passed" : "FAILED");
    return ok;
}

bool test_Dataset_copy_and_roundtrip(void) {
    printf("test_Dataset_copy_and_roundtrip...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCDictionaryRef dict = NULL;
    DatasetRef ds = NULL, copy = NULL, rt = NULL;

    DependentVariableRef dv = _make_mock_dv();
    TEST_ASSERT(dv != NULL);
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT(ds != NULL);

    // Copy
    copy = DatasetCreateCopy(ds);
    TEST_ASSERT(copy != NULL);
    TEST_ASSERT(OCTypeEqual(ds, copy));

    // Round-trip via dictionary
    dict = DatasetCopyAsDictionary(ds);
    TEST_ASSERT(dict != NULL);

    rt = DatasetCreateFromDictionary(dict, NULL);
    TEST_ASSERT(rt != NULL);
    TEST_ASSERT(OCTypeEqual(ds, rt));

    ok = true;

cleanup:
    OCRelease(rt);
    OCRelease(dict);
    OCRelease(copy);
    OCRelease(ds);
    OCRelease(dvs);
    printf("test_Dataset_copy_and_roundtrip %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
