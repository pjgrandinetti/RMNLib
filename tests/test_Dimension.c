#include <stdbool.h>
#include <stdio.h>
#include "RMNLibrary.h"
#include "test_utils.h"
bool test_CreateLongDimensionLabel(void) {
    bool ok = false;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld = NULL;
    OCStringRef longLabel = NULL;
    SIDimensionRef sidim = NULL;
    OCStringRef longLabelSI = NULL;
    SIScalarRef offset = NULL;
    OCStringRef expected = NULL;
    OCStringRef expectedSI = NULL;

    // --- LabeledDimension case ---
    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    ld = LabeledDimensionCreate(STR("LD_Label"), STR("desc"), NULL, labels);
    TEST_ASSERT(ld != NULL);

    // Should be: "LD_Label-1"
    longLabel = CreateLongDimensionLabel((DimensionRef)ld, 1);
    expected = OCStringCreateWithFormat(STR("LD_Label-1"));
    TEST_ASSERT(longLabel != NULL);
    TEST_ASSERT(OCStringEqual(longLabel, expected));
    OCRelease(longLabel);
    OCRelease(expected);

    // --- SIDimension case (should be: "foo-5/m") ---
    offset = SIScalarCreateWithDouble(3.14, SIUnitFindWithUnderivedSymbol(STR("m")));
    sidim = SIDimensionCreate(
        STR("foo"),            // label
        STR("desc"),           // description
        NULL,                  // metadata
        STR("length"),         // quantityName
        offset,                // offset
        NULL,                  // origin
        NULL,                  // period
        false,                 // periodic
        kDimensionScalingNone  // scaling
    );
    TEST_ASSERT(sidim != NULL);
    longLabelSI = CreateLongDimensionLabel((DimensionRef)sidim, 5);
    expectedSI = OCStringCreateWithFormat(STR("foo-5/m"));
    TEST_ASSERT(longLabelSI != NULL);
    TEST_ASSERT(OCStringEqual(longLabelSI, expectedSI));
    OCRelease(longLabelSI);
    OCRelease(expectedSI);

    ok = true;
cleanup:
    if (sidim) OCRelease(sidim);
    if (offset) OCRelease(offset);
    if (ld) OCRelease(ld);
    if (labels) OCRelease(labels);
    if (!ok)
        printf("CreateLongDimensionLabel test failed.\n");
    else
        printf("CreateLongDimensionLabel test passed.\n");
    return ok;
}
// -- Base Dimension API
bool test_Dimension_base(void) {
    bool ok = false;
    DimensionRef dim = NULL, copy = NULL;
    OCMutableArrayRef labels = NULL;
    OCMutableDictionaryRef meta = NULL;
    OCDictionaryRef gotMeta = NULL, metaCopy = NULL;

    // Create a mutable array and add elements using the public API
    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));

    dim = (DimensionRef)LabeledDimensionCreateWithCoordinateLabels(labels);
    TEST_ASSERT(dim);

    // Label, description, metadata default values
    TEST_ASSERT(DimensionGetLabel(dim) != NULL);
    TEST_ASSERT(DimensionGetDescription(dim) != NULL);
    TEST_ASSERT(DimensionGetMetadata(dim) != NULL);

    // Set label, description
    TEST_ASSERT(DimensionSetLabel(dim, STR("LABEL")));
    TEST_ASSERT(DimensionSetDescription(dim, STR("DESC")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(dim), STR("LABEL")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(dim), STR("DESC")));

    // Set metadata and check round-trip
    meta = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(meta, STR("foo"), STR("bar"));
    TEST_ASSERT(DimensionSetMetadata(dim, meta));
    gotMeta = DimensionGetMetadata(dim);
    TEST_ASSERT(gotMeta && OCStringEqual(OCDictionaryGetValue(gotMeta, STR("foo")), STR("bar")));

    // Deep copy should match (compare via public API)
    copy = (DimensionRef)OCTypeDeepCopy(dim);
    TEST_ASSERT(copy != NULL);
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(copy), STR("LABEL")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(copy), STR("DESC")));
    metaCopy = DimensionGetMetadata(copy);
    TEST_ASSERT(metaCopy && OCStringEqual(OCDictionaryGetValue(metaCopy, STR("foo")), STR("bar")));

    ok = true;
cleanup:
    if (copy) OCRelease(copy);
    if (dim) OCRelease(dim);
    if (meta) OCRelease(meta);
    if (labels) OCRelease(labels);
    printf("Dimension base public API test %s\n", ok ? "passed" : "failed");
    return ok;
}
bool test_LabeledDimension(void) {
    bool ok = false;
    OCMutableArrayRef labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    LabeledDimensionRef ld = NULL, ld2 = NULL;
    OCDictionaryRef dict = NULL;
    OCStringRef f1 = NULL, f2 = NULL;
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    OCArrayAppendValue(labels, STR("C"));
    ld = LabeledDimensionCreate(STR("L"), STR("desc"), NULL, labels);
    TEST_ASSERT(ld);
    // Check coordinate labels
    OCArrayRef got = LabeledDimensionGetCoordinateLabels(ld);
    TEST_ASSERT(got && OCArrayGetCount(got) == 3);
    TEST_ASSERT(OCStringEqual(LabeledDimensionGetCoordinateLabelAtIndex(ld, 2), STR("C")));
    TEST_ASSERT(OCStringEqual(OCArrayGetValueAtIndex(got, 1), STR("B")));
    // Round-trip via dictionary
    dict = LabeledDimensionCopyAsDictionary(ld);
    TEST_ASSERT(dict);
    ld2 = LabeledDimensionCreateFromDictionary(dict);
    TEST_ASSERT(ld2);
    // Formatting desc
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
    printf("LabeledDimension basic tests %s.\n", ok ? "passed" : "failed");
    return ok;
}
// -- SIDimension API
bool test_SIDimension(void) {
    bool ok = false;
    SIDimensionRef si = NULL, si2 = NULL;
    OCDictionaryRef dict = NULL;
    SIScalarRef offset = SIScalarCreateWithDouble(1.0, SIUnitFindWithUnderivedSymbol(STR("m")));
    si = SIDimensionCreate(
        STR("sidim"), STR("desc"), NULL,
        STR("length"), offset, NULL, NULL, false, kDimensionScalingNone);
    TEST_ASSERT(si);
    TEST_ASSERT(OCStringEqual(DimensionGetLabel((DimensionRef)si), STR("sidim")));
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName(si), STR("length")));
    TEST_ASSERT(SIDimensionGetOffset(si) != NULL);
    // Round-trip
    dict = SIDimensionCopyAsDictionary(si);
    TEST_ASSERT(dict != NULL);
    si2 = SIDimensionCreateFromDictionary(dict);
    TEST_ASSERT(si2 != NULL);
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName(si2), STR("length")));
    ok = true;
cleanup:
    if (offset) OCRelease(offset);
    if (dict) OCRelease(dict);
    if (si2) OCRelease(si2);
    if (si) OCRelease(si);
    printf("SIDimension public API test %s\n", ok ? "passed" : "failed");
    return ok;
}

bool test_SIMonotonic_and_SILinearDimension(void) {
    fprintf(stderr, "%s begin...\n", __func__);
    bool ok = false;
    OCArrayRef coords = NULL;
    SIScalarRef s0 = NULL, s1 = NULL;
    SIMonotonicDimensionRef mono = NULL;
    SILinearDimensionRef lin = NULL;

    fprintf(stderr, "[DEBUG] Creating coordinate array...\n");
    coords = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    if (!coords) {
        fprintf(stderr, "[ERROR] Failed to create OCArray for coordinates\n");
        goto cleanup;
    }

    fprintf(stderr, "[DEBUG] Creating SIScalar s0 = 0.0 m...\n");
    s0 = SIScalarCreateWithDouble(0.0, SIUnitFindWithUnderivedSymbol(STR("m")));
    if (!s0) {
        fprintf(stderr, "[ERROR] Failed to create SIScalar s0\n");
        goto cleanup;
    }

    fprintf(stderr, "[DEBUG] Creating SIScalar s1 = 1.0 m...\n");
    s1 = SIScalarCreateWithDouble(1.0, SIUnitFindWithUnderivedSymbol(STR("m")));
    if (!s1) {
        fprintf(stderr, "[ERROR] Failed to create SIScalar s1\n");
        goto cleanup;
    }

    OCArrayAppendValue((OCMutableArrayRef)coords, s0);
    OCArrayAppendValue((OCMutableArrayRef)coords, s1);
    mono = SIMonotonicDimensionCreate(
        STR("mono"), STR("desc"), NULL,
        kSIQuantityLength,
        s0, NULL, NULL, false, kDimensionScalingNone, coords, NULL);
    if (!mono) {
        fprintf(stderr, "[ERROR] SIMonotonicDimensionCreate failed\n");
        goto cleanup;
    } 

    OCArrayRef got_coords = SIMonotonicDimensionGetCoordinates(mono);
    if (!got_coords) {
        fprintf(stderr, "[ERROR] SIMonotonicDimensionGetCoordinates returned NULL\n");
        goto cleanup;
    } 

    lin = SILinearDimensionCreate(
        STR("lin"), STR("desc"), NULL, kSIQuantityLength, s0, NULL, NULL, false,
        kDimensionScalingNone, 2, s1, false, NULL);
    if (!lin) {
        fprintf(stderr, "[ERROR] SILinearDimensionCreate failed\n");
        goto cleanup;
    }

    OCIndex count = SILinearDimensionGetCount(lin);
    if (count != 2) {
        fprintf(stderr, "[ERROR] SILinearDimensionGetCount: expected 2, got %lld\n", (long long)count);
        goto cleanup;
    } 
    ok = true;

cleanup:
    if (coords) OCRelease(coords);
    if (s0) OCRelease(s0);
    if (s1) OCRelease(s1);
    if (mono) OCRelease(mono);
    if (lin) OCRelease(lin);
    fprintf(stderr, "%s %s.\n", __func__, ok ? "passed" : "FAILED");
    return ok;
}
