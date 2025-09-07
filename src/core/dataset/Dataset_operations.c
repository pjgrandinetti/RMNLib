/**
 * @file Dataset_operations.c
 * @brief Complex operations and transformations for Dataset
 *
 * This module contains complex operations, transformations, and analytical
 * functions that can be performed on Dataset objects.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../RMNLibrary.h"
#include "Dataset_private.h"
#ifdef __cplusplus
extern "C" {
#endif
// ============================================================================
#pragma region Dataset Operations
// ============================================================================
DependentVariableRef DatasetAddEmptyDependentVariable(DatasetRef theDataset,
                                                      OCStringRef quantityType,
                                                      OCNumberType elementType,
                                                      OCIndex size) {
    if (!theDataset) return NULL;
    if (!quantityType) return NULL;
    if (NULL == theDataset->dimensions && size < 0) return NULL;
    if (theDataset->dimensions) {
        OCIndex sizeFromDimensions = RMNCalculateSizeFromDimensions(theDataset->dimensions);
        if (size == -1) size = sizeFromDimensions;
        if (size != sizeFromDimensions) return NULL;
    }
    DependentVariableRef theDependentVariable = DependentVariableCreateDefault(quantityType,
                                                                               elementType,
                                                                               size,
                                                                               NULL);
    OCArrayAppendValue(theDataset->dependentVariables, theDependentVariable);
    OCRelease(theDependentVariable);
    return theDependentVariable;
}
#pragma endregion
#ifdef __cplusplus
}
#endif
