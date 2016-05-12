all: maxsat-serial maxsat-omp maxsat-mpi

maxsat-serial: maxsat-serial.c
	gcc -lm maxsat-serial.c -o maxsat-serial

maxsat-omp: maxsat-omp.c
	gcc -lm -fopenmp maxsat-omp.c -o maxsat-omp

maxsat-mpi: maxsat-mpi.c
	mpicc -lm maxsat-mpi.c -o maxsat-mpi

clean:
	rm maxsat-serial maxsat-omp maxsat-mpi
