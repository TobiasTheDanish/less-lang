#include "include/compiler.h"
#include "include/ast_nodes.h"
#include "include/file_util.h"
#include "include/token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void compile_expr(compiler_T* c, ast_node_T* node);

compiler_T* compiler_new(ast_node_T* program, char* output_file) {
	compiler_T* c = malloc(sizeof(compiler_T));
	c->program = program;
	c->file = open_file_m(output_file, "w");

	return c;
}

void compile_op(compiler_T* c, ast_node_T* node) {
	ast_op_T* op = (ast_op_T*) node;

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
			break;

		case T_IDENT:
			printf("[compile_value]: Identifiers currently not implemented for compilation.");
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
			printf("Unexpected node in expr. Found: %s, expects: %s or %s.\n", ast_get_name(bin_op->rhs->type), ast_get_name(AST_BIN_OP), ast_get_name(AST_VALUE));
			exit(1);
	}

	compile_value(c, bin_op->lhs);
	compile_op(c, bin_op->op);
}

void compile_cond_op(compiler_T* c, ast_node_T* node) {
	ast_cond_op_T* cond_op = (ast_cond_op_T*) node;

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
		append_file(c->file, "    ; -- dump --\n");
		append_file(c->file, "    pop rdi\n");
		append_file(c->file, "    call  _dump\n");
		//printf("Compiled %zu expressions out of %zu\n", i, block->count);
	}
}

void compile_else(compiler_T* c, ast_node_T* node) {
	ast_else_T* elze = (ast_else_T*) node;
	append_file(c->file, ".else:\n");
	compile_block(c, elze->block);
}

void compile_if(compiler_T* c, ast_node_T* node) {
	ast_if_T* if_node = (ast_if_T*) node;

	compile_conditional(c, if_node->cond);
	
	append_file(c->file, "    test al, al\n");
	if (if_node->elze == NULL) {
		append_file(c->file, "    jz .end\n");
	} else {
		append_file(c->file, "    jz .else\n");
	}

	compile_block(c, if_node->block);
	
	if (if_node->elze != NULL) {
		append_file(c->file, "    jmp .end\n");
		compile_else(c, if_node->elze);
	}

	append_file(c->file, ".end:\n");
}

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

        case AST_ELSE:
        case AST_BLOCK:
        case AST_EXPR:
        case AST_OP:
        case AST_VALUE:
        case AST_NO_OP:
		case AST_PROGRAM:
		case AST_CONDITIONAL:
		case AST_COND_OP:
			printf("Unexpected node in expr. Found: %s, expects: %s or %s.\n", ast_get_name(expr->child->type), ast_get_name(AST_BIN_OP), ast_get_name(AST_IF));
			exit(1);
        }
}


void compile_program(compiler_T* c, ast_node_T* node) {
	//printf("compile_program\n");
	//printf("program type: %u\n", program->type);
	if (node->type == AST_PROGRAM) {
		//printf("compile_block\n");
		ast_program_T* program = (ast_program_T*) node;

		for (size_t i = 0; i < program->count; i++) {
			compile_expr(c, program->expressions[i]);
			append_file(c->file, "    ; -- dump --\n");
			append_file(c->file, "    pop rdi\n");
			append_file(c->file, "    call  _dump\n");
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

	compile_program(c, c->program);

	append_file(c->file, "    ; exit program\n");
	append_file(c->file, "    mov rdi, 0\n");
	append_file(c->file, "    mov rax, 60\n");
	append_file(c->file, "    syscall\n");

	printf("Compilation finished\n");
}
