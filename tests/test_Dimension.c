#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include "RMNLibrary.h"
#include "test_utils.h"
// ----------------------------------------------------------------------------
// test_DimensionCreateAxisLabel
// ----------------------------------------------------------------------------
bool test_DimensionCreateAxisLabel(void) {
    bool ok = false;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld = NULL;
    OCStringRef longLabel = NULL;
    OCStringRef expected = NULL;
    OCStringRef err = NULL;
    SIScalarRef offset = NULL;
    SIDimensionRef sidim = NULL;
    // --- LabeledDimension case ---
    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    err = NULL;
    ld = LabeledDimensionCreate(
        STR("LD_Label"),
        STR("desc"),
        NULL,  // metadata
        labels,
        &err  // outError
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);
    longLabel = DimensionCreateAxisLabel(
        (DimensionRef)ld,
        1  // no outError for DimensionCreateAxisLabel
    );
    TEST_ASSERT(longLabel != NULL);
    expected = OCStringCreateWithFormat(STR("LD_Label-1"));
    TEST_ASSERT(OCStringEqual(longLabel, expected));
    OCRelease(longLabel);
    OCRelease(expected);
    // --- SIDimension case (should be: "foo-5/m") ---
    offset = SIScalarCreateWithDouble(
        3.14,
        SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(offset != NULL);
    err = NULL;
    sidim = SIDimensionCreate(
        STR("foo"),             // label
        STR("desc"),            // description
        NULL,                   // metadata
        kSIQuantityLength,      // quantityName
        offset,                 // offset
        NULL,                   // origin
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        &err                    // outError
    );
    TEST_ASSERT(sidim != NULL);
    TEST_ASSERT(err == NULL);
    OCStringRef longLabelSI = DimensionCreateAxisLabel(
        (DimensionRef)sidim,
        5);
    TEST_ASSERT(longLabelSI != NULL);
    OCStringRef expectedSI = OCStringCreateWithFormat(STR("foo-5/m"));
    TEST_ASSERT(OCStringEqual(longLabelSI, expectedSI));
    OCRelease(longLabelSI);
    OCRelease(expectedSI);
    ok = true;
cleanup:
    if (sidim) OCRelease(sidim);
    if (offset) OCRelease(offset);
    if (ld) OCRelease(ld);
    if (labels) OCRelease(labels);
    if (err) OCRelease(err);
    printf("DimensionCreateAxisLabel test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_Dimension_base
// ----------------------------------------------------------------------------
bool test_Dimension_base(void) {
    bool ok = false;
    DimensionRef dim = NULL, copy = NULL;
    OCMutableArrayRef labels = NULL;
    OCMutableDictionaryRef meta = NULL;
    OCDictionaryRef gotMeta = NULL, metaCopy = NULL;
    OCStringRef err = NULL;
    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    // convenience constructor no longer takes outError
    dim = (DimensionRef)LabeledDimensionCreateWithCoordinateLabels(labels);
    TEST_ASSERT(dim);
    // 1) type must be "labeled"
    TEST_ASSERT(OCStringEqual(DimensionGetType(dim), STR("labeled")));
    // 2) label & description default to ""
    OCStringRef label_copy = DimensionCopyLabel(dim);
    TEST_ASSERT(OCStringEqual(label_copy, STR("")));
    OCRelease(label_copy);
    OCStringRef desc_copy = DimensionCopyDescription(dim);
    TEST_ASSERT(OCStringEqual(desc_copy, STR("")));
    OCRelease(desc_copy);
    // 3) metadata default is an empty dictionary
    gotMeta = DimensionGetApplicationMetaData(dim);
    TEST_ASSERT(gotMeta);
    TEST_ASSERT(OCDictionaryGetCount(gotMeta) == 0);
    // 4) setters / getters
    err = NULL;
    TEST_ASSERT(DimensionSetLabel(dim, STR("MyLabel"), &err));
    TEST_ASSERT(err == NULL);
    err = NULL;
    TEST_ASSERT(DimensionSetDescription(dim, STR("MyDesc"), &err));
    TEST_ASSERT(err == NULL);
    OCStringRef get_label = DimensionCopyLabel(dim);
    TEST_ASSERT(OCStringEqual(get_label, STR("MyLabel")));
    OCRelease(get_label);
    OCStringRef get_desc = DimensionCopyDescription(dim);
    TEST_ASSERT(OCStringEqual(get_desc, STR("MyDesc")));
    OCRelease(get_desc);
    // 5) set some metadata and round-trip
    meta = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(meta, STR("foo"), STR("bar"));
    err = NULL;
    TEST_ASSERT(DimensionSetApplicationMetaData(dim, meta, &err));
    TEST_ASSERT(err == NULL);
    gotMeta = DimensionGetApplicationMetaData(dim);
    TEST_ASSERT(OCStringEqual(
        OCDictionaryGetValue(gotMeta, STR("foo")),
        STR("bar")));
    // deep copy via OCTypeDeepCopy
    copy = (DimensionRef)OCTypeDeepCopy(dim);
    TEST_ASSERT(copy);
    OCStringRef copy_label = DimensionCopyLabel(copy);
    TEST_ASSERT(OCStringEqual(copy_label, STR("MyLabel")));
    OCRelease(copy_label);
    OCStringRef copy_desc = DimensionCopyDescription(copy);
    TEST_ASSERT(OCStringEqual(copy_desc, STR("MyDesc")));
    OCRelease(copy_desc);
    metaCopy = DimensionGetApplicationMetaData(copy);
    TEST_ASSERT(OCStringEqual(
        OCDictionaryGetValue(metaCopy, STR("foo")),
        STR("bar")));
    ok = true;
cleanup:
    if (copy) OCRelease(copy);
    if (dim) OCRelease(dim);
    if (meta) OCRelease(meta);
    if (labels) OCRelease(labels);
    if (err) OCRelease(err);
    printf("Dimension base public API test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_LabeledDimension
// ----------------------------------------------------------------------------
bool test_LabeledDimension(void) {
    bool ok = false;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld = NULL, ld2 = NULL;
    OCDictionaryRef dict = NULL;
    OCStringRef f1 = NULL, f2 = NULL;
    OCStringRef err = NULL;
    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    OCArrayAppendValue(labels, STR("C"));
    err = NULL;
    ld = LabeledDimensionCreate(
        STR("L"),     // label
        STR("desc"),  // description
        NULL,         // metadata
        labels,       // coordinateLabels
        &err          // outError
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)ld),
        STR("labeled")));
    OCStringRef ld_label = DimensionCopyLabel((DimensionRef)ld);
    TEST_ASSERT(OCStringEqual(ld_label, STR("L")));
    OCRelease(ld_label);
    OCStringRef ld_desc = DimensionCopyDescription((DimensionRef)ld);
    TEST_ASSERT(OCStringEqual(ld_desc, STR("desc")));
    OCRelease(ld_desc);
    OCDictionaryRef ld_meta = DimensionGetApplicationMetaData((DimensionRef)ld);
    TEST_ASSERT(OCDictionaryGetCount(ld_meta) == 0);
    OCArrayRef got = LabeledDimensionCopyCoordinateLabels(ld);
    TEST_ASSERT(got && OCArrayGetCount(got) == 3);
    OCStringRef label_at_2 = (OCStringRef)OCArrayGetValueAtIndex(got, 2);
    TEST_ASSERT(OCStringEqual(label_at_2, STR("C")));
    OCRelease(got);
    dict = LabeledDimensionCopyAsDictionary(ld);
    TEST_ASSERT(dict);
    err = NULL;
    ld2 = LabeledDimensionCreateFromDictionary(dict, &err);
    TEST_ASSERT(ld2 != NULL);
    TEST_ASSERT(err == NULL);
    f1 = OCTypeCopyFormattingDesc((OCTypeRef)ld);
    f2 = OCTypeCopyFormattingDesc((OCTypeRef)ld2);
    TEST_ASSERT(f1 && f2 && OCStringEqual(f1, f2));
    ok = true;
cleanup:
    if (f1) OCRelease(f1);
    if (f2) OCRelease(f2);
    if (dict) OCRelease(dict);
    if (ld2) OCRelease(ld2);
    if (ld) OCRelease(ld);
    if (labels) OCRelease(labels);
    if (err) OCRelease(err);
    printf("LabeledDimension basic tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_SIDimension
// ----------------------------------------------------------------------------
bool test_SIDimension(void) {
    bool ok = false;
    SIDimensionRef si = NULL, si2 = NULL;
    OCDictionaryRef dict = NULL;
    SIScalarRef offset = NULL;
    OCStringRef err = NULL;
    offset = SIScalarCreateWithDouble(
        1.0,
        SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(offset != NULL);
    err = NULL;
    si = SIDimensionCreate(
        STR("sidim"),           // label
        STR("desc"),            // description
        NULL,                   // metadata
        kSIQuantityLength,      // quantityName
        offset,                 // coordinatesOffset
        NULL,                   // originOffset
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        &err                    // outError
    );
    TEST_ASSERT(si != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)si),
        STR("si_dimension")));
    OCStringRef si_quantity = SIDimensionCopyQuantityName(si);
    TEST_ASSERT(OCStringEqual(si_quantity, STR("length")));
    OCRelease(si_quantity);
    SIScalarRef si_coords_offset = SIDimensionCopyCoordinatesOffset(si);
    TEST_ASSERT(
        SIScalarDoubleValueInUnit(si_coords_offset,
                                  SIUnitWithSymbol(STR("m")),
                                  NULL) == 1.0);
    OCRelease(si_coords_offset);
    SIScalarRef si_origin_offset = SIDimensionCopyOriginOffset(si);
    TEST_ASSERT(
        SIScalarDoubleValueInUnit(si_origin_offset,
                                  SIUnitWithSymbol(STR("m")),
                                  NULL) == 0.0);
    OCRelease(si_origin_offset);
    TEST_ASSERT(!SIDimensionIsPeriodic(si));
    dict = SIDimensionCopyAsDictionary(si);
    TEST_ASSERT(dict);
    err = NULL;
    si2 = SIDimensionCreateFromDictionary(dict, &err);
    TEST_ASSERT(si2 != NULL);
    TEST_ASSERT(err == NULL);
    OCStringRef si2_quantity = SIDimensionCopyQuantityName(si2);
    TEST_ASSERT(OCStringEqual(si2_quantity, STR("length")));
    OCRelease(si2_quantity);
    ok = true;
cleanup:
    if (si2) OCRelease(si2);
    if (si) OCRelease(si);
    if (offset) OCRelease(offset);
    if (dict) OCRelease(dict);
    if (err) OCRelease(err);
    printf("SIDimension public API test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_SIMonotonic_and_SILinearDimension
// ----------------------------------------------------------------------------
bool test_SIMonotonic_and_SILinearDimension(void) {
    fprintf(stderr, "%s begin...\n", __func__);
    bool ok = false;
    OCStringRef err = NULL;
    OCMutableArrayRef coords = NULL;
    SIScalarRef s0 = NULL, s1 = NULL;
    SIMonotonicDimensionRef mono = NULL;
    SILinearDimensionRef lin = NULL;
    SIDimensionRef rec = NULL;
    coords = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(coords);
    s0 = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("s")));
    s1 = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("s")));
    TEST_ASSERT(s0 && s1);
    OCArrayAppendValue(coords, s0);
    OCArrayAppendValue(coords, s1);
    // Monotonic
    err = NULL;
    mono = SIMonotonicDimensionCreate(
        STR("mono"),            // label
        STR("desc"),            // description
        NULL,                   // metadata
        kSIQuantityTime,        // quantity
        s0,                     // offset
        NULL,                   // origin
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        coords,                 // coordinates
        NULL,                   // reciprocal
        &err                    // outError
    );
    TEST_ASSERT(mono != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)mono),
        STR("monotonic")));
    OCArrayRef mono_coords = SIMonotonicDimensionCopyCoordinates(mono);
    TEST_ASSERT(OCArrayGetCount(mono_coords) == 2);
    OCRelease(mono_coords);
    // Linear
    err = NULL;
    lin = SILinearDimensionCreate(
        STR("lin"),             // label
        STR("desc"),            // description
        NULL,                   // metadata
        kSIQuantityTime,        // quantity
        s0,                     // offset
        NULL,                   // origin
        NULL,                   // period
        kDimensionScalingNone,  // scaling
        3,                      // count
        s1,                     // increment
        false,                  // fft
        NULL,                   // reciprocal
        &err                    // outError
    );
    TEST_ASSERT(lin != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)lin),
        STR("linear")));
    TEST_ASSERT(SILinearDimensionGetCount(lin) == 3);
    SIScalarRef lin_increment = SILinearDimensionCopyIncrement(lin);
    TEST_ASSERT(lin_increment != NULL);
    OCRelease(lin_increment);
    // reciprocal dimension
    {
        SIScalarRef recOff = SIScalarCreateWithDouble(
            0.0,
            SIUnitWithSymbol(STR("Hz")));
        err = NULL;
        rec = SIDimensionCreate(
            STR("rlabel"),          // label
            NULL,                   // description
            NULL,                   // metadata
            kSIQuantityFrequency,   // quantityName
            recOff,                 // offset
            NULL,                   // origin
            NULL,                   // period
            kDimensionScalingNone,  // scaling
            &err                    // outError
        );
        OCRelease(recOff);
        TEST_ASSERT(rec != NULL);
        TEST_ASSERT(err == NULL);
    }
    err = NULL;
    TEST_ASSERT(SILinearDimensionSetReciprocal(lin, rec, &err));
    TEST_ASSERT(err == NULL);
    err = NULL;
    TEST_ASSERT(SIMonotonicDimensionSetReciprocal(mono, rec, &err));
    TEST_ASSERT(err == NULL);
    ok = true;
cleanup:
    if (coords) OCRelease(coords);
    if (s0) OCRelease(s0);
    if (s1) OCRelease(s1);
    if (mono) OCRelease(mono);
    if (lin) OCRelease(lin);
    if (rec) OCRelease(rec);
    if (err) OCRelease(err);
    fprintf(stderr, "%s %s\n", __func__, ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_minimal_monotonic
// ----------------------------------------------------------------------------
bool test_minimal_monotonic(void) {
    OCStringRef error = NULL;
    // Create coordinates array with SIScalar objects
    double values[] = {0.0, 1.5, 3.7, 8.2, 15.0};
    OCMutableArrayRef valueArray = OCArrayCreateMutable(5, &kOCTypeArrayCallBacks);
    for (int i = 0; i < 5; i++) {
        OCStringRef valueStr = OCStringCreateWithFormat(STR("%.1f m"), values[i]);
        SIScalarRef scalar = SIScalarCreateFromExpression(valueStr, &error);
        if (!scalar || error) {
            printf("Error creating scalar: %s\n", error ? OCStringGetCString(error) : "unknown");
            OCRelease(valueArray);
            if (error) OCRelease(error);
            return false;
        }
        OCArrayAppendValue(valueArray, (const void*)scalar);
        OCRelease(scalar);
        OCRelease(valueStr);
    }
    // Test our new minimal function
    SIMonotonicDimensionRef monotonicDim = SIMonotonicDimensionCreateMinimal(
        kSIQuantityLength,       // quantityName
        (OCArrayRef)valueArray,  // coordinates
        NULL,                    // reciprocal
        &error);                 // outError
    if (!monotonicDim || error) {
        printf("Error creating monotonic dimension: %s\n",
               error ? OCStringGetCString(error) : "unknown error");
        OCRelease(valueArray);
        if (error) OCRelease(error);
        return false;
    }
    // Verify the dimension was created correctly
    OCArrayRef coords = SIMonotonicDimensionCopyCoordinates(monotonicDim);
    OCIndex count = coords ? OCArrayGetCount(coords) : 0;
    printf("✅ SIMonotonicDimensionCreateMinimal test passed!\n");
    printf("   - Created dimension with %ld coordinates\n", (long)count);
    printf("   - Quantity: Length\n");
    printf("   - No reciprocal dimension\n");
    OCRelease(coords);
    // Clean up
    OCRelease(valueArray);
    OCRelease(monotonicDim);
    fprintf(stderr, "%s %s\n", __func__, "passed.");
    return true;
}
// ----------------------------------------------------------------------------
// test_SILinearDimensionCreateCoordinates
// ----------------------------------------------------------------------------
bool test_SILinearDimensionCreateCoordinates(void) {
    fprintf(stderr, "%s begin...\n", __func__);
    bool ok = false;
    OCStringRef err = NULL;
    SILinearDimensionRef lin = NULL;
    SIScalarRef increment = NULL, offset = NULL;
    OCArrayRef coords = NULL;
    // Test case 1: Simple linear dimension without FFT
    // Create increment: 0.5 seconds
    increment = SIScalarCreateWithDouble(0.5, SIUnitWithSymbol(STR("s")));
    TEST_ASSERT(increment != NULL);
    // Create coordinates offset: 1.0 seconds
    offset = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("s")));
    TEST_ASSERT(offset != NULL);
    // Create linear dimension with count=4, no FFT
    // Expected coordinates: [1.0, 1.5, 2.0, 2.5] seconds
    // Formula: X_k = increment × (j - 0) + offset = 0.5×j + 1.0
    err = NULL;
    lin = SILinearDimensionCreate(
        STR("test_time"),            // label
        STR("Test time dimension"),  // description
        NULL,                        // metadata
        kSIQuantityTime,             // quantityName
        offset,                      // offset (coordinates_offset)
        NULL,                        // origin
        NULL,                        // period
        kDimensionScalingNone,       // scaling
        4,                           // count
        increment,                   // increment
        false,                       // complex_fft (Z_k = 0)
        NULL,                        // reciprocal
        &err                         // outError
    );
    TEST_ASSERT(lin != NULL);
    TEST_ASSERT(err == NULL);
    // Create coordinates using our new function
    coords = SILinearDimensionCreateCoordinates(lin);
    TEST_ASSERT(coords != NULL);
    TEST_ASSERT(OCArrayGetCount(coords) == 4);
    // Verify coordinate values
    // j=0: 0.5×0 + 1.0 = 1.0
    // j=1: 0.5×1 + 1.0 = 1.5
    // j=2: 0.5×2 + 1.0 = 2.0
    // j=3: 0.5×3 + 1.0 = 2.5
    double expected_values[] = {1.0, 1.5, 2.0, 2.5};
    for (OCIndex i = 0; i < 4; i++) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
        TEST_ASSERT(coord != NULL);
        double value = SIScalarDoubleValue(coord);
        double expected = expected_values[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   Coordinate[%ld]: %.1f s (expected %.1f s) ✓\n",
               (long)i, value, expected);
    }
    // Clean up test 1
    OCRelease(coords);
    OCRelease(lin);
    OCRelease(increment);
    OCRelease(offset);
    // Test case 2: Linear dimension with complex FFT
    printf("   Testing with complex_fft=true...\n");
    // Create increment: 2.0 Hz
    increment = SIScalarCreateWithDouble(2.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(increment != NULL);
    // Create coordinates offset: 10.0 Hz
    offset = SIScalarCreateWithDouble(10.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(offset != NULL);
    // Create linear dimension with count=6 (even), with FFT
    // Z_k = T_k/2 = 6/2 = 3 (since count=6 is even, T_k = count = 6)
    // Expected coordinates with FFT shift:
    // j=0: 2.0×(0-3) + 10.0 = 2.0×(-3) + 10.0 = 4.0
    // j=1: 2.0×(1-3) + 10.0 = 2.0×(-2) + 10.0 = 6.0
    // j=2: 2.0×(2-3) + 10.0 = 2.0×(-1) + 10.0 = 8.0
    // j=3: 2.0×(3-3) + 10.0 = 2.0×( 0) + 10.0 = 10.0
    // j=4: 2.0×(4-3) + 10.0 = 2.0×( 1) + 10.0 = 12.0
    // j=5: 2.0×(5-3) + 10.0 = 2.0×( 2) + 10.0 = 14.0
    err = NULL;
    lin = SILinearDimensionCreate(
        STR("test_freq"),                 // label
        STR("Test frequency dimension"),  // description
        NULL,                             // metadata
        kSIQuantityFrequency,             // quantityName
        offset,                           // offset (coordinates_offset)
        NULL,                             // origin
        NULL,                             // period
        kDimensionScalingNone,            // scaling
        6,                                // count (even)
        increment,                        // increment
        true,                             // complex_fft (Z_k = 3)
        NULL,                             // reciprocal
        &err                              // outError
    );
    TEST_ASSERT(lin != NULL);
    TEST_ASSERT(err == NULL);
    // Create coordinates using our new function
    coords = SILinearDimensionCreateCoordinates(lin);
    TEST_ASSERT(coords != NULL);
    TEST_ASSERT(OCArrayGetCount(coords) == 6);
    // Verify FFT-shifted coordinate values
    double expected_fft_values[] = {4.0, 6.0, 8.0, 10.0, 12.0, 14.0};
    for (OCIndex i = 0; i < 6; i++) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
        TEST_ASSERT(coord != NULL);
        double value = SIScalarDoubleValue(coord);
        double expected = expected_fft_values[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   FFT Coordinate[%ld]: %.1f Hz (expected %.1f Hz) ✓\n",
               (long)i, value, expected);
    }
    ok = true;
    printf("✅ SILinearDimensionCreateCoordinates test passed!\n");
    printf("   - Verified CSDM coordinate formula implementation\n");
    printf("   - Tested both normal and complex FFT modes\n");
    printf("   - All coordinate values match expected results\n");
cleanup:
    if (coords) OCRelease(coords);
    if (lin) OCRelease(lin);
    if (increment) OCRelease(increment);
    if (offset) OCRelease(offset);
    if (err) OCRelease(err);
    fprintf(stderr, "%s %s\n", __func__, ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_AbsoluteCoordinates
// ----------------------------------------------------------------------------
bool test_AbsoluteCoordinates(void) {
    fprintf(stderr, "%s begin...\n", __func__);
    bool ok = false;
    OCStringRef err = NULL;
    SILinearDimensionRef lin = NULL;
    SIMonotonicDimensionRef mono = NULL;
    SIScalarRef increment = NULL, origin_offset = NULL;
    OCArrayRef coords = NULL, abs_coords = NULL;
    OCMutableArrayRef mono_coords = NULL;
    printf("=== Testing SILinearDimension Absolute Coordinates ===\n");
    // Test case 1: SILinearDimension with origin offset
    // Create increment: 1.0 meters
    increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(increment != NULL);
    // Create origin offset: 5.0 meters
    origin_offset = SIScalarCreateWithDouble(5.0, SIUnitWithSymbol(STR("m")));
    TEST_ASSERT(origin_offset != NULL);
    // Create linear dimension with count=4
    // Regular coordinates will be: [0.0, 1.0, 2.0, 3.0] m
    // Absolute coordinates should be: [5.0, 6.0, 7.0, 8.0] m (adding origin_offset)
    err = NULL;
    lin = SILinearDimensionCreate(
        STR("test_distance"),            // label
        STR("Test distance dimension"),  // description
        NULL,                            // metadata
        kSIQuantityLength,               // quantityName
        NULL,                            // offset (coordinates_offset) - will default to zero
        origin_offset,                   // origin (origin_offset)
        NULL,                            // period
        kDimensionScalingNone,           // scaling
        4,                               // count
        increment,                       // increment
        false,                           // complex_fft
        NULL,                            // reciprocal
        &err                             // outError
    );
    TEST_ASSERT(lin != NULL);
    TEST_ASSERT(err == NULL);
    // Get regular coordinates
    coords = SILinearDimensionCreateCoordinates(lin);
    TEST_ASSERT(coords != NULL);
    TEST_ASSERT(OCArrayGetCount(coords) == 4);
    // Verify regular coordinates: [0.0, 1.0, 2.0, 3.0]
    double expected_regular[] = {0.0, 1.0, 2.0, 3.0};
    for (OCIndex i = 0; i < 4; i++) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
        TEST_ASSERT(coord != NULL);
        double value = SIScalarDoubleValue(coord);
        double expected = expected_regular[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   Regular Coordinate[%ld]: %.1f m (expected %.1f m) ✓\n",
               (long)i, value, expected);
    }
    // Get absolute coordinates
    abs_coords = SILinearDimensionCreateAbsoluteCoordinates(lin);
    TEST_ASSERT(abs_coords != NULL);
    TEST_ASSERT(OCArrayGetCount(abs_coords) == 4);
    // Verify absolute coordinates: [5.0, 6.0, 7.0, 8.0] (regular + 5.0 origin_offset)
    double expected_absolute[] = {5.0, 6.0, 7.0, 8.0};
    for (OCIndex i = 0; i < 4; i++) {
        SIScalarRef abs_coord = (SIScalarRef)OCArrayGetValueAtIndex(abs_coords, i);
        TEST_ASSERT(abs_coord != NULL);
        double value = SIScalarDoubleValue(abs_coord);
        double expected = expected_absolute[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   Absolute Coordinate[%ld]: %.1f m (expected %.1f m) ✓\n",
               (long)i, value, expected);
    }
    // Clean up linear dimension test
    OCRelease(coords);
    OCRelease(abs_coords);
    OCRelease(lin);
    OCRelease(increment);
    OCRelease(origin_offset);
    printf("=== Testing SIMonotonicDimension Absolute Coordinates ===\n");
    // Test case 2: SIMonotonicDimension with origin offset
    // Create monotonic coordinates: [0.0, 1.5, 3.5, 6.0] seconds
    mono_coords = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(mono_coords != NULL);
    double mono_values[] = {0.0, 1.5, 3.5, 6.0};
    for (int i = 0; i < 4; i++) {
        SIScalarRef coord = SIScalarCreateWithDouble(mono_values[i], SIUnitWithSymbol(STR("s")));
        TEST_ASSERT(coord != NULL);
        OCArrayAppendValue(mono_coords, coord);
        OCRelease(coord);
    }
    // Create origin offset: 10.0 seconds
    origin_offset = SIScalarCreateWithDouble(10.0, SIUnitWithSymbol(STR("s")));
    TEST_ASSERT(origin_offset != NULL);
    // Create monotonic dimension
    // Absolute coordinates should be: [10.0, 11.5, 13.5, 16.0] s (adding origin_offset)
    err = NULL;
    mono = SIMonotonicDimensionCreate(
        STR("test_time_mono"),                 // label
        STR("Test monotonic time dimension"),  // description
        NULL,                                  // metadata
        kSIQuantityTime,                       // quantityName
        NULL,                                  // offset (coordinates_offset) - will default to zero
        origin_offset,                         // origin (origin_offset)
        NULL,                                  // period
        kDimensionScalingNone,                 // scaling
        (OCArrayRef)mono_coords,               // coordinates
        NULL,                                  // reciprocal
        &err                                   // outError
    );
    TEST_ASSERT(mono != NULL);
    TEST_ASSERT(err == NULL);
    // Get regular coordinates
    coords = SIMonotonicDimensionCopyCoordinates(mono);
    TEST_ASSERT(coords != NULL);
    TEST_ASSERT(OCArrayGetCount(coords) == 4);
    // Verify regular coordinates: [0.0, 1.5, 3.5, 6.0]
    for (OCIndex i = 0; i < 4; i++) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(coords, i);
        TEST_ASSERT(coord != NULL);
        double value = SIScalarDoubleValue(coord);
        double expected = mono_values[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   Regular Coordinate[%ld]: %.1f s (expected %.1f s) ✓\n",
               (long)i, value, expected);
    }
    // Get absolute coordinates
    abs_coords = SIMonotonicDimensionCreateAbsoluteCoordinates(mono);
    TEST_ASSERT(abs_coords != NULL);
    TEST_ASSERT(OCArrayGetCount(abs_coords) == 4);
    // Verify absolute coordinates: [10.0, 11.5, 13.5, 16.0] (regular + 10.0 origin_offset)
    double expected_mono_absolute[] = {10.0, 11.5, 13.5, 16.0};
    for (OCIndex i = 0; i < 4; i++) {
        SIScalarRef abs_coord = (SIScalarRef)OCArrayGetValueAtIndex(abs_coords, i);
        TEST_ASSERT(abs_coord != NULL);
        double value = SIScalarDoubleValue(abs_coord);
        double expected = expected_mono_absolute[i];
        TEST_ASSERT(fabs(value - expected) < 1e-10);
        printf("   Absolute Coordinate[%ld]: %.1f s (expected %.1f s) ✓\n",
               (long)i, value, expected);
    }
    ok = true;
    printf("✅ Absolute Coordinates test passed!\n");
    printf("   - Verified CSDM absolute coordinate formula: X^abs_k = X_k + o_k\n");
    printf("   - Tested both SILinearDimension and SIMonotonicDimension\n");
    printf("   - All absolute coordinate values match expected results\n");
cleanup:
    if (coords) OCRelease(coords);
    if (abs_coords) OCRelease(abs_coords);
    if (mono) OCRelease(mono);
    if (origin_offset) OCRelease(origin_offset);
    if (mono_coords) OCRelease(mono_coords);
    if (err) OCRelease(err);
    fprintf(stderr, "%s %s\n", __func__, ok ? "passed." : "FAILED!");
    return ok;
}
// ----------------------------------------------------------------------------
// test_DimensionPeriodOperations
// ----------------------------------------------------------------------------
bool test_DimensionPeriodOperations(void) {
    bool ok = false;
    OCStringRef error = NULL;
    SIScalarRef increment = NULL;
    SIScalarRef coordinatesOffset = NULL;
    SIScalarRef originOffset = NULL;
    SIScalarRef testPeriod = NULL;
    SIScalarRef copiedPeriod = NULL;
    SILinearDimensionRef dim = NULL;
    SIDimensionRef siDim = NULL;
    printf("Testing dimension period operations...\n");
    // Create test scalars
    increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(increment != NULL);
    coordinatesOffset = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(coordinatesOffset != NULL);
    originOffset = SIScalarCreateWithDouble(0.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(originOffset != NULL);
    // Create test period
    testPeriod = SIScalarCreateWithDouble(10.0, SIUnitWithSymbol(STR("Hz")));
    TEST_ASSERT(testPeriod != NULL);
    // Create dimension without period (should default to infinity)
    error = NULL;
    dim = SILinearDimensionCreate(
        STR("test"),
        STR("Test dimension for period operations"),
        NULL,  // metadata
        kSIQuantityFrequency,
        coordinatesOffset,
        originOffset,
        NULL,  // period (should be infinity)
        kDimensionScalingNone,
        5,  // count
        increment,
        false,  // not complex FFT
        NULL,   // no reciprocal
        &error);
    TEST_ASSERT(dim != NULL);
    TEST_ASSERT(error == NULL);
    // Cast to SIDimensionRef for testing period operations
    siDim = (SIDimensionRef)dim;
    // Test 1: Initially should not be periodic (period is infinity)
    TEST_ASSERT(!SIDimensionIsPeriodic(siDim));
    printf("✓ Initial dimension is not periodic\n");
    // Test 2: Copy period should return an infinite scalar
    copiedPeriod = SIDimensionCopyPeriod(siDim);
    TEST_ASSERT(copiedPeriod != NULL);
    double initialPeriodValue = SIScalarDoubleValue(copiedPeriod);
    TEST_ASSERT(isinf(initialPeriodValue));
    printf("✓ Initial period is infinity: %f\n", initialPeriodValue);
    OCRelease(copiedPeriod);
    copiedPeriod = NULL;
    // Test 3: Set a finite period
    error = NULL;
    bool setResult = SIDimensionSetPeriod(siDim, testPeriod, &error);
    if (error) {
        printf("Debug: Error message exists (cannot print OCString directly)\n");
        OCRelease(error);
        error = NULL;
    } else {
        printf("Debug: No error message\n");
    }
    // Test that setting the period was successful
    TEST_ASSERT(setResult == true);
    TEST_ASSERT(error == NULL);
    // Test 4: Now should be periodic
    TEST_ASSERT(SIDimensionIsPeriodic(siDim));
    // Test 5: Copy period should return the finite value we set
    copiedPeriod = SIDimensionCopyPeriod(siDim);
    // Also test direct access - but note SIDimensionGetPeriod is private
    // For testing, let's just use the copy function twice to verify it's consistent
    SIScalarRef copiedPeriod2 = SIDimensionCopyPeriod(siDim);
    if (copiedPeriod2) OCRelease(copiedPeriod2); // Release the second copy
    if (copiedPeriod == NULL) {
        printf("Debug: Period copy failed - this is the bug!\n");
        // Let's try to debug further - check if the dimension is still periodic
        bool stillPeriodic = SIDimensionIsPeriodic(siDim);
        printf("Debug: Still periodic after set: %s\n", stillPeriodic ? "true" : "false");
        // The bug is in SIDimensionCopyPeriod itself
        printf("Debug: Bug confirmed - SIDimensionCopyPeriod returns NULL even though dimension is periodic\n");
    }
    TEST_ASSERT(copiedPeriod != NULL);
    double finePeriodValue = SIScalarDoubleValue(copiedPeriod);
    TEST_ASSERT(!isinf(finePeriodValue));
    TEST_ASSERT(fabs(finePeriodValue - 10.0) < 1e-10);
    printf("✓ Retrieved period value is correct: %f\n", finePeriodValue);
    OCRelease(copiedPeriod);
    copiedPeriod = NULL;
    // Test 6: Set period back to infinity using NULL
    error = NULL;
    setResult = SIDimensionSetPeriod(siDim, NULL, &error);
    TEST_ASSERT(setResult == true);
    TEST_ASSERT(error == NULL);
    printf("✓ Successfully set period back to infinity\n");
    // Test 7: Should not be periodic again
    TEST_ASSERT(!SIDimensionIsPeriodic(siDim));
    printf("✓ Dimension is not periodic after setting period to NULL\n");
    // Test 8: Copy period should return infinity again
    copiedPeriod = SIDimensionCopyPeriod(siDim);
    TEST_ASSERT(copiedPeriod != NULL);
    double finalPeriodValue = SIScalarDoubleValue(copiedPeriod);
    TEST_ASSERT(isinf(finalPeriodValue));
    printf("✓ Final period is infinity again: %f\n", finalPeriodValue);
    ok = true;
cleanup:
    if (copiedPeriod) OCRelease(copiedPeriod);
    if (testPeriod) OCRelease(testPeriod);
    if (originOffset) OCRelease(originOffset);
    if (coordinatesOffset) OCRelease(coordinatesOffset);
    if (increment) OCRelease(increment);
    if (dim) OCRelease(dim);
    if (error) OCRelease(error);
    printf("Dimension period operations test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_monotonic_large_scale_values
// Test monotonic dimensions with large-scale coordinate values
// ----------------------------------------------------------------------------
bool test_monotonic_large_scale_values(void) {
    bool ok = false;
    OCStringRef error = NULL;
    SIMonotonicDimensionRef monotonicDim = NULL;
    OCMutableArrayRef coordinates = NULL;
    
    // Test with a wide range of coordinate values including very large ones
    double values[] = {1.0, 100.0, 1000.0, 1000000.0, 2.36518262e15};
    int numValues = sizeof(values) / sizeof(values[0]);
    
    // Create coordinates array with SIScalar objects (dimensionless)
    coordinates = OCArrayCreateMutable(numValues, &kOCTypeArrayCallBacks);
    TEST_ASSERT(coordinates != NULL);
    
    for (int i = 0; i < numValues; i++) {
        SIScalarRef scalar = SIScalarCreateWithDouble(values[i], SIUnitDimensionlessAndUnderived());
        TEST_ASSERT(scalar != NULL);
        OCArrayAppendValue(coordinates, scalar);
        OCRelease(scalar); // Release our reference, array retains it
    }
    
    // Test 1: Full constructor
    error = NULL;
    monotonicDim = SIMonotonicDimensionCreate(
        STR("large_scale"),         // label
        STR("Large scale values"),  // description  
        NULL,                       // metadata
        kSIQuantityDimensionless,   // quantity (dimensionless since no units)
        NULL,                       // offset (NULL for default)
        NULL,                       // origin
        NULL,                       // period
        kDimensionScalingNone,      // scaling
        coordinates,                // coordinates array
        NULL,                       // reciprocal
        &error                      // outError
    );
    
    TEST_ASSERT(monotonicDim != NULL && error == NULL);
    
    // Verify the dimension properties
    OCArrayRef retrievedCoords = SIMonotonicDimensionCopyCoordinates(monotonicDim);
    TEST_ASSERT(retrievedCoords != NULL);
    
    OCIndex coordCount = OCArrayGetCount(retrievedCoords);
    TEST_ASSERT(coordCount == numValues);
    
    // Verify coordinate values
    for (OCIndex i = 0; i < coordCount; i++) {
        SIScalarRef coord = (SIScalarRef)OCArrayGetValueAtIndex(retrievedCoords, i);
        double coordValue = SIScalarDoubleValue(coord);
        TEST_ASSERT(fabs(coordValue - values[i]) < 1e-9);
    }
    OCRelease(retrievedCoords);
    
    // Verify dimension type
    OCStringRef dimType = DimensionGetType((DimensionRef)monotonicDim);
    TEST_ASSERT(OCStringEqual(dimType, STR("monotonic")));
    
    OCRelease(monotonicDim);
    monotonicDim = NULL;
    
    // Test 2: Minimal constructor
    error = NULL;
    monotonicDim = SIMonotonicDimensionCreateMinimal(
        kSIQuantityDimensionless,   // quantityName
        coordinates,                // coordinates
        NULL,                       // reciprocal
        &error                      // outError
    );
    
    TEST_ASSERT(monotonicDim != NULL && error == NULL);
    OCRelease(monotonicDim);
    monotonicDim = NULL;
    
    ok = true;
    
cleanup:
    if (coordinates) OCRelease(coordinates);
    if (monotonicDim) OCRelease(monotonicDim);
    if (error) OCRelease(error);
    
    printf("Monotonic dimension large-scale values test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}

// ----------------------------------------------------------------------------
// test_DimensionMetadataRoundTrip
// ----------------------------------------------------------------------------
bool test_DimensionMetadataRoundTrip(void) {
    bool ok = false;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld_original = NULL, ld_restored = NULL;
    OCDictionaryRef dict = NULL;
    OCMutableDictionaryRef metadata = NULL;
    OCStringRef err = NULL;
    
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
    OCDictionarySetValue(metadata, STR("version"), STR("1.0"));
    
    // Create LabeledDimension with metadata
    err = NULL;
    ld_original = LabeledDimensionCreate(
        STR("color_channel"),       // label
        STR("RGB color channels"),  // description
        metadata,                   // application metadata
        labels,                     // coordinate labels
        &err                        // error output
    );
    TEST_ASSERT(ld_original != NULL);
    TEST_ASSERT(err == NULL);
    
    // Verify original has metadata
    OCDictionaryRef orig_meta = DimensionGetApplicationMetaData((DimensionRef)ld_original);
    TEST_ASSERT(orig_meta != NULL);
    TEST_ASSERT(OCDictionaryGetCount(orig_meta) == 2);
    
    OCStringRef orig_encoding = (OCStringRef)OCDictionaryGetValue(orig_meta, STR("encoding"));
    TEST_ASSERT(orig_encoding != NULL);
    TEST_ASSERT(OCStringEqual(orig_encoding, STR("sRGB")));
    
    OCStringRef orig_version = (OCStringRef)OCDictionaryGetValue(orig_meta, STR("version"));
    TEST_ASSERT(orig_version != NULL);
    TEST_ASSERT(OCStringEqual(orig_version, STR("1.0")));
    
    // Convert to dictionary
    dict = LabeledDimensionCopyAsDictionary(ld_original);
    TEST_ASSERT(dict != NULL);
    
    // Verify metadata is in dictionary
    OCDictionaryRef dict_meta = (OCDictionaryRef)OCDictionaryGetValue(dict, STR("application"));
    TEST_ASSERT(dict_meta != NULL);
    TEST_ASSERT(OCDictionaryGetCount(dict_meta) == 2);
    
    OCStringRef dict_encoding = (OCStringRef)OCDictionaryGetValue(dict_meta, STR("encoding"));
    TEST_ASSERT(dict_encoding != NULL);
    TEST_ASSERT(OCStringEqual(dict_encoding, STR("sRGB")));
    
    // Create dimension from dictionary (round-trip)
    err = NULL;
    ld_restored = LabeledDimensionCreateFromDictionary(dict, &err);
    TEST_ASSERT(ld_restored != NULL);
    TEST_ASSERT(err == NULL);
    
    // Verify restored dimension has metadata
    OCDictionaryRef restored_meta = DimensionGetApplicationMetaData((DimensionRef)ld_restored);
    TEST_ASSERT(restored_meta != NULL);
    TEST_ASSERT(OCDictionaryGetCount(restored_meta) == 2);
    
    OCStringRef restored_encoding = (OCStringRef)OCDictionaryGetValue(restored_meta, STR("encoding"));
    TEST_ASSERT(restored_encoding != NULL);
    TEST_ASSERT(OCStringEqual(restored_encoding, STR("sRGB")));
    
    OCStringRef restored_version = (OCStringRef)OCDictionaryGetValue(restored_meta, STR("version"));
    TEST_ASSERT(restored_version != NULL);
    TEST_ASSERT(OCStringEqual(restored_version, STR("1.0")));
    
    // Verify that other properties are preserved too
    OCStringRef restored_label = DimensionCopyLabel((DimensionRef)ld_restored);
    TEST_ASSERT(restored_label != NULL);
    TEST_ASSERT(OCStringEqual(restored_label, STR("color_channel")));
    OCRelease(restored_label);
    
    OCStringRef restored_desc = DimensionCopyDescription((DimensionRef)ld_restored);
    TEST_ASSERT(restored_desc != NULL);
    TEST_ASSERT(OCStringEqual(restored_desc, STR("RGB color channels")));
    OCRelease(restored_desc);
    
    ok = true;
    
cleanup:
    if (labels) OCRelease(labels);
    if (metadata) OCRelease(metadata);
    if (ld_original) OCRelease(ld_original);
    if (ld_restored) OCRelease(ld_restored);
    if (dict) OCRelease(dict);
    if (err) OCRelease(err);
    
    printf("Dimension metadata round-trip test %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
