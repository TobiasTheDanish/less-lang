#include "include/symbol_table.h"
#include "include/symbol.h"
#include <stdlib.h>
#include <string.h>

symbol_table_T* symbol_table_new(char* name, size_t level, symbol_table_T* parent) {
	symbol_table_T* t = malloc(sizeof(symbol_table_T));
	t->name = name;
	t->level = level;
	t->count = 0;
	t->symbols = malloc(sizeof(symbol_T*));
	t->parent = parent;

	return t;
}

void symbol_table_init_builtins(symbol_table_T* table) {
	symbol_table_put(table, symbol_new_type("int", 8));
	symbol_table_put(table, symbol_new_type("string", 8));
}

symbol_T* symbol_table_get(symbol_table_T* table, char* name) {
	for (size_t i = 0; i < table->count; i++) {
		symbol_T* current = table->symbols[i];
		if (strcmp(current->name, name) == 0) {
			return current;
		}
	}

	if (table->parent != NULL) {
		return symbol_table_get(table, name);
	}

	return NULL;
}

void symbol_table_put(symbol_table_T* table, symbol_T* symbol) {
	if (!symbol_table_contains(table, symbol->name)) {
		if (symbol->type == SYM_VAR) {
			symbol_var_T* s = (symbol_var_T*) symbol;
			s->index = table->count;
		}
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

	if (table->parent != NULL) {
		return symbol_table_contains(table, name);
	}

	return false;
}

symbol_table_T* symbol_table_get_child(symbol_table_T* table, char* name) {
	for (size_t i = 0; i < table->child_count; i++) {
		symbol_table_T* current = table->children[i];
		if (strcmp(current->name, name) == 0) {
			return current;
		}
	}

	return NULL;
}

void symbol_table_put_child(symbol_table_T* table, symbol_table_T* child) {
	table->children[table->child_count++] = child;
	table->children = realloc(table->children, (table->child_count+1) * sizeof(symbol_table_T*));
}
