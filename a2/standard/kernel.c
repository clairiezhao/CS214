#include <stdio.h>
#include <math.h>

void convert_to_YCrCb(unsigned char *rgb_pixels, unsigned char *ycc_pixels, int width, int height) {

    //rgb_pixels is 1D arr with w * h pixels in row major order

    if(width == 0 || height == 0) {
        //set ycc_pixels to 0
    }

    unsigned char red, blue, green;

    for(int i = 0; i < (width * height * 3); i += 3) {
        red = rgb_pixels[i];
        green = rgb_pixels[i + 1];
        blue = rgb_pixels[i + 2];

        // Standard BT.601 YCbCr conversion formula
        ycc_pixels[i] = (char) round(0.299 * red + 0.587 * green + 0.114 * blue); //y
        ycc_pixels[i + 1] = (char) round(128 - 0.168736 * red - 0.331264 * green + 0.5 * blue); //Cb
        ycc_pixels[i + 2] = (char) round(128 + 0.5 * red - 0.418688 * green - 0.081312 * blue); //Cr
    }
}