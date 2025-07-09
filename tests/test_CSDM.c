// tests/test_CSDM.c

#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
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
/// bin_dir is the directory part of that file. Returns true if the
/// result matches the expectation (success in “good” dirs, failure
/// under “Illegal file format”).
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

    if (expect_failure) {
        if (ds) {
            fprintf(stderr,
                    "[FAIL] Expected import of \"%s\" to fail, but it succeeded.\n",
                    relpath);
            OCRelease(ds);
            OCRelease(err);
            return false;
        }
        TEST_ASSERT(err != NULL);
    } else {
        if (!ds) {
            fprintf(stderr,
                    "[FAIL] Expected import of \"%s\" to succeed, but it failed.\n",
                    relpath);
            if (err)
                fprintf(stderr, "       Reason: %s\n", OCStringGetCString(err));
            OCRelease(err);
            return false;
        }
        TEST_ASSERT(err == NULL);

        // sanity-check that we got at least one DV
        OCArrayRef dvs = DatasetGetDependentVariables(ds);
        TEST_ASSERT(dvs && OCArrayGetCount(dvs) > 0);

        OCRelease(ds);
    }

    OCRelease(err);
    return true;

cleanup:
    // for TEST_ASSERT failures we land here
    if (ds) OCRelease(ds);
    if (err) OCRelease(err);
    return false;
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
    bool ok = true;

    const char *root = getenv("CSDM_TEST_ROOT");
    TEST_ASSERT(root != NULL);

    // build the file list
    collect_files(root, "");

    // run import & check on each
    for (struct file_node *n = file_list_head; n; n = n->next) {
        ok &= import_and_check(root, n->rel);
    }

    // free list
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }

    TEST_ASSERT(ok);

    printf("test_Dataset_import_all_csdm %s.\n",
           ok ? "passed" : "FAILED");
    return ok;

cleanup:
    // on TEST_ASSERT failure
    while (file_list_head) {
        struct file_node *next = file_list_head->next;
        free(file_list_head);
        file_list_head = next;
    }
    return false;
}
