CC = gcc
all:
	$(CC) -O2 -Wall -I../ -c -o nnf_to_sdd.o nnf_to_sdd.c
	$(CC) -O2 -Wall -I../ nnf_to_sdd.o -L../build -lsdd -lm -o nnf_to_sdd