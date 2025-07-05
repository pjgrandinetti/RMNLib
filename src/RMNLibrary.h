/**

@file RMNLibrary.h

@brief Core definitions and includes for the RMN measurement library.



This header centralizes project-wide includes and dependencies for the

RMN library, wrapping both OCTypes and SITypes core headers as well

as local modules (Datum, Dimension, Dataset).
*/
#ifndef RMNLIBRARY_H
#define RMNLIBRARY_H
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// Include the core OCTypes definitions and utilities
#include <OCLibrary.h>
// Include the core SITypes definitions and utilities
#include <SILibrary.h>

/** @cond INTERNAL */
// Centralized Ref typedefs
typedef struct impl_Datum *DatumRef;
typedef struct impl_DependentVariable *DependentVariableRef;
typedef struct impl_Dimension *DimensionRef;
typedef struct impl_LabeledDimension *LabeledDimensionRef;
typedef struct impl_SIDimension *SIDimensionRef;
typedef struct impl_SIMonotonicDimension *SIMonotonicDimensionRef;
typedef struct impl_SILinearDimension *SILinearDimensionRef;
typedef struct impl_Dataset *DatasetRef;
/** @endcond */


// Local module headers
#include "Datum.h"
#include "DependentVariable.h"
#include "Dimension.h"
#include "RMNGridUtils.h"
#include "Dataset.h"
cJSON *OCMetadataCopyJSON(OCDictionaryRef dict);
OCDictionaryRef OCMetadataCreateFromJSON(cJSON *json, OCStringRef *outError);
void RMNLibTypesShutdown(void);
#endif /* RMNLIBRARY_H */
