// DatasetTecmag.h
// RMNLib
//
// Created by Philip J. Grandinetti on 10/23/11.
// Copyright (c) 2011 PhySy Ltd. All rights reserved.
//
#ifndef DATASET_TECMAG_H
#define DATASET_TECMAG_H

#include "RMNLibrary.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file DatasetTecmag.h
 * @brief Interface for importing Tecmag NMR spectroscopy data files.
 *
 * This module provides functionality to parse and import Tecmag format files,
 * which are commonly used in NMR spectroscopy for storing experimental data.
 * The Tecmag format stores binary data along with experimental parameters
 * including acquisition settings, instrument parameters, and metadata.
 *
 * ## Tecmag File Format
 * Tecmag files contain:
 * - Experimental parameters (frequencies, timings, pulse sequences)
 * - Acquisition metadata (scans, points, dimensions)
 * - Hardware settings (transmitter/receiver gains, filter settings)
 * - Temperature and field strength information
 * - Complex-valued time-domain or frequency-domain data
 *
 * ## Usage Example
 * @code
 * OCDataRef fileContents = OCDataCreateFromFile("/path/to/data.tnt");
 * OCStringRef error = NULL;
 * DatasetRef dataset = DatasetImportTecmagCreateWithFileData(fileContents, &error);
 * if (dataset) {
 *     // Successfully imported Tecmag data
 *     // Access dimensions, dependent variables, metadata, etc.
 * } else if (error) {
 *     printf("Error importing Tecmag file: %s\n", OCStringGetCString(error));
 *     OCRelease(error);
 * }
 * OCRelease(fileContents);
 * @endcode
 *
 * @author Philip J. Grandinetti
 * @since RMNLib 2.0
 */

/**
 * @brief Import a Tecmag format file and create a Dataset.
 *
 * This function parses a Tecmag binary file format and creates a complete
 * Dataset object with properly configured dimensions, dependent variables,
 * and comprehensive metadata extracted from the Tecmag header structures.
 *
 * The function supports both Tecmag and Tecmag2 file formats, automatically
 * detecting the format version and extracting the appropriate metadata.
 * Data can be either time-domain or frequency-domain depending on the FFT
 * flags in the Tecmag2 structure.
 *
 * ## File Structure Parsing
 * The function expects a valid Tecmag file with the following structure:
 * - TNT1 version tag (8 bytes)
 * - TMAG structure containing acquisition parameters
 * - DATA section with complex-valued spectroscopic data
 * - TMG2 structure containing processing parameters
 *
 * ## Dimension Configuration
 * - Automatically determines the number of dimensions from actual_npts
 * - Creates SILinearDimension objects with appropriate scaling
 * - Configures time/frequency domain based on FFT flags
 * - Sets proper units (Hz, MHz, seconds) and origin offsets
 *
 * ## Metadata Extraction
 * Comprehensive metadata is extracted including:
 * - Experimental parameters (acquisition time, delays, scans)
 * - Instrument settings (gains, phases, frequencies)
 * - Physical conditions (temperature, magnetic field)
 * - Sequence and solvent information
 * - Processing history and parameters
 *
 * @param contents Binary file data as OCDataRef containing the complete Tecmag file
 * @param[out] error Pointer to OCStringRef that receives error description on failure.
 *                   Set to NULL if no error information is needed.
 *                   If an error occurs, this will contain a descriptive message.
 * @return New DatasetRef on success, NULL on failure.
 *         The caller is responsible for releasing the returned Dataset.
 * @retval NULL if contents is NULL, empty, or contains invalid Tecmag data
 * @retval NULL if memory allocation fails during Dataset creation
 * @retval NULL if the file format is not recognized as valid Tecmag
 *
 * @note The function automatically handles complex conjugation for proper
 *       frequency sign conventions in the rotating frame.
 * @note The returned Dataset contains deep copies of all data - the input
 *       OCDataRef can be safely released after this call.
 * @note Multi-dimensional data is supported with automatic dimension detection
 *       from the actual_npts array in the Tecmag structure.
 *
 * @warning This function allocates significant memory for large datasets.
 *          Ensure sufficient memory is available before calling.
 * @warning The function performs binary data parsing and assumes the input
 *          file is a valid Tecmag format. Corrupted files may cause undefined behavior.
 *
 * @see Dataset for information about the returned Dataset object
 * @see SILinearDimension for dimension configuration details
 * @see DependentVariable for data storage information
 *
 * @since RMNLib 2.0
 */
DatasetRef DatasetImportTecmagCreateWithFileData(OCDataRef contents, OCStringRef *error);

#ifdef __cplusplus
}
#endif

#endif // DATASET_TECMAG_H
