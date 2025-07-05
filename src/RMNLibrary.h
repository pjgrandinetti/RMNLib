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
// Local module headers
#include "Dataset.h"
#include "Datum.h"
#include "DependentVariable.h"
#include "Dimension.h"
#include "RMNGridUtils.h"
void RMNLibTypesShutdown(void);
#endif /* RMNLIBRARY_H */
