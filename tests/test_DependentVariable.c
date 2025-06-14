// tests/test_dependent_variable.c

#include <stdio.h>
#include <stdbool.h>
#include "RMNLibrary.h"
#include "test_utils.h"

bool test_DependentVariable_base(void) {
    bool ok = false;
    DependentVariableRef dv   = NULL;
    DependentVariableRef dv2  = NULL;
    OCDictionaryRef       dict = NULL;
    OCStringRef           desc1= NULL;
    OCStringRef           desc2= NULL;

    // 1) default-create scalar length=4
    dv = DependentVariableCreateDefault(
        STR("scalar"),
        kSINumberFloat64Type,
        4,
        NULL);
    TEST_ASSERT(dv != NULL);

    // default fields
    TEST_ASSERT(OCStringGetLength(DependentVariableGetName(dv)) == 0);
    TEST_ASSERT(OCStringGetLength(DependentVariableGetDescription(dv)) == 0);
    TEST_ASSERT(DependentVariableIsScalarType(dv));
    TEST_ASSERT(DependentVariableGetElementType(dv) == kSINumberFloat64Type);
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);
    TEST_ASSERT(DependentVariableGetSize(dv) == 4);

    // 2) setters / getters
    TEST_ASSERT(DependentVariableSetName(dv, STR("foo")));
    TEST_ASSERT(OCStringEqual(DependentVariableGetName(dv), STR("foo")));
    TEST_ASSERT(DependentVariableSetDescription(dv, STR("bar")));
    TEST_ASSERT(OCStringEqual(DependentVariableGetDescription(dv), STR("bar")));

    // metadata
    TEST_ASSERT(DependentVariableGetMetaData(dv) != NULL);
    TEST_ASSERT(OCDictionaryGetCount(DependentVariableGetMetaData(dv)) == 0);
    TEST_ASSERT(DependentVariableSetMetaData(dv, NULL));
    TEST_ASSERT(OCDictionaryGetCount(DependentVariableGetMetaData(dv)) == 0);

    // sparse fields
    TEST_ASSERT(DependentVariableGetSparseDimensionIndexes(dv) != NULL);
    TEST_ASSERT(DependentVariableGetSparseGridVertexes(dv) != NULL);
    TEST_ASSERT(DependentVariableSetSparseGridVertexes(dv, STR("verts")));
    TEST_ASSERT(OCStringEqual(DependentVariableGetSparseGridVertexes(dv), STR("verts")));

    // 3) copy via OCTypeDeepCopy
    dv2 = OCTypeDeepCopy(dv);
    TEST_ASSERT(dv2 != NULL);
    TEST_ASSERT(OCTypeEqual(dv, dv2));

    // 4) dict round-trip
    dict = DependentVariableCopyAsDictionary(dv);
    TEST_ASSERT(dict != NULL);
    dv2 = DependentVariableCreateFromDictionary(dict);
    TEST_ASSERT(dv2 != NULL);

    // formatting comparison
    desc1 = OCTypeCopyFormattingDesc((OCTypeRef)dv);
    desc2 = OCTypeCopyFormattingDesc((OCTypeRef)dv2);
    TEST_ASSERT(OCStringEqual(desc1, desc2));

    ok = true;
cleanup:
    if (desc1) OCRelease(desc1);
    if (desc2) OCRelease(desc2);
    if (dict)  OCRelease(dict);
    if (dv2)   OCRelease(dv2);
    if (dv)    OCRelease(dv);
    printf("DependentVariable base tests %s\n", ok ? "passed." : "failed.");
    return ok;
}

bool test_DependentVariable_components(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    OCMutableArrayRef comps = NULL;
    OCMutableDataRef extra  = NULL;
    OCIndex count;

    // build two buffers of length=3 elements
    comps = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    for (int i = 0; i < 1; i++) {
        OCMutableDataRef b = OCDataCreateMutable(0);
        OCDataSetLength(b, 3 * sizeof(float));
        OCArrayAppendValue(comps, b);
        OCRelease(b);
    }

dv = DependentVariableCreate(
    /* name              = */ STR("v"),
    /* description       = */ STR("vec"),
    /* unit              = */ SIUnitForUnderivedSymbol(STR("m")),
    /* quantityName      = */ kSIQuantityVelocity,
    /* quantityType      = */ STR("vector_1"),
    /* elementType       = */ kSINumberFloat32Type,
    /* componentLabels   = */ NULL,
    /* components        = */ comps,
    /* owner (dataset)   = */ NULL
);    
    TEST_ASSERT(dv != NULL);
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);
    TEST_ASSERT(DependentVariableGetSize(dv) == 3);

    // insert another matching buffer
    extra = OCDataCreateMutable(0);
    OCDataSetLength(extra, 3 * sizeof(float));
    TEST_ASSERT(DependentVariableInsertComponentAtIndex(dv, extra, 1));
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 2);

    // remove it
    TEST_ASSERT(DependentVariableRemoveComponentAtIndex(dv, 1));
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);

    // out-of-bounds insertion/removal should fail
    TEST_ASSERT(!DependentVariableInsertComponentAtIndex(dv, extra, 5));
    TEST_ASSERT(!DependentVariableRemoveComponentAtIndex(dv, 1));

    // resize all buffers to length=5
    TEST_ASSERT(DependentVariableSetSize(dv, 5));
    TEST_ASSERT(DependentVariableGetSize(dv) == 5);

    // component copy
    OCMutableArrayRef copy = DependentVariableCopyComponents(dv);
    TEST_ASSERT(copy != NULL);
    count = OCArrayGetCount(copy);
    TEST_ASSERT(count == 1);
    OCRelease(copy);

    ok = true;
cleanup:
    if (extra) OCRelease(extra);
    if (dv)    OCRelease(dv);
    if (comps) OCRelease(comps);
    printf("DependentVariable components tests %s\n", ok ? "passed." : "failed.");
    return ok;
}

bool test_DependentVariable_values(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    SIScalarRef sIn   = NULL;
    SIScalarRef sOut  = NULL;
    double     dVal;

    // create scalar length=4 double
    dv = DependentVariableCreateWithSize(
        NULL, NULL, NULL, NULL,
        STR("scalar"),
        kSINumberFloat64Type,
        NULL,
        4,
        NULL);
    TEST_ASSERT(dv);

    // set element 2 to 7.5
    sIn = SIScalarCreateWithDouble(7.5, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(DependentVariableSetValueAtMemOffset(dv, 0, 2, sIn, NULL));

    // get back as double
    dVal = DependentVariableGetDoubleValueAtMemOffset(dv, 0, 2);
    TEST_ASSERT(dVal == 7.5);

    // get back as scalar
    sOut = DependentVariableCreateValueFromMemOffset(dv, 0, 2);
    TEST_ASSERT(SIScalarDoubleValueInUnit(sOut, SIUnitDimensionlessAndUnderived(), NULL) == 7.5);

    ok = true;
cleanup:
    if (sIn)  OCRelease(sIn);
    if (sOut) OCRelease(sOut);
    if (dv)   OCRelease(dv);
    printf("DependentVariable values tests %s\n", ok ? "passed." : "failed.");
    return ok;
}
