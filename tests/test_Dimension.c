#include <stdio.h>
#include <stdbool.h>
#include "RMNLibrary.h"
#include "test_utils.h"


bool test_Dimension(void)
{
    bool ok = true;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld = NULL;

    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    
    OCArrayAppendValue(labels, STR("X"));
    OCArrayAppendValue(labels, STR("Y"));

    ld = LabeledDimensionCreateWithCoordinateLabels(labels);

cleanup:
    OCRelease(labels); 
    OCRelease(ld); 
    return ok;
}

bool test_Dimension_base(void) {
    bool ok = false;
    OCMutableArrayRef labels = NULL;
    LabeledDimensionRef ld = NULL;
    DimensionRef d = NULL;
    DimensionRef d2 = NULL;

    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("X"));
    OCArrayAppendValue(labels, STR("Y"));
    ld = LabeledDimensionCreateWithCoordinateLabels(labels);
    TEST_ASSERT(ld != NULL);
    
    
    TEST_ASSERT(DimensionGetLabel((DimensionRef)ld) != NULL);
    TEST_ASSERT(OCStringGetLength(DimensionGetLabel((DimensionRef)ld)) == 0);
    TEST_ASSERT(DimensionGetDescription((DimensionRef)ld) != NULL);
    TEST_ASSERT(OCStringGetLength(DimensionGetDescription((DimensionRef)ld)) == 0);
    TEST_ASSERT(DimensionGetMetadata((DimensionRef)ld) != NULL);
    TEST_ASSERT(OCDictionaryGetCount(DimensionGetMetadata((DimensionRef)ld)) == 0);


    TEST_ASSERT(DimensionSetLabel((DimensionRef)ld, STR("foo")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel((DimensionRef)ld), STR("foo")));
    TEST_ASSERT(DimensionSetDescription((DimensionRef)ld, STR("bar")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription((DimensionRef)ld), STR("bar")));

    d2 = OCTypeDeepCopy(ld);
    TEST_ASSERT(d2 != NULL);
    TEST_ASSERT(OCTypeEqual(ld, d2));

    ok = true;   // everything passed
cleanup:
    // always run—releasing whichever were allocated
    if (d2) OCRelease(d2);
    if (ld) OCRelease(ld);
    if (labels) OCRelease(labels);
    if(ok) {
        printf("Dimension base tests passed.\n");
    } else {
        printf("Dimension base tests failed.\n");
    }
    return ok;
}


bool test_LabeledDimension_basic(void) {
    bool ok = false;
    OCMutableArrayRef   labels = NULL;
    LabeledDimensionRef ld     = NULL;
    OCDictionaryRef     dict   = NULL;
    LabeledDimensionRef ld2    = NULL;
    OCStringRef         f1     = NULL;
    OCStringRef         f2     = NULL;

    labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    OCArrayAppendValue(labels, STR("C"));

    ld = LabeledDimensionCreate(
        STR("L"), STR("desc"), NULL, labels);
    TEST_ASSERT(ld != NULL);

    // Validate subclass-specific behavior
    OCArrayRef got = LabeledDimensionGetCoordinateLabels(ld);
    TEST_ASSERT(got != NULL);
    TEST_ASSERT(OCArrayGetCount(got) == 3);
    TEST_ASSERT(OCStringEqual(OCArrayGetValueAtIndex(got, 1), STR("B")));

    // Round-trip via dictionary
    dict = LabeledDimensionCopyAsDictionary(ld);
    TEST_ASSERT(dict != NULL);
    ld2 = LabeledDimensionCreateFromDictionary(dict);
    TEST_ASSERT(ld2 != NULL);

    // Compare formatting
    f1 = OCTypeCopyFormattingDesc((OCTypeRef)ld);
    TEST_ASSERT(f1 != NULL);
    f2 = OCTypeCopyFormattingDesc((OCTypeRef)ld2);
    TEST_ASSERT(f2 != NULL);
    TEST_ASSERT(OCStringEqual(f1, f2));

    ok = true;

cleanup:
    // Always release everything that was allocated or retained.
    if (f1)     OCRelease(f1);
    if (f2)     OCRelease(f2);
    if (dict)   OCRelease(dict);
    if (ld2)    OCRelease(ld2);
    if (ld)     OCRelease(ld);
    if (labels) OCRelease(labels);
    if(ok) {
        printf("LabeledDimension basic tests passed.\n");
    } else {
        printf("LabeledDimension basic tests failed.\n");
    }
    return ok;
}
