#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "my_malloc.h"
#include "file_loader.h"

#define INIT_FILENAME_SIZE 30
#define DIR_SEPARATOR '\\'
#define DIR_SEPARATOR2 '/'

char* get_filename() {
	printf("Enter file to transfer: ");
	int filename_size = INIT_FILENAME_SIZE;
	char* filename = (char*)allocate_memory(filename_size);
	
	int index = 0;
	int c = getc(stdin);
	while (c != '\n' && c > 0) {
		filename[index++] = (char)c;
		if (index >= filename_size - 1) {
			filename_size = filename_size << 1;
			filename = (char*)reallocate_memory(filename, filename_size);
		}
		c = getc(stdin);		
	}

	filename[index++] = '\0';

	filename = (char*)reallocate_memory(filename, index);

	return filename;
}

file_t get_file() {
	file_t file;
	file.name = get_filename();
	file.stream = NULL;
	errno_t err = fopen_s(&file.stream, file.name, "rb");
	file.size = 0;
	if (!file.stream) {		
		fprintf(stderr, "Error opening file %s\n", file.name);
		free_memory(file.name);
		return file;
	}

	if(!fseek(file.stream, 0, SEEK_END))
		file.size = ftell(file.stream);
	rewind(file.stream);

	int r_idx = 0;
	int w_idx = 0;
	while (file.name[r_idx] != '\0') {
		file.name[w_idx] = file.name[r_idx];
		if (file.name[r_idx] == DIR_SEPARATOR || file.name[r_idx] == DIR_SEPARATOR2) {
			w_idx = -1;
		}
		w_idx++;
		r_idx++;
	}
	file.name[w_idx++] = '\0';

	file.name = (char*)reallocate_memory(file.name, w_idx);

	return file;
}