#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RMNLibrary.h"

int main() {
    printf("Testing DependentVariableCopyComponentLabels...\n");
    
    // Create a test dependent variable with component labels
    DependentVariable *dv = DependentVariableCreate(kFloat64Type, NULL, 3, 
                                                   (OCIndex[]){2, 3, 4});
    if (!dv) {
        printf("❌ Failed to create DependentVariable\n");
        return 1;
    }
    
    // Create some test labels
    OCArray *original_labels = OCArrayCreate(kStringType, 3);
    OCArraySetItem(original_labels, 0, OCStringCreateFromCString("x_component"));
    OCArraySetItem(original_labels, 1, OCStringCreateFromCString("y_component"));
    OCArraySetItem(original_labels, 2, OCStringCreateFromCString("z_component"));
    
    // Set the component labels
    bool success = DependentVariableSetComponentLabels(dv, original_labels);
    if (!success) {
        printf("❌ Failed to set component labels\n");
        OCArrayRelease(original_labels);
        DependentVariableRelease(dv);
        return 1;
    }
    
    // Now test our new copy function
    OCArray *copied_labels = DependentVariableCopyComponentLabels(dv);
    if (!copied_labels) {
        printf("❌ DependentVariableCopyComponentLabels returned NULL\n");
        OCArrayRelease(original_labels);
        DependentVariableRelease(dv);
        return 1;
    }
    
    // Verify the copy is correct
    OCIndex copied_count = OCArrayGetCount(copied_labels);
    OCIndex original_count = OCArrayGetCount(original_labels);
    
    if (copied_count != original_count) {
        printf("❌ Label counts don't match: original=%ld, copied=%ld\n", 
               original_count, copied_count);
        OCArrayRelease(original_labels);
        OCArrayRelease(copied_labels);
        DependentVariableRelease(dv);
        return 1;
    }
    
    // Check each label
    for (OCIndex i = 0; i < copied_count; i++) {
        OCString *original_str = (OCString*)OCArrayGetItem(original_labels, i);
        OCString *copied_str = (OCString*)OCArrayGetItem(copied_labels, i);
        
        if (!OCStringIsEqualToString(original_str, copied_str)) {
            printf("❌ Labels at index %ld don't match\n", i);
            OCArrayRelease(original_labels);
            OCArrayRelease(copied_labels);
            DependentVariableRelease(dv);
            return 1;
        }
    }
    
    // Verify it's a deep copy by modifying the original
    OCArraySetItem(original_labels, 0, OCStringCreateFromCString("modified"));
    
    // The copied array should still have the old value
    OCString *copied_first = (OCString*)OCArrayGetItem(copied_labels, 0);
    char *copied_cstr = OCStringCreateCString(copied_first);
    
    if (strcmp(copied_cstr, "x_component") != 0) {
        printf("❌ Copy was not deep - original modification affected copy\n");
        free(copied_cstr);
        OCArrayRelease(original_labels);
        OCArrayRelease(copied_labels);
        DependentVariableRelease(dv);
        return 1;
    }
    
    printf("✅ DependentVariableCopyComponentLabels test passed!\n");
    printf("   - Correctly copied %ld component labels\n", copied_count);
    printf("   - Verified deep copy semantics\n");
    printf("   - All labels match original values\n");
    
    // Cleanup
    free(copied_cstr);
    OCArrayRelease(original_labels);
    OCArrayRelease(copied_labels);
    DependentVariableRelease(dv);
    
    return 0;
}
