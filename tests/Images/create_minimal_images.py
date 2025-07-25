#!/usr/bin/env python3
"""
Create a minimal test PNG image without external dependencies.
This creates a simple 2x2 pixel PNG file manually.
"""

import os

def create_minimal_png():
    """Create a minimal 2x2 RGB PNG file using raw bytes."""
    # This is a minimal 2x2 RGB PNG with 4 colored pixels
    # Created using: convert -size 2x2 xc:red xc:green xc:blue xc:yellow test_minimal.png
    # Then extracted as hex bytes
    png_data = bytes([
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,  # PNG signature
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,  # IHDR chunk
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,  # Width: 2, Height: 2
        0x08, 0x02, 0x00, 0x00, 0x00, 0x72, 0xB6, 0x0D,  # RGB, no compression
        0x24, 0x00, 0x00, 0x00, 0x16, 0x49, 0x44, 0x41,  # IDAT chunk
        0x54, 0x78, 0x9C, 0x62, 0xF8, 0x0F, 0x00, 0x01,  # Compressed data
        0x01, 0x01, 0x00, 0x18, 0xDD, 0x8D, 0xB4, 0x1C,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,  # IEND chunk
        0xAE, 0x42, 0x60, 0x82
    ])
    
    with open('test_minimal_2x2.png', 'wb') as f:
        f.write(png_data)
    print("Created test_minimal_2x2.png (2x2 RGB)")

def create_minimal_grayscale_png():
    """Create a minimal 2x2 grayscale PNG."""
    # Minimal 2x2 grayscale PNG
    png_data = bytes([
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,  # PNG signature
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,  # IHDR chunk  
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,  # Width: 2, Height: 2
        0x08, 0x00, 0x00, 0x00, 0x00, 0x91, 0x5A, 0xFB,  # Grayscale
        0xDE, 0x00, 0x00, 0x00, 0x0E, 0x49, 0x44, 0x41,  # IDAT chunk
        0x54, 0x78, 0x9C, 0x62, 0x00, 0x02, 0x00, 0x00,  # Compressed data
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,  # IEND chunk
        0x42, 0x60, 0x82
    ])
    
    with open('test_minimal_grayscale.png', 'wb') as f:
        f.write(png_data)
    print("Created test_minimal_grayscale.png (2x2 grayscale)")

if __name__ == "__main__":
    create_minimal_png()
    create_minimal_grayscale_png()
    print("\nMinimal test images created successfully!")
    print("These can be used to test basic image import functionality.")
