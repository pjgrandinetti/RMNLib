//
//  DatasetImage.c
//
//  Created by Philip J. Grandinetti on 2/16/12.
//  Copyright (c) 2012 PhySy Ltd. All rights reserved.
//  Translated to OCTypes/SITypes/RMNLib APIs for cross-platform compatibility
//

#include "../RMNLibrary.h"
#include <string.h>
#include <stdlib.h>

// We'll use stb_image for cross-platform image loading
#ifdef STB_IMAGE_AVAILABLE
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"
#endif

// Forward declarations for helper functions
static bool ProcessGrayscaleImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error);
static bool ProcessGrayscaleAlphaImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error);
static bool ProcessRGBImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error);
static bool ProcessRGBAImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error);


DatasetRef DatasetImportImageCreateSignalWithImageData(OCArrayRef imageDataArray, double frameIncrementInSec, OCStringRef *error)
{
    if (error && *error) return NULL;
    if (!imageDataArray || OCArrayGetCount(imageDataArray) == 0) {
        if (error) *error = STR("DatasetImage: No image data provided");
        return NULL;
    }
    
    // Get first image to determine dimensions
    OCDataRef firstImageData = (OCDataRef)OCArrayGetValueAtIndex(imageDataArray, 0);
    const unsigned char* buffer = (const unsigned char*)OCDataGetBytesPtr(firstImageData);
    OCIndex bufferLength = OCDataGetLength(firstImageData);
    
    int width, height, channels;
    
#ifdef STB_IMAGE_AVAILABLE
    unsigned char* imagePixels = stbi_load_from_memory(buffer, (int)bufferLength, &width, &height, &channels, 0);
    if (!imagePixels) {
        if (error) *error = STR("DatasetImage: Failed to decode image data");
        return NULL;
    }
    stbi_image_free(imagePixels); // We'll reload for each image later
#else
    // Fallback implementation - would need actual image decoding
    if (error) *error = STR("DatasetImage: Image decoding not available - need stb_image or alternative");
    return NULL;
#endif
    
    OCIndex imageCount = OCArrayGetCount(imageDataArray);
    OCIndex totalPixels = width * height * imageCount;
    
    // Create dimensions using SITypes
    SIUnitRef dimensionlessUnit = SIUnitDimensionlessAndUnderived();
    SIScalarRef increment = SIScalarCreateWithDouble(1.0, dimensionlessUnit);
    
    // Create reciprocal dimension
    SIDimensionRef reciprocalDim = SIDimensionCreateWithQuantity(kSIQuantityDimensionless, error);
    
    // Create a single dimension for all pixels
    SILinearDimensionRef dim = SILinearDimensionCreateMinimal(kSIQuantityDimensionless, totalPixels, increment, reciprocalDim, error);
    
    OCRelease(increment);
    OCRelease(reciprocalDim);
    
    if (!dim) {
        if (error) *error = STR("DatasetImage: Failed to create pixel dimension");
        return NULL;
    }
    
    // Create dimensions array
    OCMutableArrayRef dimensions = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(dimensions, dim);
    OCRelease(dim);

    // Create dataset
    DatasetRef dataset = DatasetCreateEmpty(error);
    if (!dataset) {
        OCRelease(dimensions);
        if (error) *error = STR("DatasetImage: Failed to create empty dataset");
        return NULL;
    }

    DatasetSetDimensions(dataset, dimensions);
    
    // Debug: Check if dimensions were set properly
    OCArrayRef check_dims = DatasetGetDimensions(dataset);
    if (!check_dims || OCArrayGetCount(check_dims) == 0) {
        OCRelease(dimensions);
        OCRelease(dataset);
        if (error) *error = STR("DatasetImage: Failed to set dimensions on dataset");
        return NULL;
    }
    
    OCRelease(dimensions);    // Process images based on channel count
    bool success = false;
    
#ifdef STB_IMAGE_AVAILABLE
    if (channels == 1) {
        success = ProcessGrayscaleImages(dataset, imageDataArray, width, height, error);
    } else if (channels == 2) {
        success = ProcessGrayscaleAlphaImages(dataset, imageDataArray, width, height, error);
    } else if (channels == 3) {
        success = ProcessRGBImages(dataset, imageDataArray, width, height, error);
    } else if (channels == 4) {
        success = ProcessRGBAImages(dataset, imageDataArray, width, height, error);
    } else {
        if (error) *error = STR("DatasetImage: Unsupported number of channels");
    }
#endif
    
    if (!success) {
        OCRelease(dataset);
        return NULL;
    }
    
    return dataset;
}

// Helper function for grayscale images
static bool ProcessGrayscaleImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error) {
    OCIndex imageCount = OCArrayGetCount(imageDataArray);
    OCIndex totalPixels = width * height * imageCount;
    
    DependentVariableRef dv = DatasetAddEmptyDependentVariable(dataset, STR("pixel_1"), kOCNumberFloat32Type, totalPixels);
    if (!dv) {
        if (error) *error = STR("DatasetImage: Failed to create dependent variable");
        return false;
    }
    
    // Allocate memory for all pixel data
    float *pixelData = malloc(totalPixels * sizeof(float));
    if (!pixelData) {
        if (error) *error = STR("DatasetImage: Failed to allocate memory for pixel data");
        return false;
    }
    
    OCIndex pixelIndex = 0;
    
    // Process each image
    for (OCIndex imgIdx = 0; imgIdx < imageCount; imgIdx++) {
        OCDataRef imageData = (OCDataRef)OCArrayGetValueAtIndex(imageDataArray, imgIdx);
        const unsigned char* buffer = (const unsigned char*)OCDataGetBytesPtr(imageData);
        OCIndex bufferLength = OCDataGetLength(imageData);
        
#ifdef STB_IMAGE_AVAILABLE
        int w, h, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer, (int)bufferLength, &w, &h, &channels, 1); // Force grayscale
        if (!pixels || w != width || h != height) {
            free(pixelData);
            if (pixels) stbi_image_free(pixels);
            if (error) *error = STR("DatasetImage: Failed to decode image or dimension mismatch");
            return false;
        }
        
        // Convert to float and store
        for (int i = 0; i < width * height; i++) {
            pixelData[pixelIndex++] = (float)pixels[i] / 255.0f; // Normalize to 0-1
        }
        
        stbi_image_free(pixels);
#else
        // Fallback: would need actual implementation
        free(pixelData);
        if (error) *error = STR("DatasetImage: Image decoding not available");
        return false;
#endif
    }
    
    // Create OCData and set it
    OCDataRef values = OCDataCreate((const uint8_t*)pixelData, totalPixels * sizeof(float));
    DependentVariableSetComponentAtIndex(dv, values, 0);
    DependentVariableSetComponentLabelAtIndex(dv, STR("gray"), 0);
    
    OCRelease(values);
    free(pixelData);
    
    return true;
}

// Helper function for RGB images
static bool ProcessRGBImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error) {
    OCIndex imageCount = OCArrayGetCount(imageDataArray);
    OCIndex totalPixels = width * height * imageCount;
    
    DependentVariableRef dv = DatasetAddEmptyDependentVariable(dataset, STR("pixel_3"), kOCNumberFloat32Type, totalPixels);
    if (!dv) {
        if (error) *error = STR("DatasetImage: Failed to create dependent variable");
        return false;
    }
    
    // Allocate memory for each color channel
    float *redData = malloc(totalPixels * sizeof(float));
    float *greenData = malloc(totalPixels * sizeof(float));
    float *blueData = malloc(totalPixels * sizeof(float));
    
    if (!redData || !greenData || !blueData) {
        free(redData);
        free(greenData);
        free(blueData);
        if (error) *error = STR("DatasetImage: Failed to allocate memory for RGB data");
        return false;
    }
    
    OCIndex pixelIndex = 0;
    
    // Process each image
    for (OCIndex imgIdx = 0; imgIdx < imageCount; imgIdx++) {
        OCDataRef imageData = (OCDataRef)OCArrayGetValueAtIndex(imageDataArray, imgIdx);
        const unsigned char* buffer = (const unsigned char*)OCDataGetBytesPtr(imageData);
        OCIndex bufferLength = OCDataGetLength(imageData);
        
#ifdef STB_IMAGE_AVAILABLE
        int w, h, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer, (int)bufferLength, &w, &h, &channels, 3); // Force RGB
        if (!pixels || w != width || h != height) {
            free(redData);
            free(greenData);
            free(blueData);
            if (pixels) stbi_image_free(pixels);
            if (error) *error = STR("DatasetImage: Failed to decode image or dimension mismatch");
            return false;
        }
        
        // Convert to float and separate channels
        for (int i = 0; i < width * height; i++) {
            redData[pixelIndex] = (float)pixels[i * 3 + 0] / 255.0f;
            greenData[pixelIndex] = (float)pixels[i * 3 + 1] / 255.0f;
            blueData[pixelIndex] = (float)pixels[i * 3 + 2] / 255.0f;
            pixelIndex++;
        }
        
        stbi_image_free(pixels);
#else
        free(redData);
        free(greenData);
        free(blueData);
        if (error) *error = STR("DatasetImage: Image decoding not available");
        return false;
#endif
    }
    
    // Create OCData and set components
    OCDataRef redValues = OCDataCreate((const uint8_t*)redData, totalPixels * sizeof(float));
    OCDataRef greenValues = OCDataCreate((const uint8_t*)greenData, totalPixels * sizeof(float));
    OCDataRef blueValues = OCDataCreate((const uint8_t*)blueData, totalPixels * sizeof(float));
    
    DependentVariableSetComponentAtIndex(dv, redValues, 0);
    DependentVariableSetComponentAtIndex(dv, greenValues, 1);
    DependentVariableSetComponentAtIndex(dv, blueValues, 2);
    
    DependentVariableSetComponentLabelAtIndex(dv, STR("red"), 0);
    DependentVariableSetComponentLabelAtIndex(dv, STR("green"), 1);
    DependentVariableSetComponentLabelAtIndex(dv, STR("blue"), 2);
    
    OCRelease(redValues);
    OCRelease(greenValues);
    OCRelease(blueValues);
    
    free(redData);
    free(greenData);
    free(blueData);
    
    return true;
}

// Helper function for grayscale + alpha images  
static bool ProcessGrayscaleAlphaImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error) {
    // Similar implementation to grayscale but with 2 channels
    // Implementation would be similar to ProcessRGBImages but with 2 components
    if (error) *error = STR("DatasetImage: Grayscale+Alpha not yet implemented");
    return false;
}

// Helper function for RGBA images
static bool ProcessRGBAImages(DatasetRef dataset, OCArrayRef imageDataArray, int width, int height, OCStringRef *error) {
    OCIndex imageCount = OCArrayGetCount(imageDataArray);
    OCIndex totalPixels = width * height * imageCount;
    
    DependentVariableRef dv = DatasetAddEmptyDependentVariable(dataset, STR("pixel_4"), kOCNumberFloat32Type, -1);
    if (!dv) {
        if (error) {
            // Check if dataset has dimensions
            OCArrayRef dims = DatasetGetDimensions(dataset);
            if (!dims) {
                *error = STR("DatasetImage: Failed to create dependent variable - dataset has no dimensions");
            } else {
                OCIndex dimCount = OCArrayGetCount(dims);
                if (dimCount == 0) {
                    *error = STR("DatasetImage: Failed to create dependent variable - dataset has empty dimensions array");
                } else {
                    // Let's calculate the expected size and compare
                    char errorMsg[256];
                    snprintf(errorMsg, sizeof(errorMsg), 
                            "DatasetImage: Failed to create dependent variable - have %ld dims, totalPixels=%ld, width=%d, height=%d", 
                            (long)dimCount, (long)totalPixels, width, height);
                    *error = OCStringCreateWithCString(errorMsg);
                }
            }
        }
        return false;
    }
    
    // Allocate memory for each color channel
    float *redData = malloc(totalPixels * sizeof(float));
    float *greenData = malloc(totalPixels * sizeof(float));
    float *blueData = malloc(totalPixels * sizeof(float));
    float *alphaData = malloc(totalPixels * sizeof(float));
    
    if (!redData || !greenData || !blueData || !alphaData) {
        free(redData);
        free(greenData);
        free(blueData);
        free(alphaData);
        if (error) *error = STR("DatasetImage: Failed to allocate memory for RGBA data");
        return false;
    }
    
    OCIndex pixelIndex = 0;
    
    // Process each image
    for (OCIndex imgIdx = 0; imgIdx < imageCount; imgIdx++) {
        OCDataRef imageData = (OCDataRef)OCArrayGetValueAtIndex(imageDataArray, imgIdx);
        const unsigned char* buffer = (const unsigned char*)OCDataGetBytesPtr(imageData);
        OCIndex bufferLength = OCDataGetLength(imageData);
        
#ifdef STB_IMAGE_AVAILABLE
        int w, h, channels;
        unsigned char* pixels = stbi_load_from_memory(buffer, (int)bufferLength, &w, &h, &channels, 4); // Force RGBA
        if (!pixels || w != width || h != height) {
            free(redData);
            free(greenData);
            free(blueData);
            free(alphaData);
            if (pixels) stbi_image_free(pixels);
            if (error) *error = STR("DatasetImage: Failed to decode image or dimension mismatch");
            return false;
        }
        
        // Convert to float and separate channels
        for (int i = 0; i < width * height; i++) {
            redData[pixelIndex] = (float)pixels[i * 4 + 0] / 255.0f;
            greenData[pixelIndex] = (float)pixels[i * 4 + 1] / 255.0f;
            blueData[pixelIndex] = (float)pixels[i * 4 + 2] / 255.0f;
            alphaData[pixelIndex] = (float)pixels[i * 4 + 3] / 255.0f;
            pixelIndex++;
        }
        
        stbi_image_free(pixels);
#else
        free(redData);
        free(greenData);
        free(blueData);
        free(alphaData);
        if (error) *error = STR("DatasetImage: Image decoding not available");
        return false;
#endif
    }
    
    // Create OCData and set components
    OCDataRef redValues = OCDataCreate((const uint8_t*)redData, totalPixels * sizeof(float));
    OCDataRef greenValues = OCDataCreate((const uint8_t*)greenData, totalPixels * sizeof(float));
    OCDataRef blueValues = OCDataCreate((const uint8_t*)blueData, totalPixels * sizeof(float));
    OCDataRef alphaValues = OCDataCreate((const uint8_t*)alphaData, totalPixels * sizeof(float));
    
    DependentVariableSetComponentAtIndex(dv, redValues, 0);
    DependentVariableSetComponentAtIndex(dv, greenValues, 1);
    DependentVariableSetComponentAtIndex(dv, blueValues, 2);
    DependentVariableSetComponentAtIndex(dv, alphaValues, 3);
    
    DependentVariableSetComponentLabelAtIndex(dv, STR("red"), 0);
    DependentVariableSetComponentLabelAtIndex(dv, STR("green"), 1);
    DependentVariableSetComponentLabelAtIndex(dv, STR("blue"), 2);
    DependentVariableSetComponentLabelAtIndex(dv, STR("alpha"), 3);
    
    OCRelease(redValues);
    OCRelease(greenValues);
    OCRelease(blueValues);
    OCRelease(alphaValues);
    
    free(redData);
    free(greenData);
    free(blueData);
    free(alphaData);
    
    return true;
}

DatasetRef DatasetImportImageCreateSignalWithData(OCDataRef contents, OCStringRef *error)
{
    if (error && *error) return NULL;
    if (!contents) {
        if (error) *error = STR("DatasetImage: No image data provided");
        return NULL;
    }
    
    // Create array with single image
    OCMutableArrayRef imageDataArray = OCArrayCreateMutable(0, &kOCTypeArrayCallBacks);
    OCArrayAppendValue(imageDataArray, contents);
    
    DatasetRef dataset = DatasetImportImageCreateSignalWithImageData(imageDataArray, 1.0, error);
    
    OCRelease(imageDataArray);
    return dataset;
}


