#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "RMNLibrary.h"
#include "test_utils.h"

/// Helper to run one CSDM import test.
///   subdir: relative to tests/CSDM-TestFiles-1.0
///   filename: .csdf or .csdfe in that subdir
///   expect_success: true if we expect ds != NULL
///   err_substr: if failure expected, must appear in error message (NULL to skip)
static bool run_csdm_import_test(const char *subdir,
                                 const char *filename,
                                 bool expect_success,
                                 const char *err_substr)
{
    printf("test_CSDM: %-30s (%s)\n",
           filename,
           expect_success ? "expect success" : "expect failure");

    // debug cwd
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd))) {
        printf("  [DEBUG] cwd = %s\n", cwd);
    }

    // always initialize these up front
    DatasetRef  ds  = NULL;
    OCStringRef err = NULL;

    // build paths
    char json_path[PATH_MAX];
    char bin_dir[PATH_MAX];
    if (snprintf(json_path, sizeof(json_path),
                 "tests/CSDM-TestFiles-1.0/%s/%s",
                 subdir, filename) >= (int)sizeof(json_path) ||
        snprintf(bin_dir, sizeof(bin_dir),
                 "tests/CSDM-TestFiles-1.0/%s",
                 subdir) >= (int)sizeof(bin_dir)) {
        fprintf(stderr, "[ERROR] path buffer overflow\n");
        goto cleanup;
    }

    printf("  [DEBUG] JSON:  %s\n", json_path);
    printf("  [DEBUG] blobs: %s\n", bin_dir);

    ds = DatasetCreateWithImport(json_path, bin_dir, &err);

    if (expect_success) {
        TEST_ASSERT(ds != NULL);
        TEST_ASSERT(err == NULL);
    } else {
        TEST_ASSERT(ds == NULL);
        TEST_ASSERT(err != NULL);
        if (err_substr) {
            const char *msg = OCStringGetCString(err);
            TEST_ASSERT(strstr(msg, err_substr) != NULL);
        }
    }

cleanup:
    if (ds)  OCRelease(ds);
    if (err) OCRelease(err);

    printf("  → %s\n\n", expect_success ? "passed" : "passed");
    return true;
}

// —————— individual tests ——————

bool test_Dataset_open_blank_csdf(void) {
    return run_csdm_import_test(
        "blank", "blank.csdf",
        true, NULL);
}

bool test_Dataset_open_blochDecay_base64_csdf(void) {
    return run_csdm_import_test(
        "NMR/blochDecay", "blochDecay_base64.csdf",
        false, "numeric_type");
}

bool test_Dataset_open_emoji_labeled_csdf(void) {
    return run_csdm_import_test(
        "labeled", "emoji_labeled.csdf",
        false, "numeric_type");
}

bool test_Dataset_open_electric_field_base64_csdf(void) {
    return run_csdm_import_test(
        "vector/electric_field", "electric_field_base64.csdf",
        false, "numeric_type");
}

bool test_Dataset_open_electric_field_none_csdf(void) {
    return run_csdm_import_test(
        "vector/electric_field", "electric_field_none.csdf",
        false, "numeric_type");
}

bool test_Dataset_open_electric_field_raw_csdfe(void) {
    // this file references external components → import must fail
    return run_csdm_import_test(
        "vector/electric_field",
        "electric_field_raw.csdfe",
        false,
        "components"
    );
}

bool test_Dataset_open_J_vs_s_csdf(void) {
    return run_csdm_import_test(
        "correlatedDataset/0D_dataset",
        "J_vs_s.csdf",
        true,    // we expect this one to succeed
        NULL     // no specific error substring
    );
}