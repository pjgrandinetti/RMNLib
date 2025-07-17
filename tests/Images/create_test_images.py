#!/usr/bin/env python3
"""
Create simple test images for RMNLib image import testing.
This script creates basic PNG images that can be used to test the image import functionality.
"""

import os
import sys

def create_simple_png(filename, width, height, color_type='RGB'):
    """Create a simple PNG file with basic patterns."""
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        print("PIL (Pillow) not available. Install with: pip install Pillow")
        return False
    
    if color_type == 'L':  # Grayscale
        # Create a gradient
        array = np.zeros((height, width), dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                array[y, x] = int((x / width) * 255)
        img = Image.fromarray(array, mode='L')
    
    elif color_type == 'RGB':
        # Create RGB pattern
        array = np.zeros((height, width, 3), dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                array[y, x, 0] = int((x / width) * 255)      # Red gradient
                array[y, x, 1] = int((y / height) * 255)     # Green gradient  
                array[y, x, 2] = 128                         # Blue constant
        img = Image.fromarray(array, mode='RGB')
    
    elif color_type == 'RGBA':
        # Create RGBA pattern with transparency
        array = np.zeros((height, width, 4), dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                array[y, x, 0] = int((x / width) * 255)      # Red gradient
                array[y, x, 1] = int((y / height) * 255)     # Green gradient
                array[y, x, 2] = 128                         # Blue constant
                array[y, x, 3] = 255 if (x + y) % 2 == 0 else 128  # Checkerboard alpha
        img = Image.fromarray(array, mode='RGBA')
    
    else:
        print(f"Unsupported color type: {color_type}")
        return False
    
    try:
        img.save(filename)
        print(f"Created {filename} ({width}x{height}, {color_type})")
        return True
    except Exception as e:
        print(f"Failed to save {filename}: {e}")
        return False

def main():
    """Create a set of test images."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Create test images
    images_to_create = [
        ("test_grayscale_small.png", 32, 32, 'L'),
        ("test_rgb_small.png", 32, 32, 'RGB'),
        ("test_rgba_small.png", 32, 32, 'RGBA'),
        ("test_grayscale_medium.png", 128, 128, 'L'),
        ("test_rgb_medium.png", 128, 128, 'RGB'),
        ("test_series_001.png", 64, 64, 'RGB'),
        ("test_series_002.png", 64, 64, 'RGB'),
        ("test_series_003.png", 64, 64, 'RGB'),
    ]
    
    success_count = 0
    for filename, width, height, color_type in images_to_create:
        filepath = os.path.join(script_dir, filename)
        if create_simple_png(filepath, width, height, color_type):
            success_count += 1
    
    print(f"\nCreated {success_count}/{len(images_to_create)} test images successfully.")
    
    if success_count > 0:
        print("\nTest images are ready for RMNLib image import testing.")
        print("Run 'make test' in the RMNLib directory to test image import functionality.")
    else:
        print("\nNo test images created. You may need to install Pillow: pip install Pillow")
        print("Alternatively, you can manually place PNG/JPEG images in this directory.")

if __name__ == "__main__":
    main()
