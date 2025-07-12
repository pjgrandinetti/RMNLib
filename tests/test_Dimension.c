#include <stdbool.h>
#include <stdio.h>
#include "RMNLibrary.h"
#include "test_utils.h"

// ----------------------------------------------------------------------------
// test_CreateLongDimensionLabel
// ----------------------------------------------------------------------------
bool test_CreateLongDimensionLabel(void) {
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
        NULL,       // metadata
        labels,
        &err        // outError
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);

    longLabel = CreateLongDimensionLabel(
        (DimensionRef)ld,
        1           // no outError for CreateLongDimensionLabel
    );
    TEST_ASSERT(longLabel != NULL);

    expected = OCStringCreateWithFormat(STR("LD_Label-1"));
    TEST_ASSERT(OCStringEqual(longLabel, expected));
    OCRelease(longLabel);
    OCRelease(expected);

    // --- SIDimension case (should be: "foo-5/m") ---
    offset = SIScalarCreateWithDouble(
        3.14,
        SIUnitFindWithUnderivedSymbol(STR("m"))
    );
    TEST_ASSERT(offset != NULL);

    err = NULL;
    sidim = SIDimensionCreate(
        STR("foo"),            // label
        STR("desc"),           // description
        NULL,                  // metadata
        kSIQuantityLength,     // quantityName
        offset,                // offset
        NULL,                  // origin
        NULL,                  // period
        false,                 // periodic
        kDimensionScalingNone, // scaling
        &err                   // outError
    );
    TEST_ASSERT(sidim != NULL);
    TEST_ASSERT(err == NULL);

    OCStringRef longLabelSI = CreateLongDimensionLabel(
        (DimensionRef)sidim,
        5
    );
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

    printf("CreateLongDimensionLabel test %s\n", ok ? "passed." : "FAILED!");
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
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(dim), STR("")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(dim), STR("")));

    // 3) metadata default is an empty dictionary
    gotMeta = DimensionGetMetadata(dim);
    TEST_ASSERT(gotMeta);
    TEST_ASSERT(OCDictionaryGetCount(gotMeta) == 0);

    // 4) setters / getters
    err = NULL;
    TEST_ASSERT(DimensionSetLabel(dim, STR("MyLabel"), &err));
    TEST_ASSERT(err == NULL);

    err = NULL;
    TEST_ASSERT(DimensionSetDescription(dim, STR("MyDesc"), &err));
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(OCStringEqual(DimensionGetLabel(dim), STR("MyLabel")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(dim), STR("MyDesc")));

    // 5) set some metadata and round-trip
    meta = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(meta, STR("foo"), STR("bar"));

    err = NULL;
    TEST_ASSERT(DimensionSetMetadata(dim, meta, &err));
    TEST_ASSERT(err == NULL);

    gotMeta = DimensionGetMetadata(dim);
    TEST_ASSERT(OCStringEqual(
        OCDictionaryGetValue(gotMeta, STR("foo")),
        STR("bar")
    ));

    // deep copy via OCTypeDeepCopy
    copy = (DimensionRef)OCTypeDeepCopy(dim);
    TEST_ASSERT(copy);
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(copy), STR("MyLabel")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(copy), STR("MyDesc")));

    metaCopy = DimensionGetMetadata(copy);
    TEST_ASSERT(OCStringEqual(
        OCDictionaryGetValue(metaCopy, STR("foo")),
        STR("bar")
    ));

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
        STR("L"),       // label
        STR("desc"),    // description
        NULL,           // metadata
        labels,         // coordinateLabels
        &err            // outError
    );
    TEST_ASSERT(ld != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)ld),
        STR("labeled")
    ));
    TEST_ASSERT(OCStringEqual(
        DimensionGetLabel((DimensionRef)ld),
        STR("L")
    ));
    TEST_ASSERT(OCStringEqual(
        DimensionGetDescription((DimensionRef)ld),
        STR("desc")
    ));
    TEST_ASSERT(OCDictionaryGetCount(
        DimensionGetMetadata((DimensionRef)ld)
    ) == 0);

    OCArrayRef got = LabeledDimensionGetCoordinateLabels(ld);
    TEST_ASSERT(got && OCArrayGetCount(got) == 3);
    TEST_ASSERT(OCStringEqual(
        LabeledDimensionGetCoordinateLabelAtIndex(ld, 2),
        STR("C")
    ));

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
        SIUnitFindWithUnderivedSymbol(STR("m"))
    );
    TEST_ASSERT(offset != NULL);

    err = NULL;
    si = SIDimensionCreate(
        STR("sidim"),          // label
        STR("desc"),           // description
        NULL,                  // metadata
        kSIQuantityLength,         // quantityName
        offset,                // coordinatesOffset
        NULL,                  // originOffset
        NULL,                  // period
        false,                 // periodic
        kDimensionScalingNone, // scaling
        &err                   // outError
    );
    TEST_ASSERT(si != NULL);
    TEST_ASSERT(err == NULL);

    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)si),
        STR("si_dimension")
    ));
    TEST_ASSERT(OCStringEqual(
        SIDimensionGetQuantityName(si),
        STR("length")
    ));
    TEST_ASSERT(
        SIScalarDoubleValueInUnit(
            SIDimensionGetCoordinatesOffset(si),
            SIUnitFindWithUnderivedSymbol(STR("m")),
            NULL
        ) == 1.0
    );
    TEST_ASSERT(
        SIScalarDoubleValueInUnit(
            SIDimensionGetOriginOffset(si),
            SIUnitFindWithUnderivedSymbol(STR("m")),
            NULL
        ) == 0.0
    );
    TEST_ASSERT(!SIDimensionIsPeriodic(si));

    dict = SIDimensionCopyAsDictionary(si);
    TEST_ASSERT(dict);

    err = NULL;
    si2 = SIDimensionCreateFromDictionary(dict, &err);
    TEST_ASSERT(si2 != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        SIDimensionGetQuantityName(si2),
        STR("length")
    ));

    ok = true;

cleanup:
    if (si2) OCRelease(si2);
    if (si)  OCRelease(si);
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

    s0 = SIScalarCreateWithDouble(0.0,SIUnitFindWithUnderivedSymbol(STR("s")));
    s1 = SIScalarCreateWithDouble(1.0,SIUnitFindWithUnderivedSymbol(STR("s")));
    TEST_ASSERT(s0 && s1);

    OCArrayAppendValue(coords, s0);
    OCArrayAppendValue(coords, s1);

    // Monotonic
    err = NULL;
    mono = SIMonotonicDimensionCreate(
        STR("mono"),            // label
        STR("desc"),            // description
        NULL,                   // metadata
        kSIQuantityTime,          // quantity
        s0,                     // offset
        NULL,                   // origin
        NULL,                   // period
        false,                  // periodic
        kDimensionScalingNone,  // scaling
        coords,                 // coordinates
        NULL,                   // reciprocal
        &err                    // outError
    );
    TEST_ASSERT(mono != NULL);
    TEST_ASSERT(err == NULL);
    TEST_ASSERT(OCStringEqual(
        DimensionGetType((DimensionRef)mono),
        STR("monotonic")
    ));
    TEST_ASSERT(OCArrayGetCount(
        SIMonotonicDimensionGetCoordinates(mono)
    ) == 2);

    // Linear
    err = NULL;
    lin = SILinearDimensionCreate(
        STR("lin"),             // label
        STR("desc"),            // description
        NULL,                   // metadata
    kSIQuantityTime,          // quantity
        s0,                     // offset
        NULL,                   // origin
        NULL,                   // period
        false,                  // periodic
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
        STR("linear")
    ));
    TEST_ASSERT(SILinearDimensionGetCount(lin) == 3);
    TEST_ASSERT(SILinearDimensionGetIncrement(lin) != NULL);

    // reciprocal dimension
    {
        SIScalarRef recOff = SIScalarCreateWithDouble(
            0.0,
            SIUnitFindWithUnderivedSymbol(STR("Hz"))
        );
        err = NULL;
        rec = SIDimensionCreate(
            STR("rlabel"),         // label
            NULL,                  // description
            NULL,                  // metadata
            kSIQuantityFrequency,      // quantityName
            recOff,                // offset
            NULL,                  // origin
            NULL,                  // period
            false,                 // periodic
            kDimensionScalingNone, // scaling
            &err                   // outError
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

