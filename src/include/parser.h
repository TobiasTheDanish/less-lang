#ifndef PARSER_H
#define PARSER_H

#include "ast_nodes.h"
#include "lexer.h"
#include "token.h"

typedef struct PARSER_STRUCT {
	lexer_T* lexer;
	token_T** tokens;
	size_t t_count;
	size_t t_index;
	size_t if_count;
} parser_T;

parser_T* parser_new(lexer_T* lexer, size_t t_count);

ast_node_T* parser_parse(parser_T* parser);

#endif // !PARSER_H

