#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "test_utils.h"

char *resolve_test_path(const char *relative_path) {
    const char *root = getenv("CSDM_TEST_ROOT");
    if (!root) root = ".";

    size_t len = strlen(root) + strlen(relative_path) + 2;
    char *fullpath = (char *)malloc(len);
    if (!fullpath) return NULL;

#if defined(_WIN32)
    snprintf(fullpath, len, "%s\\%s", root, relative_path);
#else
    snprintf(fullpath, len, "%s/%s", root, relative_path);
#endif
    return fullpath;
}
