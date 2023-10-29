#ifndef SYMBOL_H
#define SYMBOL_H

#include "token.h"
#include <stddef.h>

typedef enum SYMBOL_ENUM {
	SYM_VAR,
	SYM_FUNC,
	SYM_VAR_TYPE,
	SYM_PROP,
} symbol_E;

typedef struct SYMBOL_BASE_STRUCT {
	symbol_E type;
	char* name;
	location_T* loc;
} symbol_T;

typedef struct SYMBOL_VAR_STRUCT {
	symbol_T base;
	symbol_T* type;
	size_t index;
	unsigned char is_mut;
	unsigned char is_assigned;
	unsigned char is_param;
	unsigned char is_const;
	char* const_val;
} symbol_var_T;

typedef struct SYMBOL_PROP_STRUCT {
	symbol_T base;
	size_t offset;
	symbol_T* type;
} symbol_prop_T;

typedef struct SYMBOL_VAR_TYPE_STRUCT {
	symbol_T base;
	char* operand;
	size_t size;
	symbol_T** props;
	size_t prop_count;
} symbol_type_T;

typedef struct SYMBOL_FUNC_STRUCT {
	symbol_T base;
	symbol_T** params;
	size_t param_count;
} symbol_func_T;

symbol_T* symbol_new(char* name, symbol_E type, location_T* loc);

symbol_T* symbol_new_type(char* name, location_T* loc, size_t size, symbol_T** props, size_t count);

symbol_T* symbol_new_prop(char* name, size_t offset, symbol_T* type);

symbol_T* symbol_new_var(char* name, location_T* loc, symbol_T* type, unsigned char is_mut,  unsigned char is_param, unsigned char is_const, char* const_val);

symbol_T* symbol_new_func(char* name, location_T* loc);

bool symbol_is_prop(symbol_T* type, char* propname);

void func_add_param(symbol_func_T* func, symbol_T* param);

char* symbol_to_string(symbol_T* symbol);

char* symbol_get_type_string(symbol_E type);

#endif // !SYMBOL_H

