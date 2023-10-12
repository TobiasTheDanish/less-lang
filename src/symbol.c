#include "include/symbol.h"
#include <stdio.h>
#include <stdlib.h>

symbol_T* symbol_new(char* name, size_t size) {
	symbol_T* s = malloc(sizeof(symbol_T));

	s->name = name;
	s->size = size;
	s->index = -1;

	switch (size) {
		case 1:
			s->operand = "BYTE";
			break;
		case 2:
			s->operand = "WORD";
			break;
		case 4:
			s->operand = "DWORD";
			break;
		case 8:
			s->operand = "QWORD";
			break;
		ddefault:
			printf("Invalid symbol size of '%zu' bytes", size);
			exit(1);
	
	}

	return s;
}
