#include "include/data_table.h"
#include <stdlib.h>
#include <string.h>

data_table_T* data_table_new() {
	data_table_T* t = malloc(sizeof(data_table_T));
	t->count = 0;
	t->data = malloc(sizeof(token_T*));
	
	return t;
}

token_T* data_table_get(data_table_T* table, char* value) {
	for (size_t i = 0; i < table->count; i++) {
		token_T* token = table->data[i];
		if (strcmp(token->value, value) == 0) {
			return token;
		}
	}

	return NULL;
}

size_t data_table_get_index(data_table_T* table, char* value) {
	for (size_t i = 0; i < table->count; i++) {
		token_T* token = table->data[i];
		if (strcmp(token->value, value) == 0) {
			return i;
		}
	}

	return -1;
}

void data_table_put(data_table_T* table, token_T* token) {
	if (!data_table_contains(table, token->value)) {
		table->data[table->count++] = token;
		table->data = realloc(table->data, (table->count + 1) * sizeof(token_T*));
	}
}

bool data_table_contains(data_table_T* table, char* value) {
	for (size_t i = 0; i < table->count; i++) {
		token_T* token = table->data[i];
		if (strcmp(token->value, value) == 0) {
			return true;
		}
	}

	return false;
}
