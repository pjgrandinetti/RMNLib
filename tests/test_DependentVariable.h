#pragma once
#ifndef TEST_DEPENDENT_VARIABLE_H
#define TEST_DEPENDENT_VARIABLE_H

#include <stdbool.h>

/**
 * @file test_dependent_variable.h
 * @brief Prototypes for DependentVariable unit tests.
 */

/**
 * @brief Exercise basic creation, copy, dictionary round-trip and formatting.
 * @return true on success, false on failure.
 */
bool test_DependentVariable_base(void);

/**
 * @brief Exercise component insertion/removal, resizing and component-array copy.
 * @return true on success, false on failure.
 */
bool test_DependentVariable_components(void);

/**
 * @brief Exercise raw-value get/set via SIScalar APIs.
 * @return true on success, false on failure.
 */
bool test_DependentVariable_values(void);


bool test_DependentVariable_typeQueries(void);
bool test_DependentVariable_complexCopy(void);


bool test_DependentVariable_invalidCreate(void);

#endif // TEST_DEPENDENT_VARIABLE_H
