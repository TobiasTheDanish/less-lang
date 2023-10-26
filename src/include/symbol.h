#ifndef SYMBOL_H
#define SYMBOL_H

#include <stddef.h>

typedef enum SYMBOL_ENUM {
	SYM_VAR,
	SYM_FUNC,
	SYM_VAR_TYPE,
} symbol_E;

typedef struct SYMBOL_BASE_STRUCT {
	symbol_E type;
	char* name;
} symbol_T;

typedef struct SYMBOL_VAR_STRUCT {
	symbol_T base;
	symbol_T* type;
	size_t index;
	unsigned char is_param;
} symbol_var_T;

typedef struct SYMBOL_VAR_TYPE_STRUCT {
	symbol_T base;
	char* operand;
	size_t size;
} symbol_type_T;

typedef struct SYMBOL_FUNC_STRUCT {
	symbol_T base;
	symbol_T** params;
	size_t param_count;
} symbol_func_T;

symbol_T* symbol_new(char* name, symbol_E type);

symbol_T* symbol_new_type(char* name, size_t size);

symbol_T* symbol_new_var(char* name, symbol_T* type, unsigned char is_param);

symbol_T* symbol_new_func(char* name);

void func_add_param(symbol_func_T* func, symbol_T* param);

char* symbol_to_string(symbol_T* symbol);

char* symbol_get_type_string(symbol_E type);

#endif // !SYMBOL_H

