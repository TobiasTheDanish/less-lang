#include "include/parser.h"
#include "include/ast_nodes.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stdio.h>
#include <stdlib.h>

#define SYMBOL_SIZE 8

ast_node_T* expr(parser_T* parser);

parser_T* parser_new(lexer_T* lexer, size_t t_count) {
	parser_T* parser = malloc(sizeof(parser_T));
	parser->lexer = lexer;
	parser->if_count = 0;
	parser->s_table = symbol_table_new();
	parser->t_count = t_count;
	parser->t_index = 0;
	parser->tokens = malloc(t_count * sizeof(token_T*));

	for (size_t i = 0; i < t_count; i++) {
		parser->tokens[i] = lexer_next_token(lexer);
	}

	return parser;
}

void consume(parser_T* parser) {
	printf("Token: (%s, %s)\n", token_get_name(parser->tokens[parser->t_index]->type), parser->tokens[parser->t_index]->value);

	parser->tokens[parser->t_index] = lexer_next_token(parser->lexer);
	parser->t_index = (parser->t_index + 1) % parser->t_count;
}

// value: (ID | INT);
ast_node_T* value(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_IDENT:
			if (symbol_table_contains(parser->s_table, token->value)) {
				res = ast_new_value(token);
				consume(parser);
				break;
			} else {
				printf("Use of '%s', before declaration.\n", token->value);
				exit(1);
			}
		case T_INTEGER:
			res = ast_new_value(token);
			consume(parser);
			break;

		default:
			printf("Invalid token type for value. Found: %s.\n", token_get_name(token->type));
			exit(1);
	}

	return res;
}

// op: (PLUS | MINUS | MULTIPLY | DIVIDE);
ast_node_T* op(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_PLUS:
		case T_MINUS:
		case T_MULTIPLY:
		case T_DIVIDE:
			res = ast_new_op(token);
			consume(parser);
			break;

		default:
			printf("Invalid token type for op. Found: %s.\n", token_get_name(token->type));
			exit(1);
	}

	return res;
}


// bin_op: value op (bin_op | value) ;
ast_node_T* bin_op(parser_T* parser) {
	ast_node_T* lhs = value(parser);
	ast_node_T* operation = op(parser);

	ast_node_T* rhs;
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) % parser->t_count];

	if ((current->type == T_INTEGER || current->type == T_IDENT) && token_is_op(next)) {
		rhs = bin_op(parser);
	} else if (current->type == T_INTEGER || current->type == T_IDENT) {
		rhs = value(parser);
	} else {
		printf("Invalid token type for rhs in bin_op. Found: %s, expected an identifier, value or bin_op", token_get_name(current->type));
		exit(1);
	}

	return ast_new_bin_op(lhs, operation, rhs);
}

// dump: DUMP (bin_op | value);
ast_node_T* dump(parser_T* parser) {
	consume(parser);
	ast_node_T* val;
	if (token_is_op(parser->tokens[(parser->t_index+1) % parser->t_count])) {
		val = bin_op(parser);
	} else {
		val = value(parser);
	}

	return ast_new_dump(val);
}

// cond_op : (EQUALS | LESS | GREATER);
ast_node_T* cond_op(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_EQUALS:
		case T_LESS:
		case T_GREATER:
			res = ast_new_cond_op(token);
			consume(parser);
			break;

		default:
			printf("Invalid token type for cond_op. Found: %s.\n", token_get_name(token->type));
			exit(1);
	}

	return res;
}


// conditional : (value | bin_op) cond_op (value | bin_op) ;
ast_node_T* conditional(parser_T* parser) {
	ast_node_T* lhs;
	if (token_is_op(parser->tokens[(parser->t_index + 1) % parser->t_count])) {
		lhs = bin_op(parser);
	} else {
		lhs = value(parser);
	}
	ast_node_T* operation = cond_op(parser);
	ast_node_T* rhs;
	if (token_is_op(parser->tokens[(parser->t_index + 1) % parser->t_count])) {
		rhs = bin_op(parser);
	} else {
		rhs = value(parser);
	}

	return ast_new_cond(lhs, operation, rhs);
}

// block : LCURLY (expr)* RCURLY ;
ast_node_T* block(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T** expressions = malloc(sizeof(ast_node_T*));
	size_t count = 0;

	if (token->type == T_LCURLY) {
		consume(parser);

		while (token->type != T_RCURLY) {
			expressions[count++] = expr(parser);
			//count += 1;

			expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T*));

			token = parser->tokens[parser->t_index];
			//printf("number of expressions: %zu\n", count);
		} 
		consume(parser);
	}

	return ast_new_block(expressions, count);
}
//else : ELSE block ;
ast_node_T* else_block(parser_T* parser) {
	size_t index = parser->if_count;
	consume(parser);
	ast_node_T* b = block(parser);

	return ast_new_else(index, b);
}

// while : WHILE conditional block ;
ast_node_T* while_block(parser_T* parser) {
	size_t index = ++parser->if_count;
	consume(parser);
	ast_node_T* cond = conditional(parser);
	ast_node_T* b = block(parser);

	return ast_new_while(index, cond, b);
}

// if : IF conditional block (else)? ;
ast_node_T* if_block(parser_T* parser) {
	size_t index = ++parser->if_count;
	consume(parser);
	ast_node_T* cond = conditional(parser);
	ast_node_T* b = block(parser);
	ast_node_T* elze = NULL;

	if (parser->tokens[parser->t_index]->type == T_ELSE) {
		elze = else_block(parser);
	}

	return ast_new_if(index, cond, b, elze);
}

// assign : ID ASSIGN (value | bin_op) ;
ast_node_T* assign(parser_T* parser) {
	token_T* ident = parser->tokens[parser->t_index];
	if (symbol_table_contains(parser->s_table, ident->value)) {
		consume(parser);
		consume(parser);
		ast_node_T* v;
		if (token_is_op(parser->tokens[(parser->t_index + 1) %parser->t_count])) {
			v = bin_op(parser);
		} else {
			v = value(parser);
		}

		return ast_new_assign(ident, v);
	} else {
		printf("Use of '%s', before declaration.\n", ident->value);
		exit(1);
	}

}

// var_decl : LET assign SEMI ;
ast_node_T* var_decl(parser_T* parser) {
	consume(parser);
	if (!symbol_table_contains(parser->s_table, parser->tokens[parser->t_index]->value)) {
		char* name = parser->tokens[parser->t_index]->value;
		symbol_T* s = symbol_new(name, SYMBOL_SIZE);

		symbol_table_put(parser->s_table, s);
		ast_node_T* a = assign(parser);
		consume(parser);
		return ast_new_var_decl(a);
	} else {
		printf("Redeclaration of existing variable '%s'.\n", parser->tokens[parser->t_index]->value);
		exit(1);
	}

}

// expr : if | while | var_decl | assign SEMI | bin_op SEMI | dump SEMI;
ast_node_T* expr(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T* child;

	if (token->type == T_IF) {
		child = if_block(parser);
	} else if (token->type == T_WHILE) {
		child = while_block(parser);
	} else if (token->type == T_DUMP) {
		child = dump(parser);
		consume(parser);
	} else if (token->type == T_LET) {
		child = var_decl(parser);
	} else if (token->type == T_IDENT) {
		if (parser->tokens[(parser->t_index + 1) %parser->t_count]->type == T_ASSIGN) {
			child = assign(parser);
			consume(parser);
		} else {
			child = bin_op(parser);
			consume(parser);
		}
	} else if (token->type == T_INTEGER) {
		child = bin_op(parser);
		consume(parser);
	}else {
		printf("Invalid token type in expression. Found: %s.\n", token_get_name(token->type));
		exit(1);
	}

	return ast_new_expr(child);
}

// program : (expr)* ;
ast_node_T* program(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T** expressions = malloc(sizeof(ast_node_T*));
	size_t count = 0;

	while (token->type != T_EOF) {
		expressions[count++] = expr(parser);
		//count += 1;

		expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T*));

		token = parser->tokens[parser->t_index];
		//printf("number of expressions: %zu\n", count);
	} 

	return ast_new_program(expressions, count);
}

ast_node_T* parser_parse(parser_T* parser) {
	return program(parser);
}


