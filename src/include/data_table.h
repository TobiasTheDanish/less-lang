#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include "data_constant.h"
#include "token.h"
#include <stddef.h>
typedef struct DATA_TABLE_STRUCT {
	data_const_T** data;
	size_t count;
} data_table_T;

data_table_T* data_table_new();

data_const_T* data_table_get(data_table_T* table, char* value);

size_t data_table_get_index(data_table_T* table, char* value);

void data_table_put(data_table_T* table, token_T* token, char* type);

bool data_table_contains(data_table_T* table, char* value);

#endif // !DATA_TABLE_H
