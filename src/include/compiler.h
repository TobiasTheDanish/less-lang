#ifndef COMPILER_H
#define COMPILER_H

#include "ast_nodes.h"
#include "file_util.h"
typedef struct COMPILER_STRUCT {
	ast_node_T* program;
	file_T* file;
} compiler_T;

compiler_T* compiler_new(ast_node_T* program, char* output_file);

void compile(compiler_T* c);

#endif // !COMPILER_H

