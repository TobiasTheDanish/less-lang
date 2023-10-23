#include "symbol.h"
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct SYMBOL_TABLE_STRUCT {
	char* name;
	size_t level;

	symbol_T** symbols;
	size_t count;

	struct SYMBOL_TABLE_STRUCT* parent;

	struct SYMBOL_TABLE_STRUCT** children;
	size_t child_count;
} symbol_table_T;

symbol_table_T* symbol_table_new(char* name, size_t level, symbol_table_T* parent);

void symbol_table_init_builtins(symbol_table_T* table);

symbol_T* symbol_table_get(symbol_table_T* table, char* name);

void symbol_table_put(symbol_table_T* table, symbol_T* symbol);

bool symbol_table_contains(symbol_table_T* table, char* name);

symbol_table_T* symbol_table_get_child(symbol_table_T* table, char* name);

void symbol_table_put_child(symbol_table_T* table, symbol_table_T* child);

#endif // !SYMBOL_TABLE_H

