// tests/test_Image.h
#ifndef TEST_IMAGE_H
#define TEST_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

bool test_Image_import_all(void);
bool test_Image_single_file(void);
bool test_Image_grayscale(void);
bool test_Image_rgb(void);
bool test_Image_multiple_images(void);
bool test_Image_dimensions(void);
bool test_Image_memory_management(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_IMAGE_H
