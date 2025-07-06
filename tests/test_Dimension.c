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

    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));

    // use the new convenience constructor
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
    TEST_ASSERT(DimensionSetLabel(dim, STR("MyLabel")));
    TEST_ASSERT(DimensionSetDescription(dim, STR("MyDesc")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(dim), STR("MyLabel")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(dim), STR("MyDesc")));

    // 5) set some metadata and round-trip
    meta = OCDictionaryCreateMutable(0);
    OCDictionarySetValue(meta, STR("foo"), STR("bar"));
    TEST_ASSERT(DimensionSetMetadata(dim, meta));
    gotMeta = DimensionGetMetadata(dim);
    TEST_ASSERT(OCStringEqual(OCDictionaryGetValue(gotMeta, STR("foo")), STR("bar")));

    // deep copy via OCTypeDeepCopy
    copy = (DimensionRef)OCTypeDeepCopy(dim);
    TEST_ASSERT(copy);
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(copy), STR("MyLabel")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(copy), STR("MyDesc")));
    metaCopy = DimensionGetMetadata(copy);
    TEST_ASSERT(OCStringEqual(OCDictionaryGetValue(metaCopy, STR("foo")), STR("bar")));

    ok = true;

cleanup:
    if (copy) OCRelease(copy);
    if (dim)   OCRelease(dim);
    if (meta)  OCRelease(meta);
    if (labels)OCRelease(labels);
    printf("Dimension base public API test %s\n", ok ? "passed" : "FAILED");
    return ok;
}

// --- LabeledDimension API
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

    // type, label, desc, metadata
    TEST_ASSERT(OCStringEqual(DimensionGetType((DimensionRef)ld), STR("labeled")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel((DimensionRef)ld), STR("L")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription((DimensionRef)ld), STR("desc")));
    TEST_ASSERT(OCDictionaryGetCount(DimensionGetMetadata((DimensionRef)ld)) == 0);

    // coordinate labels
    OCArrayRef got = LabeledDimensionGetCoordinateLabels(ld);
    TEST_ASSERT(got && OCArrayGetCount(got) == 3);
    TEST_ASSERT(OCStringEqual(LabeledDimensionGetCoordinateLabelAtIndex(ld,2), STR("C")));

    // dictionary round-trip
    dict = LabeledDimensionCopyAsDictionary(ld);
    TEST_ASSERT(dict);
    ld2 = LabeledDimensionCreateFromDictionary(dict, NULL);
    TEST_ASSERT(ld2);

    // formatting descriptions must match
    f1 = OCTypeCopyFormattingDesc((OCTypeRef)ld);
    f2 = OCTypeCopyFormattingDesc((OCTypeRef)ld2);
    TEST_ASSERT(f1 && f2 && OCStringEqual(f1,f2));

    ok = true;
cleanup:
    if (f1) OCRelease(f1);
    if (f2) OCRelease(f2);
    if (dict)OCRelease(dict);
    if (ld2)OCRelease(ld2);
    if (ld) OCRelease(ld);
    if (labels) OCRelease(labels);
    printf("LabeledDimension basic tests %s\n", ok ? "passed" : "FAILED");
    return ok;
}

// -- SI‐based Dimension API (covers Monotonic and Linear)
bool test_SIDimension(void) {
    bool ok = false;
    SIDimensionRef si = NULL, si2 = NULL;
    OCDictionaryRef dict = NULL;
    SIScalarRef offset = SIScalarCreateWithDouble(1.0, SIUnitFindWithUnderivedSymbol(STR("m")));

    si = SIDimensionCreate(
        STR("sidim"), STR("desc"), NULL,
        STR("length"),      // quantityName
        offset,             // origin_offset
        NULL,               // coordinates_offset == default 0
        NULL,               // period == default ∞
        false,              // periodic
        kDimensionScalingNone
    );
    TEST_ASSERT(si);

    // check CSDM‐1.0 API:
    TEST_ASSERT(OCStringEqual(DimensionGetType((DimensionRef)si), STR("si_dimension")));
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName(si), STR("length")));
    TEST_ASSERT(SIScalarDoubleValueInUnit(SIDimensionGetCoordinatesOffset(si),
                SIUnitFindWithUnderivedSymbol(STR("m")), NULL) == 1.0);
    // defaults
    TEST_ASSERT(SIDimensionGetOriginOffset(si) == 0 ||
                SIScalarDoubleValueInUnit(SIDimensionGetOriginOffset(si),
                SIUnitFindWithUnderivedSymbol(STR("m")), NULL) == 0.0);
    TEST_ASSERT(!SIDimensionIsPeriodic(si));  // new helper: checks period == ∞

    // round-trip
    dict = SIDimensionCopyAsDictionary(si);
    TEST_ASSERT(dict);
    si2 = SIDimensionCreateFromDictionary(dict, NULL);
    TEST_ASSERT(si2);
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName(si2), STR("length")));

    ok = true;
cleanup:
    if (offset) OCRelease(offset);
    if (dict)   OCRelease(dict);
    if (si2)    OCRelease(si2);
    if (si)     OCRelease(si);
    printf("SIDimension public API test %s\n", ok ? "passed" : "FAILED");
    return ok;
}

// -- Monotonic & Linear + reciprocal
bool test_SIMonotonic_and_SILinearDimension(void) {
    fprintf(stderr, "%s begin...\n", __func__);
    bool ok = false;
    OCMutableArrayRef coords = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    SIScalarRef s0 = SIScalarCreateWithDouble(0.0, SIUnitFindWithUnderivedSymbol(STR("m")));
    SIScalarRef s1 = SIScalarCreateWithDouble(1.0, SIUnitFindWithUnderivedSymbol(STR("m")));
    SIMonotonicDimensionRef mono = NULL;
    SILinearDimensionRef lin = NULL;
    SIDimensionRef rec = NULL;

    OCArrayAppendValue(coords, s0);
    OCArrayAppendValue(coords, s1);

    // 1) Monotonic
    mono = SIMonotonicDimensionCreate(
        STR("mono"), STR("desc"), NULL,
        kSIQuantityLength,
        s0,         // origin_offset
        NULL,       // coordinates_offset default 0
        NULL,       // period default ∞
        false,      // periodic
        kDimensionScalingNone,
        coords,     // coordinates
        NULL        // reciprocal
    );
    TEST_ASSERT(mono);
    TEST_ASSERT(OCStringEqual(DimensionGetType((DimensionRef)mono), STR("monotonic")));
    TEST_ASSERT(OCArrayGetCount(SIMonotonicDimensionGetCoordinates(mono)) == 2);
    // defaults
    TEST_ASSERT(SIScalarDoubleValue(SIDimensionGetCoordinatesOffset((SIDimensionRef) mono)) == 0);
    TEST_ASSERT(!SIDimensionIsPeriodic((SIDimensionRef) mono));

    // 2) Linear with reciprocal
    lin = SILinearDimensionCreate(
        STR("lin"), STR("desc"), NULL,
        kSIQuantityLength,
        s0,           // origin_offset
        NULL,         // period default ∞
        NULL,         // metadata
        false,        // periodic
        kDimensionScalingNone,
        3,            // count
        s1,           // increment
        false,        // complex_fft default false
        NULL          // reciprocal
    );
    TEST_ASSERT(lin);
    TEST_ASSERT(OCStringEqual(DimensionGetType((DimensionRef)lin), STR("linear")));
    TEST_ASSERT(SILinearDimensionGetCount(lin) == 3);
    TEST_ASSERT(SILinearDimensionGetIncrement(lin) != NULL);
    TEST_ASSERT(!SILinearDimensionGetComplexFFT(lin));
    TEST_ASSERT(OCStringEqual(SIDimensionGetQuantityName((SIDimensionRef)lin), kSIQuantityLength));
    // defaults
    TEST_ASSERT(SIScalarDoubleValue(SIDimensionGetCoordinatesOffset((SIDimensionRef) lin)) == 0);
    TEST_ASSERT(!SIDimensionIsPeriodic((SIDimensionRef)lin));

    // attach a reciprocal dimension
    {
        // SIDimensionCreate requires a non-NULL offset scalar.
        // Use 0.0 in Hz for a frequency dimension.
        SIScalarRef recOffset =
            SIScalarCreateWithDouble(0.0,
              SIUnitFindWithUnderivedSymbol(STR("Hz")));
        rec = SIDimensionCreate(
            STR("rlabel"),            // label
            NULL,                     // description
            NULL,                     // metadata
            kSIQuantityFrequency,     // quantityName
            recOffset,                // offset (must be non-NULL)
            NULL,                     // origin
            NULL,                     // period
            false,                    // periodic
            kDimensionScalingNone     // scaling
        );
        OCRelease(recOffset);
    }
    TEST_ASSERT(rec);
    TEST_ASSERT(DimensionSetLabel((DimensionRef)rec, STR("r")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel((DimensionRef)rec), STR("r")));
    TEST_ASSERT(DimensionSetDescription((DimensionRef)rec, STR("rd")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription((DimensionRef)rec), STR("rd")));

    TEST_ASSERT(SILinearDimensionSetReciprocal(lin, rec));
    TEST_ASSERT(SIMonotonicDimensionSetReciprocal(mono, rec));
    
    ok = true;

cleanup:
    if (coords) OCRelease(coords);
    if (s0)     OCRelease(s0);
    if (s1)     OCRelease(s1);
    if (mono)   OCRelease(mono);
    if (lin)    OCRelease(lin);
    if (rec)    OCRelease(rec);
    fprintf(stderr, "%s %s\n", __func__, ok ? "passed." : "FAILED!");
    return ok;
}
