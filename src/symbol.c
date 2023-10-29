#include "include/symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

symbol_T* symbol_new(char* name, symbol_E type, location_T* loc) {
	symbol_T* s = malloc(sizeof(symbol_T));

	s->name = name;
	s->type = type;
	s->loc = loc;

	return s;
}

symbol_T* symbol_new_type(char* name, location_T* loc, size_t size, symbol_T** props, size_t prop_count) {
	symbol_type_T* s = malloc(sizeof(symbol_type_T));

	s->base = *symbol_new(name, SYM_VAR_TYPE, loc);
	s->props = props;
	s->prop_count = prop_count;
	s->size = size;

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

	return (symbol_T*) s;
}

symbol_T* symbol_new_prop(char* name, size_t offset, symbol_T* type) {
	symbol_prop_T* prop = malloc(sizeof(symbol_prop_T));

	prop->base = *symbol_new(name, SYM_PROP, NULL);
	prop->offset = offset;
	prop->type = type;

	return (symbol_T*) prop;
}

symbol_T* symbol_new_var(char* name, location_T* loc, symbol_T* type, unsigned char is_mut, unsigned char is_param, unsigned char is_const, char* const_val) {
	symbol_var_T* var = malloc(sizeof(symbol_var_T));

	var->base = *symbol_new(name, SYM_VAR, loc);
	var->type = type;
	var->index = -1;
	var->is_mut = is_mut;
	var->is_assigned = 0;
	var->is_param = is_param;
	var->is_const = is_const;
	var->const_val = const_val;

	return (symbol_T*) var;
}

symbol_T* symbol_new_func(char* name, location_T* loc) {
	symbol_func_T* func = malloc(sizeof(symbol_func_T));
	symbol_T* base = symbol_new(name, SYM_FUNC, loc);
	func->base = *base;
	func->params = calloc(1, sizeof(symbol_T*));
	func->param_count = 0;
	func->params[0] = (void*) 0;

	return (symbol_T*) func;
}

bool symbol_is_prop(symbol_T* type, char* propname) {
	symbol_type_T* t = (symbol_type_T*)type;

	for (size_t i = 0; i < t->prop_count; i++) {
		if (strcmp(t->props[i]->name, propname) == 0) {
			return true;
		}
	}

	return false;
}

void func_add_param(symbol_func_T* func, symbol_T* param) {
	func->params[func->param_count++] = param;
	func->params = realloc(func->params, (func->param_count+1) * sizeof(symbol_T*));
}

char* symbol_to_string(symbol_T* symbol) {
	char* s = calloc(100, sizeof(char));

	switch (symbol->type) 
	{
		case SYM_VAR_TYPE:
			{
				symbol_type_T* type = (symbol_type_T*) symbol;
				sprintf(s, "<type:%s, props: %zu>", symbol->name, type->prop_count);
			}
			break;

		case SYM_VAR:
			{
				symbol_var_T* var = (symbol_var_T*) symbol;
				sprintf(s, "<%s:%s:%zu>", var->base.name, var->type->name, var->index);
			}
			break;

		case SYM_FUNC:
			{
				symbol_func_T* proc = (symbol_func_T*) symbol;
				sprintf(s, "<%s: func, params: %zu>", proc->base.name, proc->param_count);
			}
			break;

		case SYM_PROP:
			break;
	}
	return s;
}

char* symbol_get_type_string(symbol_E type) {
	char* names[] = {
		"Variable",
		"Function",
		"Type",
		"Property",
	};

	return names[type];
}
