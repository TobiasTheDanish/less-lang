#include "include/ast_nodes.h"
#include <stdlib.h>

ast_node_T* ast_new(ast_node_E type) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = type;

	return base;
}

ast_node_T* ast_new_program(ast_node_T** expressions, size_t count) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_PROGRAM;

	ast_program_T* program = malloc(sizeof(ast_program_T));
	program->base = *base;
	program->expressions = expressions;
	program->count = count;

	return (ast_node_T*) program;
}

ast_node_T* ast_new_block(ast_node_T** expressions, size_t count) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_BLOCK;

	ast_block_T* block = malloc(sizeof(ast_block_T));
	block->base = *base;
	block->expressions = expressions;
	block->count = count;

	return (ast_node_T*) block;
}

ast_node_T* ast_new_expr(ast_node_T* child) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_EXPR;

	ast_expr_T* expr = malloc(sizeof(ast_expr_T));
	expr->base = *base;
	expr->child = child;

	return (ast_node_T*) expr;
}

ast_node_T* ast_new_if(ast_node_T* cond, ast_node_T* block, ast_node_T* elze){
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_IF;

	ast_if_T* if_node = malloc(sizeof(ast_if_T));
	if_node->base = *base;
	if_node->cond = cond;
	if_node->block = block;
	if_node->elze = elze;

	return (ast_node_T*) if_node;
}

ast_node_T* ast_new_else(ast_node_T* block){
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_ELSE;

	ast_else_T* elze = malloc(sizeof(ast_else_T));
	elze->base = *base;
	elze->block = block;

	return (ast_node_T*) elze;
}

ast_node_T* ast_new_cond(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_CONDITIONAL;

	ast_cond_T* cond = malloc(sizeof(ast_cond_T));
	cond->base = *base;
	cond->lhs = lhs;
	cond->rhs = rhs;
	cond->op = op;

	return (ast_node_T*) cond;
}

ast_node_T* ast_new_cond_op(token_T* t) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_COND_OP;

	ast_cond_op_T* op = malloc(sizeof(ast_cond_op_T));
	op->base = *base;
	op->t = t;
	return (ast_node_T*) op;
}

ast_node_T* ast_new_bin_op(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_BIN_OP;

	ast_bin_op_T* bin_op = malloc(sizeof(ast_bin_op_T));
	bin_op->base = *base;
	bin_op->lhs = lhs;
	bin_op->rhs = rhs;
	bin_op->op = op;

	return (ast_node_T*) bin_op;
}

ast_node_T* ast_new_op(token_T* t) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_OP;

	ast_op_T* op = malloc(sizeof(ast_op_T));
	op->base = *base;
	op->t = t;
	return (ast_node_T*) op;
}

ast_node_T* ast_new_value(token_T* t) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_VALUE;

	ast_value_T* value = malloc(sizeof(ast_value_T));
	value->base = *base;
	value->t = t;
	return (ast_node_T*) value;
}

char* ast_get_name(ast_node_E type) {
	char* names[] = { 
		"Program",
		"Block", 
		"Expression", 
		"If",
		"Else",
		"Conditional",
		"Conditional operater",
		"Binary operation", 
		"Operation", 
		"Value", 
		"Dump", 
		"No operation" 
	};

	return names[type];
}
