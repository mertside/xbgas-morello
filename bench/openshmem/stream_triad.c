// Description: Simple OpenSHMEM STREAM Triad benchmark
// A simplified adaptation of a basic STREAM Triad kernel.
// We could use as a baseline to evaluate memory bandwidth or latency.

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define N 1000000
#define SCALAR 3.0

double a[N], b[N], c[N];

double get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec * 1e-6;
}

int main() {
	for (int i = 0; i < N; i++) {
		b[i] = 1.0;
		c[i] = 2.0;
	}

	double start = get_time();
	for (int i = 0; i < N; i++) {
		a[i] = b[i] + SCALAR * c[i];
	}
	double end = get_time();

	printf("Time taken: %f seconds\n", end - start);
	printf("Sample result a[0]: %f\n", a[0]);
	return 0;
}