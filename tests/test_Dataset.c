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
    ds = DatasetCreate(NULL,   // dimensions
                       NULL,   // dimensionPrecedence
                       dvs,    // dependentVariables
                       NULL,   // tags
                       NULL,   // description
                       NULL,   // title
                       NULL,   // focus
                       NULL,   // previousFocus
                       NULL,   // metaData
                       NULL);  // outError
    TEST_ASSERT(ds != NULL);
    // Sanity checks
    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) == 1);
    TEST_ASSERT(OCArrayGetCount(DatasetGetDimensions(ds)) == 0);
    TEST_ASSERT(OCArrayGetCount(DatasetGetTags(ds)) == 0);
    TEST_ASSERT(OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) == 0);
    TEST_ASSERT(OCDictionaryGetCount(DatasetGetApplicationMetaData(ds)) == 0);
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
bool test_DatasetCreateMinimal(void) {
    printf("test_DatasetCreateMinimal...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    DatasetRef ds = NULL;
    DependentVariableRef dv = _make_mock_dv();
    if (!dv) {
        fprintf(stderr, "[ERROR] DatasetCreateMinimal: mock DV failed\n");
        goto cleanup;
    }
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);
    // Test our new minimal function
    ds = DatasetCreateMinimal(NULL,   // dimensions (NULL for scalar data)
                              dvs,    // dependentVariables
                              NULL);  // outError
    TEST_ASSERT(ds != NULL);
    // Sanity checks - should have same results as regular DatasetCreate with defaults
    TEST_ASSERT(OCArrayGetCount(DatasetGetDependentVariables(ds)) == 1);
    TEST_ASSERT(OCArrayGetCount(DatasetGetDimensions(ds)) == 0);
    TEST_ASSERT(OCArrayGetCount(DatasetGetTags(ds)) == 0);
    TEST_ASSERT(OCIndexArrayGetCount(DatasetGetDimensionPrecedence(ds)) == 0);
    // Check that default values are set
    TEST_ASSERT(DatasetGetDescription(ds) != NULL);
    TEST_ASSERT(DatasetGetTitle(ds) != NULL);
    TEST_ASSERT(DatasetGetFocus(ds) == NULL);
    TEST_ASSERT(DatasetGetPreviousFocus(ds) == NULL);
    ok = true;
cleanup:
    OCRelease(ds);
    OCRelease(dvs);
    printf("test_DatasetCreateMinimal %s.\n", ok ? "passed" : "FAILED");
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
    TEST_ASSERT(DatasetSetApplicationMetaData(ds, md));
    TEST_ASSERT(OCDictionaryGetCount(DatasetGetApplicationMetaData(ds)) == 1);
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

// Helper to create a DependentVariable with actual data (like Python test)
static DependentVariableRef _make_mock_dv_with_data(void) {
    // Create test data like the Python test: [1.0]
    double data[] = {1.0};
    
    // Create data component
    OCMutableDataRef comp = OCDataCreateMutable(0);
    if (!comp) return NULL;
    
    // Set length and copy data
    OCDataSetLength(comp, sizeof(data));
    double *dataPtr = (double *)OCDataGetMutableBytes(comp);
    memcpy(dataPtr, data, sizeof(data));
    
    // Create components array
    OCMutableArrayRef comps = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(comps, comp);
    OCRelease(comp);
    
    // Create DependentVariable
    SIUnitRef unit = SIUnitDimensionlessAndUnderived();
    OCStringRef err = NULL;
    DependentVariableRef dv = DependentVariableCreate(
        STR("test_dv"),                     // name  
        STR("Test dependent variable"),     // description
        unit,                               // unit
        STR("dimensionless"),               // quantity_name
        STR("scalar"),                      // quantity_type
        kOCNumberFloat64Type,               // numeric_type
        NULL,                               // componentLabels
        comps,                              // components
        &err                                // outError
    );
    
    OCRelease(comps);
    if (!dv && err) {
        printf("_make_mock_dv_with_data failed: %s\n", OCStringGetCString(err));
        OCRelease(err);
    }
    return dv;
}

bool test_Dataset_rigorous_roundtrip(void) {
    printf("test_Dataset_rigorous_roundtrip...\n");
    bool ok = false;
    OCMutableArrayRef dvs = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCDictionaryRef dict = NULL;
    DatasetRef ds = NULL, rt = NULL;
    OCStringRef err_str = NULL; // Declare early to avoid uninitialized warnings
    
    // Create a DependentVariable with actual data (like Python test)
    DependentVariableRef dv = _make_mock_dv_with_data();
    TEST_ASSERT(dv != NULL);
    OCArrayAppendValue(dvs, dv);
    OCRelease(dv);
    
    // Create Dataset with title and description (like Python test)
    ds = DatasetCreate(NULL,                           // dimensions
                       NULL,                           // dimensionPrecedence  
                       dvs,                            // dependentVariables
                       NULL,                           // tags
                       STR("Original description"),    // description
                       STR("Original Dataset"),        // title
                       NULL,                           // focus
                       NULL,                           // previousFocus
                       NULL,                           // metaData
                       NULL);                          // outError
    TEST_ASSERT(ds != NULL);
    
    // Verify original dataset properties
    OCStringRef title = DatasetGetTitle(ds);
    OCStringRef desc = DatasetGetDescription(ds);
    TEST_ASSERT(title != NULL);
    TEST_ASSERT(desc != NULL);
    TEST_ASSERT(OCStringEqual(title, STR("Original Dataset")));
    TEST_ASSERT(OCStringEqual(desc, STR("Original description")));
    
    // Round-trip via dictionary (like Python test calls to_dict() then from_dict())
    dict = DatasetCopyAsDictionary(ds);
    TEST_ASSERT(dict != NULL);
    
    // DEBUG: Print dictionary structure to help debug Python wrapper issue
    printf("=== C Dictionary Structure Debug ===\n");
    OCStringRef debug_title = (OCStringRef)OCDictionaryGetValue(dict, STR("title"));
    if (debug_title) printf("  title: %s\n", OCStringGetCString(debug_title));
    
    OCStringRef debug_desc = (OCStringRef)OCDictionaryGetValue(dict, STR("description"));
    if (debug_desc) printf("  description: %s\n", OCStringGetCString(debug_desc));
    
    OCArrayRef debug_deps = (OCArrayRef)OCDictionaryGetValue(dict, STR("dependent_variables"));
    if (debug_deps) {
        printf("  dependent_variables: array with %llu items\n", (unsigned long long)OCArrayGetCount(debug_deps));
        if (OCArrayGetCount(debug_deps) > 0) {
            OCDictionaryRef dv_dict = (OCDictionaryRef)OCArrayGetValueAtIndex(debug_deps, 0);
            if (dv_dict) {
                OCStringRef dv_type = (OCStringRef)OCDictionaryGetValue(dv_dict, STR("type"));
                if (dv_type) printf("    [0].type: %s\n", OCStringGetCString(dv_type));
                OCStringRef dv_encoding = (OCStringRef)OCDictionaryGetValue(dv_dict, STR("encoding"));
                if (dv_encoding) printf("    [0].encoding: %s\n", OCStringGetCString(dv_encoding));
            }
        }
    }
    
    OCArrayRef debug_dims = (OCArrayRef)OCDictionaryGetValue(dict, STR("dimensions"));
    if (debug_dims) printf("  dimensions: array with %llu items\n", (unsigned long long)OCArrayGetCount(debug_dims));
    
    OCArrayRef debug_prec = (OCArrayRef)OCDictionaryGetValue(dict, STR("dimension_precedence"));
    if (debug_prec) printf("  dimension_precedence: array with %llu items\n", (unsigned long long)OCArrayGetCount(debug_prec));
    printf("=== End C Dictionary Debug ===\n");
    
    // Create from dictionary
    rt = DatasetCreateFromDictionary(dict, &err_str);
    if (rt == NULL) {
        if (err_str) {
            printf("DatasetCreateFromDictionary failed: %s\n", OCStringGetCString(err_str));
            OCRelease(err_str);
        } else {
            printf("DatasetCreateFromDictionary failed with no error message\n");
        }
        goto cleanup;
    }
    
    // Verify roundtrip dataset properties match original
    OCStringRef rt_title = DatasetGetTitle(rt);
    OCStringRef rt_desc = DatasetGetDescription(rt);
    TEST_ASSERT(rt_title != NULL);
    TEST_ASSERT(rt_desc != NULL);
    TEST_ASSERT(OCStringEqual(rt_title, STR("Original Dataset")));
    TEST_ASSERT(OCStringEqual(rt_desc, STR("Original description")));
    
    // Verify dependent variables count matches
    OCArrayRef orig_dvs = DatasetGetDependentVariables(ds);
    OCArrayRef rt_dvs = DatasetGetDependentVariables(rt);
    TEST_ASSERT(OCArrayGetCount(orig_dvs) == OCArrayGetCount(rt_dvs));
    TEST_ASSERT(OCArrayGetCount(rt_dvs) == 1);
    
    // Verify the datasets are equal
    TEST_ASSERT(OCTypeEqual(ds, rt));
    
    ok = true;
cleanup:
    if (err_str) OCRelease(err_str);
    OCRelease(rt);
    OCRelease(dict);
    OCRelease(ds);
    OCRelease(dvs);
    printf("test_Dataset_rigorous_roundtrip %s.\n", ok ? "passed" : "FAILED");
    return ok;
}
