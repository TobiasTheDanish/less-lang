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

ast_node_T* ast_new_syscall(ast_node_T** params, size_t count) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_SYSCALL;

	ast_syscall_T* syscall = malloc(sizeof(ast_syscall_T));
	syscall->base = *base;
	syscall->params = params;
	syscall->count = count;

	return (ast_node_T*) syscall;
}

ast_node_T* ast_new_func_decl(token_T* ident, token_T** params, size_t param_count, ast_node_T* block) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_FUNC_DECL;

	ast_func_decl_T* func_decl = malloc(sizeof(ast_func_decl_T));
	func_decl->base = *base;
	func_decl->params = params;
	func_decl->param_count = param_count;
	func_decl->block = block;
	func_decl->ident = ident;

	return (ast_node_T*) func_decl;
}


ast_node_T* ast_new_func_call(token_T* ident, ast_node_T** params, size_t param_count) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_FUNC_CALL;

	ast_func_call_T* func_call = malloc(sizeof(ast_func_call_T));
	func_call->base = *base;
	func_call->params = params;
	func_call->param_count = param_count;
	func_call->ident = ident;

	return (ast_node_T*) func_call;
}


ast_node_T* ast_new_var_decl(ast_node_T* assign) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_VAR_DECL;

	ast_var_decl_T* decl = malloc(sizeof(ast_var_decl_T*));
	decl->base = *base;
	decl->assign = assign;
	
	return (ast_node_T*) decl;
}

ast_node_T* ast_new_assign(token_T* ident, ast_node_T* value) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_ASSIGN;

	ast_assign_T* assign = malloc(sizeof(ast_assign_T));
	assign->base = *base;
	assign->ident = ident;
	assign->value = value;

	return (ast_node_T*) assign;
}

ast_node_T* ast_new_while(size_t index, ast_node_T* cond, ast_node_T* block) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_WHILE;

	ast_while_T* w = malloc(sizeof(ast_while_T));
	w->base = *base;
	w->index = index;
	w->cond = cond;
	w->block = block;

	return (ast_node_T*) w;
}

ast_node_T* ast_new_if(size_t index, ast_node_T* cond, ast_node_T* block, ast_node_T* elze){
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_IF;

	ast_if_T* if_node = malloc(sizeof(ast_if_T));
	if_node->base = *base;
	if_node->index = index;
	if_node->cond = cond;
	if_node->block = block;
	if_node->elze = elze;

	return (ast_node_T*) if_node;
}

ast_node_T* ast_new_else(size_t index, ast_node_T* block){
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_ELSE;

	ast_else_T* elze = malloc(sizeof(ast_else_T));
	elze->base = *base;
	elze->index = index;
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

ast_node_T* ast_new_bin_op(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs, symbol_T* type) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_BIN_OP;

	ast_bin_op_T* bin_op = malloc(sizeof(ast_bin_op_T));
	bin_op->base = *base;
	bin_op->lhs = lhs;
	bin_op->rhs = rhs;
	bin_op->op = op;
	bin_op->type_sym = type;

	return (ast_node_T*) bin_op;
}

ast_node_T* ast_new_dump(ast_node_T* value) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_DUMP;

	ast_dump_T* dump = malloc(sizeof(ast_dump_T));
	dump->base = *base;
	dump->value = value;

	return (ast_node_T*) dump;
}

ast_node_T* ast_new_op(token_T* t) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_OP;

	ast_op_T* op = malloc(sizeof(ast_op_T));
	op->base = *base;
	op->t = t;
	return (ast_node_T*) op;
}

ast_node_T* ast_new_value(token_T* t, symbol_T* type) {
	ast_node_T* base = malloc(sizeof(ast_node_T));
	base->type = AST_VALUE;

	ast_value_T* value = malloc(sizeof(ast_value_T));
	value->base = *base;
	value->t = t;
	value->type_sym = type;
	return (ast_node_T*) value;
}

char* ast_get_name(ast_node_E type) {
	char* names[] = { 
		"Program",
		"Block", 
		"Expression", 
		"Syscall", 
		"Variable declaration", 
		"Function declaration", 
		"Function call", 
		"Assign", 
		"While", 
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
