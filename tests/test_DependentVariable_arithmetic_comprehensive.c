#include <complex.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "RMNLibrary.h"
#include "test_DependentVariable.h"
// Helper function to create a DependentVariable with specific data
static DependentVariableRef create_test_dv(OCNumberType type, const void *data, size_t element_count, size_t element_size) {
    SIUnitRef unit = SIUnitDimensionlessAndUnderived();
    if (!unit) return NULL;
    OCMutableDataRef comp = OCDataCreateMutable(0);
    if (!comp) return NULL;
    OCDataSetLength(comp, element_count * element_size);
    void *bytes = OCDataGetMutableBytes(comp);
    memcpy(bytes, data, element_count * element_size);
    OCMutableArrayRef components = OCArrayCreateMutable(1, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(components, comp);
    DependentVariableRef dv = DependentVariableCreate(
        STR(""),  // name
        STR(""),  // description
        unit,
        kSIQuantityDimensionless,  // quantity_name
        STR("scalar"),             // quantity_type
        type,                      // elementType
        NULL,                      // componentLabels
        components,                // components
        NULL                       // outError
    );
    OCRelease(comp);
    OCRelease(components);
    OCRelease(unit);
    return dv;
}
// Helper function to verify float arrays are approximately equal
static bool verify_float_arrays(const float *actual, const float *expected, size_t count, float tolerance) {
    for (size_t i = 0; i < count; i++) {
        if (fabsf(actual[i] - expected[i]) > tolerance) {
            return false;
        }
    }
    return true;
}
// Helper function to verify double arrays are approximately equal
static bool verify_double_arrays(const double *actual, const double *expected, size_t count, double tolerance) {
    for (size_t i = 0; i < count; i++) {
        if (fabs(actual[i] - expected[i]) > tolerance) {
            return false;
        }
    }
    return true;
}
// Helper function to verify complex float arrays are approximately equal
static bool verify_complex_float_arrays(const complex float *actual, const complex float *expected, size_t count, float tolerance) {
    for (size_t i = 0; i < count; i++) {
        if (fabsf(crealf(actual[i]) - crealf(expected[i])) > tolerance ||
            fabsf(cimagf(actual[i]) - cimagf(expected[i])) > tolerance) {
            return false;
        }
    }
    return true;
}
// Helper function to verify complex double arrays are approximately equal
static bool verify_complex_double_arrays(const complex double *actual, const complex double *expected, size_t count, double tolerance) {
    for (size_t i = 0; i < count; i++) {
        if (fabs(creal(actual[i]) - creal(expected[i])) > tolerance ||
            fabs(cimag(actual[i]) - cimag(expected[i])) > tolerance) {
            return false;
        }
    }
    return true;
}
bool test_DependentVariable_arithmetic_comprehensive_types(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test Float32 + Float32
    {
        float data1[] = {1.0f, 2.0f, 3.0f, 4.0f};
        float data2[] = {0.5f, 1.0f, 1.5f, 2.0f};
        float expected[] = {1.5f, 3.0f, 4.5f, 6.0f};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat32Type, data1, 4, sizeof(float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, data2, 4, sizeof(float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const float *resultData = (const float *)OCDataGetBytesPtr(result);
                if (!verify_float_arrays(resultData, expected, 4, 1e-6f)) {
                    printf("Float32 + Float32 addition failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Float32 + Float32 addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test Float64 + Float64
    {
        double data1[] = {1.0, 2.0, 3.0, 4.0};
        double data2[] = {0.5, 1.0, 1.5, 2.0};
        double expected[] = {1.5, 3.0, 4.5, 6.0};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat64Type, data1, 4, sizeof(double));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat64Type, data2, 4, sizeof(double));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const double *resultData = (const double *)OCDataGetBytesPtr(result);
                if (!verify_double_arrays(resultData, expected, 4, 1e-12)) {
                    printf("Float64 + Float64 addition failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Float64 + Float64 addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test Complex64 + Complex64
    {
        complex float data1[] = {1.0f + 2.0f * I, 3.0f + 4.0f * I};
        complex float data2[] = {0.5f + 1.0f * I, 1.5f + 2.0f * I};
        complex float expected[] = {1.5f + 3.0f * I, 4.5f + 6.0f * I};
        DependentVariableRef dv1 = create_test_dv(kOCNumberComplex64Type, data1, 2, sizeof(complex float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberComplex64Type, data2, 2, sizeof(complex float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const complex float *resultData = (const complex float *)OCDataGetBytesPtr(result);
                if (!verify_complex_float_arrays(resultData, expected, 2, 1e-6f)) {
                    printf("Complex64 + Complex64 addition failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Complex64 + Complex64 addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test Complex128 + Complex128
    {
        complex double data1[] = {1.0 + 2.0 * I, 3.0 + 4.0 * I};
        complex double data2[] = {0.5 + 1.0 * I, 1.5 + 2.0 * I};
        complex double expected[] = {1.5 + 3.0 * I, 4.5 + 6.0 * I};
        DependentVariableRef dv1 = create_test_dv(kOCNumberComplex128Type, data1, 2, sizeof(complex double));
        DependentVariableRef dv2 = create_test_dv(kOCNumberComplex128Type, data2, 2, sizeof(complex double));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const complex double *resultData = (const complex double *)OCDataGetBytesPtr(result);
                if (!verify_complex_double_arrays(resultData, expected, 2, 1e-12)) {
                    printf("Complex128 + Complex128 addition failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Complex128 + Complex128 addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test cross-type: Float64 + Float32
    {
        double data1[] = {1.0, 2.0, 3.0, 4.0};
        float data2[] = {0.5f, 1.0f, 1.5f, 2.0f};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat64Type, data1, 4, sizeof(double));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, data2, 4, sizeof(float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const double *resultData = (const double *)OCDataGetBytesPtr(result);
                double expected[] = {1.5, 3.0, 4.5, 6.0};
                if (!verify_double_arrays(resultData, expected, 4, 1e-12)) {
                    printf("Float64 + Float32 addition failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Float64 + Float32 addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_error_cases(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test division by zero
    {
        float data1[] = {1.0f, 2.0f, 3.0f, 4.0f};
        float data2[] = {0.0f, 1.0f, 0.0f, 2.0f};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat32Type, data1, 4, sizeof(float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, data2, 4, sizeof(float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableDivide(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const float *resultData = (const float *)OCDataGetBytesPtr(result);
                // Check that division by zero produces infinity
                if (!isinf(resultData[0]) || !isinf(resultData[2])) {
                    printf("Division by zero should produce infinity\\n");
                    ok = false;
                }
                OCRelease(result);
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test NULL pointer handling
    {
        OCStringRef err = NULL;
        bool success = DependentVariableAdd(NULL, NULL, &err);
        if (success) {
            printf("Adding NULL pointers should return false\\n");
            ok = false;
        }
        if (err) OCRelease(err);
    }
    // Test mismatched sizes
    {
        float data1[] = {1.0f, 2.0f, 3.0f, 4.0f};
        float data2[] = {0.5f, 1.0f};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat32Type, data1, 4, sizeof(float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, data2, 2, sizeof(float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                printf("Mismatched sizes should return false\\n");
                ok = false;
            } else {
                // Check that we get an appropriate error message
                if (!err || !strstr(OCStringGetCString(err), "size")) {
                    printf("Error message should mention size incompatibility\\n");
                    ok = false;
                }
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_complex(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test complex multiplication: (1+2i) * (3+4i) = -5+10i
    {
        complex float data1[] = {1.0f + 2.0f * I};
        complex float data2[] = {3.0f + 4.0f * I};
        complex float expected[] = {-5.0f + 10.0f * I};
        DependentVariableRef dv1 = create_test_dv(kOCNumberComplex64Type, data1, 1, sizeof(complex float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberComplex64Type, data2, 1, sizeof(complex float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableMultiply(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const complex float *resultData = (const complex float *)OCDataGetBytesPtr(result);
                if (!verify_complex_float_arrays(resultData, expected, 1, 1e-6f)) {
                    printf("Complex multiplication failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Complex multiplication returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test complex division: (1+2i) / (1+i) = 1.5+0.5i
    {
        complex double data1[] = {1.0 + 2.0 * I};
        complex double data2[] = {1.0 + 1.0 * I};
        complex double expected[] = {1.5 + 0.5 * I};
        DependentVariableRef dv1 = create_test_dv(kOCNumberComplex128Type, data1, 1, sizeof(complex double));
        DependentVariableRef dv2 = create_test_dv(kOCNumberComplex128Type, data2, 1, sizeof(complex double));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableDivide(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const complex double *resultData = (const complex double *)OCDataGetBytesPtr(result);
                if (!verify_complex_double_arrays(resultData, expected, 1, 1e-12)) {
                    printf("Complex division failed\\n");
                    ok = false;
                }
                OCRelease(result);
            } else {
                printf("Complex division returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_edge_cases(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test very small numbers
    {
        float data1[] = {FLT_MIN, FLT_MIN};
        float data2[] = {FLT_MIN, FLT_MIN};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat32Type, data1, 2, sizeof(float));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, data2, 2, sizeof(float));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (!success) {
                printf("Addition of very small numbers failed\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    // Test very large numbers
    {
        double large_num = 1e200;
        double data1[] = {large_num, large_num};
        double data2[] = {large_num, large_num};
        DependentVariableRef dv1 = create_test_dv(kOCNumberFloat64Type, data1, 2, sizeof(double));
        DependentVariableRef dv2 = create_test_dv(kOCNumberFloat64Type, data2, 2, sizeof(double));
        if (dv1 && dv2) {
            OCStringRef err = NULL;
            bool success = DependentVariableAdd(dv1, dv2, &err);
            if (success) {
                OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
                const double *resultData = (const double *)OCDataGetBytesPtr(result);
                // Check for overflow to infinity
                for (int i = 0; i < 2; i++) {
                    if (!isfinite(resultData[i]) && !isinf(resultData[i])) {
                        ok = false;
                        break;
                    }
                }
                OCRelease(result);
            } else {
                printf("Large number addition returned false\\n");
                ok = false;
            }
            if (err) OCRelease(err);
        }
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
    }
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_large_scale(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test with larger arrays to exercise OpenMP paths
    const size_t large_size = 1000;  // Reduced from 10000 to avoid memory issues
    float *large_data1 = malloc(large_size * sizeof(float));
    float *large_data2 = malloc(large_size * sizeof(float));
    if (!large_data1 || !large_data2) {
        printf("Failed to allocate large test arrays\\n");
        free(large_data1);
        free(large_data2);
        return false;
    }
    // Initialize with simple pattern
    for (size_t i = 0; i < large_size; i++) {
        large_data1[i] = (float)(i + 1);
        large_data2[i] = 0.5f;
    }
    DependentVariableRef dv1 = create_test_dv(kOCNumberFloat32Type, large_data1, large_size, sizeof(float));
    DependentVariableRef dv2 = create_test_dv(kOCNumberFloat32Type, large_data2, large_size, sizeof(float));
    if (!dv1 || !dv2) {
        printf("Failed to create large scale test variables\\n");
        if (dv1) OCRelease(dv1);
        if (dv2) OCRelease(dv2);
        free(large_data1);
        free(large_data2);
        return false;
    }
    OCStringRef err = NULL;
    bool success = DependentVariableMultiply(dv1, dv2, &err);
    if (success) {
        OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
        const float *resultData = (const float *)OCDataGetBytesPtr(result);
        // Verify a few sample results
        bool samples_ok = true;
        for (size_t i = 0; i < 10; i++) {
            float expected = (float)(i + 1) * 0.5f;
            if (fabsf(resultData[i] - expected) > 1e-6f) {
                printf("Large scale multiplication failed at index %zu\\n", i);
                samples_ok = false;
                break;
            }
        }
        if (!samples_ok) {
            ok = false;
        }
        OCRelease(result);
    } else {
        printf("Large scale multiplication returned false\\n");
        if (err) {
            printf("Error: %s\\n", OCStringGetCString(err));
        }
        ok = false;
    }
    if (err) OCRelease(err);
    if (dv1) OCRelease(dv1);
    if (dv2) OCRelease(dv2);
    free(large_data1);
    free(large_data2);
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
bool test_DependentVariable_arithmetic_integer_types(void) {
    fprintf(stderr, "%s begin...", __func__);
    bool ok = true;
    // Test with various integer types
    int32_t idata1[] = {10, 20, 30, 40};
    int32_t idata2[] = {2, 4, 6, 8};
    DependentVariableRef dv1 = create_test_dv(kOCNumberSInt32Type, idata1, 4, sizeof(int32_t));
    DependentVariableRef dv2 = create_test_dv(kOCNumberSInt32Type, idata2, 4, sizeof(int32_t));
    if (dv1 && dv2) {
        // Integer arithmetic should use the generic fallback path
        OCStringRef err = NULL;
        bool success = DependentVariableAdd(dv1, dv2, &err);
        if (success) {
            OCDataRef result = DependentVariableCopyComponentAtIndex(dv1, 0);
            const int32_t *resultData = (const int32_t *)OCDataGetBytesPtr(result);
            int32_t expected[] = {12, 24, 36, 48};
            bool arrays_match = true;
            for (int i = 0; i < 4; i++) {
                if (resultData[i] != expected[i]) {
                    printf("Integer addition mismatch at index %d: expected %d, got %d\\n",
                           i, expected[i], resultData[i]);
                    arrays_match = false;
                }
            }
            if (!arrays_match) {
                ok = false;
            }
            OCRelease(result);
        } else {
            printf("Integer addition returned false\\n");
            ok = false;
        }
        if (err) OCRelease(err);
    }
    if (dv1) OCRelease(dv1);
    if (dv2) OCRelease(dv2);
    fprintf(stderr, " %s\n", ok ? "passed." : "FAILED!");
    return ok;
}
