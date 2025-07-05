//
// tests/test_Dataset.c
//

#include "OCLibrary.h"    // for OCTypeID etc.
#include "OCType.h"
#include "OCArray.h"
#include "OCIndexArray.h"
#include "OCDictionary.h"
#include "OCString.h"
#include "OCNumber.h"
#include "Dataset.h"
#include "DependentVariable.h"
#include <stdio.h>
#include <stdbool.h>

// Helper: create a 1‐element scalar DependentVariable for minimal tests
static DependentVariableRef _make_mock_dv(void) {
    // quantityType = "scalar", elementType = kSINumberFloat64Type, size = 1, owner = NULL
    return DependentVariableCreateDefault(STR("scalar"),
                                          kSINumberFloat64Type,
                                          1,
                                          NULL);
}

/**
 * Minimal‐create: no dims, no precedence, one DV, no tags, NULL strings,
 * NULL focus/previousFocus, NULL ops, NULL meta.  Defaults come from impl_InitDatasetFields.
 */
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

    DatasetRef ds = DatasetCreate(
        NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL
    );
    if (!ds) {
        fprintf(stderr, "[ERROR] DatasetCreate(minimal) returned NULL\n");
        goto cleanup1;
    }

    // dimensions & tags should be empty
    if (OCArrayGetCount(DatasetGetDimensions(ds)) != 0) {
        fprintf(stderr, "[ERROR] minimal_create: expected 0 dimensions, got %llu\n",
                OCArrayGetCount(DatasetGetDimensions(ds)));
        goto cleanup;
    }
    if (OCArrayGetCount(DatasetGetTags(ds)) != 0) {
        fprintf(stderr, "[ERROR] minimal_create: expected 0 tags, got %llu\n",
                OCArrayGetCount(DatasetGetTags(ds)));
        goto cleanup;
    }

    // one DV
    if (OCArrayGetCount(DatasetGetDependentVariables(ds)) != 1) {
        fprintf(stderr, "[ERROR] minimal_create: expected 1 DV, got %llu\n",
                OCArrayGetCount(DatasetGetDependentVariables(ds)));
        goto cleanup;
    }

    // empty precedence
    if (OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) != 0) {
        fprintf(stderr, "[ERROR] minimal_create: expected 0 precedence, got %ld\n",
                OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)));
        goto cleanup;
    }

    // default strings ""
    if (!OCStringEqual(DatasetGetDescription(ds), STR(""))) {
        fprintf(stderr, "[ERROR] minimal_create: description != \"\"\n");
        goto cleanup;
    }
    if (!OCStringEqual(DatasetGetTitle(ds), STR(""))) {
        fprintf(stderr, "[ERROR] minimal_create: title != \"\"\n");
        goto cleanup;
    }

    // focus & previousFocus
    if (DatasetGetFocus(ds) != NULL) {
        fprintf(stderr, "[ERROR] minimal_create: focus not NULL\n");
        goto cleanup;
    }
    if (DatasetGetPreviousFocus(ds) != NULL) {
        fprintf(stderr, "[ERROR] minimal_create: previousFocus not NULL\n");
        goto cleanup;
    }

    // empty metadata
    if (OCDictionaryGetCount(DatasetGetMetaData(ds)) != 0) {
        fprintf(stderr, "[ERROR] minimal_create: metadata not empty\n");
        goto cleanup;
    }

    // base64
    if (DatasetGetBase64(ds)) {
        fprintf(stderr, "[ERROR] minimal_create: base64 default should be false\n");
        goto cleanup;
    }

    // OCType contract
    OCTypeID tid = OCGetTypeID(ds);
    if (tid != DatasetGetTypeID()) {
        fprintf(stderr, "[ERROR] minimal_create: typeID mismatch (got %u, want %u)\n",
                tid, DatasetGetTypeID());
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


/**
 * Exercise all setters/getters on a minimally‐created Dataset.
 */
bool test_Dataset_mutators(void) {
    printf("test_Dataset_mutators...\n");
    bool ok = false;

    // minimal create
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) { fprintf(stderr, "[ERROR] mutators: mock DV failure\n"); goto cleanup0; }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    DatasetRef ds = DatasetCreate(NULL, NULL, dvs, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!ds) { fprintf(stderr, "[ERROR] mutators: DatasetCreate failed\n"); goto cleanup0; }

    // 1) dimensions
    OCMutableArrayRef dims = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dims, STR("D0"));
    if (!DatasetSetDimensions(ds, dims)) {
        fprintf(stderr, "[ERROR] mutators: SetDimensions failed\n");
        goto cleanup1;
    }
    if (OCArrayGetCount(DatasetGetDimensions(ds)) != 1) {
        fprintf(stderr, "[ERROR] mutators: expected 1 dim, got %llu\n",
                OCArrayGetCount(DatasetGetDimensions(ds)));
        goto cleanup1;
    }

    // 2) precedence
    OCMutableIndexArrayRef order = OCIndexArrayCreateMutable(0);
    OCIndexArrayAppendValue(order, 0);
    if (!DatasetSetDimensionPrecedence(ds, order)) {
        fprintf(stderr, "[ERROR] mutators: SetDimensionPrecedence failed\n");
        goto cleanup1;
    }
    if (OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) != 1) {
        fprintf(stderr, "[ERROR] mutators: expected 1 precedence, got %ld\n",
                OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)));
        goto cleanup1;
    }

    // 3) dependentVariables → empty
    OCMutableArrayRef newDVs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!DatasetSetDependentVariables(ds, newDVs)) {
        fprintf(stderr, "[ERROR] mutators: SetDependentVariables(empty) failed\n");
        goto cleanup1;
    }
    if (OCArrayGetCount(DatasetGetDependentVariables(ds)) != 0) {
        fprintf(stderr, "[ERROR] mutators: expected 0 DVs, got %llu\n",
                OCArrayGetCount(DatasetGetDependentVariables(ds)));
        goto cleanup1;
    }

    // 4) tags
    OCMutableArrayRef tags = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(tags, STR("foo"));
    OCArrayAppendValue(tags, STR("bar"));
    if (!DatasetSetTags(ds, tags)) {
        fprintf(stderr, "[ERROR] mutators: SetTags failed\n");
        goto cleanup1;
    }
    if (OCArrayGetCount(DatasetGetTags(ds)) != 2) {
        fprintf(stderr, "[ERROR] mutators: expected 2 tags, got %llu\n",
                OCArrayGetCount(DatasetGetTags(ds)));
        goto cleanup1;
    }

    // 5) description/title
    OCStringRef desc = STR("hello");
    OCStringRef title = STR("world");
    if (!DatasetSetDescription(ds, desc) ||
        !OCStringEqual(DatasetGetDescription(ds), desc))
    {
        fprintf(stderr, "[ERROR] mutators: description round-trip failed\n");
        goto cleanup1;
    }
    if (!DatasetSetTitle(ds, title) ||
        !OCStringEqual(DatasetGetTitle(ds), title))
    {
        fprintf(stderr, "[ERROR] mutators: title round-trip failed\n");
        goto cleanup1;
    }

    // 6) metadata only
    OCDictionaryRef md = OCDictionaryCreateMutable(0);
    OCDictionarySetValue((OCMutableDictionaryRef)md, STR("k"), STR("v"));
    if (!DatasetSetMetaData(ds, md) ||
        OCDictionaryGetCount(DatasetGetMetaData(ds)) != 1)
    {
        fprintf(stderr, "[ERROR] mutators: metadata round-trip failed\n");
        goto cleanup1;
    }

    // 7) focus & previousFocus
    if (!DatasetSetFocus(ds, NULL) || DatasetGetFocus(ds) != NULL ||
        !DatasetSetPreviousFocus(ds, NULL) || DatasetGetPreviousFocus(ds) != NULL)
    {
        fprintf(stderr, "[ERROR] mutators: focus/previousFocus handling failed\n");
        goto cleanup1;
    }

    // 8) base64
    if (DatasetGetBase64(ds) || !DatasetSetBase64(ds, true) || !DatasetGetBase64(ds)) {
        fprintf(stderr, "[ERROR] mutators: base64 flag handling failed\n");
        goto cleanup1;
    }

    ok = true;

cleanup1:
    OCRelease(dims);
    OCRelease(order);
    OCRelease(newDVs);
    OCRelease(tags);
    OCRelease(desc);
    OCRelease(title);
    OCRelease(md);
    OCRelease(ds);
cleanup0:
    OCRelease(dvs);

    printf("test_Dataset_mutators %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
/**
 * Verify OCType retain/release and typeID.
 */
bool test_Dataset_type_contract(void) {
    printf("test_Dataset_type_contract...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) { fprintf(stderr, "[ERROR] type_contract: mock DV failure\n"); goto cleanup0; }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    // now passes 9 args (no operations)
    DatasetRef ds = DatasetCreate(
        NULL,        // dimensions
        NULL,        // dimensionPrecedence
        dvs,         // dependentVariables
        NULL,        // tags
        NULL,        // description
        NULL,        // title
        NULL,        // focus
        NULL,        // previousFocus
        NULL         // metaData
    );
    if (!ds) { fprintf(stderr, "[ERROR] type_contract: DatasetCreate failed\n"); goto cleanup0; }

    // typeID
    if (OCGetTypeID(ds) != DatasetGetTypeID()) {
        fprintf(stderr, "[ERROR] type_contract: OCGetTypeID mismatch\n");
        goto cleanup1;
    }
    // retain/release
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


/**
 * Deep‐copy & dictionary round‐trip should preserve equality.
 */
bool test_Dataset_copy_and_roundtrip(void) {
    printf("test_Dataset_copy_and_roundtrip...\n");
    bool ok = false;

    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) { fprintf(stderr, "[ERROR] copy_roundtrip: mock DV failure\n"); goto cleanup0; }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);

    // now passes 9 args (no operations)
    DatasetRef ds = DatasetCreate(
        NULL, NULL, dvs,
        NULL, NULL, NULL,
        NULL, NULL, NULL
    );
    if (!ds) { fprintf(stderr, "[ERROR] copy_roundtrip: DatasetCreate failed\n"); goto cleanup0; }

    // 1) CreateCopy
    DatasetRef copy = DatasetCreateCopy(ds);
    if (!copy) {
        fprintf(stderr, "[ERROR] copy_roundtrip: DatasetCreateCopy returned NULL\n");
        goto cleanup1;
    }
    if (!OCTypeEqual(ds, copy)) {
        fprintf(stderr, "[ERROR] copy_roundtrip: CreateCopy result not equal\n");
        goto cleanup2;
    }

    // 2) dict round-trip
    OCDictionaryRef dict = DatasetCopyAsDictionary(ds, NULL);
    if (!dict) {
        fprintf(stderr, "[ERROR] copy_roundtrip: CopyAsDictionary returned NULL\n");
        goto cleanup2;
    }
    DatasetRef rt = DatasetCreateFromDictionary(dict);
    if (!rt) {
        fprintf(stderr, "[ERROR] copy_roundtrip: CreateFromDictionary returned NULL\n");
        goto cleanup3;
    }
    if (!OCTypeEqual(ds, rt)) {
        fprintf(stderr, "[ERROR] copy_roundtrip: round-trip unequal\n");
        goto cleanup4;
    }

    ok = true;

cleanup4:
    OCRelease(rt);
cleanup3:
    OCRelease(dict);
cleanup2:
    OCRelease(copy);
cleanup1:
    OCRelease(ds);
cleanup0:
    OCRelease(dvs);

    printf("test_Dataset_copy_and_roundtrip %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
