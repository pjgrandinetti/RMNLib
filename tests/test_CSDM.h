#ifndef TEST_CSDM_H
#define TEST_CSDM_H

#pragma once
#include <stdbool.h>

bool test_Dataset_open_blank_csdf(void);
bool test_Dataset_open_blochDecay_base64_csdf(void);
bool test_Dataset_open_emoji_labeled_csdf(void);
bool test_Dataset_open_electric_field_base64_csdf(void);
bool test_Dataset_open_electric_field_none_csdf(void);
bool test_Dataset_open_electric_field_raw_csdfe(void);
bool test_Dataset_open_J_vs_s_csdf(void);

#endif /* TEST_CSDM_H */