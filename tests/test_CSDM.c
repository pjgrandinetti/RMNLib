// tests/test_CSDM.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "RMNLibrary.h"
#include "test_utils.h"

/// Helper: is this path under the “Illegal file format” tree?
static bool is_illegal(const char *relpath) {
    return strstr(relpath, "Illegal file format") != NULL;
}

/// Helper: does filename end with one of our CSDM extensions?
static bool has_csdf_ext(const char *name) {
    size_t L = strlen(name);
    return
        (L > 5  && strcmp(name + L - 5,  ".csdf")  == 0) ||
        (L > 6  && strcmp(name + L - 6, ".csdfe") == 0);
}

/// Attempt import of exactly one JSON (relpath) under test root.
/// Prints exactly one line per file: either [PASS] or [FAIL],
/// and includes the failure message inline.
static bool import_and_check(const char *root,
                             const char *relpath)
{
    bool expect_failure = is_illegal(relpath);
    char json_path[PATH_MAX];
    char bin_dir[PATH_MAX];
    snprintf(json_path, sizeof(json_path), "%s/%s", root, relpath);

    // binary dir is dirname(json_path)
    strncpy(bin_dir, json_path, sizeof(bin_dir));
    bin_dir[sizeof(bin_dir)-1] = '\0';
    char *slash = strrchr(bin_dir, '/');
    if (slash) *slash = '\0';

    OCStringRef err = NULL;
    DatasetRef ds = DatasetCreateWithImport(json_path, bin_dir, &err);
    const char *errmsg = err ? OCStringGetCString(err) : "";

    if (expect_failure) {
        if (ds) {
            printf("[FAIL] %-60s : expected to fail but succeeded\n", relpath);
            OCRelease(ds);
            OCRelease(err);
            return false;
        } else {
            printf("[PASS] %-60s : correctly failed (%s)\n", relpath, errmsg);
            OCRelease(err);
            return true;
        }
    } else {
        if (!ds) {
            printf("[FAIL] %-60s : expected to succeed but failed (%s)\n",
                   relpath, errmsg);
            OCRelease(err);
            return false;
        } else {
            printf("[PASS] %-60s : import succeeded\n", relpath);
            OCRelease(ds);
            OCRelease(err);
            return true;
        }
    }
}

/// A simple linked list to collect rel-paths of all .csdf/.csdfe files.
struct file_node {
    char rel[PATH_MAX];
    struct file_node *next;
};
static struct file_node *file_list_head = NULL;

/// Add one rel-path to our list
static void add_file(const char *rel) {
    struct file_node *n = malloc(sizeof(*n));
    if (!n) return;
    strncpy(n->rel, rel, sizeof(n->rel));
    n->rel[sizeof(n->rel)-1] = '\0';
    n->next = file_list_head;
    file_list_head = n;
}

/// Recurse under <root>/<cur_rel> and collect every .csdf/.csdfe
static void collect_files(const char *root, const char *cur_rel) {
    char full[PATH_MAX];
    if (cur_rel[0] == '\0')
        snprintf(full, sizeof(full), "%s", root);
    else
        snprintf(full, sizeof(full), "%s/%s", root, cur_rel);

    struct stat st;
    if (stat(full, &st) != 0) return;

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(full);
        if (!d) return;
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            const char *nm = de->d_name;
            if (strcmp(nm, ".")==0 || strcmp(nm, "..")==0) continue;
            char next_rel[PATH_MAX];
            if (cur_rel[0]=='\0')
                snprintf(next_rel, sizeof(next_rel), "%s", nm);
            else
                snprintf(next_rel, sizeof(next_rel), "%s/%s", cur_rel, nm);
            collect_files(root, next_rel);
        }
        closedir(d);
    }
    else if (S_ISREG(st.st_mode)) {
        if (has_csdf_ext(cur_rel)) {
            add_file(cur_rel);
        }
    }
}

bool test_Dataset_import_all_csdm(void) {
    printf("test_Dataset_import_all_csdm...\n");
    const char *root = getenv("CSDM_TEST_ROOT");
    TEST_ASSERT(root != NULL);

    // build the file list
    collect_files(root, "");

    int total = 0, failed = 0;

    // run import & check on each
    for (struct file_node *n = file_list_head; n; n = n->next) {
        total++;
        if (!import_and_check(root, n->rel)) {
            failed++;
        }
    }

    // free list
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }

    printf("\nSummary: tested %d files, %d passed, %d failed\n",
           total, total - failed, failed);

    TEST_ASSERT(failed == 0);
    printf("test_Dataset_import_all_csdm passed.\n");
    return true;

cleanup:
    // free list on assertion failure
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }
    return false;
}

bool test_Dataset_import_single_csdm(void) {
    printf("test_Dataset_import_single_csdm...\n");
    const char *root = getenv("CSDM_TEST_ROOT");
    TEST_ASSERT(root != NULL);

    // Test one of the failing files for debugging
    // sparse/iglu_2d.csdf
    // correlatedDataset/0D_dataset/J_vs_s.csdf
    // TEM/TEM.csdf


    const char *test_file = "sparse/iglu_2d.csdf";
    
    printf("Testing single failing file: %s\n", test_file);
    
    bool success = import_and_check(root, test_file);
    
    if (success) {
        printf("test_Dataset_import_single_csdm passed.\n");
        return true;
    } else {
        printf("test_Dataset_import_single_csdm failed.\n");
        return false;
    }

cleanup:
    return false;
}

bool test_Dataset_import_and_roundtrip(void) {
    printf("test_Dataset_import_and_roundtrip...\n");
    const char *root = getenv("CSDM_TEST_ROOT");
    TEST_ASSERT(root != NULL);

    collect_files(root, "");

    int total = 0, import_failed = 0, roundtrip_failed = 0;

    // Create temporary export directory
    const char *local_tmp = "tmp/rmnpy_roundtrip";
    mkdir("tmp", 0777);
    mkdir(local_tmp, 0777);
    char tmpdir[PATH_MAX];
    snprintf(tmpdir, sizeof(tmpdir), "%s", local_tmp);
    printf("Export dir: %s\n", tmpdir);

    for (struct file_node *n = file_list_head; n; n = n->next) {
        total++;

        // Construct full path to original JSON
        char orig_json[PATH_MAX];
        snprintf(orig_json, sizeof(orig_json), "%s/%s", root, n->rel);

        // Extract binary directory
        char orig_bin_dir[PATH_MAX];
        strncpy(orig_bin_dir, orig_json, sizeof(orig_bin_dir));
        orig_bin_dir[sizeof(orig_bin_dir) - 1] = '\0';
        char *slash = strrchr(orig_bin_dir, '/');
        if (slash) *slash = '\0';
        else strncpy(orig_bin_dir, root, sizeof(orig_bin_dir));

        // Skip illegal files for roundtrip; only test import
        if (is_illegal(n->rel)) {
            OCStringRef err = NULL;
            DatasetRef ds = DatasetCreateWithImport(orig_json, orig_bin_dir, &err);
            if (ds) {
                printf("[FAIL] %-60s : illegal file imported unexpectedly\n", n->rel);
                OCRelease(ds);
                OCRelease(err);
                import_failed++;
            } else {
                printf("[PASS] %-60s : correctly failed to import illegal file (%s)\n",
                       n->rel, err ? OCStringGetCString(err) : "");
                OCRelease(err);
            }
            continue;
        }

        // Import test (part of original function 1)
        OCStringRef err = NULL;
        DatasetRef ds = DatasetCreateWithImport(orig_json, orig_bin_dir, &err);
        if (!ds) {
            printf("[FAIL] %-60s : import failed (%s)\n", n->rel, err ? OCStringGetCString(err) : "");
            OCRelease(err);
            import_failed++;
            continue;
        }
        OCRelease(err);

        // Determine export extension (.csdf vs .csdfe)
        bool hasExternal = false;
        OCArrayRef dvs = DatasetGetDependentVariables(ds);
        OCIndex dvCount = dvs ? OCArrayGetCount(dvs) : 0;
        for (OCIndex i = 0; i < dvCount; ++i) {
            DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvs, i);
            if (dv && DependentVariableShouldSerializeExternally(dv)) {
                hasExternal = true;
                break;
            }
        }

        // Create safe output filename
        char safe_name[PATH_MAX];
        strncpy(safe_name, n->rel, sizeof(safe_name));
        safe_name[sizeof(safe_name) - 1] = '\0';
        for (char *p = safe_name; *p; ++p) {
            if (*p == '/') *p = '_';
        }

        char export_name[PATH_MAX];
        strncpy(export_name, safe_name, sizeof(export_name));
        export_name[sizeof(export_name) - 1] = '\0';
        char *dot = strrchr(export_name, '.');
        if (dot) {
            strcpy(dot, hasExternal ? ".csdfe" : ".csdf");
        } else {
            strcat(export_name, hasExternal ? ".csdfe" : ".csdf");
        }

        char export_json[PATH_MAX];
        snprintf(export_json, sizeof(export_json), "%s/%s", tmpdir, export_name);

        // Export
        err = NULL;
        bool export_ok = DatasetExport(ds, export_json, tmpdir, &err);
        if (!export_ok) {
            printf("[FAIL] %-60s : export failed (%s)\n", n->rel, err ? OCStringGetCString(err) : "");
            OCRelease(ds);
            OCRelease(err);
            roundtrip_failed++;
            continue;
        }
        OCRelease(err);

        // Re-import
        err = NULL;
        DatasetRef ds2 = DatasetCreateWithImport(export_json, tmpdir, &err);
        if (!ds2) {
            printf("[FAIL] %-60s : re-import failed (%s)\n", n->rel, err ? OCStringGetCString(err) : "");
            OCRelease(ds);
            OCRelease(err);
            roundtrip_failed++;
            continue;
        }
        OCRelease(err);

        // (Optional) Compare ds and ds2 here

        printf("[PASS] %-60s : import + roundtrip succeeded\n", n->rel);
        OCRelease(ds);
        OCRelease(ds2);
    }

    // Summary
    printf("\nSummary:\n");
    printf("  Total files tested     : %d\n", total);
    printf("  Import failures        : %d\n", import_failed);
    printf("  Roundtrip failures     : %d\n", roundtrip_failed);
    printf("  Total successful cases : %d\n", total - import_failed - roundtrip_failed);

    // Clean up file list
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }

    TEST_ASSERT(import_failed == 0 && roundtrip_failed == 0);
    printf("test_Dataset_import_and_roundtrip passed.\n");
    return true;

cleanup:
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }
    return false;
}