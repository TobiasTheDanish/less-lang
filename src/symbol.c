#include "include/symbol.h"
#include <stdlib.h>

symbol_T* symbol_new(char* name, size_t size) {
	symbol_T* s = malloc(sizeof(symbol_T));

	s->name = name;
	s->size = size;
	s->index = -1;

	return s;
}
