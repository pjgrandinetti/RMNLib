/**

@file RMNLibrary.h

@brief Core definitions and includes for the RMN measurement library.



This header centralizes project-wide includes and dependencies for the

RMN library, wrapping both OCTypes and SITypes core headers as well

as local modules (Datum, Dimension, Dataset).
*/

#ifndef RMNLIBRARY_H
#define RMNLIBRARY_H

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <complex.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Include the core OCTypes definitions and utilities
#include <OCLibrary.h>
// Include the core SITypes definitions and utilities
#include <SILibrary.h>

// Local module headers
#include "Datum.h"
#include "Dimension.h"
#include "DependentVariable.h"
#include "Dataset.h"
#include "RMNGridUtils.h"

#endif /* RMNLIBRARY_H */

