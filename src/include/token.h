#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef struct LOCATION {
	char* filePath;
	unsigned long row;
	unsigned long col;
} location_T;

typedef enum TOKEN_E {
	T_INTEGER,
	T_STRING,
	T_PLUS,
	T_MINUS,
	T_MULTIPLY,
	T_DIVIDE,
	T_MODULUS,
	T_AND,
	T_OR,
	T_EQUALS,
	T_NOT_EQUALS,
	T_LESS,
	T_GREATER,
	T_MUT,
	T_LET,
	T_CONST,
	T_FUNC,
	T_ASSIGN,
	T_IDENT,
	T_COMMA,
	T_DOT,
	T_SEMI,
	T_COLON,
	T_LSQUARE,
	T_RSQUARE,
	T_LCURLY,
	T_RCURLY,
	T_LPAREN,
	T_RPAREN,
	T_POINTER,
	T_WHILE,
	T_IF,
	T_ELSE,
	T_SYSCALL,
	T_DUMP,
	T_EOF,
} token_E;

typedef struct TOKEN_T {
	token_E type;
	char* value;
	location_T* loc;
} token_T;

token_T* token_new(token_E type, char* value, location_T* loc);

char* token_get_name(token_E type);

bool token_is_op(token_T* token);
bool token_is_logical(token_T* token);
#endif // !TOKEN_H

