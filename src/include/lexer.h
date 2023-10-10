#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stddef.h>
typedef struct LEXER_STRUCT {
	char* content;
	char c;
	size_t i;
} lexer_T;

lexer_T* lexer_from_file(char* filePath);
lexer_T* lexer_from_string(char* content);
token_T* lexer_next_token(lexer_T* lexer);

#endif // !LEXER_H
