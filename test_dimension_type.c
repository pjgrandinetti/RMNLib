#include <stdio.h>
#include "RMNLibrary.h"

int main() {
    OCStringRef error = NULL;
    
    // Test SILinearDimension
    printf("=== Testing SILinearDimension ===\n");
    SIScalarRef increment = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("Hz")));
    SILinearDimensionRef linear_dim = SILinearDimensionCreate(
        STR("test_linear"),
        STR("Test linear dimension"),
        NULL,
        kSIQuantityFrequency,
        NULL, NULL, NULL,
        kDimensionScalingNone,
        10, increment, false, NULL,
        &error);
    
    if (linear_dim) {
        OCDictionaryRef dict = SILinearDimensionCopyAsDictionary(linear_dim);
        if (dict) {
            OCStringRef type_value = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
            printf("Linear dimension 'type' field: %s\n", 
                   type_value ? OCStringGetCString(type_value) : "NULL");
            
            // Print all keys
            printf("All keys in dictionary:\n");
            OCArrayRef keys = OCDictionaryCreateArrayWithAllKeys(dict);
            for (OCIndex i = 0; i < OCArrayGetCount(keys); i++) {
                OCStringRef key = (OCStringRef)OCArrayGetValueAtIndex(keys, i);
                printf("  - %s\n", OCStringGetCString(key));
            }
            OCRelease(keys);
            OCRelease(dict);
        }
        OCRelease(linear_dim);
    }
    
    // Test SIMonotonicDimension
    printf("\n=== Testing SIMonotonicDimension ===\n");
    OCMutableArrayRef coords = OCArrayCreateMutable(3, &kOCTypeArrayCallBacks);
    SIScalarRef coord1 = SIScalarCreateWithDouble(1.0, SIUnitWithSymbol(STR("Hz")));
    SIScalarRef coord2 = SIScalarCreateWithDouble(2.0, SIUnitWithSymbol(STR("Hz")));
    SIScalarRef coord3 = SIScalarCreateWithDouble(3.0, SIUnitWithSymbol(STR("Hz")));
    OCArrayAppendValue(coords, coord1);
    OCArrayAppendValue(coords, coord2);
    OCArrayAppendValue(coords, coord3);
    
    SIMonotonicDimensionRef mono_dim = SIMonotonicDimensionCreate(
        STR("test_monotonic"),
        STR("Test monotonic dimension"),
        NULL,
        kSIQuantityFrequency,
        NULL, NULL, NULL,
        kDimensionScalingNone,
        (OCArrayRef)coords, NULL,
        &error);
    
    if (mono_dim) {
        OCDictionaryRef dict = SIMonotonicDimensionCopyAsDictionary(mono_dim);
        if (dict) {
            OCStringRef type_value = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
            printf("Monotonic dimension 'type' field: %s\n", 
                   type_value ? OCStringGetCString(type_value) : "NULL");
            OCRelease(dict);
        }
        OCRelease(mono_dim);
    }
    
    // Test LabeledDimension  
    printf("\n=== Testing LabeledDimension ===\n");
    OCMutableArrayRef labels = OCArrayCreateMutable(3, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(labels, STR("A"));
    OCArrayAppendValue(labels, STR("B"));
    OCArrayAppendValue(labels, STR("C"));
    
    LabeledDimensionRef labeled_dim = LabeledDimensionCreate(
        STR("test_labeled"),
        STR("Test labeled dimension"),
        NULL,
        (OCArrayRef)labels,
        &error);
    
    if (labeled_dim) {
        OCDictionaryRef dict = LabeledDimensionCopyAsDictionary(labeled_dim);
        if (dict) {
            OCStringRef type_value = (OCStringRef)OCDictionaryGetValue(dict, STR("type"));
            printf("Labeled dimension 'type' field: %s\n", 
                   type_value ? OCStringGetCString(type_value) : "NULL");
            OCRelease(dict);
        }
        OCRelease(labeled_dim);
    }
    
    // Cleanup
    OCRelease(increment);
    OCRelease(coord1);
    OCRelease(coord2);
    OCRelease(coord3);
    OCRelease(coords);
    OCRelease(labels);
    
    return 0;
}
