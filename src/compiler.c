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

void compile_if(compiler_T* c, ast_node_T* node);
void compile_expr(compiler_T* c, ast_node_T* node);

compiler_T* compiler_new(ast_node_T* program, symbol_table_T* s_table, data_table_T* data_table, char* output_file) {
	compiler_T* c = malloc(sizeof(compiler_T));
	c->program = program;
	c->file = open_file_m(output_file, "w");
	c->s_table = s_table;
	c->data_table = data_table;
	c->stack_pointer = 0;
	c->mem_pointer = 0;

	return c;
}

// prop : ID DOT ID ; 
void compile_prop(compiler_T* c, ast_node_T* node) {
	log_info("compile prop\n");
	ast_prop_T* prop = (ast_prop_T*) node;

	symbol_var_T* arr_sym = (symbol_var_T*)prop->parent_sym;
	if (symbol_is_prop(arr_sym->type, prop->prop->value)) {
		size_t offset = symbol_get_prop_offset(arr_sym->type, prop->prop->value);

		append_file(c->file, "    xor rax, rax\n");
		char str[50];
		snprintf(str, 50, "    mov rax, QWORD [rbp+%zu]\n", (8 * (arr_sym->index+1))+2);
		append_file(c->file, str);
		snprintf(str, 50, "    add rax, %zu\n", offset);
		append_file(c->file, str);
		append_file(c->file, "    push QWORD [rax]\n");
	} else {
		log_error(node->loc, 1, "Invalid property '%s' for type '%s'\n", prop->prop->value, prop->parent_sym->name);
	}
}

void compile_op(compiler_T* c, ast_node_T* node) {
	log_info("compile op\n");
	ast_op_T* op = (ast_op_T*) node;
	if (c->stack_pointer < 2) {
		log_error(node->loc, 1, "Stack pointer to small for binary operation.\n");
	}

	switch (op->t->type) {
        case T_PLUS:
			append_file(c->file, "    ; -- plus --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    add rax, [rsp]\n");
			append_file(c->file, "    add rsp, 8\n");
			append_file(c->file, "    push rax\n");

			break;
        case T_MINUS:
			append_file(c->file, "    ; -- minus --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    sub rax, [rsp]\n");
			append_file(c->file, "    add rsp, 8\n");
			append_file(c->file, "    push rax\n");
			break;
        case T_MULTIPLY:
			append_file(c->file, "    ; -- multipy --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    imul rax, [rsp]\n");
			append_file(c->file, "    add rsp, 8\n");
			append_file(c->file, "    push rax\n");
			break;
        case T_DIVIDE:
			append_file(c->file, "    ; -- divide --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    cqo\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    idiv rbx\n");
			append_file(c->file, "    push rax\n");
			break;
        case T_MODULUS:
			append_file(c->file, "    ; -- modulus --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    cqo\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    idiv rbx\n");
			append_file(c->file, "    push rdx\n");
			break;

		default:
			log_error(node->loc, 1, "Unknown operand in binary operation '%s'.\n", token_get_name(op->t->type));
	}

	c->stack_pointer -= 1;
}

void compile_value(compiler_T* c, ast_node_T* node) {
	log_info("compile value\n");
	ast_value_T* value = (ast_value_T*) node;

	switch (value->t->type) {
		case T_INTEGER:
			append_file(c->file, "    ; -- push ");
			append_file(c->file, value->t->value);
			append_file(c->file, "--\n");

			append_file(c->file, "    push ");
			append_file(c->file, value->t->value);
			append_file(c->file, "\n");
			c->stack_pointer += 1;
			break;

		case T_IDENT:
			append_file(c->file, "    ; -- push ");
			append_file(c->file, value->t->value);
			append_file(c->file, "--\n");

			symbol_T* sym = symbol_table_get(c->s_table, value->t->value);
			
			switch (sym->type) {
				case SYM_VAR:
					{
						symbol_var_T* var_sym = (symbol_var_T*) sym;
						symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
						append_file(c->file, "    push ");
						append_file(c->file, var_type->operand);

						char str[20];
							if (!var_sym->is_const) {
								snprintf(str, 20, " [rbp+%zu]\n", (var_type->size * (var_sym->index+1))+2);
							} else if (var_sym->is_const) {
								size_t index = data_table_get_index(c->data_table, var_sym->const_val);
								snprintf(str, 20, " [const%zu]\n", index);
							}

						append_file(c->file, str);
						c->stack_pointer += 1;
					}
					break;

				default:
					log_error(node->loc, 1, "Unexpected symbol type, expected %s, found %s.\n", symbol_get_type_string(SYM_VAR), symbol_get_type_string(sym->type));
			}
			break;

		case T_POINTER:
			{
				append_file(c->file, "    ; -- push ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				symbol_T* sym = symbol_table_get(c->s_table, value->t->value);
				switch (sym->type) {
					case SYM_VAR:
						{
							symbol_var_T* var_sym = (symbol_var_T*) sym;
							symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
							append_file(c->file, "    push ");
							append_file(c->file, var_type->operand);

							char str[20];
							if (!var_sym->is_const) {
								snprintf(str, 20, " rbp+%zu\n", (var_type->size * (var_sym->index+1))+2);
							} else if (var_sym->is_const) {
								size_t index = data_table_get_index(c->data_table, var_sym->const_val);
								snprintf(str, 20, " const%zu\n", index);
							}
							append_file(c->file, str);
							c->stack_pointer += 1;
						}
						break;

					default:
						log_error(node->loc, 1, "Unexpected symbol type, expected %s, found %s.\n", symbol_get_type_string(SYM_VAR), symbol_get_type_string(sym->type));
				}
				break;
			}

		case T_STRING:
			{
				append_file(c->file, "    ; -- push ");
				append_file(c->file, value->t->value);
				append_file(c->file, "--\n");

				size_t index = data_table_get_index(c->data_table, value->t->value);
				append_file(c->file, "    push ");
				char str[20];
				snprintf(str, 20, "const%zu\n", index);
				append_file(c->file, str);
				c->stack_pointer += 1;
				break;
			}

		default:
			log_error(node->loc, 1, "Unreachable code in compile_value\n");
	}
}

void compile_bin_op(compiler_T* c, ast_node_T* node) {
	log_info("compile bin op\n");
	ast_bin_op_T* bin_op = (ast_bin_op_T*) node;
	switch (bin_op->rhs->type) {
		case AST_BIN_OP:
			compile_bin_op(c, bin_op->rhs);
			break;

		case AST_VALUE:
			compile_value(c, bin_op->rhs);
			break;

		case AST_PROP:
			compile_prop(c, bin_op->rhs);
			break;

		default:
			log_error(bin_op->rhs->loc, 1, "Unexpected node in bin_op rhs. Found: %s.\n", ast_get_name(bin_op->rhs->type));
	}


	switch (bin_op->lhs->type) {
		case AST_VALUE:
			compile_value(c, bin_op->lhs);
			break;

		case AST_PROP:
			compile_prop(c, bin_op->lhs);
			break;

		default:
			log_error(bin_op->lhs->loc, 1, "Unexpected node in bin_op lhs. Found: %s.\n", ast_get_name(bin_op->rhs->type));
	}
	compile_op(c, bin_op->op);
}

// array_element : ID LSQUARE (array_element | IDENT | INTEGER | bin_op) RSQUARE ;
void compile_array_element(compiler_T* c, ast_node_T* node) {
	log_info("compile array element\n");
	ast_array_element_T* arr = (ast_array_element_T*) node;

	symbol_var_T* arr_sym = (symbol_var_T*)symbol_table_get(c->s_table, arr->ident->value);
	if (arr_sym == NULL) {
		log_error(node->loc, 1, "Uninitialized symbol '%s' used in compile_array_element\n", arr->ident->value);
	}

	append_file(c->file, "    ; -- load array element --\n");

	switch (arr->offset->type) {
		case AST_ARRAY_ELEMENT:
			compile_array_element(c, arr->offset);
			append_file(c->file,  "    pop rax\n");
			append_file(c->file,  "    push QWORD [rax]\n");
			break;
		case AST_BIN_OP:
			compile_bin_op(c, arr->offset);
			break;
		case AST_VALUE:
			compile_value(c, arr->offset);
			break;

		default:
			log_error(node->loc, 1, "%s cannot be used to index into array.\n", ast_get_name(arr->offset->type));
	}
	symbol_type_T* type_sym = (symbol_type_T*)arr_sym->type;

	append_file(c->file,  "    xor rax, rax\n");
	append_file(c->file,  "    xor rbx, rbx\n");
	char str[50];
	snprintf(str, 50, "    mov rax, %s [rbp+%zu]\n",type_sym->operand, (8 * (arr_sym->index+1))+2);
	append_file(c->file, str);
	append_file(c->file,  "    add rax, 8\n");
	append_file(c->file,  "    pop rbx\n");
	append_file(c->file,  "    imul rbx, 8\n");
	append_file(c->file,  "    add rax, rbx\n");
	append_file(c->file,  "    push rax\n");
	c->stack_pointer += 1;
}

// array : ID LSQUARE INTEGER RSQUARE ;
void compile_array(compiler_T* c, ast_node_T* node) {
	log_info("compile array\n");
	ast_array_T* arr = (ast_array_T*) node;

	append_file(c->file, "    ; -- init array --\n");
	char str[50];
	snprintf(str, 50, "    mov QWORD [mem+%zu], %s\n", (c->mem_pointer*8), arr->len->value);
	append_file(c->file, str);
	snprintf(str, 50, "    push mem+%zu\n", c->mem_pointer*8);
	append_file(c->file, str);

	c->mem_pointer += atoi(arr->len->value);
	c->stack_pointer += 1;
}

void compile_dump(compiler_T* c, ast_node_T* node) {
	log_info("compile dump\n");
	ast_dump_T* dump = (ast_dump_T*) node;

	switch (dump->value->type) {
		case AST_BIN_OP:
			compile_bin_op(c, dump->value);
			break;

		case AST_VALUE:
			compile_value(c, dump->value);
			break;

		case AST_PROP:
			compile_prop(c, dump->value);
			break;

		case AST_ARRAY_ELEMENT:
			compile_array_element(c, dump->value);
			append_file(c->file,  "    pop rax\n");
			append_file(c->file,  "    push QWORD [rax]\n");
			break;

		default:
			log_error(dump->value->loc, 1, "Unexpected node in dump. Found: %s, expects: %s or %s.\n", ast_get_name(dump->value->type), ast_get_name(AST_BIN_OP), ast_get_name(AST_VALUE));
	}

	if (c->stack_pointer < 1) {
		log_error(node->loc, 1, "Stack pointer to small for dump operation.\n");
	}
	
	append_file(c->file, "    ; -- dump --\n");
	append_file(c->file, "    pop rdi\n");
	append_file(c->file, "    call  _dump\n");
	c->stack_pointer -= 1;
}

void compile_cond_op(compiler_T* c, ast_node_T* node) {
	ast_cond_op_T* cond_op = (ast_cond_op_T*) node;
	if (c->stack_pointer < 2) {
		log_error(node->loc, 1, "Stack pointer to small for conditional operation.\n");
	}

	switch (cond_op->t->type) {
		case T_EQUALS:
			append_file(c->file, "    ; -- equals --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    cmp rbx, rax\n");
			append_file(c->file, "    sete al\n");
			break;
		case T_NOT_EQUALS:
			append_file(c->file, "    ; -- equals --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    cmp rbx, rax\n");
			append_file(c->file, "    setne al\n");
			break;
		case T_LESS:
			append_file(c->file, "    ; -- less than --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    cmp rbx, rax\n");
			append_file(c->file, "    setl al\n");
			break;
		case T_GREATER:
			append_file(c->file, "    ; -- greater than --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    cmp rbx, rax\n");
			append_file(c->file, "    setg al\n");
			break;

		default:
			log_error(cond_op->base.loc, 1, "Unreachable code in compile_cond_op\n");

	}

	c->stack_pointer -= 2;
}

void compile_logical(compiler_T* c, ast_node_T* node) {
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

	if (cond->lhs->type == AST_BIN_OP) {
		compile_bin_op(c, cond->lhs);
	} else if(cond->lhs->type == AST_PROP) {
		compile_prop(c, cond->lhs);
	} else {
		compile_value(c, cond->lhs);
	}

	if (cond->rhs->type == AST_BIN_OP) {
		compile_bin_op(c, cond->rhs);
	} else if(cond->rhs->type == AST_PROP) {
		compile_prop(c, cond->rhs);
	} else if(cond->rhs->type == AST_ARRAY_ELEMENT) {
		compile_array_element(c, cond->rhs);
		append_file(c->file,  "    pop rax\n");
		append_file(c->file,  "    push QWORD [rax]\n");
	} else {
		compile_value(c, cond->rhs);
	}

	compile_cond_op(c, cond->op);

	if (cond->logical != NULL) {
		append_file(c->file, "    push rax\n");
		compile_conditional(c, cond->cond);
		compile_logical(c, cond->logical);
	}
}

void compile_block(compiler_T* c, ast_node_T* node) {
	ast_block_T* block = (ast_block_T*) node;

	for (size_t i = 0; i < block->count; i++) {
		log_info("Block expr #%u type: %s\n", i, ast_get_name(block->expressions[i]->type));
		compile_expr(c, block->expressions[i]);
	}
}

void compile_else(compiler_T* c, ast_node_T* node) {
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
	log_info("compile while\n");
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
	log_info("compile while END\n");
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

// assign : (array_element | ID) ASSIGN (value | bin_op | array | array_element) ;
void compile_assign(compiler_T* c, ast_node_T* node) {
	ast_assign_T* a = (ast_assign_T*) node;
	symbol_T* sym = symbol_table_get(c->s_table, a->ident->value);
	if (sym == NULL) {
		log_error(node->loc, 1, "Uninitialized symbol '%s' used in compile_assign\n", token_get_name(a->ident->type));
	}
	switch (sym->type) {
		case SYM_VAR:
			if (a->value->type == AST_VALUE) {
				compile_value(c, a->value);
			} else if (a->value->type == AST_BIN_OP) {
				compile_bin_op(c, a->value);
			} else if (a->value->type == AST_ARRAY) {
				compile_array(c, a->value);
			} else if (a->value->type == AST_ARRAY_ELEMENT) {
				compile_array_element(c, a->value);
				append_file(c->file,  "    pop rax\n");
				append_file(c->file,  "    push QWORD [rax]\n");
			} else {
				log_error(a->value->loc, 1, "Unexpected ast type for rhs of compile_assign. Found: %s\n", ast_get_name(a->base.type));
			}


			if (a->lhs->type == AST_ARRAY_ELEMENT) {
				compile_array_element(c, a->lhs);
				append_file(c->file, "    pop rax\n");
				append_file(c->file, "    pop rsi\n");
				append_file(c->file, "    mov QWORD [rax], rsi\n");
			} else if (a->lhs->type == AST_VALUE) {
				symbol_var_T* var_sym = (symbol_var_T*) sym;
				symbol_type_T* var_type = (symbol_type_T*) var_sym->type;
				char str[20];
				append_file(c->file, "    pop ");
				append_file(c->file, var_type->operand);
				append_file(c->file, " [rbp+");

				snprintf(str, 20, "%zu]\n", (var_type->size * (var_sym->index+1))+2);
				append_file(c->file, str);
			}
			break;

		default:
			log_error(node->loc, 1, "Unexpected symbol type in assignment. Found %s, expected %s.\n", symbol_get_type_string(sym->type), symbol_get_type_string(SYM_VAR));
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
			compile_bin_op(c, node);
			break;
        case AST_VALUE:
			compile_value(c, node);
			break;
        case AST_PROP:
			compile_prop(c, node);
			break;

		default:
			log_error(node->loc, 1, "Invalid node type in compile_sys_arg. Found: %s\n", ast_get_name(node->type));
	}
}

// syscall : LPAREN (sys_arg (COMMA sys_arg)*)? RPAREN;
void compile_syscall(compiler_T* c, ast_node_T* node) {
	ast_syscall_T* syscall = (ast_syscall_T*) node;

	if (syscall->count < 1) {
		log_error(node->loc, 1, "Not enough params for syscall\n");
	} else if (syscall->count > 7) {
		log_error(node->loc, 1, "Too many params for syscall\n");
	}

	append_file(c->file, "    ; -- syscall --\n");

	if (syscall->count > 1) {
		compile_sys_arg(c, syscall->params[1]);
		append_file(c->file, "    pop rdi\n");
	}

	if (syscall->count > 2) {
		compile_sys_arg(c, syscall->params[2]);
		append_file(c->file, "    pop rsi\n");
	}

	if (syscall->count > 3) {
		compile_sys_arg(c, syscall->params[3]);
		append_file(c->file, "    pop rdx\n");
	}

	if (syscall->count > 4) {
		compile_sys_arg(c, syscall->params[4]);
		append_file(c->file, "    pop r10\n");
	}
	
	if (syscall->count > 5) {
		compile_sys_arg(c, syscall->params[5]);
		append_file(c->file, "    pop r8\n");
	}

	if (syscall->count > 6) {
		compile_sys_arg(c, syscall->params[6]);
		append_file(c->file, "    pop r9\n");
	}

	compile_sys_arg(c, syscall->params[0]);
	append_file(c->file, "    pop rax\n");

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
	char str[50];
	snprintf(str, 49, "    mov %s [rbp+%zu]", param_type->operand, (param_type->size * (param_sym->index+1))+2);
	append_file(c->file, str);
	switch (param_index) {
		case 0:
			append_file(c->file, ", rdi\n");
			break;
		case 1:
			append_file(c->file, ", rsi\n");
			break;
		case 2:
			append_file(c->file, ", rdx\n");
			break;
		case 3:
			append_file(c->file, ", rcx\n");
			break;
		case 4:
			append_file(c->file, ", r8\n");
			break;
		case 5:
			append_file(c->file, ", r9\n");
			break;
	}

}

// func_decl : FUNC ID LPAREN (func_param (COMMA func_param)*)? RPAREN block ;
void compile_func_decl(compiler_T* c, ast_node_T* node) {
	ast_func_decl_T* decl = (ast_func_decl_T*)node;

	append_file(c->file, "_");
	append_file(c->file, decl->ident->value);
	append_file(c->file, ":\n");
	append_file(c->file, "    push rbp\n");
	append_file(c->file, "    mov rbp, rsp\n");
	if (strcmp(decl->ident->value, "main") == 0) {
		append_file(c->file, "    sub rsp, ");
		char str[20];
		snprintf(str, 20, "%zu", (c->s_table->child_count+1)*8);
		append_file(c->file, str);
	}

	c->s_table = symbol_table_get_child(c->s_table, decl->ident->value);
	for (char i = decl->param_count - 1; i >= 0; i--) {
		compile_func_param(c, decl->params[i], i);
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

// arg : (value | bin_op | array_element) ;
void compile_func_arg(compiler_T* c, ast_node_T* node, size_t param_index) {
	switch (node->type) {
		case AST_VALUE:
			compile_value(c, node);
			break;

		case AST_BIN_OP:
			compile_bin_op(c, node);
			break;

		case AST_ARRAY_ELEMENT:
			compile_array_element(c, node);
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
		case AST_PROP:
			log_error(node->loc, 1, "Invalid node in function call argument. Found %s\n", ast_get_name(node->type));
	}

        switch (param_index) {
		case 0:
			append_file(c->file, "    pop rdi\n");
			break;
		case 1:
			append_file(c->file, "    pop rsi\n");
			break;
		case 2:
			append_file(c->file, "    pop rdx\n");
			break;
		case 3:
			append_file(c->file, "    pop rcx\n");
			break;
		case 4:
			append_file(c->file, "    pop r8\n");
			break;
		case 5:
			append_file(c->file, "    pop r9\n");
			break;
	}
}

// func_call : ID LPAREN (arg (COMMA arg)*)? RPAREN ;
void compile_func_call(compiler_T* c, ast_node_T* node) {
	ast_func_call_T* call = (ast_func_call_T*) node;
	char* name = call->ident->value;
	
	for (int i = call->param_count - 1; i >= 0; i--) {
		compile_func_arg(c, call->params[i], i);
	}
	append_file(c->file, "    call _");
	append_file(c->file, name);
	append_file(c->file, "\n");

	if (call->param_count > 6) {
		char str[20];
		snprintf(str, 20, "    add rsp, %zu\n", (8 * (call->param_count - 6)));
		append_file(c->file, str);
	}
}

// array_expr : array_element (ASSIGN | op) (value | bin_op | array) ;
void compile_array_expr(compiler_T* c, ast_node_T* node) {
	log_info("compile array expr \n");
	ast_array_expr_T* expr = (ast_array_expr_T*) node;

	if (expr->rhs->type == AST_VALUE) {
		compile_value(c, expr->rhs);
	} else if (expr->rhs->type == AST_BIN_OP) {
		compile_bin_op(c, expr->rhs);
	} else if (expr->rhs->type == AST_ARRAY) {
		compile_array(c, expr->rhs);
	} else if (expr->rhs->type == AST_ARRAY_ELEMENT) {
		compile_array_element(c, expr->rhs);
		append_file(c->file,  "    pop rax\n");
		append_file(c->file,  "    push QWORD [rax]\n");
	} else if (expr->rhs->type == AST_ARRAY_EXPR) {
		compile_array_expr(c, expr->rhs);
	} else {
		log_error(expr->rhs->loc, 1, "Unexpected node type as rhs in array_expr. Found %s\n", ast_get_name(expr->rhs->type));
	}

	compile_array_element(c, expr->array_element);

	if (expr->op == NULL) {
		append_file(c->file, "    pop rax\n");
		append_file(c->file, "    pop rsi\n");
		append_file(c->file, "    mov QWORD [rax], rsi\n");
	} else {
		append_file(c->file, "    pop rax\n");
		append_file(c->file, "    push QWORD [rax]\n");
		compile_op(c, expr->op);
	}
}

// expr : syscall SEMI | if | while | var_decl | const_decl | array_expr SEMI | assign SEMI | bin_op SEMI | dump SEMI | func_decl | func_call SEMI ;
void compile_expr(compiler_T* c, ast_node_T* node) {
	ast_expr_T* expr = (ast_expr_T*) node;
	log_info("expr child type: %s\n", ast_get_name(expr->child->type));

	switch (expr->child->type) {
		case AST_SYSCALL:
			compile_syscall(c, expr->child);
			break;
		case AST_BIN_OP:
			compile_bin_op(c, expr->child);
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
			compile_array_expr(c, expr->child);
			break;
		case AST_CONST_DECL:
			break;

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
			log_error(expr->child->loc, 1, "Unexpected node in expr, found: %s.\n", ast_get_name(expr->child->type));
			break;
	}
}


void compile_program(compiler_T* c, ast_node_T* node) {
	if (node->type == AST_PROGRAM) {
		ast_program_T* program = (ast_program_T*) node;

		for (size_t i = 0; i < program->count; i++) {
			log_info("Program expr #%u type: %s\n", i, ast_get_name(program->expressions[i]->type));
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
		} else if (strcmp(data->type, "int") == 0) {
			char s[25];
			snprintf(s, 25, "    const%zu: dq ", i);
			append_file(c->file, s);
			append_file(c->file, data->t->value);
			append_file(c->file, "\n");
		}
	}

	close_file(c->file);

	printf("[INFO]: Compilation finished\n");
}
