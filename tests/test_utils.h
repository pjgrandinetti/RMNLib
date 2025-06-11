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


#endif // TEST_UTILS_H