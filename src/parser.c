#include "include/parser.h"
#include "include/ast_nodes.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SYMBOL_SIZE 8

ast_node_T* expr(parser_T* parser);
ast_node_T* if_block(parser_T* parser);

parser_T* parser_new(lexer_T* lexer, size_t t_count) {
	parser_T* parser = malloc(sizeof(parser_T));
	parser->lexer = lexer;
	parser->if_count = 0;
	parser->s_table = symbol_table_new("global", 1, NULL);
	parser->data_table = data_table_new();
	parser->t_count = t_count;
	parser->t_index = 0;
	parser->tokens = malloc(t_count * sizeof(token_T*));

	for (size_t i = 0; i < t_count; i++) {
		parser->tokens[i] = lexer_next_token(lexer);
	}

	return parser;
}

void consume(parser_T* parser) {
	//printf("Token: (%s, %s)\n", token_get_name(parser->tokens[parser->t_index]->type), parser->tokens[parser->t_index]->value);

	parser->tokens[parser->t_index] = lexer_next_token(parser->lexer);
	parser->t_index = (parser->t_index + 1) % parser->t_count;
}

// value: (POINTER | ID | INT | STRING);
ast_node_T* value(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_POINTER: 
			if (symbol_table_contains(parser->s_table, token->value)) {
				symbol_var_T* var = (symbol_var_T*)symbol_table_get(parser->s_table, token->value);
				//printf("<%s, %s>\n", token->value, var->type->name);
				res = ast_new_value(token, var->type);
				consume(parser);
				break;
			} else {
				printf("Use of '%s', before declaration.\n", token->value);
				exit(1);
			}
		case T_IDENT:
			if (symbol_table_contains(parser->s_table, token->value)) {
				symbol_var_T* var = (symbol_var_T*)symbol_table_get(parser->s_table, token->value);
				//printf("<%s, %s>\n", token->value, var->type->name);
				res = ast_new_value(token, var->type);
				consume(parser);
				break;
			} else {
				printf("Use of '%s', before declaration.\n", token->value);
				exit(1);
			}
		case T_INTEGER:
			//printf("<%s, %s>\n", token->value, "int");
			res = ast_new_value(token, symbol_table_get(parser->s_table, "int"));
			consume(parser);
			break;
		case T_STRING:
			data_table_put(parser->data_table, token);
			//printf("<%s, %s>\n", token->value, "string");
			res = ast_new_value(token, symbol_table_get(parser->s_table, "string"));
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
		case T_MODULUS:
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
	ast_value_T* lhs = (ast_value_T*) value(parser);
	ast_node_T* operation = op(parser);

	ast_node_T* rhs;
	symbol_type_T* type;
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) % parser->t_count];

	if ((current->type == T_INTEGER || current->type == T_IDENT) && token_is_op(next)) {
		rhs = bin_op(parser);
		ast_bin_op_T* b = (ast_bin_op_T*) rhs;
		if (strcmp(lhs->type_sym->name, b->type_sym->name) == 0) {
			type = (symbol_type_T*) lhs->type_sym;
		} else {
			printf("Mismatched types for bin op. Found: '%s' and '%s'\n", lhs->type_sym->name, b->type_sym->name);
			exit(1);
		}
	} else if (current->type == T_INTEGER || current->type == T_IDENT) {
		rhs = value(parser);
		ast_value_T* b = (ast_value_T*) rhs;
		if (lhs->type_sym != NULL && b->type_sym != NULL && strcmp(lhs->type_sym->name, b->type_sym->name) == 0) {
			type = (symbol_type_T*) lhs->type_sym;
		} else {
			printf("Mismatched types for bin op. Found: '%s' and '%s'\n", lhs->type_sym->name, b->type_sym->name);
			exit(1);
		}
	} else {
		printf("Invalid token type for rhs in bin_op. Found: %s, expected an identifier, value or bin_op\n", token_get_name(current->type));
		exit(1);
	}

	return ast_new_bin_op((ast_node_T*)lhs, operation, rhs, (symbol_T*)type);
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
//else : ELSE (if | block) ;
ast_node_T* else_block(parser_T* parser, size_t index) {
	consume(parser);
	token_T* next = parser->tokens[parser->t_index];
	ast_node_T* b;

	switch (next->type) {
		case T_IF: 
			b = if_block(parser);
			break;
		case T_LCURLY:
			b = block(parser);
			break;

		default:
			printf("Invalid token type for else_block. Found: %s.\n", token_get_name(next->type));
			exit(1);
	}

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
	size_t index = parser->if_count++;
	consume(parser);
	ast_node_T* cond = conditional(parser);
	ast_node_T* b = block(parser);
	ast_node_T* elze = NULL;

	if (parser->tokens[parser->t_index]->type == T_ELSE) {
		elze = else_block(parser, index);
	}

	return ast_new_if(index, cond, b, elze);
}

// assign : ID ASSIGN (value | bin_op) ;
ast_node_T* assign(parser_T* parser) {
	token_T* ident = parser->tokens[parser->t_index];
	if (symbol_table_contains(parser->s_table, ident->value)) {
		symbol_var_T* symbol = (symbol_var_T*)symbol_table_get(parser->s_table, ident->value);
		consume(parser);
		consume(parser);
		ast_node_T* v;
		if (token_is_op(parser->tokens[(parser->t_index + 1) %parser->t_count])) {
			v = bin_op(parser);
			ast_bin_op_T* val = (ast_bin_op_T*) v;
			symbol->type = val->type_sym;
		} else {
			v = value(parser);
			ast_value_T* val = (ast_value_T*) v;
			symbol->type = val->type_sym;
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
		symbol_T* s = symbol_new_var(name, NULL, 0);

		symbol_table_put(parser->s_table, s);
		ast_node_T* a = assign(parser);
		consume(parser);
		return ast_new_var_decl(a);
	} else {
		printf("Redeclaration of existing variable '%s'.\n", parser->tokens[parser->t_index]->value);
		exit(1);
	}

}

// sys_arg : (bin_op | value);
ast_node_T* sys_arg(parser_T* parser) {
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) %parser->t_count];
	if (current->type == T_IDENT || current->type == T_POINTER || current->type == T_INTEGER || current->type == T_STRING) { 
		if (token_is_op(next)) {
			return bin_op(parser);
		} else {
			return value(parser);
		}
	} else {
		printf("Unexpected token in param, found: %s.\n", token_get_name(current->type));
		exit(1);
	}
}

// syscall : LPAREN (sys_arg (COMMA sys_arg)*)? RPAREN;
ast_node_T* syscall(parser_T* parser) {
	consume(parser);
	if (parser->tokens[parser->t_index]->type == T_LPAREN) {
		consume(parser);

		ast_node_T** params = malloc(sizeof(ast_node_T*));
		size_t count = 0;

		while (parser->tokens[parser->t_index]->type != T_RPAREN) {
			//printf("current param: (%s, %s)\n", token_get_name(parser->tokens[parser->t_index]->type), parser->tokens[parser->t_index]->value);
			params[count++] = sys_arg(parser);
			params = realloc(params, (count + 1) * sizeof(ast_node_T*));
			if (parser->tokens[parser->t_index]->type == T_COMMA) {
				consume(parser);
			}
		}
		consume(parser);

		return ast_new_syscall(params, count);
	}

	printf("Expected list of params for syscall.\n");
	exit(1);
}

// func_param : ID COLON ID ;
token_T* func_param(parser_T* parser, symbol_func_T* func) {
	token_T* param = parser->tokens[parser->t_index];
	consume(parser);
	consume(parser);
	token_T* param_type = parser->tokens[parser->t_index];

	symbol_T* type_sym = symbol_table_get(parser->s_table, param_type->value);

	if (type_sym != NULL) {
		switch (type_sym->type) {
			case SYM_VAR_TYPE:
				{
					symbol_T* var_sym = symbol_new_var(param->value, type_sym, 1);
					func_add_param(func, var_sym);
					symbol_table_put(parser->s_table, var_sym);
				}
				break;

			default:
				printf("Unknown type annotation. Found %s\n", param_type->value);
				exit(1);
		}
	} else {
		printf("Unknown type annotation. Found %s\n", param_type->value);
		exit(1);
	}
	consume(parser);

	return param;
}

// func_decl : FUNC ID LPAREN (func_param (COMMA func_param)*)? RPAREN block ;
ast_node_T* func_decl(parser_T* parser) {
	consume(parser);
	token_T* ident = parser->tokens[parser->t_index];
	symbol_T* func_sym = symbol_new_func(ident->value);

	symbol_table_T* func_table = symbol_table_new(ident->value, parser->s_table->level+1, parser->s_table);
	symbol_table_T* parent = parser->s_table;
	parser->s_table = func_table;

	consume(parser);
	consume(parser);

	token_T** params = malloc(sizeof(token_T*));
	size_t count = 0;

	while (parser->tokens[parser->t_index]->type != T_RPAREN) {
		params[count++] = func_param(parser, (symbol_func_T*) func_sym);
		params = realloc(params, (count + 1) * sizeof(token_T*));

		if (parser->tokens[parser->t_index]->type == T_COMMA) {
			consume(parser);
		}
	}
	consume(parser);

	ast_node_T* b = block(parser);

	symbol_table_print(parser->s_table);

	func_table = parser->s_table;
	parser->s_table = parent;
	symbol_table_put_child(parser->s_table, func_table);
	symbol_table_put(parser->s_table, (symbol_T*) func_sym);

	return ast_new_func_decl(ident, params, count, b);
}

// arg : (value | bin_op) ;
ast_node_T* arg(parser_T* parser, symbol_var_T* param) {
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[((parser->t_index + 1) %parser->t_count)];
	if (current->type == T_IDENT || current->type == T_POINTER || current->type == T_INTEGER || current->type == T_STRING) { 
		if (token_is_op(next)) {
			ast_node_T* n = bin_op(parser);
			ast_bin_op_T* b = (ast_bin_op_T*)n;
			if (strcmp(b->type_sym->name, param->type->name) != 0) {
				printf("Mismatched argument types in function call, found: %s, expected: %s.\n", b->type_sym->name, param->type->name);
				exit(1);
			}

			return n;
		} else {
			ast_node_T* n = value(parser);
			ast_value_T* val = (ast_value_T*)n;
			if (strcmp(val->type_sym->name, param->type->name) != 0) {
				printf("Mismatched argument types in function call, found: %s, expected: %s.\n", val->type_sym->name, param->type->name);
				exit(1);
			}

			return n;
		}
	} else {
		printf("Unexpected token in param, found: %s.\n", token_get_name(current->type));
		exit(1);
	}
}

// func_call : ID LPAREN (arg (COMMA arg)*)? RPAREN ;
ast_node_T* func_call(parser_T* parser) {
	token_T* ident = parser->tokens[parser->t_index];
	symbol_T* func_sym = symbol_table_get(parser->s_table, ident->value);
	if (func_sym == NULL || func_sym->type != SYM_FUNC) {
		printf("Unknown function, is attempted to be called, found: %s.\n", token_get_name(ident->type));
		exit(1);
	}
	symbol_func_T* func = (symbol_func_T*) func_sym;
	consume(parser);
	consume(parser);

	ast_node_T** params = calloc(1, sizeof(ast_node_T*));
	size_t count = 0;

	while (parser->tokens[parser->t_index]->type != T_RPAREN) {
		if (count >= func->param_count) {
			printf("To many arguments for function %s. Found %zu, expected %zu\n", func->base.name, count, func->param_count);
			exit(1);
		}

		params[count] = arg(parser, (symbol_var_T*)func->params[count]);
		count += 1;
		params = realloc(params, (count + 1) * sizeof(ast_node_T*));
		if (parser->tokens[parser->t_index]->type == T_COMMA) {
			consume(parser);
		}
	}
	if (count < func->param_count) {
		printf("To few arguments for function %s. Found %zu, expected %zu\n", func->base.name, count, func->param_count);
		exit(1);
	}
	consume(parser);

	return ast_new_func_call(ident, params, count);
}

// expr : syscall SEMI | if | while | var_decl | assign SEMI | bin_op SEMI | dump SEMI | func_decl | func_call SEMI ;
ast_node_T* expr(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T* child;

	switch (token->type) {
		case T_SYSCALL:
			child = syscall(parser);
			consume(parser);
			break;
		case T_IF:
			child = if_block(parser);
			break;
		case T_WHILE:
			child = while_block(parser);
			break;
		case T_DUMP:
			child = dump(parser);
			consume(parser);
			break;
		case T_LET:
			child = var_decl(parser);
			break;
		case T_FUNC:
			child = func_decl(parser);
			break;
		case T_IDENT:
			{
				token_T* next = parser->tokens[(parser->t_index + 1) %parser->t_count];
				if (next->type == T_ASSIGN) {
					child = assign(parser);
					consume(parser);
				} else if (next->type == T_LPAREN) {
					child = func_call(parser);
					consume(parser);
				} else {
					child = bin_op(parser);
					consume(parser);
				}
				break;
			}
		case T_INTEGER:
			child = bin_op(parser);
			consume(parser);
			break;

		default:
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

	symbol_table_init_builtins(parser->s_table);

	while (token->type != T_EOF) {
		expressions[count++] = expr(parser);
		//count += 1;

		expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T*));

		token = parser->tokens[parser->t_index];
		//printf("number of expressions: %zu\n", count);
	} 

	symbol_table_print(parser->s_table);

	return ast_new_program(expressions, count);
}

ast_node_T* parser_parse(parser_T* parser) {
	return program(parser);
}


