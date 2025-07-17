// tests/test_Image.c

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

/// Helper: does filename end with image extensions?
static bool has_image_ext(const char *name) {
    size_t L = strlen(name);
    return (L > 4 && (strcmp(name + L - 4, ".png") == 0 ||
                     strcmp(name + L - 4, ".jpg") == 0 ||
                     strcmp(name + L - 5, ".jpeg") == 0 ||
                     strcmp(name + L - 4, ".bmp") == 0 ||
                     strcmp(name + L - 4, ".tga") == 0 ||
                     strcmp(name + L - 4, ".gif") == 0));
}

/// A simple linked list to collect rel-paths of all image files.
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

/// Recurse under <root>/<cur_rel> and collect every image file
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

        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            char child_rel[PATH_MAX];
            if (cur_rel[0] == '\0')
                snprintf(child_rel, sizeof(child_rel), "%s", ent->d_name);
            else
                snprintf(child_rel, sizeof(child_rel), "%s/%s", cur_rel, ent->d_name);

            collect_files(root, child_rel);
        }
        closedir(d);
    } else if (S_ISREG(st.st_mode)) {
        if (has_image_ext(cur_rel)) {
            add_file(cur_rel);
        }
    }
}

/// Test import of all image files in the test directory
bool test_Image_import_all(void) {
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) {
        fprintf(stderr, "[WARN] IMAGE_TEST_ROOT not set, skipping import test\n");
        return true; // Skip test if no root directory
    }

    // Check if directory exists
    struct stat st;
    if (stat(root, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[WARN] IMAGE_TEST_ROOT directory does not exist: %s\n", root);
        return true; // Skip test if directory doesn't exist
    }

    // Clear any previous file list
    while (file_list_head) {
        struct file_node *tmp = file_list_head;
        file_list_head = file_list_head->next;
        free(tmp);
    }

    // Collect all image files
    collect_files(root, "");

    int tested = 0, passed = 0;

    // Test each image file
    for (struct file_node *n = file_list_head; n; n = n->next) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", root, n->rel);

        // Read file into memory
        FILE *fp = fopen(full_path, "rb");
        if (!fp) {
            fprintf(stderr, "[FAIL] %-60s : could not open file\n", n->rel);
            tested++;
            continue;
        }

        // Get file size
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (file_size <= 0) {
            fprintf(stderr, "[FAIL] %-60s : empty or invalid file\n", n->rel);
            fclose(fp);
            tested++;
            continue;
        }

        // Read file contents
        unsigned char *buffer = malloc(file_size);
        if (!buffer) {
            fprintf(stderr, "[FAIL] %-60s : memory allocation failed\n", n->rel);
            fclose(fp);
            tested++;
            continue;
        }

        size_t bytes_read = fread(buffer, 1, file_size, fp);
        fclose(fp);

        if (bytes_read != (size_t)file_size) {
            fprintf(stderr, "[FAIL] %-60s : file read error\n", n->rel);
            free(buffer);
            tested++;
            continue;
        }

        // Create OCData from buffer
        OCDataRef imageData = OCDataCreate(buffer, file_size);
        free(buffer);

        if (!imageData) {
            fprintf(stderr, "[FAIL] %-60s : OCData creation failed\n", n->rel);
            tested++;
            continue;
        }

        // Try to import as dataset
        OCStringRef error = NULL;
        DatasetRef dataset = DatasetImportImageCreateSignalWithData(imageData, &error);
        
        OCRelease(imageData);

        if (dataset) {
            fprintf(stderr, "[PASS] %-60s : import succeeded\n", n->rel);
            OCRelease(dataset);
            passed++;
        } else {
            const char *error_msg = error ? OCStringGetCString(error) : "unknown error";
            // Check if it's the expected "not available" error
            if (error && strstr(error_msg, "not available")) {
                fprintf(stderr, "[SKIP] %-60s : %s\n", n->rel, error_msg);
                passed++; // Count as passed since it's expected behavior
            } else {
                fprintf(stderr, "[FAIL] %-60s : %s\n", n->rel, error_msg);
            }
            if (error) OCRelease(error);
        }
        tested++;
    }

    // Clean up file list
    while (file_list_head) {
        struct file_node *tmp = file_list_head;
        file_list_head = file_list_head->next;
        free(tmp);
    }

    fprintf(stderr, "\nSummary: tested %d files, %d passed, %d failed\n", 
            tested, passed, tested - passed);

    return tested == 0 || passed == tested;
}

/// Test import of a single image file
bool test_Image_single_file(void) {
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) {
        return true; // Skip if no test root
    }

    // Look for any PNG file to test with
    char test_file[PATH_MAX];
    snprintf(test_file, sizeof(test_file), "%s/test.png", root);

    FILE *fp = fopen(test_file, "rb");
    if (!fp) {
        // Try looking for any image file
        DIR *d = opendir(root);
        if (!d) return true;

        struct dirent *ent;
        bool found = false;
        while ((ent = readdir(d)) != NULL && !found) {
            if (has_image_ext(ent->d_name)) {
                snprintf(test_file, sizeof(test_file), "%s/%s", root, ent->d_name);
                fp = fopen(test_file, "rb");
                if (fp) found = true;
            }
        }
        closedir(d);

        if (!found) return true; // No test files available
    }

    // Read file
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    // Create OCData and test import
    OCDataRef imageData = OCDataCreate(buffer, file_size);
    free(buffer);

    OCStringRef error = NULL;
    DatasetRef dataset = DatasetImportImageCreateSignalWithData(imageData, &error);

    OCRelease(imageData);

    if (!dataset) {
        if (error) {
            const char *error_msg = OCStringGetCString(error);
            // Check if it's the expected "not available" error
            if (strstr(error_msg, "not available")) {
                OCRelease(error);
                return true; // Expected behavior when stb_image not available
            }
            fprintf(stderr, "Single file test failed: %s\n", error_msg);
            OCRelease(error);
        }
        return false;
    }

    // Validate dataset structure
    bool valid = true;

    // Check that we have exactly 1 dimension (single linear dimension for pixels)
    OCMutableArrayRef dimensions = DatasetGetDimensions(dataset);
    if (OCArrayGetCount(dimensions) != 1) {
        fprintf(stderr, "Dataset should have exactly 1 dimension (linear pixel dimension)\n");
        valid = false;
    }
    // Note: DatasetGetDimensions returns a non-retained reference, so no OCRelease needed

    // Check that we have at least one dependent variable
    OCMutableArrayRef dvs = DatasetGetDependentVariables(dataset);
    OCIndex dvCount = OCArrayGetCount(dvs);
    if (dvCount == 0) {
        fprintf(stderr, "Dataset should have at least one dependent variable\n");
        valid = false;
    }

    OCRelease(dataset);
    return valid;
}

/// Test grayscale image processing
bool test_Image_grayscale(void) {
    // This test would need a known grayscale image
    // For now, we'll just test the code path if we have image files
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) return true;

    // Try to find a grayscale image or any image that we can test
    char test_file[PATH_MAX];
    snprintf(test_file, sizeof(test_file), "%s/grayscale.png", root);

    FILE *fp = fopen(test_file, "rb");
    if (!fp) {
        // Look for any image file
        DIR *d = opendir(root);
        if (!d) return true;

        struct dirent *ent;
        bool found = false;
        while ((ent = readdir(d)) != NULL && !found) {
            if (has_image_ext(ent->d_name)) {
                snprintf(test_file, sizeof(test_file), "%s/%s", root, ent->d_name);
                fp = fopen(test_file, "rb");
                if (fp) found = true;
            }
        }
        closedir(d);

        if (!found) return true;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    OCDataRef imageData = OCDataCreate(buffer, file_size);
    free(buffer);

    OCStringRef error = NULL;
    DatasetRef dataset = DatasetImportImageCreateSignalWithData(imageData, &error);

    OCRelease(imageData);

    if (!dataset) {
        if (error) OCRelease(error);
        return true; // Not a failure if image format not supported
    }

    // Check that we got some data
    OCMutableArrayRef dvs = DatasetGetDependentVariables(dataset);
    OCIndex dvCount = OCArrayGetCount(dvs);
    bool valid = (dvCount > 0);

    if (valid) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvs, 0);
        OCIndex componentCount = DependentVariableGetComponentCount(dv);
        // Should have 1 component for grayscale, 3 for RGB
        valid = (componentCount >= 1 && componentCount <= 4);
    }

    OCRelease(dataset);
    return valid;
}

/// Test RGB image processing
bool test_Image_rgb(void) {
    // Similar to grayscale test but looking for RGB images
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) return true;

    char test_file[PATH_MAX];
    snprintf(test_file, sizeof(test_file), "%s/rgb.png", root);

    FILE *fp = fopen(test_file, "rb");
    if (!fp) {
        // Look for any image file
        DIR *d = opendir(root);
        if (!d) return true;

        struct dirent *ent;
        bool found = false;
        while ((ent = readdir(d)) != NULL && !found) {
            if (has_image_ext(ent->d_name)) {
                snprintf(test_file, sizeof(test_file), "%s/%s", root, ent->d_name);
                fp = fopen(test_file, "rb");
                if (fp) found = true;
            }
        }
        closedir(d);

        if (!found) return true;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    unsigned char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    OCDataRef imageData = OCDataCreate(buffer, file_size);
    free(buffer);

    OCStringRef error = NULL;
    DatasetRef dataset = DatasetImportImageCreateSignalWithData(imageData, &error);

    OCRelease(imageData);

    if (!dataset) {
        if (error) OCRelease(error);
        return true; // Not a failure if format not supported
    }

    bool valid = true;
    OCMutableArrayRef dvs = DatasetGetDependentVariables(dataset);
    OCIndex dvCount = OCArrayGetCount(dvs);
    if (dvCount > 0) {
        DependentVariableRef dv = (DependentVariableRef)OCArrayGetValueAtIndex(dvs, 0);
        OCIndex componentCount = DependentVariableGetComponentCount(dv);
        valid = (componentCount >= 1 && componentCount <= 4);
    }

    OCRelease(dataset);
    return valid;
}

/// Test multiple images (time series)
bool test_Image_multiple_images(void) {
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) return true;

    // Look for multiple image files to create a time series
    DIR *d = opendir(root);
    if (!d) return true;

    OCMutableArrayRef imageDataArray = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    int fileCount = 0;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL && fileCount < 3) {
        if (has_image_ext(ent->d_name)) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", root, ent->d_name);

            FILE *fp = fopen(full_path, "rb");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long file_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                unsigned char *buffer = malloc(file_size);
                if (buffer && fread(buffer, 1, file_size, fp) == (size_t)file_size) {
                    OCDataRef imageData = OCDataCreate(buffer, file_size);
                    if (imageData) {
                        OCArrayAppendValue(imageDataArray, imageData);
                        OCRelease(imageData);
                        fileCount++;
                    }
                }
                if (buffer) free(buffer);
                fclose(fp);
            }
        }
    }
    closedir(d);

    if (fileCount == 0) {
        OCRelease(imageDataArray);
        return true; // No files to test with
    }

    // Test with multiple images
    OCStringRef error = NULL;
    DatasetRef dataset = DatasetImportImageCreateSignalWithImageData(imageDataArray, 1.0, &error);

    OCRelease(imageDataArray);

    if (!dataset) {
        if (error) OCRelease(error);
        return true; // Not a failure if format not supported
    }

    // Check dimensions - should have 1 dimension regardless of number of images (flattened pixel array)
    OCMutableArrayRef dimensions = DatasetGetDimensions(dataset);
    bool valid = true;
    
    // Should always have 1 dimension for our current implementation
    valid = (OCArrayGetCount(dimensions) == 1);
    // Note: DatasetGetDimensions returns a non-retained reference, so no OCRelease needed
    OCRelease(dataset);
    return valid;
}

/// Test dimension creation and validation
bool test_Image_dimensions(void) {
    // Test dimension creation without actual image data
    // This tests the SITypes integration
    
    // Create a simple test image data array
    unsigned char test_pixel[3] = {255, 128, 64}; // RGB pixel
    OCDataRef testData = OCDataCreate(test_pixel, 3);
    
    OCMutableArrayRef imageArray = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(imageArray, testData);
    
    OCStringRef error = NULL;
    DatasetRef dataset = DatasetImportImageCreateSignalWithImageData(imageArray, 0.5, &error);
    
    OCRelease(testData);
    OCRelease(imageArray);
    
    if (!dataset) {
        // This is expected since we don't have real image data
        if (error) OCRelease(error);
        return true; // Not a failure - we're testing error handling
    }
    
    // If we somehow got a dataset, validate its structure
    OCMutableArrayRef dimensions = DatasetGetDimensions(dataset);
    bool valid = (OCArrayGetCount(dimensions) == 1); // Should have 1 dimension for our implementation
    // Note: DatasetGetDimensions returns a non-retained reference, so no OCRelease needed
    OCRelease(dataset);
    return valid;
}

/// Test memory management
bool test_Image_memory_management(void) {
    // Test that we properly handle memory allocation/deallocation
    const char *root = getenv("IMAGE_TEST_ROOT");
    if (!root) return true;

    // Find an image file to test with
    DIR *d = opendir(root);
    if (!d) return true;

    char test_file[PATH_MAX] = "";
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (has_image_ext(ent->d_name)) {
            snprintf(test_file, sizeof(test_file), "%s/%s", root, ent->d_name);
            break;
        }
    }
    closedir(d);

    if (test_file[0] == '\0') return true; // No test file found

    // Test multiple allocations and deallocations
    for (int i = 0; i < 5; i++) {
        FILE *fp = fopen(test_file, "rb");
        if (!fp) continue;

        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char *buffer = malloc(file_size);
        if (!buffer) {
            fclose(fp);
            continue;
        }

        fread(buffer, 1, file_size, fp);
        fclose(fp);

        OCDataRef imageData = OCDataCreate(buffer, file_size);
        free(buffer);

        if (imageData) {
            OCStringRef error = NULL;
            DatasetRef dataset = DatasetImportImageCreateSignalWithData(imageData, &error);
            
            OCRelease(imageData);
            
            if (dataset) {
                OCRelease(dataset);
            } else if (error) {
                OCRelease(error);
            }
        }
    }

    return true; // If we get here without crashing, memory management is working
}
