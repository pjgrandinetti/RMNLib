# Test Images for RMNLib Image Import

This directory contains test images for validating the image import functionality of RMNLib.

## Standard Test Image Sources

### 1. USC-SIPI Image Database
**Website**: http://sipi.usc.edu/database/database.php?volume=misc
**Description**: The USC-SIPI Image Database is a collection of digitized images. It is maintained primarily to support research in image processing, image analysis, and machine vision.

**Recommended test images:**
- `lena.512.png` (512x512 grayscale) - Classic test image
- `peppers.png` (512x512 color) - Standard color test image  
- `baboon.png` (512x512 color) - High detail test image
- `boat.512.png` (512x512 grayscale) - Good for testing detail preservation

### 2. OpenCV Test Images
**Website**: https://github.com/opencv/opencv/tree/master/samples/data
**Description**: OpenCV provides various test images in their sample data.

**Recommended downloads:**
- `lena.jpg` - Classic Lena image
- `fruits.jpg` - Multi-colored fruit image
- `pic1.png`, `pic2.png`, etc. - Various test images

### 3. ImageMagick Test Images
**Website**: https://github.com/ImageMagick/ImageMagick/tree/main/images
**Description**: ImageMagick provides various test images for different scenarios.

### 4. Sample Images for Testing Different Formats

#### PNG Images (Lossless)
- Download from: https://sample-videos.com/zip/10/png/
- Good for testing exact pixel preservation

#### JPEG Images (Lossy)
- Download from: https://sample-videos.com/zip/10/jpg/
- Good for testing compression artifacts handling

#### BMP Images (Uncompressed)
- Download from: https://filesamples.com/formats/bmp
- Good for testing raw bitmap handling

### 5. Create Simple Test Images Programmatically

You can also create simple test images using ImageMagick:

```bash
# Create a simple gradient
convert -size 100x100 gradient: test_gradient.png

# Create a simple checkerboard
convert -size 100x100 pattern:checkerboard test_checkerboard.png

# Create solid color images
convert -size 50x50 xc:red test_red.png
convert -size 50x50 xc:green test_green.png  
convert -size 50x50 xc:blue test_blue.png

# Create grayscale gradient
convert -size 100x100 gradient:black-white test_grayscale.png
```

## Recommended Test Set

For comprehensive testing, include:

1. **test_grayscale.png** - 8-bit grayscale image
2. **test_rgb.png** - 24-bit RGB color image  
3. **test_small.png** - Small image (32x32) for memory tests
4. **test_large.png** - Large image (1024x1024) for performance tests
5. **test_rgba.png** - RGBA image with transparency
6. **test_series_001.png**, **test_series_002.png**, etc. - Multiple images for time series testing

## Usage

The test suite will automatically discover and test all image files in this directory. Supported formats depend on the stb_image library configuration:

- PNG (recommended for testing)
- JPEG/JPG
- BMP  
- TGA
- GIF (first frame only)

## Adding New Test Images

1. Place image files in this directory
2. Use descriptive filenames that indicate the test purpose
3. Include a variety of:
   - Different dimensions (small, medium, large)
   - Different color depths (grayscale, RGB, RGBA)
   - Different file formats
   - Different content types (synthetic patterns, natural images)

## Expected Test Behavior

The test suite validates:
- Successful image loading and parsing
- Correct dimension detection (width × height)
- Proper color channel handling (1, 3, or 4 channels)
- Memory management (no leaks)
- Dataset structure creation
- Multiple image handling (time series)

If the IMAGE_TEST_ROOT directory is empty or doesn't exist, the tests will pass gracefully (skip with warning) rather than fail, allowing the build to succeed even without test images.
