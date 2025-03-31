#!/usr/bin/python

# This  program takes in a 24 bit color image as input and outputs an 8 bit version with a colormap
# Example usage:
# python3 24Bit.py input.png output_image_Py.png output_image_C.png colormap_Py.txt colormap_C.txt --seed 214 --num_centroids 256 --max_iters 3

import numpy as np
from PIL import Image
import random
import argparse
import ctypes
import time


#C library
lib = ctypes.CDLL('./libkernel.so')
lib.kmeans_clustering.argtypes = [
    np.ctypeslib.ndpointer(dtype=np.float32, flags='C_CONTIGUOUS'),  # pixels
    ctypes.c_int,                                                     # num_pixels
    ctypes.c_int,                                                     # num_centroids
    ctypes.c_int,                                                     # max_iters
    ctypes.c_int,                                                     # seed
    np.ctypeslib.ndpointer(dtype=np.float32, flags='C_CONTIGUOUS'),     # centroids (output)
    np.ctypeslib.ndpointer(dtype=np.int32, flags='C_CONTIGUOUS')        # labels (output)
]
lib.kmeans_clustering.restype = None

def initialize_centroids(seed, num_centroids=256):
    random.seed(seed)
    centroids = np.array([[random.randint(0, 255) for _ in range(3)] for _ in range(num_centroids)], dtype=np.float32)
    return centroids

def find_closest_centroid(pixel, centroids):
    distances = np.linalg.norm(centroids - pixel, axis=1)
    return np.argmin(distances)

def kmeans_clustering_c(image, seed, num_centroids=256, max_iters=5):
    # Convert image to a contiguous array of pixels (float32) with shape (num_pixels, 3)
    pixels = np.array(image).reshape(-1, 3).astype(np.float32)
    num_pixels = pixels.shape[0]
    
    # Allocate output arrays for centroids and labels.
    centroids = np.empty((num_centroids, 3), dtype=np.float32)
    labels = np.empty((num_pixels,), dtype=np.int32)
    
    # Call the C kernel function.
    lib.kmeans_clustering(pixels, num_pixels, num_centroids, max_iters, seed, centroids, labels)
    return centroids, labels

# this is the Python version of the K-means cluserting
def kmeans_clustering(image, seed, num_centroids=256, max_iters=5):
    pixels = np.array(image).reshape(-1, 3)
    centroids = initialize_centroids(seed, num_centroids)
    # print(f'Initialized centroids: {centroids}')

    for i in range(max_iters):
        print(f'Iteration {i + 1}/{max_iters}')
        # Assign pixels to closest centroid
        labels = np.array([find_closest_centroid(pixel, centroids) for pixel in pixels])
        # print(f'Labels: {labels}')
        # Update centroids
        new_centroids = np.zeros_like(centroids)
        for i in range(num_centroids):
            cluster_points = pixels[labels == i]
            if len(cluster_points) > 0:
                new_centroids[i] = cluster_points.mean(axis=0)
        
        # Check for convergence and break early if converged
        if np.all(centroids == new_centroids):
            break
        centroids = new_centroids
    # print(f'Total reassigned: {total_reassigned}')
    return centroids, labels
    
def map_image_to_centroids(image, centroids):
    pixels = np.array(image).reshape(-1, 3)
    labels = np.array([find_closest_centroid(pixel, centroids) for pixel in pixels])
    indexed_image = labels.reshape(image.size[1], image.size[0])
    return indexed_image

def save_output(indexed_image, color_map, output_image_path, output_colormap_path):
    # Save 8-bit indexed image
    indexed_image = Image.fromarray(indexed_image.astype(np.uint8), mode='P')
    indexed_image.putpalette(color_map.flatten().astype(np.uint8))
    indexed_image.save(output_image_path)
    
    # Save color map
    np.savetxt(output_colormap_path, color_map, fmt='%d')

def main():
    parser = argparse.ArgumentParser(description="Perform K-means clustering on an image.")
    parser.add_argument("input_image", help="Path to the input image file")
    parser.add_argument("output_image", help="Path to save the indexed output image")
    parser.add_argument("output_image_C",default="output_image_C", help="Path to save the indexed output image")
    parser.add_argument("output_colormap", help="Path to save the colormap")
    parser.add_argument("output_colormap_C",default="output_colormap_C", help="Path to save the colormap")
    parser.add_argument("--seed", type=int, default=214, help="Random seed for centroid initialization")
    parser.add_argument("--num_centroids", type=int, default=256, help="Number of centroids (default: 256)")
    parser.add_argument("--max_iters", type=int, default=10, help="Maximum iterations for K-means (default: 5)")

    args = parser.parse_args()

    # Load image
    image = Image.open(args.input_image).convert('RGB')

    # Perform K-means clustering
    # Python does not need labels to save the output image, however, we will need labels to compare the student's output
    start_time = time.time()
    centroids, labels = kmeans_clustering(image, args.seed, args.num_centroids, args.max_iters)
    end_time = time.time()
    elapsed_time_python = end_time - start_time
    # Map image to 8-bit indexed image
    indexed_image = map_image_to_centroids(image, centroids)
    # Save outputs
    save_output(indexed_image, centroids, args.output_image, args.output_colormap)

    #time the call to the C kernel 
    start_time = time.time()
    centroids, labels = kmeans_clustering_c(image, args.seed, args.num_centroids, args.max_iters)
    end_time = time.time()
    elapsed_time_C = end_time - start_time
    
    indexed_image = map_image_to_centroids(image, centroids)
    save_output(indexed_image, centroids, args.output_image_C, args.output_colormap_C)

    print("K-means clusterting time (sec) input: %s iterations: %d seed: %d Python: %.6f C: %.6f " % (args.input_image,args.max_iters,args.seed,elapsed_time_python, elapsed_time_C))
    
if __name__ == "__main__":
    main()
