default:
	gcc -c -fPIC kernel.c -o kernel.o
	gcc kernel.o -shared -o libkernel.so -O3 -lm