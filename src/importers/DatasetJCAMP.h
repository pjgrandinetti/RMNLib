// DatasetJCAMP.h
// RMN 2.0
//
// Created by Philip J. Grandinetti on 2/21/14.
// Copyright (c) 2014 PhySy. All rights reserved.
//
#ifndef DATASET_JCAMP_H
#define DATASET_JCAMP_H

#include "../RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parse JCAMP lines into a dictionary.
 * @param lines Array of OCStringRef lines.
 * @param index Pointer to OCIndex for parsing state.
 * @return OCDictionaryRef with parsed JCAMP data.
 */
OCDictionaryRef DatasetImportJCAMPCreateDictionaryWithLines(OCArrayRef lines,
                                            OCIndex      *indexOut,
                                            OCStringRef  *error);

/**
 * @brief Create a Dataset from JCAMP data contents.
 * @param contents OCDataRef with JCAMP file contents.
 * @param[out] error Set to OCStringRef error message on failure.
 * @return DatasetRef or NULL on error.
 */
DatasetRef DatasetImportJCAMPCreateSignalWithData(OCDataRef contents, OCStringRef *error);

#ifdef __cplusplus
}
#endif

#endif // DATASET_JCAMP_H
