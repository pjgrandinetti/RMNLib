#ifndef TEST_DIMENSION_H
#define TEST_DIMENSION_H

#include <stdbool.h>

bool test_CreateDimensionLongLabel(void);
bool test_Dimension_base(void);
bool test_LabeledDimension(void);
bool test_SIDimension(void);
bool test_SIMonotonic_and_SILinearDimension(void);
bool test_minimal_monotonic(void);

#endif // TEST_DIMENSION_H