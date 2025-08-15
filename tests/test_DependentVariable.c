#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "RMNLibrary.h"
#include "SparseSampling.h"
#include "test_utils.h"
// helper to build a “scalar” internal DV of a given length
static DependentVariableRef _make_internal_scalar(OCIndex length) {
    OCMutableArrayRef comps = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableDataRef buf = OCDataCreateMutable(0);
    OCDataSetLength(buf, length * sizeof(double));
    OCArrayAppendValue(comps, buf);
    OCRelease(buf);
    DependentVariableRef dv = NULL;
    OCStringRef err = NULL;
    dv = DependentVariableCreate(
        STR(""),  // name
        STR(""),  // description
        SIUnitDimensionlessAndUnderived(),
        kSIQuantityDimensionless,  // quantity_name
        STR("scalar"),             // quantity_type
        kOCNumberFloat64Type,      // numeric_type
        NULL,                      // componentLabels
        comps,                     // components
        &err                       // outError
    );
    if (!dv && err) {
        printf("_make_internal_scalar failed: %s\n", OCStringGetCString(err));
    }
    OCRelease(err);
    OCRelease(comps);
    return dv;
}
bool test_DependentVariable_base(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    OCStringRef name_str = NULL, desc_str = NULL, qt_str = NULL, qn_str = NULL, enc_str = NULL;
    // 1) default‐internal DV of length=4
    dv = _make_internal_scalar(4);
    TEST_ASSERT(dv != NULL);
    // defaults
    name_str = DependentVariableCopyName(dv);
    TEST_ASSERT(OCStringGetLength(name_str) == 0);
    OCRelease((OCTypeRef)name_str);
    desc_str = DependentVariableCopyDescription(dv);
    TEST_ASSERT(OCStringGetLength(desc_str) == 0);
    OCRelease((OCTypeRef)desc_str);
    qt_str = DependentVariableCopyQuantityType(dv);
    TEST_ASSERT(OCStringEqual(qt_str, STR("scalar")));
    OCRelease((OCTypeRef)qt_str);
    TEST_ASSERT(DependentVariableGetNumericType(dv) == kOCNumberFloat64Type);
    TEST_ASSERT(OCTypeEqual(
        SIQuantityGetUnit((SIQuantityRef)dv),
        SIUnitDimensionlessAndUnderived()));
    qn_str = DependentVariableCopyQuantityName(dv);
    TEST_ASSERT(OCStringEqual(qn_str, kSIQuantityDimensionless));
    OCRelease((OCTypeRef)qn_str);
    enc_str = DependentVariableCopyEncoding(dv);
    TEST_ASSERT(OCStringEqual(enc_str, STR("base64")));
    OCRelease((OCTypeRef)enc_str);
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);
    TEST_ASSERT(DependentVariableGetSize(dv) == 4);
    // 2) setters / getters for CSDM fields
    TEST_ASSERT(DependentVariableSetName(dv, STR("foo")));
    name_str = DependentVariableCopyName(dv);
    TEST_ASSERT(OCStringEqual(name_str, STR("foo")));
    OCRelease((OCTypeRef)name_str);
    TEST_ASSERT(DependentVariableSetDescription(dv, STR("bar")));
    desc_str = DependentVariableCopyDescription(dv);
    TEST_ASSERT(OCStringEqual(desc_str, STR("bar")));
    OCRelease((OCTypeRef)desc_str);
    SIUnitRef m_per_s = SIUnitWithSymbol(STR("m/s"));
    TEST_ASSERT(SIQuantitySetUnit((SIMutableQuantityRef)dv, m_per_s));
    TEST_ASSERT(OCTypeEqual(
        SIQuantityGetUnit((SIQuantityRef)dv),
        m_per_s));
    TEST_ASSERT(DependentVariableSetQuantityName(dv, STR("velocity")));
    qn_str = DependentVariableCopyQuantityName(dv);
    TEST_ASSERT(OCStringEqual(qn_str, STR("velocity")));
    OCRelease((OCTypeRef)qn_str);
    TEST_ASSERT(DependentVariableSetEncoding(dv, STR("base64")));
    enc_str = DependentVariableCopyEncoding(dv);
    TEST_ASSERT(OCStringEqual(enc_str, STR("base64")));
    OCRelease((OCTypeRef)enc_str);
    ok = true;
cleanup:
    if (dv) OCRelease(dv);
    printf("DependentVariable base tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_internal_vs_external(void) {
    bool ok = false;
    DependentVariableRef intdv = NULL, extdv = NULL;
    OCStringRef err = NULL, enc_str = NULL;
    // ── internal: components present, no URL ──
    intdv = _make_internal_scalar(3);
    TEST_ASSERT(intdv);
    TEST_ASSERT(DependentVariableGetComponentCount(intdv) == 1);
    TEST_ASSERT(DependentVariableGetComponentsURL(intdv) == NULL);
    enc_str = DependentVariableCopyEncoding(intdv);
    TEST_ASSERT(OCStringEqual(enc_str, STR("base64")));
    OCRelease((OCTypeRef)enc_str);
    // ── external: must supply a non-NULL URL now ──
    err = NULL;
    printf("Creating external DV...\n");
    extdv = DependentVariableCreateExternal(
        STR("ext"),            // name
        STR("external test"),  // description
        SIUnitDimensionlessAndUnderived(),
        kSIQuantityDimensionless,  // quantityName
        STR("scalar"),             // quantityType
        kOCNumberFloat32Type,      // elementType
        STR("file:./data.bin"),    // componentsURL (no longer allowed to pass NULL)
        &err                       // outError
    );
    if (!extdv) {
        printf("DependentVariableCreateExternal failed: %s\n",
               err ? OCStringGetCString(err) : "unknown error");
    }
    printf("External DV created successfully\n");
    TEST_ASSERT(extdv);
    // no in-memory buffers
    TEST_ASSERT(DependentVariableGetComponentCount(extdv) == 0);
    // URL should match exactly what we passed
    {
        OCStringRef url = DependentVariableGetComponentsURL(extdv);
        TEST_ASSERT(url && OCStringEqual(url, STR("file:./data.bin")));
    }
    // encoding should be "base64" for externals too (new default)
    {
        OCStringRef enc = DependentVariableCopyEncoding(extdv);
        printf("External DV encoding: %s\n", enc ? OCStringGetCString(enc) : "NULL");
        TEST_ASSERT(enc && OCStringEqual(enc, STR("base64")));
        OCRelease((OCTypeRef)enc);
    }
    ok = true;
cleanup:
    if (intdv) OCRelease(intdv);
    if (extdv) OCRelease(extdv);
    if (err) OCRelease(err);
    printf("DependentVariable internal/external tests %s\n",
           ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_values_and_accessors(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    SIScalarRef sIn = NULL, sOut = NULL;
    OCStringRef err = NULL;
    double got;
    // make a 1-component scalar of length 4
    err = NULL;
    dv = DependentVariableCreateWithSize(
        STR(""), STR(""),
        SIUnitDimensionlessAndUnderived(),
        /* quantityName */ NULL,  // must be NULL (not empty string) so the impl skips the name/dimensionality check
        STR("scalar"),
        kOCNumberFloat64Type,
        NULL,  // componentLabels
        4,     // explicitSize
        &err   // outError
    );
    if (!dv && err) {
        printf("DependentVariableCreateWithSize failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv);
    // set a couple of values
    sIn = SIScalarCreateWithDouble(3.14, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(DependentVariableSetValueAtMemOffset(dv, 0, 2, sIn, &err));
    OCRelease(sIn);
    got = DependentVariableGetDoubleValueAtMemOffset(dv, 0, 2);
    TEST_ASSERT(fabs(got - 3.14) < 1e-12);
    sOut = DependentVariableCreateValueFromMemOffset(dv, 0, 2);
    TEST_ASSERT(
        fabs(
            SIScalarDoubleValueInUnit(
                sOut,
                SIUnitDimensionlessAndUnderived(),
                NULL) -
            3.14) < 1e-12);
    OCRelease(sOut);
    ok = true;
cleanup:
    if (dv) OCRelease(dv);
    OCRelease(err);
    printf("DependentVariable values tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_type_queries(void) {
    bool ok = false;
    OCIndex r, c, count;
    OCStringRef err = NULL;
    // scalar
    DependentVariableRef dv1 = _make_internal_scalar(1);
    TEST_ASSERT(dv1);
    TEST_ASSERT(DependentVariableIsScalarType(dv1));
    TEST_ASSERT(!DependentVariableIsVectorType(dv1, &count));
    TEST_ASSERT(!DependentVariableIsPixelType(dv1, &count));
    TEST_ASSERT(!DependentVariableIsMatrixType(dv1, &r, &c));
    OCRelease(dv1);
    // vector_3
    err = NULL;
    DependentVariableRef dv2 = DependentVariableCreateDefault(
        STR("vector_3"), kOCNumberFloat32Type, 3, &err);
    if (!dv2 && err) {
        printf("DependentVariableCreateDefault(vector_3) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv2);
    TEST_ASSERT(!DependentVariableIsScalarType(dv2));
    TEST_ASSERT(DependentVariableIsVectorType(dv2, &count) && count == 3);
    OCRelease(dv2);
    // pixel_2
    err = NULL;
    DependentVariableRef dv3 = DependentVariableCreateDefault(
        STR("pixel_2"), kOCNumberFloat32Type, 2, &err);
    if (!dv3 && err) {
        printf("DependentVariableCreateDefault(pixel_2) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv3);
    TEST_ASSERT(DependentVariableIsPixelType(dv3, &count) && count == 2);
    OCRelease(dv3);
    // matrix_2_2
    err = NULL;
    DependentVariableRef dv4 = DependentVariableCreateDefault(
        STR("matrix_2_2"), kOCNumberFloat64Type, 4, &err);
    if (!dv4 && err) {
        printf("DependentVariableCreateDefault(matrix_2_2) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv4);
    TEST_ASSERT(DependentVariableIsMatrixType(dv4, &r, &c) && r == 2 && c == 2);
    OCRelease(dv4);
    ok = true;
cleanup:
    OCRelease(err);
    printf("DependentVariable type-queries tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_sparse_sampling(void) {
    bool ok = false;
    DependentVariableRef dv = _make_internal_scalar(10);
    TEST_ASSERT(dv);
    // initially none
    TEST_ASSERT(DependentVariableGetSparseSampling(dv) == NULL);
    // build a sparse‐sampling object
    OCMutableIndexSetRef dims = OCIndexSetCreateMutable();
    OCIndexSetAddIndex(dims, 1);
    OCMutableArrayRef verts = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableIndexPairSetRef pset = OCIndexPairSetCreateMutable();
    OCIndexPairSetAddIndexPair(pset, 1, 3);
    OCArrayAppendValue(verts, pset);
    OCRelease(pset);
    SparseSamplingRef ss = SparseSamplingCreate(
        dims,
        verts,
        kOCNumberUInt16Type,
        STR("base64"),
        STR("sparse-desc"),
        NULL,
        NULL);
    OCRelease(dims);
    OCRelease(verts);
    TEST_ASSERT(ss != NULL);
    // attach to DV
    TEST_ASSERT(DependentVariableSetSparseSampling(dv, ss));
    // retrieve and inspect
    SparseSamplingRef got = DependentVariableGetSparseSampling(dv);
    TEST_ASSERT(got);
    TEST_ASSERT(SparseSamplingGetUnsignedIntegerType(got) == kOCNumberUInt16Type);
    TEST_ASSERT(OCStringEqual(SparseSamplingGetEncoding(got), STR("base64")));
    TEST_ASSERT(OCStringEqual(SparseSamplingGetDescription(got), STR("sparse-desc")));
    OCRelease(ss);
    ok = true;
cleanup:
    if (dv) OCRelease(dv);
    printf("DependentVariable sparse-sampling tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_copy_and_roundtrip(void) {
    bool ok = false;
    DependentVariableRef dv = NULL, dv2 = NULL, dv3 = NULL;
    OCDictionaryRef dict = NULL;
    OCStringRef err = NULL;
    dv = _make_internal_scalar(5);
    TEST_ASSERT(dv);
    // deep‐copy
    dv2 = OCTypeDeepCopy(dv);
    TEST_ASSERT(dv2);
    TEST_ASSERT(OCTypeEqual(dv, dv2));
    // dict round‐trip
    dict = DependentVariableCopyAsDictionary(dv);
    TEST_ASSERT(dict);
    err = NULL;
    dv3 = DependentVariableCreateFromDictionary(dict, &err);
    if (!dv3 && err) {
        printf("DependentVariableCreateFromDictionary failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv3);
    TEST_ASSERT(OCTypeEqual(dv, dv3));
    ok = true;
cleanup:
    if (dv3) OCRelease(dv3);
    if (dict) OCRelease(dict);
    if (dv2) OCRelease(dv2);
    if (dv) OCRelease(dv);
    OCRelease(err);
    printf("DependentVariable copy/roundtrip tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_invalid_create(void) {
    bool ok = false;
    OCMutableArrayRef comps = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableDataRef b = OCDataCreateMutable(0);
    OCDataSetLength(b, 4 * sizeof(float));
    OCArrayAppendValue(comps, b);
    OCRelease(b);
    OCStringRef err = NULL;
    DependentVariableRef dv = DependentVariableCreate(
        STR("x"), STR("desc"),
        SIUnitDimensionlessAndUnderived(),
        kSIQuantityDimensionless, STR("vector_2"),
        kOCNumberFloat32Type,
        NULL,
        comps,
        &err);
    if (dv == NULL && err) {
        printf("Expected failure: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv == NULL);
    ok = true;
cleanup:
    OCRelease(comps);
    OCRelease(err);
    printf("DependentVariable invalid-create tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_components(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    OCMutableArrayRef comps = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableDataRef extra = NULL;
    OCStringRef err = NULL;
    OCIndex count;
    {
        OCMutableDataRef b = OCDataCreateMutable(0);
        OCDataSetLength(b, 3 * sizeof(float));
        OCArrayAppendValue(comps, b);
        OCRelease(b);
    }
    dv = DependentVariableCreate(
        STR("v"),
        STR("vec"),
        SIUnitWithSymbol(STR("m/s")),
        STR("velocity"),
        STR("vector_1"),
        kOCNumberFloat32Type,
        NULL,
        comps,
        &err);
    if (!dv && err) {
        printf("DependentVariableCreate failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv);
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);
    TEST_ASSERT(DependentVariableGetSize(dv) == 3);
    extra = OCDataCreateMutable(0);
    OCDataSetLength(extra, 3 * sizeof(float));
    TEST_ASSERT(DependentVariableInsertComponentAtIndex(dv, extra, 1));
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 2);
    TEST_ASSERT(DependentVariableRemoveComponentAtIndex(dv, 1));
    TEST_ASSERT(DependentVariableGetComponentCount(dv) == 1);
    TEST_ASSERT(!DependentVariableInsertComponentAtIndex(dv, extra, 5));
    TEST_ASSERT(!DependentVariableRemoveComponentAtIndex(dv, 1));
    TEST_ASSERT(DependentVariableSetSize(dv, 5));
    TEST_ASSERT(DependentVariableGetSize(dv) == 5);
    OCMutableArrayRef copy = DependentVariableCopyComponents(dv);
    TEST_ASSERT(copy != NULL);
    count = OCArrayGetCount(copy);
    TEST_ASSERT(count == 1);
    OCRelease(copy);
    ok = true;
cleanup:
    if (extra) OCRelease(extra);
    if (dv) OCRelease(dv);
    if (comps) OCRelease(comps);
    OCRelease(err);
    printf("DependentVariable components tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_values(void) {
    bool ok = false;
    DependentVariableRef dv = NULL;
    SIScalarRef sIn = NULL, sOut = NULL;
    OCStringRef err = NULL;
    double dVal;
    err = NULL;
    dv = DependentVariableCreateWithSize(
        NULL, NULL, NULL, NULL,
        STR("scalar"),
        kOCNumberFloat64Type,
        NULL,
        4,
        &err);
    if (!dv && err) {
        printf("DependentVariableCreateWithSize failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv);
    sIn = SIScalarCreateWithDouble(7.5, SIUnitDimensionlessAndUnderived());
    TEST_ASSERT(DependentVariableSetValueAtMemOffset(dv, 0, 2, sIn, &err));
    OCRelease(sIn);
    dVal = DependentVariableGetDoubleValueAtMemOffset(dv, 0, 2);
    TEST_ASSERT(dVal == 7.5);
    sOut = DependentVariableCreateValueFromMemOffset(dv, 0, 2);
    TEST_ASSERT(SIScalarDoubleValueInUnit(sOut, SIUnitDimensionlessAndUnderived(), NULL) == 7.5);
    OCRelease(sOut);
    ok = true;
cleanup:
    if (dv) OCRelease(dv);
    OCRelease(err);
    printf("DependentVariable values tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_typeQueries(void) {
    bool ok = false;
    OCIndex count, r, c;
    OCStringRef err = NULL;
    err = NULL;
    DependentVariableRef dv1 = DependentVariableCreateDefault(STR("scalar"), kOCNumberFloat64Type, 1, &err);
    if (!dv1 && err) {
        printf("DVCreateDefault(scalar) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv1);
    TEST_ASSERT(DependentVariableIsScalarType(dv1));
    TEST_ASSERT(!DependentVariableIsVectorType(dv1, &count));
    TEST_ASSERT(!DependentVariableIsPixelType(dv1, &count));
    TEST_ASSERT(!DependentVariableIsMatrixType(dv1, &r, &c));
    OCRelease(dv1);
    err = NULL;
    DependentVariableRef dv2 = DependentVariableCreateDefault(STR("vector_3"), kOCNumberFloat32Type, 3, &err);
    if (!dv2 && err) {
        printf("DVCreateDefault(vector_3) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv2);
    TEST_ASSERT(!DependentVariableIsScalarType(dv2));
    TEST_ASSERT(DependentVariableIsVectorType(dv2, &count) && count == 3);
    TEST_ASSERT(DependentVariableComponentsCountFromQuantityType(STR("vector_3")) == 3);
    OCRelease(dv2);
    err = NULL;
    DependentVariableRef dv3 = DependentVariableCreateDefault(STR("pixel_2"), kOCNumberFloat32Type, 2, &err);
    if (!dv3 && err) {
        printf("DVCreateDefault(pixel_2) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv3);
    TEST_ASSERT(DependentVariableIsPixelType(dv3, &count) && count == 2);
    OCRelease(dv3);
    err = NULL;
    DependentVariableRef dv4 = DependentVariableCreateDefault(STR("matrix_2_2"), kOCNumberFloat32Type, 4, &err);
    if (!dv4 && err) {
        printf("DVCreateDefault(matrix_2_2) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv4);
    TEST_ASSERT(DependentVariableIsMatrixType(dv4, &r, &c) && r == 2 && c == 2);
    OCRelease(dv4);
    ok = true;
cleanup:
    OCRelease(err);
    printf("DependentVariable type-queries tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_complexCopy(void) {
    bool ok = false;
    DependentVariableRef src = NULL, dst = NULL;
    OCMutableArrayRef comps = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableDataRef blob = OCDataCreateMutable(0);
    OCStringRef err = NULL;
    OCDataSetLength(blob, 3 * sizeof(double));
    OCArrayAppendValue(comps, blob);
    OCRelease(blob);
    src = DependentVariableCreate(
        STR("dv"), STR("desc"),
        SIUnitDimensionlessAndUnderived(),
        STR("dimensionless"), STR("scalar"),
        kOCNumberFloat64Type,
        NULL, comps,
        &err);
    if (!src && err) {
        printf("DependentVariableCreate(src) failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(src);
    dst = DependentVariableCreateComplexCopy(src, NULL);
    TEST_ASSERT(dst);
    TEST_ASSERT(DependentVariableGetNumericType(dst) == kOCNumberComplex128Type);
    ok = true;
cleanup:
    if (dst) OCRelease(dst);
    if (src) OCRelease(src);
    if (comps) OCRelease(comps);
    OCRelease(err);
    printf("DependentVariable complex-copy tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_invalidCreate(void) {
    bool ok = false;
    OCMutableArrayRef oneBuf = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCMutableDataRef b = OCDataCreateMutable(0);
    OCStringRef err = NULL;
    OCDataSetLength(b, 10);
    OCArrayAppendValue(oneBuf, b);
    OCRelease(b);
    DependentVariableRef dv = DependentVariableCreate(
        STR("x"),
        STR("desc"),
        SIUnitDimensionlessAndUnderived(),
        STR("dimensionless"),
        STR("vector_2"),
        kOCNumberFloat32Type,
        NULL,
        oneBuf,
        &err);
    if (dv == NULL && err) {
        printf("Expected invalid-create error: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv == NULL);
    ok = true;
cleanup:
    OCRelease(oneBuf);
    OCRelease(err);
    printf("DependentVariable invalid-create tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_operations(void) {
    bool ok = false;
    DependentVariableRef dv1 = NULL, dv2 = NULL;
    OCMutableArrayRef components1 = NULL, components2 = NULL;
    OCMutableDataRef comp1 = NULL, comp2 = NULL;
    OCDataRef result = NULL;
    SIUnitRef unit = NULL;
    OCStringRef err = NULL;
    const float *resultData = NULL;
    float *data1Ptr = NULL, *data2Ptr = NULL;
    // Create test data
    float data1[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float data2[] = {0.5f, 1.0f, 1.5f, 2.0f};
    // Create unit
    unit = SIUnitDimensionlessAndUnderived();
    TEST_ASSERT(unit != NULL);
    // Create data components
    comp1 = OCDataCreateMutable(0);
    comp2 = OCDataCreateMutable(0);
    TEST_ASSERT(comp1 != NULL && comp2 != NULL);
    // Set length and copy data
    OCDataSetLength(comp1, sizeof(data1));
    OCDataSetLength(comp2, sizeof(data2));
    data1Ptr = (float *)OCDataGetMutableBytes(comp1);
    data2Ptr = (float *)OCDataGetMutableBytes(comp2);
    memcpy(data1Ptr, data1, sizeof(data1));
    memcpy(data2Ptr, data2, sizeof(data2));
    // Create component arrays
    components1 = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    components2 = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(components1, comp1);
    OCArrayAppendValue(components2, comp2);
    // Create first DependentVariable
    dv1 = DependentVariableCreateMinimal(
        unit,
        NULL,  // NULL quantityName for test - arbitrary names not supported
        STR("scalar"),
        kOCNumberFloat32Type,
        components1,
        &err);
    if (!dv1 && err) {
        printf("DependentVariable create failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv1 != NULL);
    // Create second DependentVariable
    dv2 = DependentVariableCreateMinimal(
        unit,
        NULL,  // NULL quantityName for test - arbitrary names not supported
        STR("scalar"),
        kOCNumberFloat32Type,
        components2,
        &err);
    if (!dv2 && err) {
        printf("DependentVariable create failed: %s\n", OCStringGetCString(err));
    }
    TEST_ASSERT(dv2 != NULL);
    // Test addition: dv1 = dv1 + dv2
    TEST_ASSERT(DependentVariableAdd(dv1, dv2, &err));
    if (err) {
        printf("Addition error: %s\n", OCStringGetCString(err));
        goto cleanup;
    }
    // Check addition results (should be [1.5, 3.0, 4.5, 6.0])
    result = DependentVariableGetComponentAtIndex(dv1, 0);
    TEST_ASSERT(result != NULL);
    resultData = (const float *)OCDataGetBytesPtr(result);
    TEST_ASSERT(fabs(resultData[0] - 1.5f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[1] - 3.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[2] - 4.5f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[3] - 6.0f) < 1e-6f);
    // Test subtraction: dv1 = dv1 - dv2 (should restore original values)
    TEST_ASSERT(DependentVariableSubtract(dv1, dv2, &err));
    if (err) {
        printf("Subtraction error: %s\n", OCStringGetCString(err));
        goto cleanup;
    }
    // Check subtraction results (should be back to [1.0, 2.0, 3.0, 4.0])
    result = DependentVariableGetComponentAtIndex(dv1, 0);
    TEST_ASSERT(result != NULL);
    resultData = (const float *)OCDataGetBytesPtr(result);
    TEST_ASSERT(fabs(resultData[0] - 1.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[1] - 2.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[2] - 3.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[3] - 4.0f) < 1e-6f);
    // Test multiplication: dv1 = dv1 * dv2
    TEST_ASSERT(DependentVariableMultiply(dv1, dv2, &err));
    if (err) {
        printf("Multiplication error: %s\n", OCStringGetCString(err));
        goto cleanup;
    }
    // Check multiplication results (should be [0.5, 2.0, 4.5, 8.0])
    result = DependentVariableGetComponentAtIndex(dv1, 0);
    TEST_ASSERT(result != NULL);
    resultData = (const float *)OCDataGetBytesPtr(result);
    TEST_ASSERT(fabs(resultData[0] - 0.5f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[1] - 2.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[2] - 4.5f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[3] - 8.0f) < 1e-6f);
    // Test division: dv1 = dv1 / dv2 (should restore original values)
    TEST_ASSERT(DependentVariableDivide(dv1, dv2, &err));
    if (err) {
        printf("Division error: %s\n", OCStringGetCString(err));
        goto cleanup;
    }
    // Check division results (should be back to [1.0, 2.0, 3.0, 4.0])
    result = DependentVariableGetComponentAtIndex(dv1, 0);
    TEST_ASSERT(result != NULL);
    resultData = (const float *)OCDataGetBytesPtr(result);
    TEST_ASSERT(fabs(resultData[0] - 1.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[1] - 2.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[2] - 3.0f) < 1e-6f);
    TEST_ASSERT(fabs(resultData[3] - 4.0f) < 1e-6f);
    ok = true;
cleanup:
    if (dv1) OCRelease(dv1);
    if (dv2) OCRelease(dv2);
    if (components1) OCRelease(components1);
    if (components2) OCRelease(components2);
    if (comp1) OCRelease(comp1);
    if (comp2) OCRelease(comp2);
    OCRelease(err);
    printf("DependentVariable arithmetic operations tests %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
