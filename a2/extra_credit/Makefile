libkernel.so: kernel.c
	gcc -O3 -lm -shared -o libkernel.so kernel.c

clean:
	rm -f *.so *.o

run:
	python3 24Bit.py schiele.jpg schiele_output_Py.png schiele_output_C.png colormap_Py.txt colormap_C.txt --seed 214 --num_centroids 256 --max_iters 3