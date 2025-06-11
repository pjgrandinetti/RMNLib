// tests/test_utils.h
#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <stdio.h>
#include <stdbool.h>

/// Abort the current test (returning false) and print a failure message.
#define TEST_ASSERT(cond) do {                                      \
    if (!(cond)) {                                                  \
        fprintf(stderr,                                             \
                "Assertion failed: %s, function %s, line %d.\\n",   \
                #cond, __func__, __LINE__);                         \
        return false;                                               \
    }                                                               \
} while (0)

/// Run a single test function, print its name and result, and return 0/1
static inline int run_test(const char *name, bool (*test_fn)(void)) {
    fprintf(stderr, "  %-30s ... ", name);
    if (test_fn()) {
        fprintf(stderr, "OK\n");
        return 0;
    } else {
        fprintf(stderr, "FAIL\n");
        return 1;
    }
}

#endif // TEST_UTILS_H