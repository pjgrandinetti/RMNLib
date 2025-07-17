// Image.h
// RMN 2.0
//
// Created by Philip J. Grandinetti on 2/16/12.
// Copyright (c) 2012 PhySy Ltd. All rights reserved.
// Translated to OCTypes/SITypes/RMNLib APIs for cross-platform compatibility
//
#ifndef IMAGE_H
#define IMAGE_H

#include "../RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a dataset from multiple image data objects (for time series).
 * @param imageDataArray Array of OCDataRef objects containing image file data.
 * @param frameIncrementInSec Time increment between frames in seconds.
 * @param error Pointer to error string (optional).
 * @return DatasetRef with image data or NULL on failure.
 */
DatasetRef DatasetImportImageCreateSignalWithImageData(OCArrayRef imageDataArray, 
                                                       double frameIncrementInSec, 
                                                       OCStringRef *error);

/**
 * @brief Create a dataset from a single image data object.
 * @param contents OCDataRef containing image file data (PNG, JPEG, etc.).
 * @param error Pointer to error string (optional).
 * @return DatasetRef with image data or NULL on failure.
 */
DatasetRef DatasetImportImageCreateSignalWithData(OCDataRef contents, 
                                                  OCStringRef *error);

#ifdef __cplusplus
}
#endif

#endif // IMAGE_H
