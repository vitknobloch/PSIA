#pragma once

typedef struct file_t {
	FILE* stream;
	char* name;
	size_t size;
};

char* get_filename();

file_t get_file();