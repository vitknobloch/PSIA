#include <stdlib.h>
#include <stdio.h>

#include "my_malloc.h"

void* allocate_memory(size_t size) {
	void* ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "Error allocating memory!\n");
		exit(-1);
	}
	return ptr;
}

void* reallocate_memory(void* ptr, size_t new_size) {
	ptr = realloc(ptr, new_size);
	if (!ptr) {
		fprintf(stderr, "Error reallocating memory!\n");
		exit(-1);
	}
	return ptr;
}

void free_memory(void* ptr) {
	if (ptr) {
		free(ptr);
	}
}