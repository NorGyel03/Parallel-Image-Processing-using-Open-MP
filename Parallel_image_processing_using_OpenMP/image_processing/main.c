#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "headers/stb_image.h"
#include "headers/stb_image_write.h"

void image_to_grayscale_critical(unsigned char* image, int width, int height, int channels) {
    #pragma omp parallel for
    for (int i = 0; i < width * height * channels; i += channels) {
        unsigned char r = image[i];
        unsigned char g = image[i + 1];
        unsigned char b = image[i + 2];

        unsigned char local_gray;

        #pragma omp critical
        {
            local_gray = (unsigned char)(0.21 * r + 0.71 * g + 0.07 * b);
        }

        for (int j = 0; j < channels; ++j) {
            image[i + j] = local_gray;
        }
    }
}

void image_blur(unsigned char* image, int width, int height, int channels) {
    float blur_kernel[3][3] = {
        {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
        {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
    };

    #pragma omp parallel for
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            for (int c = 0; c < channels; ++c) {
                float blurred_value = 0.0;

                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int pixel_index = ((y + ky) * width + (x + kx)) * channels + c;
                        blurred_value += image[pixel_index] * blur_kernel[ky + 1][kx + 1];
                    }
                }

                image[(y * width + x) * channels + c] = (unsigned char)blurred_value;
            }
        }
    }
}

void image_rotate_90(unsigned char* image, int width, int height, int channels) {
    unsigned char* rotated_image = (unsigned char*)malloc(width * height * channels);

    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int c = 0; c < channels; ++c) {
                rotated_image[(x * height + (height - y - 1)) * channels + c] = image[(y * width + x) * channels + c];
            }
        }
    }

    memcpy(image, rotated_image, width * height * channels);

    free(rotated_image);
}

void image_invert(unsigned char* image, int width, int height, int channels) {
    #pragma omp parallel for
    for (int i = 0; i < width * height * channels; ++i) {
        image[i] = 255 - image[i];
    }
}


// Global variable for grayscale value
unsigned char gray; // (1) race condition variable

// Global variable for loop index
int i; // (2) race condition variable

// Function with intentional race condition
void image_to_grayscale_race(unsigned char* image, int width, int height, int channels) {
    // OpenMP directive to parallelize the loop
    #pragma omp parallel for
    // Loop over the image data, incrementing by the number of channels at each iteration
    for (i = 0; i < width * height * channels; i += channels) {
        // Extract the RGB values for the current pixel
        unsigned char r = image[i];
        unsigned char g = image[i + 1];
        unsigned char b = image[i + 2];

        // Introduce a race condition by using a global variable to store the grayscale value
        #pragma omp parallel // (2) race condition block
        {
            // Calculate the grayscale value using a weighted sum of RGB values
            gray = (unsigned char)(0.21 * r + 0.71 * g + 0.07 * b);
        }

        // Update all channels with the same grayscale value
        for (int j = 0; j < channels; ++j) { // (4) for loop to update channels
            image[i + j] = gray;
        }
    }
}



int main() {
    int width, height, channels;
    unsigned char* image = stbi_load("Image.JPG", &width, &height, &channels, 0);

    if (!image) {
        fprintf(stderr, "Error loading image.\n");
        return 1;
    }

    if (channels != 3) {
        fprintf(stderr, "Unsupported number of channels. This code expects a 3-channel (RGB) image.\n");
        stbi_image_free(image);
        return 1;
    }

    unsigned char* image_race = (unsigned char*)malloc(width * height * channels);
    memcpy(image_race, image, width * height * channels);

    // Process and save the output for each function
    image_to_grayscale_critical(image_race, width, height, channels);
    stbi_write_png("output_grayscale.png", width, height, channels, image_race, width * channels);

    memcpy(image_race, image, width * height * channels); // Reset image_race to the original image

    image_blur(image_race, width, height, channels);
    stbi_write_png("output_blur.png", width, height, channels, image_race, width * channels);

    memcpy(image_race, image, width * height * channels);

    image_rotate_90(image_race, width, height, channels);
    stbi_write_png("output_rotate_90.png", width, height, channels, image_race, width * channels);

    memcpy(image_race, image, width * height * channels);

    image_invert(image_race, width, height, channels);
    stbi_write_png("output_invert.png", width, height, channels, image_race, width * channels);

    image_to_grayscale_race(image_race, width, height, channels);
    stbi_write_png("output_race_condition.png", width, height, channels, image_race, width * channels);
    
    memcpy(image_race, image, width * height * channels);


    stbi_image_free(image);
    free(image_race);

    return 0;
}
