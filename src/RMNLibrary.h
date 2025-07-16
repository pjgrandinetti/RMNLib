/**

@file RMNLibrary.h

@brief Core definitions and includes for the RMN measurement library
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

/* 
 -------------------------------------------------------------
 BLAS & LAPACK — pick the right include for each platform
 -------------------------------------------------------------
 macOS      : use Apple’s Accelerate.framework, which bundles CBLAS + LAPACK
 Linux      : assume a system-installed CBLAS + LAPACKE (e.g. OpenBLAS, Netlib)
 Windows    : assume user has installed OpenBLAS or Intel MKL with CBLAS + LAPACKE
 -------------------------------------------------------------
 Linker flags you will typically need:
   macOS    : -framework Accelerate
   Linux    : -lcblas -llapacke  (or -lopenblas -llapacke)
   Windows  : link against OpenBLAS (e.g. openblas.lib) or MKL import libs
 -------------------------------------------------------------
*/
#if defined(__APPLE__)
  /* All BLAS & LAPACK lives in Accelerate.framework on macOS */
  #include <Accelerate/Accelerate.h>
#else
  /* CBLAS interface */
  #include <cblas.h>
  /* LAPACKE C interface to LAPACK */
  #include <lapacke.h>
#endif

// Include the core OCTypes definitions and utilities
#include <OCLibrary.h>
// Include the core SITypes definitions and utilities
#include <SILibrary.h>
/** @cond INTERNAL */
// Centralized Ref typedefs
typedef struct impl_GeographicCoordinate *GeographicCoordinateRef;
typedef struct impl_Datum *DatumRef;
typedef struct impl_SparseSampling *SparseSamplingRef;
typedef struct impl_DependentVariable *DependentVariableRef;
typedef struct impl_Dimension *DimensionRef;
typedef struct impl_LabeledDimension *LabeledDimensionRef;
typedef struct impl_SIDimension *SIDimensionRef;
typedef struct impl_SIMonotonicDimension *SIMonotonicDimensionRef;
typedef struct impl_SILinearDimension *SILinearDimensionRef;
typedef struct impl_Dataset *DatasetRef;
/** @endcond */
#define DependentVariableComponentsFileName STR("dependent_variable-%ld.data")
// Local module headers
#include "Dataset.h"
#include "Datum.h"
#include "DependentVariable.h"
#include "Dimension.h"
#include "GeographicCoordinate.h"
#include "RMNGridUtils.h"
#include "SparseSampling.h"
cJSON *OCMetadataCopyJSON(OCDictionaryRef dict);
OCDictionaryRef OCMetadataCreateFromJSON(cJSON *json, OCStringRef *outError);
void RMNLibTypesShutdown(void);

// Specialized Fields
#include "NMRSpectroscopy.h"
#endif /* RMNLIBRARY_H */
