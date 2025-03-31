libkernel.so: kernel.c
	gcc -O3 -lm -shared -o libkernel.so kernel.c

clean:
	rm -f *.so *.o

run:
	python3 24Bit.py input.png output_image_Py.png output_image_C.png colormap_Py.txt colormap_C.txt --seed 214 --num_centroids 256 --max_iters 3