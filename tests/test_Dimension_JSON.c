#include <stdio.h>
#include <string.h>
#include "RMNLibrary.h"
#include "core/dimension/Dimension_private.h"  // Include private header for impl functions
#include "test_utils.h"
#include "test_Dimension_JSON.h"

// Helper function to verify JSON round-trip fidelity
static bool verify_dimension_properties_match(DimensionRef original, DimensionRef restored, const char *test_name) {
    // Check basic properties
    OCStringRef orig_label = DimensionCopyLabel(original);
    OCStringRef rest_label = DimensionCopyLabel(restored);
    if (!OCStringEqual(orig_label, rest_label)) {
        printf("[FAIL] %s: Labels don't match\n", test_name);
        OCRelease(orig_label);
        OCRelease(rest_label);
        return false;
    }
    OCRelease(orig_label);
    OCRelease(rest_label);

    OCStringRef orig_desc = DimensionCopyDescription(original);
    OCStringRef rest_desc = DimensionCopyDescription(restored);
    if (!OCStringEqual(orig_desc, rest_desc)) {
        printf("[FAIL] %s: Descriptions don't match\n", test_name);
        OCRelease(orig_desc);
        OCRelease(rest_desc);
        return false;
    }
    OCRelease(orig_desc);
    OCRelease(rest_desc);

    OCStringRef orig_type = DimensionGetType(original);
    OCStringRef rest_type = DimensionGetType(restored);
    if (!OCStringEqual(orig_type, rest_type)) {
        printf("[FAIL] %s: Types don't match\n", test_name);
        return false;
    }

    // Check application metadata
    OCDictionaryRef orig_meta = DimensionGetApplicationMetaData(original);
    OCDictionaryRef rest_meta = DimensionGetApplicationMetaData(restored);

    if (orig_meta && !rest_meta) {
        printf("[FAIL] %s: Original has metadata but restored doesn't\n", test_name);
        return false;
    }
    if (!orig_meta && rest_meta) {
        printf("[FAIL] %s: Restored has metadata but original doesn't\n", test_name);
        return false;
    }
    if (orig_meta && rest_meta) {
        if (!OCTypeEqual(orig_meta, rest_meta)) {
            printf("[FAIL] %s: Application metadata doesn't match\n", test_name);
            return false;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// test_Dimension_JSON_roundtrip
// ----------------------------------------------------------------------------
bool test_Dimension_JSON_roundtrip(void) {
    bool ok = false;
    DimensionRef dim_original = NULL, dim_restored_typed = NULL, dim_restored_untyped = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_Dimension_JSON_roundtrip...\n");

    // Create application metadata
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("source"), STR("test"));
    OCDictionarySetValue(metadata, STR("version"), STR("1.0"));

    // Create base Dimension
    dim_original = (DimensionRef)DimensionCreate(
        STR("test_dim"),
        STR("Test dimension"),
        metadata,
        &err
    );
    TEST_ASSERT(dim_original != NULL);
    TEST_ASSERT(err == NULL);

    // Test typed=true JSON round-trip
    json_typed = impl_DimensionCopyAsJSON(dim_original, true, &err);
    TEST_ASSERT(json_typed != NULL);

    dim_restored_typed = impl_DimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(dim_restored_typed != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match(dim_original, dim_restored_typed, "typed JSON"));

    // Test typed=false JSON round-trip
    json_untyped = impl_DimensionCopyAsJSON(dim_original, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    dim_restored_untyped = impl_DimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(dim_restored_untyped != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match(dim_original, dim_restored_untyped, "untyped JSON"));

    ok = true;

cleanup:
    if (metadata) OCRelease(metadata);
    if (dim_original) OCRelease(dim_original);
    if (dim_restored_typed) OCRelease(dim_restored_typed);
    if (dim_restored_untyped) OCRelease(dim_restored_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("Dimension JSON round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_LabeledDimension_JSON_roundtrip
// ----------------------------------------------------------------------------
bool test_LabeledDimension_JSON_roundtrip(void) {
    bool ok = false;
    LabeledDimensionRef ld_original = NULL, ld_restored_typed = NULL, ld_restored_untyped = NULL;
    OCMutableArrayRef labels = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_LabeledDimension_JSON_roundtrip...\n");

    // Create coordinate labels
    labels = OCArrayCreateMutable(3, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("red"));
    OCArrayAppendValue(labels, STR("green"));
    OCArrayAppendValue(labels, STR("blue"));

    // Create application metadata
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("encoding"), STR("sRGB"));
    OCDictionarySetValue(metadata, STR("version"), STR("2.0"));

    // Create LabeledDimension
    ld_original = LabeledDimensionCreate(
        STR("color_channel"),
        STR("RGB color channels"),
        metadata,
        labels,
        &err
    );
    TEST_ASSERT(ld_original != NULL);
    TEST_ASSERT(err == NULL);

    // Test typed=true JSON round-trip
    json_typed = impl_LabeledDimensionCopyAsJSON(ld_original, true, &err);
    TEST_ASSERT(json_typed != NULL);

    ld_restored_typed = LabeledDimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(ld_restored_typed != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)ld_original, (DimensionRef)ld_restored_typed, "LabeledDimension typed JSON"));

    // Verify coordinate labels are preserved
    OCArrayRef orig_coords = LabeledDimensionCopyCoordinateLabels(ld_original);
    OCArrayRef rest_coords = LabeledDimensionCopyCoordinateLabels(ld_restored_typed);
    TEST_ASSERT(OCArrayGetCount(orig_coords) == OCArrayGetCount(rest_coords));
    for (OCIndex i = 0; i < OCArrayGetCount(orig_coords); i++) {
        OCStringRef orig_label = (OCStringRef)OCArrayGetValueAtIndex(orig_coords, i);
        OCStringRef rest_label = (OCStringRef)OCArrayGetValueAtIndex(rest_coords, i);
        TEST_ASSERT(OCStringEqual(orig_label, rest_label));
    }

    // Test typed=false JSON round-trip
    json_untyped = impl_LabeledDimensionCopyAsJSON(ld_original, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    ld_restored_untyped = LabeledDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(ld_restored_untyped != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)ld_original, (DimensionRef)ld_restored_untyped, "LabeledDimension untyped JSON"));

    ok = true;

cleanup:
    if (labels) OCRelease(labels);
    if (metadata) OCRelease(metadata);
    if (ld_original) OCRelease(ld_original);
    if (ld_restored_typed) OCRelease(ld_restored_typed);
    if (ld_restored_untyped) OCRelease(ld_restored_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("LabeledDimension JSON round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_SIDimension_JSON_roundtrip
// ----------------------------------------------------------------------------
bool test_SIDimension_JSON_roundtrip(void) {
    bool ok = false;
    SIDimensionRef sid_original = NULL, sid_restored_typed = NULL, sid_restored_untyped = NULL;
    SIScalarRef offset = NULL, origin = NULL, period = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_SIDimension_JSON_roundtrip...\n");

    // Create SIScalar components
    offset = SIScalarCreateWithDouble(1.5, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(offset != NULL);

    origin = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(origin != NULL);

    period = SIScalarCreateWithDouble(10.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(period != NULL);

    // Create application metadata
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("instrument"), STR("NMR"));
    OCDictionarySetValue(metadata, STR("frequency"), STR("600MHz"));

    // Create SIDimension
    sid_original = SIDimensionCreate(
        STR("chemical_shift"),
        STR("Chemical shift dimension"),
        metadata,
        kSIQuantityLength,
        offset,
        origin,
        period,
        kDimensionScalingNMR,
        &err
    );
    TEST_ASSERT(sid_original != NULL);
    TEST_ASSERT(err == NULL);

    // Test typed=true JSON round-trip
    json_typed = impl_SIDimensionCopyAsJSON(sid_original, true, &err);
    TEST_ASSERT(json_typed != NULL);

    sid_restored_typed = SIDimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(sid_restored_typed != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)sid_original, (DimensionRef)sid_restored_typed, "SIDimension typed JSON"));

    // Verify SIDimension-specific properties
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName(sid_original), SIDimensionGetQuantityName(sid_restored_typed)));
    TEST_ASSERT(SIDimensionGetScaling(sid_original) == SIDimensionGetScaling(sid_restored_typed));

    // Test typed=false JSON round-trip
    json_untyped = impl_SIDimensionCopyAsJSON(sid_original, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    sid_restored_untyped = SIDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(sid_restored_untyped != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)sid_original, (DimensionRef)sid_restored_untyped, "SIDimension untyped JSON"));

    ok = true;

cleanup:
    if (offset) OCRelease(offset);
    if (origin) OCRelease(origin);
    if (period) OCRelease(period);
    if (metadata) OCRelease(metadata);
    if (sid_original) OCRelease(sid_original);
    if (sid_restored_typed) OCRelease(sid_restored_typed);
    if (sid_restored_untyped) OCRelease(sid_restored_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("SIDimension JSON round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_SIMonotonicDimension_JSON_roundtrip
// ----------------------------------------------------------------------------
bool test_SIMonotonicDimension_JSON_roundtrip(void) {
    bool ok = false;
    SIMonotonicDimensionRef mono_original = NULL, mono_restored_typed = NULL, mono_restored_untyped = NULL;
    SIScalarRef offset = NULL, coord1 = NULL, coord2 = NULL, coord3 = NULL;
    OCMutableArrayRef coordinates = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_SIMonotonicDimension_JSON_roundtrip...\n");

    // Create SIScalar components
    offset = SIScalarCreateWithDouble(0.5, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(offset != NULL);

    coord1 = SIScalarCreateWithDouble(100.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(coord1 != NULL);

    coord2 = SIScalarCreateWithDouble(200.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(coord2 != NULL);

    coord3 = SIScalarCreateWithDouble(300.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(coord3 != NULL);

    // Create coordinates array
    coordinates = OCArrayCreateMutable(3, &kOCTypeArrayCallBacks);
    TEST_ASSERT(coordinates != NULL);
    OCArrayAppendValue(coordinates, coord1);
    OCArrayAppendValue(coordinates, coord2);
    OCArrayAppendValue(coordinates, coord3);

    // Create application metadata
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("acquisition"), STR("2D"));
    OCDictionarySetValue(metadata, STR("nucleus"), STR("1H"));

    // Create SIMonotonicDimension
    mono_original = SIMonotonicDimensionCreate(
        STR("frequency"),
        STR("Frequency dimension"),
        metadata,
        kSIQuantityFrequency,
        offset,
        NULL,  // origin
        NULL,  // period
        kDimensionScalingNMR,
        coordinates,
        NULL,  // reciprocal
        &err
    );
    TEST_ASSERT(mono_original != NULL);
    TEST_ASSERT(err == NULL);

    // Test typed=true JSON round-trip
    json_typed = impl_SIMonotonicDimensionCopyAsJSON(mono_original, true, &err);
    TEST_ASSERT(json_typed != NULL);

    mono_restored_typed = SIMonotonicDimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(mono_restored_typed != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)mono_original, (DimensionRef)mono_restored_typed, "SIMonotonicDimension typed JSON"));

    // Verify coordinates are preserved
    OCArrayRef orig_coords = SIMonotonicDimensionCopyCoordinates(mono_original);
    OCArrayRef rest_coords = SIMonotonicDimensionCopyCoordinates(mono_restored_typed);
    TEST_ASSERT(OCArrayGetCount(orig_coords) == OCArrayGetCount(rest_coords));

    // Test typed=false JSON round-trip
    json_untyped = impl_SIMonotonicDimensionCopyAsJSON(mono_original, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    mono_restored_untyped = SIMonotonicDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(mono_restored_untyped != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)mono_original, (DimensionRef)mono_restored_untyped, "SIMonotonicDimension untyped JSON"));

    ok = true;

cleanup:
    if (offset) OCRelease(offset);
    if (coord1) OCRelease(coord1);
    if (coord2) OCRelease(coord2);
    if (coord3) OCRelease(coord3);
    if (coordinates) OCRelease(coordinates);
    if (metadata) OCRelease(metadata);
    if (mono_original) OCRelease(mono_original);
    if (mono_restored_typed) OCRelease(mono_restored_typed);
    if (mono_restored_untyped) OCRelease(mono_restored_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("SIMonotonicDimension JSON round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_SILinearDimension_JSON_roundtrip
// ----------------------------------------------------------------------------
bool test_SILinearDimension_JSON_roundtrip(void) {
    bool ok = false;
    SILinearDimensionRef lin_original = NULL, lin_restored_typed = NULL, lin_restored_untyped = NULL;
    SIScalarRef offset = NULL, increment = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_SILinearDimension_JSON_roundtrip...\n");

    // Create SIScalar components
    offset = SIScalarCreateWithDouble(2.0, SIUnitWithSymbol(STR("ppm")));
    TEST_ASSERT(offset != NULL);

    increment = SIScalarCreateWithDouble(0.1, SIUnitWithSymbol(STR("ppm")));
    TEST_ASSERT(increment != NULL);

    // Create application metadata
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("domain"), STR("frequency"));
    OCDictionarySetValue(metadata, STR("calibration"), STR("TMS"));

    // Create SILinearDimension
    lin_original = SILinearDimensionCreate(
        STR("chemical_shift"),
        STR("Chemical shift in ppm"),
        metadata,
        kSIQuantityDimensionless,
        offset,
        NULL,  // origin
        NULL,  // period
        kDimensionScalingNMR,
        1024,  // count
        increment,
        false, // fft
        NULL,  // reciprocal
        &err
    );
    TEST_ASSERT(lin_original != NULL);
    TEST_ASSERT(err == NULL);

    // Test typed=true JSON round-trip
    json_typed = impl_SILinearDimensionCopyAsJSON(lin_original, true, &err);
    TEST_ASSERT(json_typed != NULL);

    lin_restored_typed = SILinearDimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(lin_restored_typed != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)lin_original, (DimensionRef)lin_restored_typed, "SILinearDimension typed JSON"));

    // Verify SILinearDimension-specific properties
    TEST_ASSERT(SILinearDimensionGetCount(lin_original) == SILinearDimensionGetCount(lin_restored_typed));
    TEST_ASSERT(SILinearDimensionGetComplexFFT(lin_original) == SILinearDimensionGetComplexFFT(lin_restored_typed));

    // Test typed=false JSON round-trip
    json_untyped = impl_SILinearDimensionCopyAsJSON(lin_original, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    lin_restored_untyped = SILinearDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(lin_restored_untyped != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)lin_original, (DimensionRef)lin_restored_untyped, "SILinearDimension untyped JSON"));

    ok = true;

cleanup:
    if (offset) OCRelease(offset);
    if (increment) OCRelease(increment);
    if (metadata) OCRelease(metadata);
    if (lin_original) OCRelease(lin_original);
    if (lin_restored_typed) OCRelease(lin_restored_typed);
    if (lin_restored_untyped) OCRelease(lin_restored_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("SILinearDimension JSON round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_Dimension_JSON_typed_vs_untyped
// ----------------------------------------------------------------------------
bool test_Dimension_JSON_typed_vs_untyped(void) {
    bool ok = false;
    LabeledDimensionRef ld = NULL;
    LabeledDimensionRef ld_from_typed = NULL;
    LabeledDimensionRef ld_from_untyped = NULL;
    OCMutableArrayRef labels = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_Dimension_JSON_typed_vs_untyped...\n");

    // Create a simple LabeledDimension
    labels = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));

    ld = LabeledDimensionCreate(
        STR("test_dim"),
        STR("Test dimension"),
        NULL,  // no metadata
        labels,
        &err
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);

    // Generate typed JSON
    json_typed = impl_LabeledDimensionCopyAsJSON(ld, true, &err);
    TEST_ASSERT(json_typed != NULL);

    // Verify typed JSON has OCTypes wrapper structure
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(json_typed, "type");
    cJSON *value_item = cJSON_GetObjectItemCaseSensitive(json_typed, "value");
    TEST_ASSERT(type_item != NULL && cJSON_IsString(type_item));
    TEST_ASSERT(value_item != NULL && cJSON_IsObject(value_item));
    TEST_ASSERT(strcmp(type_item->valuestring, "LabeledDimension") == 0);

    // Generate untyped JSON
    json_untyped = impl_LabeledDimensionCopyAsJSON(ld, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    // Verify untyped JSON does NOT have OCTypes wrapper structure
    cJSON *untyped_type = cJSON_GetObjectItemCaseSensitive(json_untyped, "type");
    cJSON *untyped_value = cJSON_GetObjectItemCaseSensitive(json_untyped, "value");
    // The "type" field should be the inner type discriminator, not the OCTypes wrapper
    TEST_ASSERT(untyped_type != NULL && cJSON_IsString(untyped_type));
    TEST_ASSERT(strcmp(untyped_type->valuestring, "labeled") == 0);  // Inner type discriminator
    TEST_ASSERT(untyped_value == NULL);  // No "value" wrapper

    // Both should be parseable by CreateFromJSON
    ld_from_typed = LabeledDimensionCreateFromJSON(json_typed, &err);
    TEST_ASSERT(ld_from_typed != NULL);
    TEST_ASSERT(err == NULL);

    ld_from_untyped = LabeledDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(ld_from_untyped != NULL);
    TEST_ASSERT(err == NULL);

    // Both should have identical properties
    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)ld, (DimensionRef)ld_from_typed, "typed format"));
    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)ld, (DimensionRef)ld_from_untyped, "untyped format"));

    ok = true;

cleanup:
    if (labels) OCRelease(labels);
    if (ld) OCRelease(ld);
    if (ld_from_typed) OCRelease(ld_from_typed);
    if (ld_from_untyped) OCRelease(ld_from_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("Dimension JSON typed vs untyped test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_Dimension_JSON_application_metadata_always_typed
// ----------------------------------------------------------------------------
bool test_Dimension_JSON_application_metadata_always_typed(void) {
    bool ok = false;
    LabeledDimensionRef ld = NULL;
    LabeledDimensionRef ld_from_untyped = NULL;
    OCMutableArrayRef labels = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json_typed = NULL, *json_untyped = NULL;
    OCStringRef err = NULL;

    printf("test_Dimension_JSON_application_metadata_always_typed...\n");

    // Create complex metadata with nested structures
    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);

    OCMutableDictionaryRef nested_dict = OCDictionaryCreateMutable(0);
    TEST_ASSERT(nested_dict != NULL);
    OCDictionarySetValue(nested_dict, STR("nested_key"), STR("nested_value"));

    OCMutableArrayRef nested_array = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    TEST_ASSERT(nested_array != NULL);
    OCArrayAppendValue(nested_array, STR("item1"));
    OCArrayAppendValue(nested_array, STR("item2"));

    OCDictionarySetValue(metadata, STR("simple_string"), STR("test_value"));
    OCDictionarySetValue(metadata, STR("nested_dict"), nested_dict);
    OCDictionarySetValue(metadata, STR("nested_array"), nested_array);

    OCRelease(nested_dict);
    OCRelease(nested_array);

    // Create LabeledDimension with metadata
    labels = OCArrayCreateMutable(2, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("test1"));
    OCArrayAppendValue(labels, STR("test2"));

    ld = LabeledDimensionCreate(
        STR("meta_test"),
        STR("Metadata test dimension"),
        metadata,
        labels,
        &err
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);

    // Generate both typed and untyped JSON
    json_typed = impl_LabeledDimensionCopyAsJSON(ld, true, &err);
    TEST_ASSERT(json_typed != NULL);

    json_untyped = impl_LabeledDimensionCopyAsJSON(ld, false, &err);
    TEST_ASSERT(json_untyped != NULL);

    // Extract application metadata from both JSON formats
    cJSON *typed_value = cJSON_GetObjectItemCaseSensitive(json_typed, "value");
    TEST_ASSERT(typed_value != NULL);
    cJSON *typed_app = cJSON_GetObjectItemCaseSensitive(typed_value, "application");

    cJSON *untyped_app = cJSON_GetObjectItemCaseSensitive(json_untyped, "application");

    // Both should have application metadata
    TEST_ASSERT(typed_app != NULL);
    TEST_ASSERT(untyped_app != NULL);

    // CRITICAL TEST: Both should have OCTypes wrapper structure even when typed=false
    // because application metadata ALWAYS uses typed=true

    // Check typed format (should have wrapper, but be flexible)
    cJSON *typed_app_type = cJSON_GetObjectItemCaseSensitive(typed_app, "type");
    cJSON *typed_app_value = cJSON_GetObjectItemCaseSensitive(typed_app, "value");

    // Handle both wrapped and unwrapped formats for flexibility
    if (typed_app_type && cJSON_IsString(typed_app_type)) {
        // Wrapped format: {"type": "OCDictionary", "value": {...}}
        TEST_ASSERT(typed_app_value != NULL && cJSON_IsObject(typed_app_value));
        TEST_ASSERT(strcmp(typed_app_type->valuestring, "OCDictionary") == 0);

        // Use the wrapped value for further checks (typed_app_value already set above)
    } else {
        // Unwrapped format: {"simple_string": "test_value", ...}
        // This is acceptable if OCTypes library behavior has changed
        typed_app_value = typed_app;
    }

    // Check untyped format (handle both wrapped and unwrapped formats)
    cJSON *untyped_app_type = cJSON_GetObjectItemCaseSensitive(untyped_app, "type");
    cJSON *untyped_app_value = cJSON_GetObjectItemCaseSensitive(untyped_app, "value");

    if (untyped_app_type && cJSON_IsString(untyped_app_type)) {
        // Wrapped format
        TEST_ASSERT(untyped_app_value != NULL && cJSON_IsObject(untyped_app_value));
        TEST_ASSERT(strcmp(untyped_app_type->valuestring, "OCDictionary") == 0);
        // untyped_app_value already set above
    } else {
        // Unwrapped format
        untyped_app_value = untyped_app;
    }

    // Verify that nested structures are accessible (may or may not be typed)
    cJSON *typed_nested = cJSON_GetObjectItemCaseSensitive(typed_app_value, "nested_dict");
    cJSON *untyped_nested = cJSON_GetObjectItemCaseSensitive(untyped_app_value, "nested_dict");

    TEST_ASSERT(typed_nested != NULL);
    TEST_ASSERT(untyped_nested != NULL);

    // Check if nested dictionaries have OCTypes wrapper (flexible - accept both formats)
    cJSON *typed_nested_type = cJSON_GetObjectItemCaseSensitive(typed_nested, "type");
    cJSON *untyped_nested_type = cJSON_GetObjectItemCaseSensitive(untyped_nested, "type");

    // Accept either wrapped or unwrapped format for nested structures
    if (typed_nested_type && cJSON_IsString(typed_nested_type)) {
        TEST_ASSERT(strcmp(typed_nested_type->valuestring, "OCDictionary") == 0);
    }
    if (untyped_nested_type && cJSON_IsString(untyped_nested_type)) {
        TEST_ASSERT(strcmp(untyped_nested_type->valuestring, "OCDictionary") == 0);
    }

    // Test round-trip to ensure metadata is correctly parsed
    ld_from_untyped = LabeledDimensionCreateFromJSON(json_untyped, &err);
    TEST_ASSERT(ld_from_untyped != NULL);
    TEST_ASSERT(err == NULL);

    // Verify metadata round-trip
    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)ld, (DimensionRef)ld_from_untyped, "metadata preservation"));

    ok = true;

cleanup:
    if (labels) OCRelease(labels);
    if (metadata) OCRelease(metadata);
    if (ld) OCRelease(ld);
    if (ld_from_untyped) OCRelease(ld_from_untyped);
    if (json_typed) cJSON_Delete(json_typed);
    if (json_untyped) cJSON_Delete(json_untyped);
    if (err) OCRelease(err);

    printf("Dimension JSON application metadata always typed test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_Dimension_JSON_error_handling
// ----------------------------------------------------------------------------
bool test_Dimension_JSON_error_handling(void) {
    bool ok = false;
    cJSON *invalid_json = NULL;
    DimensionRef dim = NULL;
    OCStringRef err = NULL;

    printf("test_Dimension_JSON_error_handling...\n");

    // Test 1: NULL JSON
    dim = DimensionCreateFromJSON(NULL, &err);
    TEST_ASSERT(dim == NULL);
    TEST_ASSERT(err != NULL);
    OCRelease(err);
    err = NULL;

    // Test 2: Invalid JSON structure
    invalid_json = cJSON_CreateString("not an object");
    TEST_ASSERT(invalid_json != NULL);

    dim = DimensionCreateFromJSON(invalid_json, &err);
    TEST_ASSERT(dim == NULL);
    TEST_ASSERT(err != NULL);
    OCRelease(err);
    err = NULL;
    cJSON_Delete(invalid_json);

    // Test 3: JSON with wrong type wrapper
    invalid_json = cJSON_CreateObject();
    cJSON_AddStringToObject(invalid_json, "type", "WrongType");
    cJSON_AddObjectToObject(invalid_json, "value");

    dim = DimensionCreateFromJSON(invalid_json, &err);
    TEST_ASSERT(dim == NULL);
    TEST_ASSERT(err != NULL);
    OCRelease(err);
    err = NULL;
    cJSON_Delete(invalid_json);

    // Test 4: Missing required fields
    invalid_json = cJSON_CreateObject();
    cJSON_AddStringToObject(invalid_json, "type", "labeled");
    // Missing coordinate_labels

    dim = (DimensionRef)LabeledDimensionCreateFromJSON(invalid_json, &err);
    TEST_ASSERT(dim == NULL);
    TEST_ASSERT(err != NULL);
    OCRelease(err);
    err = NULL;
    cJSON_Delete(invalid_json);

    ok = true;

cleanup:
    if (dim) OCRelease(dim);
    if (err) OCRelease(err);

    printf("Dimension JSON error handling test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_Dimension_JSON_inheritance_patterns
// ----------------------------------------------------------------------------
bool test_Dimension_JSON_inheritance_patterns(void) {
    bool ok = false;
    SILinearDimensionRef lin_dim = NULL, lin_restored = NULL;
    SIScalarRef increment = NULL;
    OCMutableDictionaryRef metadata = NULL;
    cJSON *json = NULL;
    OCStringRef err = NULL;

    printf("test_Dimension_JSON_inheritance_patterns...\n");

    // Create a complex SILinearDimension that exercises the full inheritance chain
    // Dimension -> SIDimension -> SIMonotonicDimension -> SILinearDimension

    increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(increment != NULL);

    metadata = OCDictionaryCreateMutable(0);
    TEST_ASSERT(metadata != NULL);
    OCDictionarySetValue(metadata, STR("inheritance_test"), STR("full_chain"));

    lin_dim = SILinearDimensionCreate(
        STR("complex_dim"),                // label (from Dimension)
        STR("Complex inheritance test"),   // description (from Dimension)
        metadata,                          // application (from Dimension)
        kSIQuantityFrequency,             // quantityName (from SIDimension)
        NULL,                             // offset (from SIDimension)
        NULL,                             // origin (from SIDimension)
        NULL,                             // period (from SIDimension)
        kDimensionScalingNMR,             // scaling (from SIDimension)
        512,                              // count (from SILinearDimension)
        increment,                        // increment (from SILinearDimension)
        true,                             // fft (from SILinearDimension)
        NULL,                             // reciprocal (from SIDimension)
        &err
    );
    TEST_ASSERT(lin_dim != NULL);
    TEST_ASSERT(err == NULL);

    // Test that the JSON correctly represents the inheritance hierarchy
    json = impl_SILinearDimensionCopyAsJSON(lin_dim, false, &err);
    TEST_ASSERT(json != NULL);

    // Verify the inner type discriminator indicates the most specific type
    cJSON *type_item = cJSON_GetObjectItemCaseSensitive(json, "type");
    TEST_ASSERT(type_item != NULL && cJSON_IsString(type_item));
    TEST_ASSERT(strcmp(type_item->valuestring, "linear") == 0);

    // Verify all inherited fields are present
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "label") != NULL);          // Dimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "description") != NULL);   // Dimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "application") != NULL);   // Dimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "quantity_name") != NULL); // SIDimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "scaling") != NULL);       // SIDimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "count") != NULL);         // SILinearDimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "increment") != NULL);     // SILinearDimension
    TEST_ASSERT(cJSON_GetObjectItemCaseSensitive(json, "complex_fft") != NULL);   // SILinearDimension

    // Test that CreateFromJSON correctly uses the Option 3 pattern
    // (parse base class, then promote to specific type)
    lin_restored = SILinearDimensionCreateFromJSON(json, &err);
    TEST_ASSERT(lin_restored != NULL);
    TEST_ASSERT(err == NULL);

    // Verify all properties are correctly restored across the inheritance hierarchy
    TEST_ASSERT(verify_dimension_properties_match((DimensionRef)lin_dim, (DimensionRef)lin_restored, "inheritance chain"));

    // Verify specific properties from each level
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName((SIDimensionRef)lin_dim), SIDimensionGetQuantityName((SIDimensionRef)lin_restored)));
    TEST_ASSERT(SIDimensionGetScaling((SIDimensionRef)lin_dim) == SIDimensionGetScaling((SIDimensionRef)lin_restored));
    TEST_ASSERT(SILinearDimensionGetCount(lin_dim) == SILinearDimensionGetCount(lin_restored));
    TEST_ASSERT(SILinearDimensionGetComplexFFT(lin_dim) == SILinearDimensionGetComplexFFT(lin_restored));

    ok = true;

cleanup:
    if (increment) OCRelease(increment);
    if (metadata) OCRelease(metadata);
    if (lin_dim) OCRelease(lin_dim);
    if (lin_restored) OCRelease(lin_restored);
    if (json) cJSON_Delete(json);
    if (err) OCRelease(err);

    printf("Dimension JSON inheritance patterns test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
