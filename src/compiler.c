#include "include/compiler.h"
#include "include/ast_nodes.h"
#include "include/file_util.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void compile_expr(compiler_T* c, ast_node_T* node);

compiler_T* compiler_new(ast_node_T* program, symbol_table_T* s_table, char* output_file) {
	compiler_T* c = malloc(sizeof(compiler_T));
	c->program = program;
	c->file = open_file_m(output_file, "w");
	c->s_table = s_table;
	c->stack_pointer = 0;

	return c;
}

void compile_dump(compiler_T* c, ast_node_T* node) {
	if (c->stack_pointer < 1) {
		printf("Stack pointer to small for dump operation.\n");
		exit(1);
	}
	append_file(c->file, "    ; -- dump --\n");
	append_file(c->file, "    pop rdi\n");
	append_file(c->file, "    call  _dump\n");
}

void compile_op(compiler_T* c, ast_node_T* node) {
	ast_op_T* op = (ast_op_T*) node;
	if (c->stack_pointer < 2) {
		printf("Stack pointer to small for binary operation.\n");
		exit(1);
	}

	switch (op->t->type) {
        case T_PLUS:
			append_file(c->file, "    ; -- plus --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    add rax, rbx\n");
			append_file(c->file, "    push rax\n");

			break;
        case T_MINUS:
			append_file(c->file, "    ; -- minus --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    sub rax, rbx\n");
			append_file(c->file, "    push rax\n");
			break;
        case T_MULTIPLY:
			append_file(c->file, "    ; -- multipy --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    imul rax, rbx\n");
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

		default:
			printf("Unreachable code in compile_op");
			exit(1);
	}

	c->stack_pointer -= 1;
}

void compile_value(compiler_T* c, ast_node_T* node) {
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

			append_file(c->file, "    push ");
			append_file(c->file, sym->operand);

			char str[20];
			snprintf(str, 20, " [mem+%zu]\n", (sym->size * (sym->index+1)));
			append_file(c->file, str);
			c->stack_pointer += 1;
			break;

		default:
			printf("Unreachable code in compile_value\n");
			exit(1);
	}
}

void compile_bin_op(compiler_T* c, ast_node_T* node) {
	ast_bin_op_T* bin_op = (ast_bin_op_T*) node;
	switch (bin_op->rhs->type) {
		case AST_BIN_OP:
			compile_bin_op(c, bin_op->rhs);
			break;

		case AST_VALUE:
			compile_value(c, bin_op->rhs);
			break;

		default:
			printf("Unexpected node in bin_op. Found: %s, expects: %s or %s.\n", ast_get_name(bin_op->rhs->type), ast_get_name(AST_BIN_OP), ast_get_name(AST_VALUE));
			exit(1);
	}

	compile_value(c, bin_op->lhs);
	compile_op(c, bin_op->op);
}

void compile_cond_op(compiler_T* c, ast_node_T* node) {
	ast_cond_op_T* cond_op = (ast_cond_op_T*) node;
	if (c->stack_pointer < 2) {
		printf("Stack pointer to small for conditional operation.\n");
		exit(1);
	}

	switch (cond_op->t->type) {
		case T_EQUALS:
			append_file(c->file, "    ; -- equals --\n");
			append_file(c->file, "    pop rax\n");
			append_file(c->file, "    pop rbx\n");
			append_file(c->file, "    cmp rbx, rax\n");
			append_file(c->file, "    sete al\n");
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
			printf("Unreachable code in compile_cond_op\n");
			exit(1);

	}

	c->stack_pointer -= 2;
}

void compile_conditional(compiler_T* c, ast_node_T* node) {
	ast_cond_T* cond = (ast_cond_T*) node;

	if (cond->lhs->type == AST_BIN_OP) {
		compile_bin_op(c, cond->lhs);
	} else {
		compile_value(c, cond->lhs);
	}

	if (cond->rhs->type == AST_BIN_OP) {
		compile_bin_op(c, cond->rhs);
	} else {
		compile_value(c, cond->rhs);
	}

	compile_cond_op(c, cond->op);
}

void compile_block(compiler_T* c, ast_node_T* node) {
	//printf("compile_block\n");
	ast_block_T* block = (ast_block_T*) node;

	for (size_t i = 0; i < block->count; i++) {
		compile_expr(c, block->expressions[i]);
		//printf("Compiled %zu expressions out of %zu\n", i, block->count);
	}
}

void compile_else(compiler_T* c, ast_node_T* node) {
	ast_else_T* elze = (ast_else_T*) node;
	char str[50];
	sprintf(str, ".else_%zu:\n", elze->index);
	append_file(c->file, str);
	compile_block(c, elze->block);
}

// while : WHILE conditional block ;
void compile_while(compiler_T* c, ast_node_T* node) {
	ast_while_T* while_node = (ast_while_T*) node;

	char str[30];
	sprintf(str, "    jmp .end_%zu\n", while_node->index);
	append_file(c->file, str);

	sprintf(str, ".W_%zu:\n", while_node->index);
	append_file(c->file, str);

	compile_block(c, while_node->block);

	sprintf(str, ".end_%zu:\n", while_node->index);
	append_file(c->file, str);

	compile_conditional(c, while_node->cond);
	append_file(c->file, "    test al, al\n");

	sprintf(str, "    jnz .W_%zu\n", while_node->index);
	append_file(c->file, str);
}

void compile_if(compiler_T* c, ast_node_T* node) {
	ast_if_T* if_node = (ast_if_T*) node;

	compile_conditional(c, if_node->cond);
	
	append_file(c->file, "    test al, al\n");
	if (if_node->elze == NULL) {
		char str[50];
		sprintf(str, "    jz .end_%zu\n", if_node->index);
		append_file(c->file, str);
	} else {
		char str[50];
		sprintf(str, "    jz .else_%zu\n", if_node->index);
		append_file(c->file, str);
	}

	compile_block(c, if_node->block);
	
	if (if_node->elze != NULL) {
		char str[50];
		sprintf(str, "    jmp .end_%zu\n", if_node->index);
		append_file(c->file, str);
		compile_else(c, if_node->elze);
	}

	char str[50];
	sprintf(str, ".end_%zu:\n", if_node->index);
	append_file(c->file, str);
}

// assign : ID ASSIGN (value | bin_op) ;
void compile_assign(compiler_T* c, ast_node_T* node) {
	ast_assign_T* a = (ast_assign_T*) node;
	symbol_T* sym = symbol_table_get(c->s_table, a->ident->value);
	if (sym == NULL) {
		printf("Uninitialized symbol '%s' used in compile_assign\n", token_get_name(a->ident->type));
		exit(1);
	}

	if (a->value->type == AST_VALUE) {
		compile_value(c, a->value);
	} else if (a->value->type == AST_BIN_OP) {
		compile_bin_op(c, a->value);
	} else {
		printf("Unexpected ast type for rhs of compile_assign. Found: %s\n", ast_get_name(a->base.type));
		exit(1);
	}

	char str[20];
	append_file(c->file, "    pop ");
	append_file(c->file, sym->operand);
	append_file(c->file, " [mem+");

	snprintf(str, 20, "%zu]\n", (sym->size * (sym->index+1)));
	append_file(c->file, str);

}

// var_decl : LET assign SEMI ;
void compile_var_decl(compiler_T* c, ast_node_T* node) {
	ast_var_decl_T* decl = (ast_var_decl_T*) node;

	compile_assign(c, decl->assign);
}

// expr : if | while | var_decl | assign SEMI | bin_op SEMI | dump SEMI;
void compile_expr(compiler_T* c, ast_node_T* node) {
	//printf("compile_expr\n");
	ast_expr_T* expr = (ast_expr_T*) node;

	switch (expr->child->type) {
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

		case AST_ELSE:
		case AST_BLOCK:
		case AST_EXPR:
		case AST_OP:
		case AST_VALUE:
		case AST_NO_OP:
		case AST_PROGRAM:
		case AST_CONDITIONAL:
		case AST_COND_OP:
			printf("Unexpected node in expr, found: %s.\n", ast_get_name(expr->child->type));
			exit(1);
	}
}


void compile_program(compiler_T* c, ast_node_T* node) {
	//printf("compile_program\n");
	//printf("program type: %u\n", program->type);
	if (node->type == AST_PROGRAM) {
		ast_program_T* program = (ast_program_T*) node;

		for (size_t i = 0; i < program->count; i++) {
			compile_expr(c, program->expressions[i]);
			//printf("Compiled %zu expressions out of %zu\n", i, block->count);
		}
	} else {
		printf("Unexpected node to start program.\n Found: %s, expects: %s.\n", ast_get_name(node->type), ast_get_name(AST_PROGRAM));
		exit(1);
	}
}

void compile(compiler_T* c) {
	//printf("compile\n");
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
	append_file(c->file, "    push    rbp\n");
	append_file(c->file, "    mov     rbp, rsp\n");
	append_file(c->file, "    sub     rsp, ");
	char str[12];
	snprintf(str, 12, "%zu\n", ((c->s_table->count + 1) * 8));
	append_file(c->file, str);

	compile_program(c, c->program);

	append_file(c->file, "    ; exit program\n");
	append_file(c->file, "    mov rdi, 0\n");
	append_file(c->file, "    mov rax, 60\n");
	append_file(c->file, "    syscall\n");

	append_file(c->file, "segment .bss\n");
	append_file(c->file, "    mem: resb 64000\n");

	printf("Compilation finished\n");
}
