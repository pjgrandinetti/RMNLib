// tests/test_JCAMP.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "RMNLibrary.h"
#include "test_utils.h"

/// Helper: does filename end with .jdx or .dx (JCAMP extensions)?
static bool has_jcamp_ext(const char *name) {
    size_t L = strlen(name);
    return (L > 4 && strcmp(name + L - 4, ".jdx") == 0) ||
           (L > 3 && strcmp(name + L - 3, ".dx") == 0);
}

/// A simple linked list to collect rel-paths of all JCAMP files.
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

/// Recurse under <root>/<cur_rel> and collect every JCAMP file
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
        if (has_jcamp_ext(cur_rel)) {
            add_file(cur_rel);
        }
    }
}

/// Attempt import of exactly one JCAMP file (relpath) under test root.
static bool
import_jcamp_and_check(const char *root, const char *relpath)
{
    // Build the full path to the JCAMP file
    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", root, relpath);

    // Load the entire file into an OCDataRef
    OCStringRef fileErr = NULL;
    OCDataRef contents = OCDataCreateWithContentsOfFile(file_path, &fileErr);
    if (!contents) {
        printf("[FAIL] %-60s : could not read file\n", relpath);
        if (fileErr && OCStringGetLength(fileErr) > 0) {
            printf("    I/O Error: %s\n", OCStringGetCString(fileErr));
        } else {
            printf("    I/O Error: (no error message)\n");
        }
        OCRelease(fileErr);
        return false;
    }

    // Attempt to import as JCAMP
    OCStringRef err = NULL;
    DatasetRef ds = DatasetImportJCAMPCreateSignalWithData(contents, &err);
    OCRelease(contents);

    if (!ds) {
        if (err && OCStringGetLength(err) > 0 && strcmp(OCStringGetCString(err), "JCAMP Peak Table file is unsupported.") == 0) {
            printf("[SKIP] %-60s : peak table unsupported\n", relpath);
            OCRelease(err);
            return true; // treat as skipped, not failed
        }
        printf("[FAIL] %-60s : import failed\n", relpath);
        if (err && OCStringGetLength(err) > 0) {
            printf("    Error: %s\n", OCStringGetCString(err));
        } else {
            printf("    Error: (no error message)\n");
        }
        OCRelease(err);
        return false;
    }

    // Success
    printf("[PASS] %-60s : import succeeded\n", relpath);
    OCRelease(ds);
    OCRelease(err);
    return true;
}

/// Test specifically the gh37cj.jdx file that had parsing issues
bool test_JCAMP_single_file(void) {
    printf("test_JCAMP_single_file...\n");
    
    // Get test root directory
    const char *root = getenv("JCAMP_TEST_ROOT");
    if (!root) {
        // Fallback to tests/JCAMP if env var not set
        root = "tests/JCAMP";
    }

    // Try to find GC/guava.jdx in the test directory
    const char *test_file = "GC/guava.jdx";
    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", root, test_file);
    
    // Check if file exists
    struct stat st;
    if (stat(file_path, &st) != 0) {
        printf("[SKIP] %-60s : file not found\n", test_file);
        return true; // Skip if file doesn't exist
    }
    
    printf("Testing specific JCAMP file: %s\n", test_file);
    
    // Load the file
    OCStringRef fileErr = NULL;
    OCDataRef contents = OCDataCreateWithContentsOfFile(file_path, &fileErr);
    if (!contents) {
        printf("[FAIL] %-60s : could not read file\n", test_file);
        if (fileErr && OCStringGetLength(fileErr) > 0) {
            printf("    I/O Error: %s\n", OCStringGetCString(fileErr));
        }
        OCRelease(fileErr);
        return false;
    }
    
    // Attempt to import as JCAMP
    OCStringRef err = NULL;
    DatasetRef ds = DatasetImportJCAMPCreateSignalWithData(contents, &err);
    OCRelease(contents);
    
    if (!ds) {
        printf("[FAIL] %-60s : import failed\n", test_file);
        if (err && OCStringGetLength(err) > 0) {
            printf("    Error: %s\n", OCStringGetCString(err));
        } else {
            printf("    Error: (no error message)\n");
        }
        OCRelease(err);
        return false;
    }
    
    // Success - verify we got a valid dataset
    printf("[PASS] %-60s : import succeeded\n", test_file);
    
    // Optional: Print some basic info about the dataset
    OCArrayRef dimensions = DatasetGetDimensions(ds);
    if (dimensions) {
        OCIndex dimCount = OCArrayGetCount(dimensions);
        printf("    Dataset has %lu dimension(s)\n", (unsigned long)dimCount);
        
        if (dimCount > 0) {
            SIDimensionRef firstDim = (SIDimensionRef)OCArrayGetValueAtIndex(dimensions, 0);
            OCIndex size = DimensionGetCount((DimensionRef)firstDim);
            printf("    First dimension size: %lu points\n", (unsigned long)size);
        }
    }
    
    OCRelease(ds);
    OCRelease(err);
    
    printf("test_JCAMP_single_file passed.\n");
    return true;
}

bool test_JCAMP_import_all(void) {
    printf("test_JCAMP_import_all...\n");
    const char *root = getenv("JCAMP_TEST_ROOT");
    TEST_ASSERT(root != NULL);

    // build the file list
    collect_files(root, "");

    int total = 0, failed = 0;

    // run import & check on each
    for (struct file_node *n = file_list_head; n; n = n->next) {
        total++;
        if (!import_jcamp_and_check(root, n->rel)) {
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
    printf("test_JCAMP_import_all passed.\n");
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
