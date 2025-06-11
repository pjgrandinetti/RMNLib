#include <stdio.h>
#include <stdbool.h>
#include "RMNLibrary.h"
#include "test_utils.h"

bool test_Dimension_base(void) {
    // Exercise the base-class defaults via a concrete LabeledDimension
    OCMutableArrayRef labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("X"));
    OCArrayAppendValue(labels, STR("Y"));

    // Create a LabeledDimension (concrete)
    LabeledDimensionRef ld = LabeledDimensionCreateWithCoordinateLabels(labels);
    TEST_ASSERT(ld != NULL);

    // Cast to the abstract base for testing default behavior
    DimensionRef d = (DimensionRef)ld;

    // Base defaults
    TEST_ASSERT(DimensionGetLabel(d) != NULL);
    TEST_ASSERT(OCStringGetLength(DimensionGetLabel(d)) == 0);
    TEST_ASSERT(DimensionGetDescription(d) != NULL);
    TEST_ASSERT(OCStringGetLength(DimensionGetDescription(d)) == 0);
    TEST_ASSERT(DimensionGetMetadata(d) != NULL);
    TEST_ASSERT(OCDictionaryGetCount(DimensionGetMetadata(d)) == 0);

    // Mutate via the base API
    TEST_ASSERT(DimensionSetLabel(d, STR("foo")));
    TEST_ASSERT(OCStringEqual(DimensionGetLabel(d), STR("foo")));
    TEST_ASSERT(DimensionSetDescription(d, STR("bar")));
    TEST_ASSERT(OCStringEqual(DimensionGetDescription(d), STR("bar")));

    // Deep-copy and compare
    DimensionRef d2 = DimensionDeepCopy(ld);
    TEST_ASSERT(d2 != NULL);
    TEST_ASSERT(OCTypeEqual(ld, d2));

    // Cleanup
    OCRelease(d2);
    OCRelease(ld);
    OCRelease(labels);
    return true;
}

bool test_LabeledDimension_basic(void) {
    // Prepare
    OCMutableArrayRef labels = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    TEST_ASSERT(labels != NULL);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    OCArrayAppendValue(labels, STR("C"));

    // Concrete creation
    LabeledDimensionRef ld = LabeledDimensionCreate(
        STR("L"), STR("desc"), NULL, labels);
    TEST_ASSERT(ld != NULL);

    // Validate subclass‐specific behavior
    OCArrayRef got = LabeledDimensionGetCoordinateLabels(ld);
    TEST_ASSERT(got != NULL);
    TEST_ASSERT(OCArrayGetCount(got) == 3);
    TEST_ASSERT(OCStringEqual(OCArrayGetValueAtIndex(got, 1), STR("B")));

    // Round-trip via dictionary
    OCDictionaryRef dict = LabeledDimensionCopyAsDictionary(ld);
    TEST_ASSERT(dict != NULL);
    LabeledDimensionRef ld2 = LabeledDimensionCreateFromDictionary(dict);
    TEST_ASSERT(ld2 != NULL);

    // Compare formatting
    OCStringRef f1 = OCTypeCopyFormattingDesc((OCTypeRef)ld);
    OCStringRef f2 = OCTypeCopyFormattingDesc((OCTypeRef)ld2);
    TEST_ASSERT(OCStringEqual(f1, f2));

    // Cleanup
    OCRelease(f1);
    OCRelease(f2);
    OCRelease(dict);
    OCRelease(ld2);
    OCRelease(ld);
    OCRelease(labels);
    return true;
}
