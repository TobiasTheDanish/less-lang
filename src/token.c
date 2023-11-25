#include "include/token.h"
#include <stdlib.h>

char* token_names[] = { 
	"Integer",
	"String",
	"Plus",
	"Minus",
	"Multiply",
	"Divide",
	"Modulus",
	"And",
	"Or",
	"Equals",
	"Not equals",
	"Less than",
	"Greater than",
	"Mut",
	"Let",
	"Const",
	"Func",
	"Struct",
	"Assign",
	"Identifier",
	"Comma",
	"Dot",
	"Semi",
	"Colon",
	"Left Square Bracket",
	"Right Square Bracket",
	"Left Curly Bracket",
	"Right Curly Bracket",
	"Left Parenthesis",
	"Right Parenthesis",
	"Pointer",
	"While",
	"If",
	"Else",
	"Syscall",
	"Dump",
	"EOF"
};

token_T* token_new(token_E type, char* value, location_T* loc) {
	token_T* token = malloc(sizeof(token_T));

	token->type = type;
	token->value = value;
	token->loc = loc;

	return token;
}

char* token_get_name(token_E type) {
	return token_names[type];
}

bool token_is_op(token_T* token) {
	return token->type == T_PLUS || token->type == T_MINUS || token->type == T_MULTIPLY || token->type == T_DIVIDE || token->type == T_MODULUS;
}

bool token_is_logical(token_T* token) {
	return token->type == T_AND || token->type == T_OR;
}
