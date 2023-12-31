#include "include/symbol_table.h"
#include "include/symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

symbol_table_T* symbol_table_new(char* name, size_t level, symbol_table_T* parent) {
	symbol_table_T* t = malloc(sizeof(symbol_table_T));
	t->name = name;
	t->level = level;
	t->count = 0;
	t->symbols = malloc(sizeof(symbol_T*));
	t->parent = parent;
	t->children = malloc(sizeof(symbol_table_T*));
	t->child_count = 0;

	return t;
}

void symbol_table_init_builtins(symbol_table_T* table) {
	symbol_table_put(table, symbol_new_type("i8", NULL, 1, NULL, 0));
	symbol_table_put(table, symbol_new_type("i16", NULL, 2, NULL, 0));
	symbol_table_put(table, symbol_new_type("i32", NULL, 4, NULL, 0));
	symbol_table_put(table, symbol_new_type("i64", NULL, 8, NULL, 0));
	symbol_type_T* array_type = (symbol_type_T*) symbol_new_type("array", NULL, 8, NULL, 0);
	array_type->prop_count = 1;
	array_type->props = malloc(sizeof(symbol_T));
	array_type->props[0] = symbol_new_prop("len", 0, symbol_table_get(table, "i32"), NULL);
	symbol_table_put(table, (symbol_T*)array_type);
	symbol_type_T* str_type = (symbol_type_T*) symbol_new_type("string", NULL, 8, NULL, 0);
	str_type->prop_count = 2;
	str_type->props = malloc(sizeof(symbol_T)*2);
	str_type->props[0] = symbol_new_prop("len", 0, symbol_table_get(table, "i32"), NULL);
	str_type->props[1] = symbol_new_prop("chars", 0, symbol_table_get(table, "array"), symbol_table_get(table, "i8"));
	symbol_table_put(table, (symbol_T*)str_type);
}

symbol_T* symbol_table_get(symbol_table_T* table, char* name) {
	for (size_t i = 0; i < table->count; i++) {
		symbol_T* current = table->symbols[i];
		if (strcmp(current->name, name) == 0) {
			return current;
		}
	}

	if (table->parent != NULL) {
		return symbol_table_get(table->parent, name);
	}

	return NULL;
}

size_t symbol_table_calc_index(symbol_table_T* table) {
	if (table->parent != NULL) {
		return table->count + symbol_table_calc_index(table->parent);
	} 

	return table->count;
}

void symbol_table_put(symbol_table_T* table, symbol_T* symbol) {
	if (!symbol_table_contains(table, symbol->name)) {
		if (symbol->type == SYM_VAR) {
			symbol_var_T* s = (symbol_var_T*) symbol;
			s->index = symbol_table_calc_index(table);
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
		return symbol_table_contains(table->parent, name);
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

void symbol_table_print(symbol_table_T* table) {
	printf("\nScope '%s', level %zu\n", table->name, table->level);
	if (table->parent != NULL) 
	{
		printf("Parent: '%s'\n", table->parent->name);
	}
	else 
	{
		printf("Enclosing scope: None\n");
	}
	for (size_t i = 0; i < table->count; i++)
	{
		printf("\tSymbol #%lu: %s\n", (i+1), symbol_to_string(table->symbols[i]));
	}
}
