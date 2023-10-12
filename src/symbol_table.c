#include "include/symbol_table.h"
#include <stdlib.h>
#include <string.h>

symbol_table_T* symbol_table_new() {
	symbol_table_T* t = malloc(sizeof(symbol_table_T));
	t->count = 0;
	t->symbols = malloc(sizeof(symbol_T*));

	return t;
}

symbol_T* symbol_table_get(symbol_table_T* table, char* name) {
	for (size_t i = 0; i < table->count; i++) {
		symbol_T* current = table->symbols[i];
		if (strcmp(current->name, name) == 0) {
			return current;
		}
	}

	return NULL;
}

void symbol_table_put(symbol_table_T* table, symbol_T* symbol) {
	if (!symbol_table_contains(table, symbol->name)) {
		symbol->index = table->count;
		table->symbols[table->count++] = symbol;
		table->symbols = realloc(table->symbols, (table->count+1) * sizeof(symbol_T*));
	}
}

bool symbol_table_contains(symbol_table_T* table, char* name) {
	for (size_t i = 0; i < table->count; i++) {
		symbol_T* current = table->symbols[i];
		if (strcmp(current->name, name) == 0) {
			return true;
		}
	}

	return false;
}
