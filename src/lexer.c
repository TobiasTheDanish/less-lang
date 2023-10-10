#include "include/lexer.h"
#include "include/file_util.h"
#include "include/token.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

lexer_T* lexer_from_file(char* filePath){
	file_T* file = open_file(filePath);
	char* content = read_file(file);
	printf("src: \n%s\n", content);

	lexer_T* lexer = malloc(sizeof(lexer_T));
	lexer->content = content;
	lexer->i = 0;
	lexer->c = content[0];

	return lexer;
}

lexer_T* lexer_from_string(char* content) {
	lexer_T* lexer = malloc(sizeof(lexer_T));
	lexer->content = content;
	lexer->i = 0;
	lexer->c = content[0];

	return lexer;
}

void advance(lexer_T* lexer) {
	lexer->i += 1;
	lexer->c = lexer->content[lexer->i];
}

token_T* read_ident(lexer_T* lexer) {
	char* value = malloc(sizeof(char));
	size_t i = 0;

	while (isalnum(lexer->c)) {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	if (strcmp("else", value) == 0) {
		return token_new(T_ELSE, value);
	}

	return token_new(T_IDENT, value);
}

token_T* read_number(lexer_T* lexer) {
	char* value = malloc(sizeof(char));
	size_t i = 0;

	while (isdigit(lexer->c)) {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	return token_new(T_INTEGER, value);

}

void skip_whitespace(lexer_T* lexer) {
	while (isspace(lexer->c)) {
		advance(lexer);
	}
}

token_T* read_if(lexer_T* lexer) {
	char* str = malloc(2 * sizeof(char));
	for (size_t i = 0; i < 2; i++) {
		str[i] = lexer->c;
		advance(lexer);
	}

	return token_new(T_IF, str);
}

token_T* advance_with_token(lexer_T* lexer, token_E type) {
	char* val = calloc(2, sizeof(char));
	val[0] = lexer->c;
	token_T* t = token_new(type, val);
	advance(lexer);
	return t;
}

char peek(lexer_T* lexer) {
	return lexer->content[lexer->i + 1];
}

token_T* lexer_next_token(lexer_T* lexer) {
	if (lexer->c != '\0') {
		if (isspace(lexer->c)) {
			skip_whitespace(lexer);
		}

		if (isalpha(lexer->c)) {
			if (lexer->c == 'i' && peek(lexer) == 'f') {
				return read_if(lexer);
			}

			return read_ident(lexer);
		}
		
		if (isdigit(lexer->c)) {
			return read_number(lexer);
		}

		switch (lexer->c) {
			case ';':
				return advance_with_token(lexer, T_SEMI);

			case '+':
				return advance_with_token(lexer, T_PLUS);

			case '-':
				return advance_with_token(lexer, T_MINUS);

			case '*':
				return advance_with_token(lexer, T_MULTIPLY);

			case '/':
				return advance_with_token(lexer, T_DIVIDE);

			case '{':
				return advance_with_token(lexer, T_LCURLY);

			case '}':
				return advance_with_token(lexer, T_RCURLY);

			case '=':
				{
					if (peek(lexer) == '=') {
						advance(lexer);
						advance(lexer);
						return token_new(T_EQUALS, "==");
					} else {
						printf("Unexpected double character when lexing: '%c' '%c'.\n", lexer->c, peek(lexer));
						exit(1);
					}
				}

			case '<':
				return advance_with_token(lexer, T_LESS);

			case '>':
				return advance_with_token(lexer, T_GREATER);
		
			default:
				printf("Unexpected character when lexing: '%c'.\n", lexer->c);
				return token_new(T_EOF, "EOF");
		}
	}

	return token_new(T_EOF, "EOF");
}

