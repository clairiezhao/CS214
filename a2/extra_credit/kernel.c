#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void closest_centroid(float r, float g, float b, int num_centroids, float* centroids, double* sum_rgb, int* num_pixels_to_centroid, int* labels);

void kmeans_clustering(float* pixels, int num_pixels, int num_centroids, int max_iters, int seed, float* centroids, int* labels) {

    if(num_centroids == 0 || num_pixels == 0) {
        return;
    }

    //initialize centroids
    srand(seed);
    for(int i = 0; i < num_centroids * 3; i++) {
        centroids[i] = rand() % num_centroids;
    }

    //double array of sum of r,g,b comp of every pixel assigned to each centroid
    double* sum_rgb = (double*)calloc(num_centroids * 3, sizeof(double));
    //int array of number of pixels assigned to each centroid
    int* num_pixels_to_centroid = (int*)calloc(num_centroids, sizeof(int));
    //int that checks if the centroids converged early: 0 for false and 1 for true
    int converge = 0, assigned_pixels = 0;
    float red, green, blue;

    for(int n = 0; n < max_iters; n++) {
        printf("Iteration: %d\n", n);
        for(int i = 0; i < (num_pixels * 3); i += 3) {
            closest_centroid(pixels[i], pixels[i + 1], pixels[i + 2], num_centroids, centroids, sum_rgb, num_pixels_to_centroid, labels);
        }

        //set new centroids and check if centroids converge
        converge = 1;
        for(int i = 0; i < (num_centroids * 3); i += 3) {
            assigned_pixels = num_pixels_to_centroid[i / 3];
            if(assigned_pixels != 0) {
                sum_rgb[i] /= assigned_pixels;
                sum_rgb[i + 1] /= assigned_pixels;
                sum_rgb[i + 2] /= assigned_pixels;
            }
            
            if(converge != 0 && centroids[i] != sum_rgb[i] && centroids[i + 1] != sum_rgb[i + 1] && centroids[i + 2] != sum_rgb[i + 2]) {
                converge = 0;
            }

            centroids[i] = sum_rgb[i];
            centroids[i + 1] = sum_rgb[i + 1];
            centroids[i + 2] = sum_rgb[i + 2];

            //reset new_centroids to 0
            sum_rgb[i] = 0;
            sum_rgb[i + 1] = 0;
            sum_rgb[i + 2] = 0;
            num_pixels_to_centroid[i / 3] = 0;
        }

        //break early if centroids converged
        if(converge == 1)
            break;
    }

    free(sum_rgb);
    free(num_pixels_to_centroid);
}

void closest_centroid(float r, float g, float b, int num_centroids, float* centroids, double* sum_rgb, int* num_pixels_to_centroid, int* labels) {

    int closest_centroid = 0;
    float dist_r, dist_g, dist_b, dist_sq, min_dist;

    dist_r = (r - centroids[0]);
    dist_g = (g - centroids[1]);
    dist_b = (b - centroids[2]);
    min_dist = (dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b);

    for(int i = 3; i < num_centroids * 3; i += 3) {
        //calculate sq of distance of pixel from each centroid
        dist_r = (r - centroids[i]);
        dist_g = (g - centroids[i + 1]);
        dist_b = (b - centroids[i + 2]);
        dist_sq = (dist_r * dist_r) + (dist_g * dist_g) + (dist_b * dist_b);

        if(dist_sq < min_dist) {
            min_dist = dist_sq;
            closest_centroid = i;
        }
    }

    //increment sum of r,g,b comp of pixels assigned to this centroid by r,g,b of curr pixel
    sum_rgb[closest_centroid] += r;
    sum_rgb[closest_centroid + 1] += g;
    sum_rgb[closest_centroid + 2] += b;
    //increment number of pixels assigned to this centroid by 1
    num_pixels_to_centroid[(closest_centroid / 3)] += 1;
    //set label of current pixel
    labels[(closest_centroid / 3)] = closest_centroid;
}