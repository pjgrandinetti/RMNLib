// tests/test_CSDM.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

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
/// Prints exactly one line per file: either [ PASS] or [FAIL],
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
            printf("[ PASS] %-60s : correctly failed (%s)\n", relpath, errmsg);
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
            printf("[ PASS] %-60s : import succeeded\n", relpath);
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

    // Test a specific file - change this to test different files
    const char *test_file = "image/raccoon_image.csdf";
    
    printf("Testing single file: %s\n", test_file);
    
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
