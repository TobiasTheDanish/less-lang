#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdio.h>

typedef struct FILE_STRUCT {
	FILE* f;
	char* filePath;
	char* modes;
} file_T;

file_T* open_file(char* filePath);
file_T* open_file_m(char* filePath, char* modes);

char* read_file(file_T* file);

void append_file(file_T* file, char* content);

#endif // !FILE_READER_H


