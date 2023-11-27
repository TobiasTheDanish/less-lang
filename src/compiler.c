#include "netinet/in.h"
#include "include/compiler.h"
#include "include/ast_nodes.h"
#include "include/data_constant.h"
#include "include/data_table.h"
#include "include/file_util.h"
#include "include/logger.h"
#include "include/string_util.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REGS 13
char* regs_64bit[MAX_REGS] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "r10", "r11", "r12", "r13", "r14", "r15" };
char* all_regs[MAX_REGS*4] = { 
	"dil", "sil", "dl", "cl", "r8b", "r9b", "al", "r10b","r11b", "r12b", "r13b", "r14b", "r15b",
	"di", "si", "dx", "cx", "r8w", "r9w", "ax", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
	"edi", "esi", "edx", "ecx", "r8d", "r9d", "eax", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
	"rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "r10", "r11", "r12", "r13", "r14", "r15"
};

size_t min(size_t a, size_t b) {
	if (a < b) return a;

	return b;
}
char* get_reg_same_size(char* reg, size_t reg_no) {
	size_t reg_lvl;
	for (size_t i = 0; i < MAX_REGS * 4; i++) {
		if (strcmp(all_regs[i], reg) == 0) {
			reg_lvl = i / MAX_REGS;
			break;
		}
	}

	return all_regs[reg_lvl * MAX_REGS + reg_no];
}

char* get_operand_from_reg(char* reg) {
	size_t reg_lvl;
	for (size_t i = 0; i < MAX_REGS * 4; i++) {
		if (strcmp(all_regs[i], reg) == 0) {
			reg_lvl = i / MAX_REGS;
			break;
		}
	}

	switch (reg_lvl) {
		case 0:
			return "BYTE";
	
		case 1:
			return "WORD";
	
		case 2:
			return "DWORD";

		default:
			return "QWORD";
	}
}

char* match_reg_to_type(size_t size, char* reg) {
	size_t reg_index;
	for (size_t i = 0; i < MAX_REGS * 4; i++) {
		if (strcmp(all_regs[i], reg) == 0) {
			reg_index = i % MAX_REGS;
			break;
		}
	}
	size_t lvl;

	switch (size) {
		case 1:
			lvl = 0;
			break;
		case 2:
			lvl = 1;
			break;
		case 4:
			lvl = 2;
			break;
		default:
			lvl = 3;
			break;
	}

	return all_regs[lvl * MAX_REGS + reg_index];
}

void match_regs(char* reg1, char* reg2, char* buf1, char* buf2) {
	if (reg1 == NULL || reg2 == NULL || buf1 == NULL || buf2 == NULL) {
		log_error(NULL, 1, "Uninitialized strings in match_regs\n");
	}
	int reg1_lvl = -1;
	int reg1_index = -1;
	int reg2_lvl = -1;
	int reg2_index = -1;

	for (size_t i = 0; i < MAX_REGS*4; i++) {
		if (strcmp(all_regs[i], reg1) == 0) {
			reg1_lvl = i/MAX_REGS;
			reg1_index = i%MAX_REGS;
			break;
		}
	}
	if (reg1_lvl < 0 || reg1_index < 0) {
		log_error(NULL, 1, "Could not find '%s' in regs array\n", reg1);
	}

	for (size_t i = 0; i < MAX_REGS*4; i++) {
		if (strcmp(all_regs[i], reg2) == 0) {
			reg2_lvl = i/MAX_REGS;
			reg2_index = i%MAX_REGS;
			break;
		}
	}
	if (reg2_lvl < 0 || reg2_index < 0) {
		log_error(NULL, 2, "Could not find '%s' in regs array\n", reg2);
	}

	if (reg1_lvl == reg2_lvl) {
		strncpy(buf1, reg1, 4);
		strncpy(buf2, reg2, 4);
		return;
	} else if (reg1_lvl < reg2_lvl) {
		strncpy(buf1, all_regs[reg2_lvl * MAX_REGS + reg1_index], 5);
		strncpy(buf2, reg2, 4);
	} else if (reg1_lvl > reg2_lvl) {
		strncpy(buf2, all_regs[reg1_lvl * MAX_REGS + reg2_index], 5);
		strncpy(buf1, reg1, 4);
	}
}

char* compile_array_element(compiler_T* c, ast_node_T* node, size_t reg_no, symbol_T* parent_sym);
char* compile_func_call(compiler_T* c, ast_node_T* node);
void compile_syscall(compiler_T* c, ast_node_T* node);
void compile_if(compiler_T* c, ast_node_T* node);
void compile_expr(compiler_T* c, ast_node_T* node);

compiler_T* compiler_new(ast_node_T* program, symbol_table_T* s_table, data_table_T* data_table, char* output_file, unsigned char debug) {
	compiler_T* c = malloc(sizeof(compiler_T));
	c->program = program;
	c->file = open_file_m(output_file, "w");
	c->s_table = s_table;
	c->data_table = data_table;
	c->stack_pointer = 0;
	c->mem_pointer = 0;
	c->debug = debug;

	return c;
}

// prop : ID DOT ID ; 
char* compile_prop(compiler_T* c, ast_node_T* node, size_t reg_no) {
	log_debug(c->debug, "compile prop\n");
	ast_prop_T* prop = (ast_prop_T*) node;

	//log_todo("Implement properties to be more than just idents\n");

	symbol_var_T* prop_sym = (symbol_var_T*)prop->parent_sym;
	
	if (symbol_is_prop(prop_sym->type, prop->prop->value)) {
		if (prop->node == NULL) {
			symbol_type_T* prop_type = (symbol_type_T*)symbol_get_prop_type(prop_sym->type, prop->prop->value);
			char* reg = prop_type->regs[reg_no];
			size_t offset = symbol_get_prop_offset(prop_sym->type, prop->prop->value);

			append_file(c->file, "    ;-- mov prop --\n");
			char str[50];
			snprintf(str, 50, "    xor %s, %s\n", regs_64bit[reg_no], regs_64bit[reg_no]);
			append_file(c->file, str);
			snprintf(str, 50, "    mov %s, %s [rbp+%zu]\n", reg, prop_type->operand, (8 * (prop_sym->index+1))+2);
			append_file(c->file, str);
			snprintf(str, 50, "    add %s, %zu\n", reg, offset);
			append_file(c->file, str);

			if (!prop->is_pointer) {
				char* reg2 = prop_type->regs[6];
				snprintf(str, 50, "    mov %s, %s\n", reg2, reg);
				append_file(c->file, str);
				snprintf(str, 50, "    mov %s, [%s]\n", reg, reg2);
				append_file(c->file, str);
			}

			return reg;
		} else if (prop->node->type == AST_ARRAY_ELEMENT) {
			return compile_array_element(c, prop->node, reg_no, prop->parent_sym);
		} else if (prop->node->type == AST_PROP) {
			return compile_prop(c, prop->node, reg_no);
		}
	} else {
		log_error(node->loc, 1, "Invalid property '%s' for type '%s'\n", prop->prop->value, prop->parent_sym->name);
	}

	return NULL;
}

char* compile_op(compiler_T* c, ast_node_T* node, char* reg1, char* reg2) {
	char* res = malloc(sizeof(char) * 5);
	char buf1[5] = {0};
	char buf2[5] = {0};
	match_regs(reg1, reg2, buf1, buf2);
	log_debug(c->debug, "compile op\n");
	ast_op_T* op = (ast_op_T*) node;

	char str[50];
	switch (op->t->type) {
        case T_PLUS:
			append_file(c->file, "    ; -- plus --\n");
			snprintf(str, 50, "    add %s, %s\n", buf1, buf2);
			append_file(c->file, str);
			strncpy(res, buf1, 5);
			break;
        case T_MINUS:
			append_file(c->file, "    ; -- minus --\n");
			snprintf(str, 50, "    sub %s, %s\n", buf1, buf2);
			append_file(c->file, str);
			strncpy(res, buf1, 5);
			break;
        case T_MULTIPLY:
			append_file(c->file, "    ; -- multiply --\n");
			char* sized_buf1 = match_reg_to_type(8, buf1);
			char* sized_buf2 = match_reg_to_type(8, buf2);
			snprintf(str, 50, "    imul %s, %s\n", sized_buf1, sized_buf2);
			append_file(c->file, str);
			strncpy(res, buf1, 5);
			break;
        case T_DIVIDE:
			// division puts the result into rax
			{
				char* sized_buf1 = match_reg_to_type(8, buf1);
				char* sized_buf2 = match_reg_to_type(8, buf2);
				append_file(c->file, "    ; -- divide --\n");
				if (strcmp(sized_buf2, "rdx") == 0) {
					char* next_reg = get_reg_same_size(sized_buf2, 3);
					snprintf(str, 50, "    mov %s, %s\n", next_reg, sized_buf2);
					append_file(c->file, str);
					sized_buf2 = next_reg;
				}
				snprintf(str, 50, "    mov rax, %s\n", sized_buf1);
				append_file(c->file, str);
				append_file(c->file, "    cqo\n");
				snprintf(str, 50, "    idiv %s\n", sized_buf2);
				append_file(c->file, str);
				snprintf(str, 50, "    mov %s, rax\n", sized_buf1);
				append_file(c->file, str);
				strncpy(res, buf1, 5);
			}
			break;
        case T_MODULUS:
			// division puts the remainder into rdx, therefore we return rdx
			{
				char* sized_buf1 = match_reg_to_type(8, buf1);
				char* sized_buf2 = match_reg_to_type(8, buf2);
				append_file(c->file, "    ; -- modulus --\n");
				if (strcmp(sized_buf2, "rdx") == 0) {
					char* next_reg = get_reg_same_size(sized_buf2, 3);
					snprintf(str, 50, "    mov %s, %s\n", next_reg, sized_buf1);
					append_file(c->file, str);
					sized_buf2 = next_reg;
				}
				snprintf(str, 50, "    mov rax, %s\n", sized_buf1);
				append_file(c->file, str);
				append_file(c->file, "    cqo\n");
				snprintf(str, 50, "    idiv %s\n", sized_buf2);
				append_file(c->file, str);
				snprintf(str, 50, "    mov %s, rdx\n", sized_buf1);
				append_file(c->file, str);
				strncpy(res, buf1, 5);
			}
			break;

		default:
			log_error(node->loc, 1, "Unknown operand in binary operation '%s'.\n", token_get_name(op->t->type));
	}

	c->stack_pointer -= 1;
	return res;
}

char* compile_value(compiler_T* c, ast_node_T* node, size_t reg_no) {
	log_debug(c->debug, "compile value\n");
	ast_value_T* value = (ast_value_T*) node;

	switch (value->t->type) {
		case T_INTEGER:
			{
				append_file(c->file, "    ; -- mov value ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				char* reg_64 = regs_64bit[reg_no];
				append_file(c->file, "    xor ");
				append_file(c->file, reg_64);
				append_file(c->file, ", ");
				append_file(c->file, reg_64);
				append_file(c->file, "\n");

				symbol_type_T* int_type = (symbol_type_T*) value->type_sym;
				char* reg = int_type->regs[reg_no];

				append_file(c->file, "    mov ");
				append_file(c->file, reg);
				append_file(c->file, ", ");
				append_file(c->file, value->t->value);
				append_file(c->file, "\n");
				c->stack_pointer += 1;
				return reg;
			}

		case T_IDENT:
			{
				append_file(c->file, "    ; -- mov value ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				char* reg_64 = regs_64bit[reg_no];
				append_file(c->file, "    xor ");
				append_file(c->file, reg_64);
				append_file(c->file, ", ");
				append_file(c->file, reg_64);
				append_file(c->file, "\n");

				symbol_T* sym = symbol_table_get(c->s_table, value->t->value);

				switch (sym->type) {
					case SYM_VAR:
						{
							symbol_var_T* var_sym = (symbol_var_T*) sym;
							symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
							char* reg = var_type->regs[reg_no];
							append_file(c->file, "    mov ");

							char str[40];
							if (!var_sym->is_const) {
								snprintf(str, 40, "%s, %s [rbp+%zu]\n",reg, var_type->operand, (8 * (var_sym->index+1))+2);
							} else if (var_sym->is_const) {
								size_t index = data_table_get_index(c->data_table, var_sym->const_val);
								snprintf(str, 40, "%s, %s [const%zu]\n", reg, var_type->operand, index);
							}

							append_file(c->file, str);
							return reg;
						}

					default:
						log_error(node->loc, 1, "Unexpected symbol type, expected %s, found %s.\n", symbol_get_type_string(SYM_VAR), symbol_get_type_string(sym->type));
				}
				break;
			}

		case T_POINTER:
			{
				append_file(c->file, "    ; -- mov value ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				char* reg_64 = regs_64bit[reg_no];
				append_file(c->file, "    xor ");
				append_file(c->file, reg_64);
				append_file(c->file, ", ");
				append_file(c->file, reg_64);
				append_file(c->file, "\n");

				symbol_T* sym = symbol_table_get(c->s_table, value->t->value);
				switch (sym->type) {
					case SYM_VAR:
						{
							symbol_var_T* var_sym = (symbol_var_T*) sym;
							symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
							char* reg = var_type->regs[reg_no];
							append_file(c->file, "    mov ");

							char str[40];
							if (!var_sym->is_const) {
								snprintf(str, 40, "%s, %s rbp+%zu\n",reg, var_type->operand, (8 * (var_sym->index+1))+2);
							} else if (var_sym->is_const) {
								size_t index = data_table_get_index(c->data_table, var_sym->const_val);
								snprintf(str, 40, "%s, %s const%zu\n", reg, var_type->operand, index);
							}

							append_file(c->file, str);
							return reg;
						}

					default:
						log_error(node->loc, 1, "Unexpected symbol type, expected %s, found %s.\n", symbol_get_type_string(SYM_VAR), symbol_get_type_string(sym->type));
				}
				break;
			}

		case T_STRING:
			{
				append_file(c->file, "    ; -- mov value ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				char* reg_64 = regs_64bit[reg_no];
				append_file(c->file, "    xor ");
				append_file(c->file, reg_64);
				append_file(c->file, ", ");
				append_file(c->file, reg_64);
				append_file(c->file, "\n");

				symbol_type_T* type = (symbol_type_T*) value->type_sym;
				char* reg = type->regs[reg_no];

				size_t index = data_table_get_index(c->data_table, value->t->value);
				append_file(c->file, "    mov ");
				char str[40];
				snprintf(str, 40, "%s, %s const%zu\n", reg, type->operand, index);
				append_file(c->file, str);
				return reg;
			}

		case T_CHAR: 
			{
				append_file(c->file, "    ; -- mov value ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				char* reg_64 = regs_64bit[reg_no];
				append_file(c->file, "    xor ");
				append_file(c->file, reg_64);
				append_file(c->file, ", ");
				append_file(c->file, reg_64);
				append_file(c->file, "\n");

				symbol_type_T* type = (symbol_type_T*) value->type_sym;
				char* reg = type->regs[reg_no];

				append_file(c->file, "    mov ");
				char str[40];
				snprintf(str, 40, "%s, %d\n", reg, *value->t->value);
				append_file(c->file, str);
				return reg;
			}

		default:
			log_error(node->loc, 1, "Unreachable code in compile_value\n");
	}

	return NULL;
}

char* compile_bin_op(compiler_T* c, ast_node_T* node, size_t reg_no) {
	log_debug(c->debug, "compile bin op\n");
	ast_bin_op_T* bin_op = (ast_bin_op_T*) node;
	char* reg1;
	char* reg2;
	switch (bin_op->rhs->type) {
		case AST_BIN_OP:
			reg1 = compile_bin_op(c, bin_op->rhs, (reg_no+1)%MAX_REGS);
			break;

		case AST_VALUE:
			reg1 = compile_value(c, bin_op->rhs, (reg_no+1)%MAX_REGS);
			break;

		case AST_PROP:
			reg1 = compile_prop(c, bin_op->rhs, (reg_no+1)%MAX_REGS);
			/*
			char* reg3 = get_reg_same_size(reg1, (reg_no+2)%MAX_REGS);
			char str[50];
			snprintf(str, 50, "    mov %s, %s\n", reg3, reg1);
			append_file(c->file, str);
			snprintf(str, 50, "    mov %s, [%s]\n", reg1, reg3);
			append_file(c->file, str);
			*/
			break;

		default:
			log_error(bin_op->rhs->loc, 1, "Unexpected node in bin_op rhs. Found: %s.\n", ast_get_name(bin_op->rhs->type));
	}


	switch (bin_op->lhs->type) {
		case AST_VALUE:
			reg2 = compile_value(c, bin_op->lhs, reg_no);
			break;

		case AST_PROP:
			reg2 = compile_prop(c, bin_op->lhs, reg_no);
			/*
			char* reg3 = get_reg_same_size(reg2, (reg_no+2)%MAX_REGS);
			char str[50];
			snprintf(str, 50, "    mov %s, %s\n", reg3, reg2);
			append_file(c->file, str);
			snprintf(str, 50, "    mov %s, [%s]\n", reg2, reg3);
			append_file(c->file, str);
			*/
			break;

		default:
			log_error(bin_op->lhs->loc, 1, "Unexpected node in bin_op lhs. Found: %s.\n", ast_get_name(bin_op->rhs->type));
	}
	return compile_op(c, bin_op->op, reg2, reg1);
}

// array_element : ID LSQUARE (array_element | IDENT | INTEGER | bin_op | prop) RSQUARE ;
char* compile_array_element(compiler_T* c, ast_node_T* node, size_t reg_no, symbol_T* parent_sym) {
	log_debug(c->debug, "compile array element\n");
	ast_array_element_T* arr = (ast_array_element_T*) node;

	symbol_type_T* elem_type;
	symbol_type_T* type_sym;
	char mem_location[15];
	size_t offset = 0;
	if (parent_sym == NULL) {
		symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);
		if (arr_sym == NULL) {
			log_error(node->loc, 1, "Uninitialized symbol '%s' used in compile_array_element\n", arr->ident->value);
		}
		elem_type = (symbol_type_T*)arr_sym->elem_type;
		type_sym = (symbol_type_T*)arr_sym->type;
		snprintf(mem_location, 15, "[rbp+%zu]", (8 * (arr_sym->index+1))+2);
	} else {
		symbol_var_T* parent = (symbol_var_T*)parent_sym;
		symbol_prop_T* prop_sym = (symbol_prop_T*) symbol_get_prop(parent->type, arr->ident->value);
		if (prop_sym == NULL) {
			log_error(node->loc, 1, "Invalid property '%s' for type '%s'\n", arr->ident->value, parent_sym->name);
		}
		elem_type = (symbol_type_T*)prop_sym->elem_type;
		type_sym = (symbol_type_T*)prop_sym->type;
		offset = prop_sym->offset;
		snprintf(mem_location, 15, "[rbp+%zu]", (8 * (parent->index+1))+2);
	}

	append_file(c->file, "    ; -- load array element --\n");
	char str[50];
	char* reg;

	switch (arr->offset->type) {
		case AST_ARRAY_ELEMENT:
			reg = compile_array_element(c, arr->offset, (reg_no + 1) % MAX_REGS, NULL);
			ast_array_element_T* arr2 = (ast_array_element_T*)arr->offset;
			symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr2->ident->value);

			char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[(reg_no + 1) % MAX_REGS];
			char* temp = regs_64bit[(reg_no + 2) % MAX_REGS];
			char str[50];
			snprintf(str, 50, "    mov %s, %s\n", temp, reg);
			append_file(c->file, str);
			snprintf(str, 50, "    xor %s, %s\n", reg, reg);
			append_file(c->file, str);
			snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
			append_file(c->file, str);
			break;
		case AST_BIN_OP:
			reg = compile_bin_op(c, arr->offset, (reg_no + 1) % MAX_REGS);
			break;
		case AST_VALUE:
			reg = compile_value(c, arr->offset, (reg_no + 1) % MAX_REGS);
			break;
		case AST_PROP:
			reg = compile_prop(c, arr->offset, (reg_no + 1) % MAX_REGS);
			snprintf(str, 50, "    mov %s, [%s]\n", reg, reg);
			append_file(c->file, str);
			break;

		default:
			log_error(node->loc, 1, "%s cannot be used to index into array.\n", ast_get_name(arr->offset->type));
	}

	snprintf(str, 50,  "    xor %s, %s\n", regs_64bit[reg_no], regs_64bit[reg_no]);
	append_file(c->file, str);
	snprintf(str, 50, "    mov %s, %s %s\n", regs_64bit[reg_no], type_sym->operand, mem_location);
	if (parent_sym != NULL) {
		append_file(c->file, str);
		snprintf(str, 50, "    add %s, %zu\n", regs_64bit[reg_no], offset);
	}
	append_file(c->file, str);
	snprintf(str, 50, "    add %s, 8\n", regs_64bit[reg_no]);
	append_file(c->file, str);
	snprintf(str, 50, "    imul %s, %zu\n", regs_64bit[(reg_no + 1) % MAX_REGS], elem_type->size);
	append_file(c->file, str);
	snprintf(str, 50, "    add %s, %s\n", regs_64bit[reg_no], regs_64bit[(reg_no + 1) % MAX_REGS]);
	append_file(c->file, str);

	return regs_64bit[reg_no];
}

// array : ID LSQUARE INTEGER RSQUARE ;
char* compile_array(compiler_T* c, ast_node_T* node, size_t reg_no) {
	log_debug(c->debug, "compile array\n");
	ast_array_T* arr = (ast_array_T*) node;
	symbol_type_T* elem_type = (symbol_type_T*) arr->elem_type;

	char* reg = regs_64bit[reg_no];
	append_file(c->file, "    ; -- init array --\n");
	char str[50];
	snprintf(str, 50, "    mov QWORD [mem+%zu], %s\n", (c->mem_pointer), arr->len->value);
	append_file(c->file, str);
	snprintf(str, 50, "    mov %s, mem+%zu\n", reg, c->mem_pointer);
	append_file(c->file, str);

	c->mem_pointer += (atoi(arr->len->value) * elem_type->size) + 8;
	c->stack_pointer += 1;

	return reg;
}

void compile_dump(compiler_T* c, ast_node_T* node) {
	log_debug(c->debug, "compile dump\n");
	ast_dump_T* dump = (ast_dump_T*) node;

	append_file(c->file, "    ; -- dump --\n");

	switch (dump->value->type) {
		case AST_BIN_OP:
			compile_bin_op(c, dump->value, 0);
			break;

		case AST_VALUE:
			compile_value(c, dump->value, 0);
			break;

		case AST_PROP:
			compile_prop(c, dump->value, 0);
			break;

		case AST_ARRAY_ELEMENT:
			{
				char* reg2 = compile_array_element(c, dump->value, 0, NULL);
				ast_array_element_T* arr = (ast_array_element_T*)dump->value;
				symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);

				char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[0];
				char* temp = regs_64bit[6];
				char str[50];
				snprintf(str, 50, "    mov %s, %s\n", temp, reg2);
				append_file(c->file, str);
				snprintf(str, 50, "    xor %s, %s\n", reg2, reg2);
				append_file(c->file, str);
				snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
				append_file(c->file, str);
			}
			break;

		default:
			log_error(dump->value->loc, 1, "Unexpected node in dump. Found: %s, expects: %s or %s.\n", ast_get_name(dump->value->type), ast_get_name(AST_BIN_OP), ast_get_name(AST_VALUE));
	}

	if (c->stack_pointer < 1) {
		log_error(node->loc, 1, "Stack pointer to small for dump operation.\n");
	}
	
	append_file(c->file, "\n    call  _dump\n");
	c->stack_pointer -= 1;
}

void compile_cond_op(compiler_T* c, ast_node_T* node, char* reg1, char* reg2) {
	log_debug(c->debug, "Compile cond op\n");
	ast_cond_op_T* cond_op = (ast_cond_op_T*) node;
	append_file(c->file, "    xor rax, rax\n");

	char buf1[5] = {0};
	char buf2[5] = {0};
	match_regs(reg1, reg2, buf1, buf2);
	char str[30];
	snprintf(str, 30, "    cmp %s, %s\n", buf1, buf2);

	switch (cond_op->t->type) {
		case T_EQUALS:
			append_file(c->file, "    ; -- equals --\n");
			append_file(c->file, str);
			append_file(c->file, "    sete al\n");
			break;
		case T_NOT_EQUALS:
			append_file(c->file, "    ; -- not equals --\n");
			append_file(c->file, str);
			append_file(c->file, "    setne al\n");
			break;
		case T_LESS:
			append_file(c->file, "    ; -- less than --\n");
			append_file(c->file, str);
			append_file(c->file, "    setl al\n");
			break;
		case T_GREATER:
			append_file(c->file, "    ; -- greater than --\n");
			append_file(c->file, str);
			append_file(c->file, "    setg al\n");
			break;

		default:
			log_error(cond_op->base.loc, 1, "Unreachable code in compile_cond_op\n");

	}
}

void compile_logical(compiler_T* c, ast_node_T* node) {
	//log_error(NULL, 1, "Logical op not implemented for move yet\n");
	ast_logical_op_T* log = (ast_logical_op_T*) node;
	append_file(c->file, "    pop rbx\n");

	switch (log->t->type) {
		case T_AND:
			append_file(c->file, "    and al, bl\n");
			break;

		case T_OR:
			append_file(c->file, "    or al, bl\n");
			break;

		default:
			log_error(log->base.loc, 1, "Unreachable code in compile_logical\n");
	}
}

// conditional : (value | bin_op | prop) cond_op (value | bin_op | prop | array_element) ;
void compile_conditional(compiler_T* c, ast_node_T* node) {
	ast_cond_T* cond = (ast_cond_T*) node;
	char* reg1;
	char* reg2;

	if (cond->lhs->type == AST_BIN_OP) {
		reg1 = compile_bin_op(c, cond->lhs, 0);
	} else if(cond->lhs->type == AST_PROP) {
		reg1 = compile_prop(c, cond->lhs, 0);
	} else if(cond->lhs->type == AST_ARRAY_ELEMENT) {
		reg1 = compile_array_element(c, cond->lhs, 0, NULL);
		ast_array_element_T* arr = (ast_array_element_T*)cond->lhs;
		symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);

		char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[0];
		char* temp = regs_64bit[6];
		char str[50];
		snprintf(str, 50, "    mov %s, %s\n", temp, reg1);
		append_file(c->file, str);
		snprintf(str, 50, "    xor %s, %s\n", reg1, reg1);
		append_file(c->file, str);
		snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
		append_file(c->file, str);
	} else {
		reg1 = compile_value(c, cond->lhs, 0);
	}

	if (cond->rhs->type == AST_BIN_OP) {
		reg2 = compile_bin_op(c, cond->rhs, 1);
	} else if(cond->rhs->type == AST_PROP) {
		reg2 = compile_prop(c, cond->rhs, 1);
	} else if(cond->rhs->type == AST_ARRAY_ELEMENT) {
		reg2 = compile_array_element(c, cond->rhs, 1, NULL);
		ast_array_element_T* arr = (ast_array_element_T*)cond->rhs;
		symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);

		char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[1];
		char* temp = regs_64bit[6];
		char str[50];
		snprintf(str, 50, "    mov %s, %s\n", temp, reg2);
		append_file(c->file, str);
		snprintf(str, 50, "    xor %s, %s\n", reg2, reg2);
		append_file(c->file, str);
		snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
		append_file(c->file, str);
		reg2 = end;
	} else {
		reg2 = compile_value(c, cond->rhs, 1);
	}

	compile_cond_op(c, cond->op, reg1, reg2);

	if (cond->logical != NULL) {
		log_debug(c->debug, "conditional with logical op\n");
		append_file(c->file, "    push rax\n");
		compile_conditional(c, cond->cond);
		compile_logical(c, cond->logical);
	}
}

void compile_block(compiler_T* c, ast_node_T* node) {
	ast_block_T* block = (ast_block_T*) node;

	for (size_t i = 0; i < block->count; i++) {
		log_debug(c->debug, "Block expr #%u type: %s\n", i, ast_get_name(block->expressions[i]->type));
		compile_expr(c, block->expressions[i]);
	}
}

void compile_else(compiler_T* c, ast_node_T* node) {
	//log_error(NULL, 1, "Compile else not implemented for move yet\n");
	ast_else_T* elze = (ast_else_T*) node;
	char str[50];
	snprintf(str, 50, ".else_%zu:\n", elze->index);
	append_file(c->file, str);
	switch (elze->block->type) {
		case AST_IF:
			compile_if(c, elze->block);
			break;
		case AST_BLOCK:
			compile_block(c, elze->block);
			break;

		default:
			log_error(elze->block->loc, 1, "Unexpected ast type for compile_else. Found: %s\n", ast_get_name(elze->block->type));

	}
}

// while : WHILE conditional block ;
void compile_while(compiler_T* c, ast_node_T* node) {
	log_debug(c->debug, "compile while\n");
	ast_while_T* while_node = (ast_while_T*) node;

	char str[30];
	snprintf(str, 30, "    jmp .W_end_%zu\n", while_node->index);
	append_file(c->file, str);

	snprintf(str, 30, ".W_%zu:\n", while_node->index);
	append_file(c->file, str);

	compile_block(c, while_node->block);

	snprintf(str, 30, ".W_end_%zu:\n", while_node->index);
	append_file(c->file, str);

	compile_conditional(c, while_node->cond);
	append_file(c->file, "    test al, al\n");

	snprintf(str, 30, "    jnz .W_%zu\n", while_node->index);
	append_file(c->file, str);
	log_debug(c->debug, "compile while END\n");
}

void compile_if(compiler_T* c, ast_node_T* node) {
	ast_if_T* if_node = (ast_if_T*) node;

	compile_conditional(c, if_node->cond);
	
	append_file(c->file, "    test al, al\n");
	if (if_node->elze == NULL) {
		char str[50];
		snprintf(str, 50, "    jz .end_%zu\n", if_node->index);
		append_file(c->file, str);
	} else {
		char str[50];
		snprintf(str, 50, "    jz .else_%zu\n", if_node->index);
		append_file(c->file, str);
	}

	compile_block(c, if_node->block);
	
	if (if_node->elze != NULL) {
		char str[50];
		snprintf(str, 50, "    jmp .end_%zu\n", if_node->index);
		append_file(c->file, str);
		compile_else(c, if_node->elze);
	}

	char str[50];
	snprintf(str,50, ".end_%zu:\n", if_node->index);
	append_file(c->file, str);
}

// attribute : ID ASSIGN (value | array_element | prop | bin_op | array) COMMA?
void compile_prop_init(compiler_T* c, ast_node_T* node, symbol_T* type, size_t base) {
	ast_attribute_T* attr = (ast_attribute_T*)node;

	size_t offset = symbol_get_prop_offset(type, attr->name->value);
	symbol_type_T* prop_type = (symbol_type_T*)symbol_get_prop_type(type, attr->name->value);

	char* reg = prop_type->regs[0];
	if (attr->value->type == AST_PROP) {
		compile_prop(c, attr->value, 0);
	} else if (attr->value->type == AST_ARRAY_ELEMENT) {
		reg = compile_array_element(c, attr->value, 0, NULL);
		ast_array_element_T* arr = (ast_array_element_T*)attr->value;
		symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);

		char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[0];
		char* temp = regs_64bit[6];
		char str[50];
		snprintf(str, 50, "    mov %s, %s\n", temp, reg);
		append_file(c->file, str);
		snprintf(str, 50, "    xor %s, %s\n", reg, reg);
		append_file(c->file, str);
		snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
		append_file(c->file, str);
	} else if (attr->value->type == AST_ARRAY) {
		compile_array(c, attr->value, 0);
	} else if (attr->value->type == AST_BIN_OP) {
		compile_bin_op(c, attr->value, 0);
	} else if (attr->value->type == AST_VALUE) {
		compile_value(c, attr->value, 0);
	} else {
		log_error(attr->value->loc, 1, "Unexpected node type as prop initialization, found '%s'.\n", ast_get_name(attr->value->type));
	}

	char str[50];
	snprintf(str, 50, "    mov %s [mem+%zu], %s\n",prop_type->operand, base + offset, reg);
	append_file(c->file, str);
}

// struct_init : ID LCURLY (attribute)* RCURLY ;
char* compile_struct_init(compiler_T* c, ast_node_T* node, size_t reg_no) {
	ast_struct_init_T* structure = (ast_struct_init_T*) node;
	symbol_type_T* symbol = (symbol_type_T*)symbol_table_get(c->s_table, structure->struct_name);

	append_file(c->file, "    ; -- init struct ");
	append_file(c->file, structure->struct_name);
	append_file(c->file, " --\n");

	size_t base_addr = c->mem_pointer;

	for (size_t i = 0; i < structure->attr_count; i++) {
		compile_prop_init(c, structure->attributes[i], (symbol_T*)symbol, base_addr);
	}

	c->mem_pointer += symbol->size;

	char* reg = symbol->regs[reg_no];
	char str[50];
	snprintf(str, 50, "    mov %s, mem+%zu\n", reg, base_addr);
	append_file(c->file, str);

	return reg;
}

// assign : (ID | array_element) ASSIGN (value | bin_op | array | array_element | prop) ;
void compile_assign(compiler_T* c, ast_node_T* node) {
	ast_assign_T* a = (ast_assign_T*) node;
	symbol_T* sym = symbol_table_get(c->s_table, a->ident->value);
	if (sym == NULL && a->lhs->type != AST_PROP) {
		log_error(node->loc, 1, "Uninitialized symbol '%s' used in compile_assign\n", a->ident->value);
	}

	if (sym == NULL) {
		ast_prop_T* p = (ast_prop_T*)a->lhs;
		symbol_var_T* parent = (symbol_var_T*)p->parent_sym;
		sym = symbol_get_prop(parent->type, p->prop->value);
	}

	char* reg;
	log_debug(c->debug, "Compile rhs\n");
	if (a->value->type == AST_VALUE) {
		reg = compile_value(c, a->value, 0);
	} else if (a->value->type == AST_BIN_OP) {
		reg = compile_bin_op(c, a->value, 0);
	} else if (a->value->type == AST_PROP) {
		reg = compile_prop(c, a->value, 0);
	} else if (a->value->type == AST_ARRAY) {
		reg = compile_array(c, a->value, 0);
	} else if (a->value->type == AST_STRUCT_INIT) {
		reg = compile_struct_init(c, a->value, 0);
	} else if (a->value->type == AST_ARRAY_ELEMENT) {
		reg = compile_array_element(c, a->value, 0, NULL);
		ast_array_element_T* arr = (ast_array_element_T*)a->value;
		symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);

		char* end = ((symbol_type_T*)arr_sym->elem_type)->regs[0];
		char* temp = regs_64bit[6];
		char str[50];
		snprintf(str, 50, "    mov %s, %s\n", temp, reg);
		append_file(c->file, str);
		snprintf(str, 50, "    xor %s, %s\n", reg, reg);
		append_file(c->file, str);
		snprintf(str, 50, "    mov %s, [%s]\n", end, temp);
		append_file(c->file, str);
		reg = end;
	} else if (a->value->type == AST_SYSCALL) {
		compile_syscall(c, a->value);
		symbol_type_T* t;
		if (sym->type == SYM_VAR) {
			symbol_var_T* v = (symbol_var_T*) sym;
			t = (symbol_type_T*)v->type;
		} else {
			symbol_prop_T* p = (symbol_prop_T*) sym;
			t = (symbol_type_T*)p->type;
		}
		char str[40];
		char* reg_64 = regs_64bit[0];
		reg = t->regs[0];
		snprintf(str, 40, "    xor %s, %s\n", reg_64, reg_64);
		append_file(c->file, str);
		snprintf(str, 40, "    mov %s, %s\n", reg_64, regs_64bit[6]);
		append_file(c->file, str);
	} else if (a->value->type == AST_FUNC_CALL) {
		reg = compile_func_call(c, a->value);
	} else {
		log_error(a->value->loc, 1, "Unexpected ast type for rhs of compile_assign. Found: %s\n", ast_get_name(a->base.type));
	}

	log_debug(c->debug, "Compile lhs %s\n", a->ident->value);
	switch (sym->type) {
		case SYM_PROP:
			if (a->lhs->type == AST_PROP) {
				((ast_prop_T*)a->lhs)->is_pointer = 1;
				char* reg2 = compile_prop(c, a->lhs, 2);
				char buf1[5];
				char buf2[5];
				match_regs(reg, reg2, buf1, buf2);
				char* operand = get_operand_from_reg(buf2);

				append_file(c->file, "    ;-- assign prop --\n");
				char str[50];
				snprintf(str, 50, "    mov %s [%s], %s\n", operand, buf2, buf1);
				append_file(c->file, str);
			} else { 
				log_error(node->loc, 1, "Something is seriously wrong with assignment of properties: symbol: %s\n", sym->name);
			}
			break;

		case SYM_VAR:
			{
				symbol_var_T* var_sym = (symbol_var_T*) sym;
				char str[40];
				if (a->lhs->type == AST_ARRAY_ELEMENT) {
					symbol_type_T* var_type = (symbol_type_T*) var_sym->elem_type;
					char* reg2 = compile_array_element(c, a->lhs, 2, NULL);
					char* sized_reg = match_reg_to_type(var_type->size, reg);

					snprintf(str, 40, "    mov %s [%s], %s\n",var_type->operand, reg2, sized_reg);
					append_file(c->file, str);
				} else if (a->lhs->type == AST_VALUE) {
					symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
					char* sized_reg = match_reg_to_type(var_type->size, reg);
					append_file(c->file, "    mov ");
					append_file(c->file, var_type->operand);
					append_file(c->file, " [rbp+");
					snprintf(str, 40, "%zu], %s\n", (8 * (var_sym->index+1))+2, sized_reg);
					append_file(c->file, str);
				}
			}
			break;

		default:
			log_error(node->loc, 1, "Unexpected symbol type in assignment. Found %s, expected %s or %s.\n", symbol_get_type_string(sym->type), symbol_get_type_string(SYM_VAR), symbol_get_type_string(SYM_PROP));
	}

}

// var_decl : LET assign SEMI ;
void compile_var_decl(compiler_T* c, ast_node_T* node) {
	ast_var_decl_T* decl = (ast_var_decl_T*) node;

	compile_assign(c, decl->assign);
}

// sys_arg : (bin_op | value);
void compile_sys_arg(compiler_T* c, ast_node_T* node) {
	switch (node->type) {

        case AST_BIN_OP:
			compile_bin_op(c, node, 6);
			break;
        case AST_VALUE:
			compile_value(c, node, 6);
			break;
        case AST_PROP:
			compile_prop(c, node, 6);
			break;
		case AST_ARRAY_ELEMENT:
			compile_array_element(c, node, 6, NULL);
			break;

		default:
			log_error(node->loc, 1, "Invalid node type in compile_sys_arg. Found: %s\n", ast_get_name(node->type));
	}
}

// syscall : LPAREN (sys_arg (COMMA sys_arg)*)? RPAREN;
void compile_syscall(compiler_T* c, ast_node_T* node) {
	log_debug(c->debug, "Compile syscall\n");
	ast_syscall_T* syscall = (ast_syscall_T*) node;

	if (syscall->count < 1) {
		log_error(node->loc, 1, "Not enough params for syscall\n");
	} else if (syscall->count > 7) {
		log_error(node->loc, 1, "Too many params for syscall\n");
	}

	append_file(c->file, "    ; -- syscall --\n");

	if (syscall->count > 1) {
		append_file(c->file, "    xor rdi, rdi\n");
		compile_sys_arg(c, syscall->params[1]);
		append_file(c->file, "    mov rdi, rax\n");
	}

	if (syscall->count > 2) {
		append_file(c->file, "    xor rsi, rsi\n");
		compile_sys_arg(c, syscall->params[2]);
		append_file(c->file, "    mov rsi, rax\n");
	}

	if (syscall->count > 3) {
		append_file(c->file, "    xor rdx, rdx\n");
		compile_sys_arg(c, syscall->params[3]);
		append_file(c->file, "    mov rdx, rax\n");
	}

	if (syscall->count > 4) {
		append_file(c->file, "    xor r10, r10\n");
		compile_sys_arg(c, syscall->params[4]);
		append_file(c->file, "    mov r10, rax\n");
	}
	
	if (syscall->count > 5) {
		append_file(c->file, "    xor r8, r8\n");
		compile_sys_arg(c, syscall->params[5]);
		append_file(c->file, "    mov r8, rax\n");
	}

	if (syscall->count > 6) {
		append_file(c->file, "    xor r9, r9\n");
		compile_sys_arg(c, syscall->params[6]);
		append_file(c->file, "    mov r9, rax\n");
	}

	compile_sys_arg(c, syscall->params[0]);

	append_file(c->file, "    syscall\n");
}

// func_param : ID COLON ID ;
void compile_func_param(compiler_T* c, token_T* token, size_t param_index) {
	symbol_T* sym = symbol_table_get(c->s_table, token->value);
	if (sym == NULL) {
		log_error(token->loc, 1, "Could not find symbol '%s' in symbol_table\n", token->value);
	}
	if (sym->type != SYM_VAR) {
		log_error(token->loc, 1, "Unexpected symbol type found when compiling function params. Found %s, expected %s\n", symbol_get_type_string(sym->type), symbol_get_type_string(SYM_VAR));
	}

	symbol_var_T* param_sym = (symbol_var_T*) sym;
	symbol_type_T* param_type = (symbol_type_T*) param_sym->type;
	log_debug(c->debug, "param type name: %s\n", param_type->base.name);
	char str[50];
	snprintf(str, 49, "    mov %s [rbp+%zu], %s\n",
			param_type->operand,
			(8 * (param_sym->index+1))+2, 
			param_type->regs[param_index]
		);
	log_debug(c->debug, "\t%s\n", str);
	append_file(c->file, str);
}

// func_decl : FUNC ID LPAREN (func_param (COMMA func_param)*)? RPAREN block ;
void compile_func_decl(compiler_T* c, ast_node_T* node) {
	ast_func_decl_T* decl = (ast_func_decl_T*)node;
	log_debug(c->debug, "func decl name: %s\n", decl->ident->value);

	append_file(c->file, "_");
	append_file(c->file, decl->ident->value);
	append_file(c->file, ":\n");
	append_file(c->file, "    push rbp\n");
	append_file(c->file, "    mov rbp, rsp\n");
	if (strcmp(decl->ident->value, "main") == 0) {
		append_file(c->file, "    sub rsp, ");
		char str[20];
		snprintf(str, 20, "%zu\n", (c->s_table->child_count+1)*8);
		append_file(c->file, str);
	}

	c->s_table = symbol_table_get_child(c->s_table, decl->ident->value);
	for (unsigned char i = decl->param_count; i > 0; i--) {
		compile_func_param(c, decl->params[i-1], i-1);
	}
	
	compile_block(c, decl->block);

	c->s_table = c->s_table->parent;
	if (strcmp(decl->ident->value, "main") == 0) {
		append_file(c->file, "    ; exit program\n");
		append_file(c->file, "    mov rdi, 0\n");
		append_file(c->file, "    mov rax, 60\n");
		append_file(c->file, "    syscall\n");
	} else {
		append_file(c->file, "    mov rsp, rbp\n");
		append_file(c->file, "    pop rbp\n");
		append_file(c->file, "    ret\n");
	}
}

// arg : (value | bin_op | array_element | prop) ;
void compile_func_arg(compiler_T* c, ast_node_T* node, size_t param_index, symbol_T* param) {
	switch (node->type) {
		case AST_VALUE:
			compile_value(c, node, param_index);
			break;

		case AST_BIN_OP:
			compile_bin_op(c, node, param_index);
			break;

		case AST_ARRAY_ELEMENT:
			compile_array_element(c, node, param_index, NULL);
			break;

		case AST_PROP:
			compile_prop(c, node, param_index);
			break;

		case AST_PROGRAM:
		case AST_BLOCK:
		case AST_EXPR:
		case AST_SYSCALL:
		case AST_VAR_DECL:
		case AST_CONST_DECL:
		case AST_FUNC_DECL:
		case AST_FUNC_CALL:
		case AST_ASSIGN:
		case AST_WHILE:
		case AST_IF:
		case AST_ELSE:
		case AST_CONDITIONAL:
		case AST_COND_OP:
		case AST_OP:
		case AST_DUMP:
		case AST_NO_OP:
		case AST_ARRAY_EXPR:
		case AST_ARRAY:
		case AST_LOGICAL_OP:
		case AST_STRUCT_INIT:
		case AST_ATTRIBUTE:
			log_error(node->loc, 1, "Invalid node in function call argument. Found %s\n", ast_get_name(node->type));
	}
}

// func_call : ID LPAREN (arg (COMMA arg)*)? RPAREN ;
char* compile_func_call(compiler_T* c, ast_node_T* node) {
	ast_func_call_T* call = (ast_func_call_T*) node;
	char* name = call->ident->value;
	log_debug(c->debug, "Function: %s\n", name);
	symbol_func_T* func_sym = (symbol_func_T*) symbol_table_get(c->s_table, name);
	
	for (int i = call->param_count - 1; i >= 0; i--) {
		compile_func_arg(c, call->params[i], i, func_sym->params[i]);
	}
	append_file(c->file, "    call _");
	append_file(c->file, name);
	append_file(c->file, "\n");

	if (call->param_count > 6) {
		char str[20];
		snprintf(str, 20, "    add rsp, %zu\n", (8 * (call->param_count - 6)));
		append_file(c->file, str);
	}
	if (func_sym->ret_type != NULL) {
		symbol_type_T* ret_type = (symbol_type_T*)func_sym->ret_type;
		char* reg = ret_type->regs[3];
		char str[40];
		snprintf(str, 40, "    mov %s, %s\n", reg, ret_type->regs[6]);
		append_file(c->file, str);
		return reg;
	}

	return "";
}

/*
// array_expr : array_element (ASSIGN | op) (value | bin_op | array) ;
void compile_array_expr(compiler_T* c, ast_node_T* node) {
	log_error(NULL, 1, "Compile array_expr not implemented for move yet\n");
	log_debug(c->debug, "compile array expr \n");
	ast_array_expr_T* expr = (ast_array_expr_T*) node;

	if (expr->rhs->type == AST_VALUE) {
		compile_value(c, expr->rhs, 0);
	} else if (expr->rhs->type == AST_BIN_OP) {
		compile_bin_op(c, expr->rhs, 0);
	} else if (expr->rhs->type == AST_ARRAY) {
		compile_array(c, expr->rhs, 0);
	} else if (expr->rhs->type == AST_ARRAY_ELEMENT) {
		compile_array_element(c, expr->rhs, 0, NULL);
		append_file(c->file,  "    pop rax\n");
		append_file(c->file,  "    push QWORD [rax]\n");
	} else if (expr->rhs->type == AST_ARRAY_EXPR) {
		compile_array_expr(c, expr->rhs);
	} else {
		log_error(expr->rhs->loc, 1, "Unexpected node type as rhs in array_expr. Found %s\n", ast_get_name(expr->rhs->type));
	}

	compile_array_element(c, expr->array_element, 0, NULL);

	if (expr->op == NULL) {
		append_file(c->file, "    pop rax\n");
		append_file(c->file, "    pop rsi\n");
		append_file(c->file, "    mov QWORD [rax], rsi\n");
	} else {
		append_file(c->file, "    pop rax\n");
		append_file(c->file, "    push QWORD [rax]\n");
		compile_op(c, expr->op, "", "");
	}
}
*/

// expr : syscall SEMI | if | while | var_decl | const_decl | array_expr SEMI | assign SEMI | bin_op SEMI | dump SEMI | func_decl | func_call SEMI ;
void compile_expr(compiler_T* c, ast_node_T* node) {
	ast_expr_T* expr = (ast_expr_T*) node;
	log_debug(c->debug, "expr child type: %s\n", ast_get_name(expr->child->type));

	switch (expr->child->type) {
		case AST_SYSCALL:
			compile_syscall(c, expr->child);
			break;
		case AST_BIN_OP:
			compile_bin_op(c, expr->child, 0);
			break;
		case AST_IF:
			compile_if(c, expr->child);
			break;
		case AST_DUMP:
			compile_dump(c, expr->child);
			break;
		case AST_VAR_DECL:
			compile_var_decl(c, expr->child);
			break;
		case AST_ASSIGN:
			compile_assign(c, expr->child);
			break;
		case AST_WHILE:
			compile_while(c, expr->child);
			break;
		case AST_FUNC_DECL:
			compile_func_decl(c, expr->child);
			break;
		case AST_FUNC_CALL:
			compile_func_call(c, expr->child);
			break;
		case AST_ARRAY_EXPR:
			log_error(NULL, 1, "Array expressions should be faced out, compile_expr\n");
			//compile_array_expr(c, expr->child);
			break;
		case AST_CONST_DECL:
			break;

		case AST_STRUCT_INIT:
		case AST_ATTRIBUTE:
		case AST_ARRAY:
		case AST_PROP:
		case AST_ARRAY_ELEMENT:
		case AST_ELSE:
		case AST_BLOCK:
		case AST_EXPR:
		case AST_OP:
		case AST_VALUE:
		case AST_NO_OP:
		case AST_PROGRAM:
		case AST_CONDITIONAL:
		case AST_COND_OP:
		case AST_LOGICAL_OP:
			log_error(expr->child->loc, 1, "Unexpected node in expr, found: %s.\n", ast_get_name(expr->child->type));
			break;
	}
}


void compile_program(compiler_T* c, ast_node_T* node) {
	if (node->type == AST_PROGRAM) {
		ast_program_T* program = (ast_program_T*) node;

		for (size_t i = 0; i < program->count; i++) {
			log_debug(c->debug, "Program expr #%u type: %s\n", i, ast_get_name(program->expressions[i]->type));
			compile_expr(c, program->expressions[i]);
		}
	} else {
		log_error(node->loc, 1, "Unexpected node to start program.\n Found: %s, expects: %s.\n", ast_get_name(node->type), ast_get_name(AST_PROGRAM));
	}
}

void compile(compiler_T* c) {
	append_file(c->file, "BITS 64\n");
	append_file(c->file, "global _start\n");
	append_file(c->file, "section .text\n");

	append_file(c->file, "_dump:\n");
	append_file(c->file, "		mov     r9, -3689348814741910323\n");
	append_file(c->file, "		sub     rsp, 40\n");
	append_file(c->file, "		mov     BYTE [rsp+31], 10\n");
	append_file(c->file, "		lea     rcx, [rsp+30]\n");
	append_file(c->file, ".L2:\n");
	append_file(c->file, "		mov     rax, rdi\n");
	append_file(c->file, "		lea     r8, [rsp+32]\n");
	append_file(c->file, "		mul     r9\n");
	append_file(c->file, "		mov     rax, rdi\n");
	append_file(c->file, "		sub     r8, rcx\n");
	append_file(c->file, "		shr     rdx, 3\n");
	append_file(c->file, "		lea     rsi, [rdx+rdx*4]\n");
	append_file(c->file, "		add     rsi, rsi\n");
	append_file(c->file, "		sub     rax, rsi\n");
	append_file(c->file, "		add     eax, 48\n");
	append_file(c->file, "		mov     BYTE [rcx], al\n");
	append_file(c->file, "		mov     rax, rdi\n");
	append_file(c->file, "		mov     rdi, rdx\n");
	append_file(c->file, "		mov     rdx, rcx\n");
	append_file(c->file, "		sub     rcx, 1\n");
	append_file(c->file, "		cmp     rax, 9\n");
	append_file(c->file, "		ja      .L2\n");
	append_file(c->file, "		lea     rax, [rsp+32]\n");
	append_file(c->file, "		mov     edi, 1\n");
	append_file(c->file, "		sub     rdx, rax\n");
	append_file(c->file, "		xor     eax, eax\n");
	append_file(c->file, "		lea     rsi, BYTE [rsp+32+rdx]\n");
	append_file(c->file, "		mov     rdx, r8\n");
	append_file(c->file, "		mov     rax, 1\n");
	append_file(c->file, "		syscall\n");
	append_file(c->file, "		add     rsp, 40\n");
	append_file(c->file, "		ret\n\n");
	append_file(c->file, "_start:\n");
	append_file(c->file, "    ; init global variables\n");
	append_file(c->file, "    ; NOT CURRENTLY IMPLEMENTED\n");
	append_file(c->file, "    ; start execution\n");
	append_file(c->file, "    jmp _main\n");

	compile_program(c, c->program);


	append_file(c->file, "segment .bss\n");
	append_file(c->file, "    mem: resq 8000\n");

	append_file(c->file, "segment .data\n");
	for (size_t i = 0; i < c->data_table->count; i++) {
		data_const_T* data = c->data_table->data[i];

		if (strcmp(data->type, "string") == 0) {
			char s[25];
			snprintf(s, 25, "    const%zu: db ", i);
			append_file(c->file, s);
			decode_escaped_characters(data->t->value);
			for (int j = 0; data->t->value[j] != '\0'; j++) {
				char ch[6];
				snprintf(ch, 6, "%d, ", data->t->value[j]);
				append_file(c->file, ch);
			}
			append_file(c->file, "0\n");
		} else if (strcmp(data->type, "i32") == 0) {
			char s[25];
			snprintf(s, 25, "    const%zu: dq ", i);
			append_file(c->file, s);
			append_file(c->file, data->t->value);
			append_file(c->file, "\n");
		} else if (strcmp(data->type, "i16") == 0) {
			char s[25];
			snprintf(s, 25, "    const%zu: dq ", i);
			append_file(c->file, s);
			append_file(c->file, data->t->value);
			append_file(c->file, "\n");
		} else if (strcmp(data->type, "i8") == 0) {
			char s[25];
			snprintf(s, 25, "    const%zu: dq ", i);
			append_file(c->file, s);
			append_file(c->file, data->t->value);
			append_file(c->file, "\n");
		}


	}

	close_file(c->file);

	log_info("Compilation finished\n");
}
