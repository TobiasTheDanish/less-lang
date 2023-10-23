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
	char* value = calloc(2, sizeof(char));
	size_t i = 0;

	do {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	} while (isalnum(lexer->c));

	if (strcmp("else", value) == 0) {
		return token_new(T_ELSE, value);
	} else if (strcmp("dump", value) == 0) {
		return token_new(T_DUMP, value);
	} else if (strcmp("let", value) == 0) {
		return token_new(T_LET, value);
	} else if (strcmp("while", value) == 0) {
		return token_new(T_WHILE, value);
	} else if (strcmp("syscall", value) == 0) {
		return token_new(T_SYSCALL, value);
	} else if (strcmp("func", value) == 0) {
		return token_new(T_FUNC, value);
	}

	return token_new(T_IDENT, value);
}

token_T* read_number(lexer_T* lexer) {
	char* value = calloc(2, sizeof(char));
	size_t i = 0;

	while (isdigit(lexer->c)) {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	return token_new(T_INTEGER, value);

}

char peek(lexer_T* lexer) {
	return lexer->content[lexer->i + 1];
}

void skip_whitespace(lexer_T* lexer) {
	while (isspace(lexer->c)) {
		advance(lexer);
	}
}

void skip_singleline_comment(lexer_T* lexer) {
	while (lexer->c != '\n') {
		advance(lexer);
	}
	advance(lexer);
}

void skip_multiline_comment(lexer_T* lexer) {
	while (!(lexer->c == '*' && peek(lexer) == '/')) {
		advance(lexer);
	}
	advance(lexer);
	advance(lexer);
}

token_T* read_if(lexer_T* lexer) {
	char* str = calloc(3, sizeof(char));
	for (size_t i = 0; i < 2; i++) {
		str[i] = lexer->c;
		advance(lexer);
	}

	return token_new(T_IF, str);
}

token_T* read_pointer(lexer_T* lexer) {
	advance(lexer);
	char* value = calloc(1, sizeof(char));
	size_t i = 0;

	while (isalnum(lexer->c)) {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	return token_new(T_POINTER, value);
}

token_T* read_string(lexer_T* lexer) {
	advance(lexer);
	char* value = calloc(1, sizeof(char));
	size_t i = 0;

	while (lexer->c != '"') {
		if (lexer->c == '\\') {
			value[i++] = lexer->c;

			value = realloc(value, (i + 1) * sizeof(char));
			advance(lexer);
		}

		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}
	advance(lexer);

	return token_new(T_STRING, value);
}

token_T* advance_with_token(lexer_T* lexer, token_E type) {
	char* val = calloc(2, sizeof(char));
	val[0] = lexer->c;
	token_T* t = token_new(type, val);
	advance(lexer);
	return t;
}


token_T* lexer_next_token(lexer_T* lexer) {
	while (lexer->c != '\0' && lexer->c != -1) {
		if (isspace(lexer->c)) {
			skip_whitespace(lexer);
			continue;
		}

		if (lexer->c == '"') {
			return read_string(lexer);
		}

		if (lexer->c == '&') {
			return read_pointer(lexer);
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
			case ':':
				return advance_with_token(lexer, T_COLON);

			case ';':
				return advance_with_token(lexer, T_SEMI);

			case ',':
				return advance_with_token(lexer, T_COMMA);

			case '+':
				return advance_with_token(lexer, T_PLUS);

			case '-':
				return advance_with_token(lexer, T_MINUS);

			case '*':
				return advance_with_token(lexer, T_MULTIPLY);

			case '/':
				{
					char next = peek(lexer);
					switch (next) {
						case '/':
							skip_singleline_comment(lexer);
							break;
						case '*':
							skip_multiline_comment(lexer);
							break;
						default:
							return advance_with_token(lexer, T_DIVIDE);
					}
					break;
				}

			case '%':
				return advance_with_token(lexer, T_MODULUS);

			case '{':
				return advance_with_token(lexer, T_LCURLY);

			case '}':
				return advance_with_token(lexer, T_RCURLY);

			case '(':
				return advance_with_token(lexer, T_LPAREN);

			case ')':
				return advance_with_token(lexer, T_RPAREN);

			case '=':
				{
					if (peek(lexer) == '=') {
						advance(lexer);
						advance(lexer);
						return token_new(T_EQUALS, "==");
					} else {
						return advance_with_token(lexer, T_ASSIGN);
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

