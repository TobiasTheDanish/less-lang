#ifndef PARSER_H
#define PARSER_H

#include "ast_nodes.h"
#include "data_table.h"
#include "lexer.h"
#include "symbol_table.h"
#include "token.h"

typedef struct PARSER_STRUCT {
	lexer_T* lexer;
	token_T** tokens;
	size_t t_count;
	size_t t_index;
	size_t if_count;
	symbol_table_T* s_table;
	data_table_T* data_table;
	unsigned char debug;
} parser_T;

parser_T* parser_new(lexer_T* lexer, size_t t_count, unsigned char debug);

ast_node_T* parser_parse(parser_T* parser);

#endif // !PARSER_H

