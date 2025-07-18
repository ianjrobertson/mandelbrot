/*
  WebAssembly adaptation of Mandelbrot set generator
  
  Compile with:
  emcc mandelbrot.c -o mandelbrot.js \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_generate_mandelbrot", "_free_pixels", "_get_dimensions"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -O3
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <emscripten.h>

// Structure to hold computed dimensions (for JavaScript to query)
typedef struct {
    int width;
    int height;
} Dimensions;

static Dimensions last_dimensions = {0, 0};

// Calculate the height based on aspect ratio
static int calculate_height(double xmin, double xmax, double ymin, double ymax, int xres) {
    return (int)((xres * (ymax - ymin)) / (xmax - xmin));
}

// Core Mandelbrot iteration function
static int mandelbrot_iterations(double x, double y, int maxiter) {
    double u = 0.0, v = 0.0;
    double u2 = u * u;
    double v2 = v * v;
    int k;
    
    for (k = 1; k < maxiter && (u2 + v2 < 4.0); k++) {
        v = 2 * u * v + y;
        u = u2 - v2 + x;
        u2 = u * u;
        v2 = v * v;
    }
    
    return k;
}

// Main function to generate Mandelbrot set
// Returns pointer to RGBA pixel data
EMSCRIPTEN_KEEPALIVE
unsigned char* generate_mandelbrot(double xmin, double xmax, double ymin, double ymax, 
                                  int maxiter, int xres) {
    // Calculate height maintaining aspect ratio
    int yres = calculate_height(xmin, xmax, ymin, ymax, xres);
    
    // Store dimensions for JavaScript to query
    last_dimensions.width = xres;
    last_dimensions.height = yres;
    
    // Allocate memory for RGBA pixels
    unsigned char* pixels = malloc(xres * yres * 4);
    if (!pixels) return NULL;
    
    // Precompute pixel width and height
    double dx = (xmax - xmin) / xres;
    double dy = (ymax - ymin) / yres;
    
    // Generate the fractal
    for (int j = 0; j < yres; j++) {
        double y = ymax - j * dy;
        for (int i = 0; i < xres; i++) {
            double x = xmin + i * dx;
            
            // Calculate iterations for this point
            int k = mandelbrot_iterations(x, y, maxiter);
            
            // Convert to pixel index
            int idx = (j * xres + i) * 4;
            
            if (k >= maxiter) {
                // Interior - black
                pixels[idx] = 0;     // Red
                pixels[idx + 1] = 0; // Green
                pixels[idx + 2] = 0; // Blue
                pixels[idx + 3] = 255; // Alpha
            } else {
                // Exterior - grayscale based on iteration count
                // Normalize to 0-255 range
                unsigned char intensity = (unsigned char)((k * 255) / maxiter);
                
                pixels[idx] = intensity;     // Red
                pixels[idx + 1] = intensity; // Green
                pixels[idx + 2] = intensity; // Blue
                pixels[idx + 3] = 255;       // Alpha
            }
        }
    }
    
    return pixels;
}

// Enhanced version with color mapping
EMSCRIPTEN_KEEPALIVE
unsigned char* generate_mandelbrot_color(double xmin, double xmax, double ymin, double ymax, 
                                        int maxiter, int xres) {
    int yres = calculate_height(xmin, xmax, ymin, ymax, xres);
    
    last_dimensions.width = xres;
    last_dimensions.height = yres;
    
    unsigned char* pixels = malloc(xres * yres * 4);
    if (!pixels) return NULL;
    
    double dx = (xmax - xmin) / xres;
    double dy = (ymax - ymin) / yres;
    
    for (int j = 0; j < yres; j++) {
        double y = ymax - j * dy;
        for (int i = 0; i < xres; i++) {
            double x = xmin + i * dx;
            int k = mandelbrot_iterations(x, y, maxiter);
            int idx = (j * xres + i) * 4;
            
            if (k >= maxiter) {
                // Interior - black
                pixels[idx] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 255;
            } else {
                // Exterior - color mapping
                double t = (double)k / maxiter;
                
                // HSV-like color mapping
                pixels[idx] = (unsigned char)(255 * sin(t * 3.14159 * 2));     // Red
                pixels[idx + 1] = (unsigned char)(255 * sin(t * 3.14159 * 4)); // Green
                pixels[idx + 2] = (unsigned char)(255 * sin(t * 3.14159 * 6)); // Blue
                pixels[idx + 3] = 255; // Alpha
            }
        }
    }
    
    return pixels;
}

// Function to get the dimensions of the last generated image
EMSCRIPTEN_KEEPALIVE
Dimensions* get_dimensions() {
    return &last_dimensions;
}

// Convenience function to get width
EMSCRIPTEN_KEEPALIVE
int get_width() {
    return last_dimensions.width;
}

// Convenience function to get height
EMSCRIPTEN_KEEPALIVE
int get_height() {
    return last_dimensions.height;
}

// Free the pixel memory
EMSCRIPTEN_KEEPALIVE
void free_pixels(void* ptr) {
    free(ptr);
}

// Quick preview generation (lower quality, faster)
EMSCRIPTEN_KEEPALIVE
unsigned char* generate_mandelbrot_preview(double xmin, double xmax, double ymin, double ymax, 
                                          int maxiter, int xres) {
    // Use lower maxiter for preview
    int preview_maxiter = maxiter / 4;
    if (preview_maxiter < 10) preview_maxiter = 10;
    
    return generate_mandelbrot(xmin, xmax, ymin, ymax, preview_maxiter, xres);
}

// Function to get a specific pixel's iteration count (for debugging)
EMSCRIPTEN_KEEPALIVE
int get_pixel_iterations(double xmin, double xmax, double ymin, double ymax, 
                        int maxiter, int xres, int pixel_x, int pixel_y) {
    int yres = calculate_height(xmin, xmax, ymin, ymax, xres);
    
    if (pixel_x < 0 || pixel_x >= xres || pixel_y < 0 || pixel_y >= yres) {
        return -1; // Out of bounds
    }
    
    double dx = (xmax - xmin) / xres;
    double dy = (ymax - ymin) / yres;
    
    double x = xmin + pixel_x * dx;
    double y = ymax - pixel_y * dy;
    
    return mandelbrot_iterations(x, y, maxiter);
}