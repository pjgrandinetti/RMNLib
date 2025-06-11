// tests/test_utils.h
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdbool.h>

/// Abort the current test and jump to cleanup block.
#define TEST_ASSERT(cond) do {                                       \
    if (!(cond)) {                                                   \
        fprintf(stderr,                                              \
                "Assertion failed: %s, function %s, line %d.\n",     \
                #cond, __func__, __LINE__);                          \
        goto cleanup;                                                \
    }                                                                \
} while (0)


// PRINTERROR macro for consistent error reporting in tests
// Added __FILE__ to identify which test file failed.
#define PRINTERROR do { \
    fprintf(stderr, "Error: Test failed in function %s, file %s, line %d\n", __FUNCTION__, __FILE__, __LINE__); \
    return false; \
} while (0)

// Assertion Macros
#define ASSERT_TRUE(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "ASSERTION FAILED: (%s) - '%s' in %s, file %s, line %d\n", #condition, message, __FUNCTION__, __FILE__, __LINE__); \
        PRINTERROR; \
    } \
} while (0)

#define ASSERT_FALSE(condition, message) do { \
    if (condition) { \
        fprintf(stderr, "ASSERTION FAILED: (!%s) - '%s' in %s, file %s, line %d\n", #condition, message, __FUNCTION__, __FILE__, __LINE__); \
        PRINTERROR; \
    } \
} while (0)

#define ASSERT_EQUAL(actual, expected, message) do { \
    if ((actual) != (expected)) { \
        /* Note: This generic version won't print actual/expected values well for all types. */ \
        /* For specific types, consider casting to long long for printing if appropriate, or use type-specific checks. */ \
        fprintf(stderr, "ASSERTION FAILED: (%s == %s) - '%s' in %s, file %s, line %d. Values differ.\n", #actual, #expected, message, __FUNCTION__, __FILE__, __LINE__); \
        PRINTERROR; \
    } \
} while (0)

#define ASSERT_NOT_NULL(pointer, message) do { \
    if ((pointer) == NULL) { \
        fprintf(stderr, "ASSERTION FAILED: (%s != NULL) - '%s' in %s, file %s, line %d\n", #pointer, message, __FUNCTION__, __FILE__, __LINE__); \
        PRINTERROR; \
    } \
} while (0)

#define ASSERT_NULL(pointer, message) do { \
    if ((pointer) != NULL) { \
        fprintf(stderr, "ASSERTION FAILED: (%s == NULL) - '%s' in %s, file %s, line %d\n", #pointer, message, __FUNCTION__, __FILE__, __LINE__); \
        PRINTERROR; \
    } \
} while (0)


#endif // TEST_UTILS_H