#ifndef TEST_DIMENSION_JSON_H
#define TEST_DIMENSION_JSON_H
#include <stdbool.h>
// Basic JSON round-trip tests for each dimension type
bool test_Dimension_JSON_roundtrip(void);
bool test_LabeledDimension_JSON_roundtrip(void);
bool test_SIDimension_JSON_roundtrip(void);
bool test_SIMonotonicDimension_JSON_roundtrip(void);
bool test_SILinearDimension_JSON_roundtrip(void);
// Test typed vs untyped JSON formats
bool test_Dimension_JSON_typed_vs_untyped(void);
// Test application metadata always uses typed=true
bool test_Dimension_JSON_application_metadata_always_typed(void);
// Test JSON error handling
bool test_Dimension_JSON_error_handling(void);
// Test complex scenarios with inheritance
bool test_Dimension_JSON_inheritance_patterns(void);
#endif  // TEST_DIMENSION_JSON_H
