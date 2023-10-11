#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

typedef enum TOKEN_E {
	T_INTEGER,
	T_PLUS,
	T_MINUS,
	T_MULTIPLY,
	T_DIVIDE,
	T_EQUALS,
	T_LESS,
	T_GREATER,
	T_IDENT,
	T_SEMI,
	T_LCURLY,
	T_RCURLY,
	T_IF,
	T_ELSE,
	T_SYSCALL,
	T_DUMP,
	T_EOF,
} token_E;

typedef struct TOKEN_T {
	token_E type;
	char* value;
} token_T;

token_T* token_new(token_E type, char* value);

char* token_get_name(token_E type);

bool token_is_op(token_T* token);
#endif // !TOKEN_H

