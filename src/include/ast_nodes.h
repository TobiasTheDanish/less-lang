#ifndef AST_NODES_H
#define AST_NODES_H

#include "symbol.h"
#include "token.h"
#include <stddef.h>
typedef enum AST_NODE_E {
	AST_PROGRAM,
	AST_BLOCK,
	AST_EXPR,
	AST_ARRAY_EXPR,
	AST_SYSCALL,
	AST_VAR_DECL,
	AST_CONST_DECL,
	AST_FUNC_DECL,
	AST_FUNC_CALL,
	AST_STRUCT_INIT,
	AST_ATTRIBUTE,
	AST_ASSIGN,
	AST_WHILE,
	AST_IF,
	AST_ELSE,
	AST_CONDITIONAL,
	AST_COND_OP,
	AST_LOGICAL_OP,
	AST_BIN_OP,
	AST_OP,
	AST_VALUE,
	AST_ARRAY,
	AST_ARRAY_ELEMENT,
	AST_PROP,
	AST_DUMP,
	AST_NO_OP,
} ast_node_E;

typedef struct AST_NODE_STRUCT {
	ast_node_E type;
	location_T* loc;
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

typedef struct AST_NODE_ARRAY_EXPR {
	ast_node_T base;
	ast_node_T* array_element;
	ast_node_T* op;
	ast_node_T* rhs;
} ast_array_expr_T;

typedef struct AST_NODE_SYSCALL {
	ast_node_T base;
	ast_node_T** params;
	size_t count;
} ast_syscall_T;

typedef struct AST_NODE_FUNC_DECL {
	ast_node_T base;
	token_T* ident;
	token_T** params;
	size_t param_count;
	ast_node_T* block;
} ast_func_decl_T;

typedef struct AST_NODE_FUNC_CALL {
	ast_node_T base;
	token_T* ident;
	ast_node_T** params;
	size_t param_count;
} ast_func_call_T;

typedef struct AST_NODE_STRUCT_INIT {
	ast_node_T base;
	ast_node_T** attributes;
	size_t attr_count;
	char* struct_name;
} ast_struct_init_T;

typedef struct AST_NODE_ATTRIBUTE {
	ast_node_T base;
	token_T* name;
	ast_node_T* value;
} ast_attribute_T;

typedef struct AST_NODE_VAR_DECL {
	ast_node_T base;
	ast_node_T* assign;
} ast_var_decl_T;

typedef struct AST_NODE_CONST_DECL {
	ast_node_T base;
	token_T* ident;
	ast_node_T* value;
	char* type;
} ast_const_decl_T;

typedef struct AST_NODE_ASSIGN {
	ast_node_T base;
	token_T* ident;
	ast_node_T* lhs;
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
	ast_node_T* logical;
	ast_node_T* cond;
} ast_cond_T;

typedef struct AST_NODE_COND_OP {
	ast_node_T base;
	token_T* t;
} ast_cond_op_T;

typedef struct AST_NODE_LOGICAL_OP {
	ast_node_T base;
	token_T* t;
} ast_logical_op_T;

typedef struct AST_NODE_BIN_OP {
	ast_node_T base;
	ast_node_T* lhs;
	ast_node_T* op;
	ast_node_T* rhs;
	symbol_T* type_sym;
} ast_bin_op_T;

typedef struct AST_NODE_DUMP {
	ast_node_T base;
	ast_node_T* value;
} ast_dump_T;

typedef struct AST_NODE_OP {
	ast_node_T base;
	token_T* t;
} ast_op_T;

typedef struct AST_NODE_VALUE {
	ast_node_T base;
	token_T* t;
	symbol_T* type_sym;
} ast_value_T;

typedef struct AST_NODE_ARRAY {
	ast_node_T base;
	symbol_T* type;
	symbol_T* elem_type;
	token_T* len;
} ast_array_T;

typedef struct AST_NODE_ARRAY_ELEMENT {
	ast_node_T base;
	token_T* ident;
	ast_node_T* offset;
} ast_array_element_T;

typedef struct AST_NODE_PROP {
	ast_node_T base;
	symbol_T* parent_sym;
	token_T* prop;
	ast_node_T* node;
	unsigned char is_pointer;
} ast_prop_T;

ast_node_T* ast_new(ast_node_E type, location_T* loc);
ast_node_T* ast_new_program(ast_node_T** expressions, size_t count);
ast_node_T* ast_new_block(ast_node_T** expressions, size_t count);
ast_node_T* ast_new_expr(ast_node_T* child);
ast_node_T* ast_new_array_expr(ast_node_T* array_element, ast_node_T* op, ast_node_T* rhs);
ast_node_T* ast_new_syscall(ast_node_T** params, size_t count);
ast_node_T* ast_new_func_decl(token_T* ident, token_T** params, size_t param_count, ast_node_T* block);
ast_node_T* ast_new_func_call(token_T* ident, ast_node_T** params, size_t param_count);
ast_node_T* ast_new_struct_init(ast_node_T** attributes, size_t attr_count, char* struct_name);
ast_node_T* ast_new_attribute(token_T* name, ast_node_T* value);
ast_node_T* ast_new_var_decl(ast_node_T* assign);
ast_node_T* ast_new_const_decl(token_T* ident, ast_node_T* value, char* type);
ast_node_T* ast_new_assign(token_T* ident,ast_node_T* lhs, ast_node_T* value);
ast_node_T* ast_new_while(size_t index, ast_node_T* cond, ast_node_T* block);
ast_node_T* ast_new_if(size_t index, ast_node_T* cond, ast_node_T* block, ast_node_T* elze);
ast_node_T* ast_new_else(size_t index, ast_node_T* block);
ast_node_T* ast_new_cond(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs, ast_node_T* logical, ast_node_T* cond);
ast_node_T* ast_new_cond_op(token_T* t);
ast_node_T* ast_new_logical_op(token_T* t);
ast_node_T* ast_new_bin_op(ast_node_T* lhs, ast_node_T* op, ast_node_T* rhs, symbol_T* type);
ast_node_T* ast_new_op(token_T* t);
ast_node_T* ast_new_value(token_T* t, symbol_T* type);
ast_node_T* ast_new_array(symbol_T* type, symbol_T* elem_type, token_T* len);
ast_node_T* ast_new_array_element(token_T* ident, ast_node_T* offset);
ast_node_T* ast_new_prop(symbol_T* parent_sym, token_T* prop, ast_node_T* node, unsigned char is_pointer);
ast_node_T* ast_new_dump(ast_node_T* value);

char* ast_get_name(ast_node_E type);
#endif // !AST_NODES_H
