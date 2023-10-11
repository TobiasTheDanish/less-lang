#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "symbol.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct SYMBOL_TABLE_STRUCT {
	size_t count;
	symbol_T** symbols;
} symbol_table_T;

symbol_table_T* symbol_table_new();

symbol_T* symbol_table_get(symbol_table_T* table, char* name);

void symbol_table_put(symbol_table_T* table, symbol_T* symbol);

bool symbol_table_contains(symbol_table_T* table, char* name);

#endif // !SYMBOL_TABLE_H

