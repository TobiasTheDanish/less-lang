#include "include/lexer.h"
#include "include/file_util.h"
#include "include/logger.h"
#include "include/token.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

lexer_T* lexer_from_file(char* filePath){
	file_T* file = open_file(filePath);
	char* content = read_file(file);
	log_info("src: \n%s\n", content);

	lexer_T* lexer = malloc(sizeof(lexer_T));
	lexer->content = content;
	lexer->i = 0;
	lexer->c = content[0];
	lexer->loc = calloc(1, sizeof(location_T));
	lexer->loc->filePath = filePath;
	lexer->loc->col = 1;
	lexer->loc->row = 1;

	return lexer;
}

lexer_T* lexer_from_string(char* content) {
	lexer_T* lexer = malloc(sizeof(lexer_T));
	lexer->content = content;
	lexer->i = 0;
	lexer->c = content[0];
	lexer->loc = calloc(1, sizeof(location_T));
	lexer->loc->filePath = "String literal";

	return lexer;
}

void advance(lexer_T* lexer) {
	if (lexer->c == '\n') {
		lexer->loc->col = 1;
		lexer->loc->row += 1;
	} else {
		lexer->loc->col += 1;
	}

	lexer->i += 1;
	lexer->c = lexer->content[lexer->i];
}

token_T* read_ident(lexer_T* lexer, location_T* loc) {
	char* value = calloc(2, sizeof(char));
	size_t i = 0;

	do {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	} while (isalnum(lexer->c) || lexer->c == '_');

	if (strcmp("else", value) == 0) {
		return token_new(T_ELSE, value, loc);
	} else if (strcmp("dump", value) == 0) {
		return token_new(T_DUMP, value, loc);
	} else if (strcmp("mut", value) == 0) {
		return token_new(T_MUT, value, loc);
	} else if (strcmp("let", value) == 0) {
		return token_new(T_LET, value, loc);
	} else if (strcmp("const", value) == 0) {
		return token_new(T_CONST, value, loc);
	} else if (strcmp("while", value) == 0) {
		return token_new(T_WHILE, value, loc);
	} else if (strcmp("syscall", value) == 0) {
		return token_new(T_SYSCALL, value, loc);
	} else if (strcmp("func", value) == 0) {
		return token_new(T_FUNC, value, loc);
	}

	return token_new(T_IDENT, value, loc);
}

token_T* read_number(lexer_T* lexer, location_T* loc) {
	char* value = calloc(2, sizeof(char));
	size_t i = 0;

	while (isdigit(lexer->c)) {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	return token_new(T_INTEGER, value, loc);

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

token_T* read_if(lexer_T* lexer, location_T* loc) {
	char* str = calloc(3, sizeof(char));
	for (size_t i = 0; i < 2; i++) {
		str[i] = lexer->c;
		advance(lexer);
	}

	return token_new(T_IF, str, loc);
}

token_T* read_pointer(lexer_T* lexer, location_T* loc) {
	advance(lexer);
	char* value = calloc(1, sizeof(char));
	size_t i = 0;

	while (isalnum(lexer->c) || lexer->c == '_') {
		value[i++] = lexer->c;

		value = realloc(value, (i + 1) * sizeof(char));
		advance(lexer);
	}

	return token_new(T_POINTER, value, loc);
}

token_T* read_string(lexer_T* lexer, location_T* loc) {
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

	return token_new(T_STRING, value, loc);
}

token_T* advance_with_token(lexer_T* lexer, token_E type, location_T* loc) {
	char* val = calloc(2, sizeof(char));
	val[0] = lexer->c;
	token_T* t = token_new(type, val, loc);
	advance(lexer);
	return t;
}


token_T* lexer_next_token(lexer_T* lexer) {
	while (lexer->c != '\0' && lexer->c != -1) {
		if (isspace(lexer->c)) {
			skip_whitespace(lexer);
			continue;
		}

		location_T* loc = malloc(sizeof(location_T));
		memcpy(loc, lexer->loc, sizeof(location_T));

		if (lexer->c == '"') {
			return read_string(lexer, loc);
		}

		if (lexer->c == '&') {
			return read_pointer(lexer, loc);
		}

		if (isalpha(lexer->c) || lexer->c == '_') {
			if (lexer->c == 'i' && peek(lexer) == 'f') {
				return read_if(lexer, loc);
			}

			return read_ident(lexer, loc);
		}
		
		if (isdigit(lexer->c)) {
			return read_number(lexer, loc);
		}

		switch (lexer->c) {
			case ':':
				return advance_with_token(lexer, T_COLON, loc);

			case ';':
				return advance_with_token(lexer, T_SEMI, loc);

			case ',':
				return advance_with_token(lexer, T_COMMA, loc);

			case '.':
				return advance_with_token(lexer, T_DOT, loc);

			case '+':
				return advance_with_token(lexer, T_PLUS, loc);

			case '-':
				return advance_with_token(lexer, T_MINUS, loc);

			case '*':
				return advance_with_token(lexer, T_MULTIPLY, loc);

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
							return advance_with_token(lexer, T_DIVIDE, loc);
					}
					break;
				}

			case '%':
				return advance_with_token(lexer, T_MODULUS, loc);

			case '[':
				return advance_with_token(lexer, T_LSQUARE, loc);

			case ']':
				return advance_with_token(lexer, T_RSQUARE, loc);

			case '{':
				return advance_with_token(lexer, T_LCURLY, loc);

			case '}':
				return advance_with_token(lexer, T_RCURLY, loc);

			case '(':
				return advance_with_token(lexer, T_LPAREN, loc);

			case ')':
				return advance_with_token(lexer, T_RPAREN, loc);

			case '=':
				{
					if (peek(lexer) == '=') {
						advance(lexer);
						advance(lexer);
						return token_new(T_EQUALS, "==", loc);
					} else {
						return advance_with_token(lexer, T_ASSIGN, loc);
					}
				}

			case '<':
				return advance_with_token(lexer, T_LESS, loc);

			case '>':
				return advance_with_token(lexer, T_GREATER, loc);
		
			default:
				log_warning("Unexpected character when lexing: '%c' '%s'.\n", lexer->c, lexer->c);
				return token_new(T_EOF, "EOF", loc);
		}
	}

	return token_new(T_EOF, "EOF", lexer->loc);
}

