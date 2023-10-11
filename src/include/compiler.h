#ifndef COMPILER_H
#define COMPILER_H

#include "ast_nodes.h"
#include "file_util.h"
#include "symbol_table.h"
typedef struct COMPILER_STRUCT {
	ast_node_T* program;
	symbol_table_T* s_table;
	file_T* file;
	size_t stack_pointer;
} compiler_T;

compiler_T* compiler_new(ast_node_T* program, symbol_table_T* s_table, char* output_file);

void compile(compiler_T* c);

#endif // !COMPILER_H

