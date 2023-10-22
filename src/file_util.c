#include "include/file_util.h"
#include <stdio.h>
#include <stdlib.h>

file_T* open_file(char* filePath) {
	FILE* f = fopen(filePath, "a+");
	if (f == NULL) {
		perror("File not found");
		exit(1);
	}

	file_T* file = malloc(sizeof(file_T));
	file->f = f;
	file->filePath = filePath;
	file->modes = "a+";

	return file;
}

file_T* open_file_m(char* filePath, char* modes) {
	FILE* f = fopen(filePath, modes);
	if (f == NULL) {
		perror("File not found");
		exit(1);
	}

	file_T* file = malloc(sizeof(file_T));
	file->f = f;
	file->filePath = filePath;
	file->modes = "a+";

	return file;
}

void close_file(file_T *file) {
	fclose(file->f);
}

char* read_file(file_T* file) {
	size_t capacity = 100;
	char* contents = calloc(capacity, sizeof(char));
	size_t size = 0;

	while (!feof(file->f)) 
	{
		contents[size] = fgetc(file->f);
		size += 1;

		if (size == capacity) 
		{
			capacity *= 1.5;

			contents = realloc(contents, capacity * sizeof(char));
		}
	}

	fclose(file->f);

	return contents;
}

void append_file(file_T* file, char* content) {
	fprintf(file->f, "%s", content);
}
