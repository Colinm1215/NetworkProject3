# Makefile for main

all:
	gcc -o main main.c helping_structures.c

clean:
	rm -f main