#include "include/token.h"
#include <stdlib.h>

char* token_names[] = { 
	"Integer",
	"Plus",
	"Minus",
	"Multiply",
	"Divide",
	"Equals",
	"Less than",
	"Greater than",
	"Let",
	"Assign",
	"Identifier",
	"Semi",
	"Left Curly Bracket",
	"Right Curly Bracket",
	"While",
	"If",
	"Else",
	"Syscall",
	"Dump",
	"EOF"
};

token_T* token_new(token_E type, char* value) {
	token_T* token = malloc(sizeof(token_T));

	token->type = type;
	token->value = value;

	return token;
}

char* token_get_name(token_E type) {
	return token_names[type];
}

bool token_is_op(token_T* token) {
	return token->type == T_PLUS || token->type == T_MINUS || token->type == T_MULTIPLY || token->type == T_DIVIDE;
}
