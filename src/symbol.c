#include "include/symbol.h"
#include "include/logger.h"
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
			s->regs[0] = "dil";
			s->regs[1] = "sil";
			s->regs[2] = "dl";
			s->regs[3] = "cl";
			s->regs[4] = "r8b";
			s->regs[5] = "r9b";
			s->regs[6] = "al";
			s->regs[7] = "r10b";
			break;
		case 2:
			s->operand = "WORD";
			s->regs[0] = "di";
			s->regs[1] = "si";
			s->regs[2] = "dx";
			s->regs[3] = "cx";
			s->regs[4] = "r8w";
			s->regs[5] = "r9w";
			s->regs[6] = "ax";
			s->regs[7] = "r10w";
			break;
		case 4:
			s->operand = "DWORD";
			s->regs[0] = "edi";
			s->regs[1] = "esi";
			s->regs[2] = "edx";
			s->regs[3] = "ecx";
			s->regs[4] = "r8d";
			s->regs[5] = "r9d";
			s->regs[6] = "eax";
			s->regs[7] = "r10d";
			break;
		default:
			s->operand = "QWORD";
			s->regs[0] = "rdi";
			s->regs[1] = "rsi";
			s->regs[2] = "rdx";
			s->regs[3] = "rcx";
			s->regs[4] = "r8";
			s->regs[5] = "r9";
			s->regs[6] = "rax";
			s->regs[7] = "r10";
			break;
	}

	return (symbol_T*) s;
}

symbol_T* symbol_new_prop(char* name, size_t offset, symbol_T* type, symbol_T* elem_type) {
	symbol_prop_T* prop = malloc(sizeof(symbol_prop_T));

	prop->base = *symbol_new(name, SYM_PROP, NULL);
	prop->offset = offset;
	prop->type = type;
	prop->elem_type = elem_type;

	return (symbol_T*) prop;
}

symbol_T* symbol_new_var(char* name, location_T* loc, symbol_T* type, unsigned char is_mut, unsigned char is_param, unsigned char is_const, char* const_val) {
	symbol_var_T* var = malloc(sizeof(symbol_var_T));

	var->base = *symbol_new(name, SYM_VAR, loc);
	var->type = type;
	var->elem_type = NULL;
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
	func->ret_type = NULL;

	return (symbol_T*) func;
}

bool symbol_is_prop(symbol_T* type, char* propname) {
	//printf("[symbol.c]: Checking if '%s' is prop of '%s'\n", propname, type->name);
	if (type == NULL) {
		log_error(NULL, 1, "Cannot find property from NULL symbol.\n");
	}
	if (type->type != SYM_VAR_TYPE) {
		log_error(NULL, 1, "Cannot check if symbol is prop on non type symbol: %s.\n", type->name);
	}
	symbol_type_T* t = (symbol_type_T*)type;
	for (size_t i = 0; i < t->prop_count; i++) {
		//printf("[symbol.c]: Comparing '%s' to '%s'\n", propname, t->props[i]->name);
		if (strcmp(t->props[i]->name, propname) == 0) {
			return true;
		}
	}

	return false;
}

size_t symbol_get_prop_offset(symbol_T* type, char* propname) {
	if (type == NULL) {
		log_error(NULL, 1, "Cannot find property from NULL symbol.\n");
	}

	if (type->type != SYM_VAR_TYPE) {
		log_error(NULL, 1, "Cannot get prop offset from non type symbol: %s.\n", type->name);
	}
	symbol_type_T* t = (symbol_type_T*)type;

	for (size_t i = 0; i < t->prop_count; i++) {
		if (strcmp(t->props[i]->name, propname) == 0) {
			symbol_prop_T* p = (symbol_prop_T*)t->props[i];
			return p->offset;
		}
	}

	return -1;
}

symbol_T* symbol_get_prop(symbol_T* type, char* propname) {
	if (type == NULL) {
		log_error(NULL, 1, "Cannot find property from NULL symbol.\n");
	}

	if (type->type != SYM_VAR_TYPE) {
		log_error(NULL, 1, "Cannot get prop from non type symbol: %s.\n", type->name);
	}
	symbol_type_T* t = (symbol_type_T*)type;

	for (size_t i = 0; i < t->prop_count; i++) {
		if (strcmp(t->props[i]->name, propname) == 0) {
			return t->props[i];
		}
	}

	return NULL;
}

symbol_T* symbol_get_prop_type(symbol_T* type, char* propname) {
	if (type == NULL) {
		log_error(NULL, 1, "Cannot find property from NULL symbol.\n");
	}

	if (type->type != SYM_VAR_TYPE) {
		log_error(NULL, 1, "Cannot get prop type from non type symbol: %s.\n", type->name);
	}
	symbol_type_T* t = (symbol_type_T*)type;

	for (size_t i = 0; i < t->prop_count; i++) {
		if (strcmp(t->props[i]->name, propname) == 0) {
			symbol_prop_T* p = (symbol_prop_T*)t->props[i];
			return p->type;
		}
	}

	return NULL;
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
				sprintf(s, "<%s:%zu>", var->base.name,  var->index);
				//sprintf(s, "<%s:%s:%zu>", var->base.name, var->type->name var->index);
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
