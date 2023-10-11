#ifndef SYMBOL_H
#define SYMBOL_H

#include <stddef.h>
typedef struct SYMBOL_STRUCT {
	char* name;
	size_t size;
	size_t index;
} symbol_T;

symbol_T* symbol_new(char* name, size_t size);

#endif // !SYMBOL_H

