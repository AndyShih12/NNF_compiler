CC = gcc
all:
	$(CC) -O2 -Wall -I../ -c -o nnf_to_sdd.o nnf_to_sdd.c
	$(CC) -O2 -Wall -I../ nnf_to_sdd.o -L../build -lsdd -lm -o nnf_to_sdd
	$(CC) -O2 -Wall -I../ -c -o neuralnetwork_compile.o neuralnetwork_compile.c
	$(CC) -O2 -Wall -I../ neuralnetwork_compile.o -L../build -lsdd -lm -o neuralnetwork_compile