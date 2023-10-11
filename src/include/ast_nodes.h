#ifndef AST_NODES_H
#define AST_NODES_H

#include "token.h"
#include <stddef.h>
typedef enum AST_NODE_E {
	AST_PROGRAM,
	AST_BLOCK,
	AST_EXPR,
	AST_VAR_DECL,
	AST_ASSIGN,
	AST_WHILE,
	AST_IF,
	AST_ELSE,
	AST_CONDITIONAL,
	AST_COND_OP,
	AST_BIN_OP,
	AST_OP,
	AST_VALUE,
	AST_DUMP,
	AST_NO_OP,
} ast_node_E;

typedef struct AST_NODE_STRUCT {
	ast_node_E type;
} ast_node_T;

typedef struct AST_NODE_PROGRAM {
	ast_node_T base;
	ast_node_T** expressions;
	size_t count;
} ast_program_T;

typedef struct AST_NODE_BLOCK {
	ast_node_T base;
	ast_node_T** expressions;
	size_t count;
} ast_block_T;

typedef struct AST_NODE_EXPR {
	ast_node_T base;
	ast_node_T* child;
} ast_expr_T;

typedef struct AST_NODE_VAR_DECL {
	ast_node_T base;
	ast_node_T* assign;
} ast_var_decl_T;

typedef struct AST_NODE_ASSIGN {
	ast_node_T base;
	token_T* ident;
	ast_node_T* value;
} ast_assign_T;

typedef struct AST_NODE_WHILE {
	ast_node_T base;
	size_t index;
	ast_node_T* cond;
	ast_node_T* block;
} ast_while_T;

typedef struct AST_NODE_IF {
	ast_node_T base;
	size_t index;
	ast_node_T* cond;
	ast_node_T* block;
	ast_node_T* elze;
} ast_if_T;

typedef struct AST_NODE_ELSE {
	ast_node_T base;
	size_t index;
	ast_node_T* block;
} ast_else_T;

typedef struct AST_NODE_CONDITIONAL {
	ast_node_T base;
	ast_node_T* lhs;
	ast_node_T* op;
	ast_node_T* rhs;
} ast_cond_T;

typedef struct AST_NODE_COND_OP {
	ast_node_T base;
	token_T* t;
} ast_cond_op_T;

typedef struct AST_NODE_BIN_OP {
	ast_node_T base;
	ast_node_T* lhs;
	ast_node_T* op;
	ast_node_T* rhs;
} ast_bin_op_T;

typedef struct AST_NODE_OP {
	ast_node_T base;
	token_T* t;
} ast_op_T;

typedef struct AST_NODE_VALUE {
	ast_node_T base;
	token_T* t;
} ast_value_T;

ast_node_T* ast_new(ast_node_E type);
ast_node_T* ast_new_program(ast_node_T** expressions, size_t count);
ast_node_T* ast_new_block(ast_node_T** expressions, size_t count);
ast_node_T* ast_new_expr(ast_node_T* child);
ast_node_T* ast_new_var_decl(ast_node_T* assign);
ast_node_T* ast_new_assign(token_T* ident, ast_node_T* value);
ast_node_T* ast_new_while(size_t index, ast_node_T* cond, ast_node_T* block);
ast_node_T* ast_new_if(size_t index, ast_node_T* cond, ast_node_T* block, ast_node_T* elze);
ast_node_T* ast_new_else(size_t index, ast_node_T* block);
ast_node_T* ast_new_cond(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs);
ast_node_T* ast_new_cond_op(token_T* t);
ast_node_T* ast_new_bin_op(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs);
ast_node_T* ast_new_op(token_T* t);
ast_node_T* ast_new_value(token_T* t);

char* ast_get_name(ast_node_E type);
#endif // !AST_NODES_H
