#include <stdio.h>
#include <stdlib.h>
#include "bitmap.h"


/*
 * Read in the location of the pixel array, the image width, and the image 
 * height in the given bitmap file.
 */
void read_bitmap_metadata(FILE *image, int *pixel_array_offset, int *width, int *height) {
    int error = fseek(image, 10, SEEK_SET);
    if (error != 0) {
        fprintf(stderr, "Error: fseek failed");
        exit(1);
    }
    error = fread(pixel_array_offset, sizeof(unsigned int), 4, image);
    if (error != 4) {
        fprintf(stderr, "Error: fread failed");
        exit(1);
    }
    error = fseek(image, 18, SEEK_SET);
    if (error != 0) {
        fprintf(stderr, "Error: fseek failed");
        exit(1);
    }
    error = fread(width, sizeof(unsigned int), 4, image);
    if (error != 4) {
        fprintf(stderr, "Error: fread failed");
        exit(1);
    }
    error = fseek(image, 22, SEEK_SET);
    if (error != 0) {
        fprintf(stderr, "Error: fseek failed");
        exit(1);
    }
    error = fread(height, sizeof(unsigned int), 4, image);
    if (error != 4) {
        fprintf(stderr, "Error: fread failed");
        exit(1);
    }
}

/*
 * Read in pixel array by following these instructions:
 *
 * 1. First, allocate space for m "struct pixel *" values, where m is the 
 *    height of the image.  Each pointer will eventually point to one row of 
 *    pixel data.
 * 2. For each pointer you just allocated, initialize it to point to 
 *    heap-allocated space for an entire row of pixel data.
 * 3. Use the given file and pixel_array_offset to initialize the actual 
 *    struct pixel values. 
 * 4. Return the address of the first "struct pixel *" you initialized.
 */
struct pixel **read_pixel_array(FILE *image, int pixel_array_offset, int width, int height) {
    struct pixel **pixel_array = malloc(sizeof(struct pixel *) * height);
    int error = fseek(image, pixel_array_offset, SEEK_SET);
    if (error != 0) {
        fprintf(stderr, "Error: fseek failed");
        exit(1);
    }
    int i;
    for (i = 0; i < height; i++) {
        struct pixel *p = malloc(sizeof(struct pixel) * width);
        pixel_array[i] = p;
        error = fread(p, sizeof(struct pixel), width, image);
        if (error != width) {
            fprintf(stderr, "Error: fread failed");
            exit(1);
        }
    }
    return pixel_array;
}


/*
 * Print the blue, green, and red colour values of a pixel.
 * You don't need to change this function.
 */
void print_pixel(struct pixel p) {
    printf("(%u, %u, %u)\n", p.blue, p.green, p.red);
}
