#include "include/parser.h"
#include "include/ast_nodes.h"
#include "include/data_table.h"
#include "include/logger.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SYMBOL_SIZE 8

ast_node_T* func_call(parser_T* parser, ast_node_T* arg1);
ast_node_T* syscall(parser_T* parser);
ast_node_T* array_expr(parser_T* parser);
ast_node_T* expr(parser_T* parser);
ast_node_T* if_block(parser_T* parser);
ast_node_T* array_element(parser_T* parser);
ast_node_T* value(parser_T* parser);

parser_T* parser_new(lexer_T* lexer, size_t t_count, unsigned char debug_info) {
	parser_T* parser = malloc(sizeof(parser_T));
	parser->lexer = lexer;
	parser->if_count = 0;
	parser->s_table = symbol_table_new("global", 1, NULL);
	parser->data_table = data_table_new();
	parser->t_count = t_count;
	parser->t_index = 0;
	parser->tokens = malloc(t_count * sizeof(token_T*));
	parser->debug = debug_info;

	for (size_t i = 0; i < t_count; i++) {
		parser->tokens[i] = lexer_next_token(lexer);
	}

	return parser;
}

void consume(parser_T* parser, token_E expected_type) {
	token_T* t = parser->tokens[parser->t_index];
	log_debug(parser->debug, "Token: (%s, %s)\n", token_get_name(t->type), t->value);
	if (t->type != expected_type) {
		log_error(t->loc, 1, "Parsing error: Token '%s' of type '%s', is not of the expected type '%s'\n", t->value, token_get_name(t->type), token_get_name(expected_type));
	}

	parser->tokens[parser->t_index] = lexer_next_token(parser->lexer);
	parser->t_index = (parser->t_index + 1) % parser->t_count;
}

// type_annotation : COLON ID (LSQUARE RSQUARE)? ;
void type_annotation(parser_T* parser, symbol_T* type_sym, symbol_T* elem_type) {
	log_debug(parser->debug, "parse type annotation\n");
	consume(parser, T_COLON);
	token_T* type = parser->tokens[parser->t_index];
	symbol_T* temp = symbol_table_get(parser->s_table, type->value);
	if (temp == NULL || temp->type != SYM_VAR_TYPE) {
		log_error(type->loc, 1, "Type '%s' is not a known type.\n", type->value);
	}
	consume(parser, T_IDENT);
	log_debug(parser->debug, "Type '%s' found\n", temp->name);

	if (parser->tokens[parser->t_index]->type == T_LSQUARE) {
		memcpy(elem_type, temp, sizeof(symbol_type_T));
		symbol_T* arr = symbol_table_get(parser->s_table, "array");
		if (arr == NULL) {
			log_error(NULL, 1, "Could not find symbol 'array' in symbol table\n");
		}
		memcpy(type_sym, arr, sizeof(symbol_type_T));
		consume(parser, T_LSQUARE);
		consume(parser, T_RSQUARE);
	} else {
		memcpy(type_sym, temp, sizeof(symbol_type_T));
		elem_type = NULL;
	}
}

// prop : (&)? ID DOT (array_element | prop | ID) ; 
ast_node_T* prop(parser_T* parser) {
	log_debug(parser->debug, "Parse prop\n");
	token_T* parent = parser->tokens[parser->t_index];
	symbol_T* parent_sym = symbol_table_get(parser->s_table, parent->value);
	if (parent_sym->type != SYM_VAR) {
		log_error(parent->loc, 1, "Cannot get prop of non variable symbol %s\n", parent->value);
	}
	symbol_var_T* v = (symbol_var_T*) parent_sym;
	unsigned char is_pointer = 0;

	if (parser->tokens[parser->t_index]->type == T_POINTER) {
		is_pointer = 1;
		consume(parser, T_POINTER);
	} else {
		consume(parser, T_IDENT);
	}

	consume(parser, T_DOT);

	token_T* prop_token = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index+1) % parser->t_count];
	if (!symbol_is_prop(v->type, prop_token->value)) {
		if (next->type == T_LPAREN) {
			ast_node_T* arg1 = ast_new_value(parent, v->type);

			ast_node_T* res = func_call(parser, arg1);
			return res;
		}

		log_error(prop_token->loc, 1, "Unknown property '%s' for identifier '%s' of type '%s'\n", prop_token->value, parent->value, v->type->name);
	}

	ast_node_T* child;
	if (next->type == T_LSQUARE) {
		child = array_element(parser);
	} else if (next->type == T_DOT) {
		child = prop(parser);
	} else {
		child = NULL;
		consume(parser, T_IDENT);
	}

	return ast_new_prop(parent_sym, prop_token, child, is_pointer);
}

// value: (POINTER | ID | INT | STRING | CHAR);
ast_node_T* value(parser_T* parser) {
	log_debug(parser->debug, "Parse value\n");
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_POINTER: 
			if (symbol_table_contains(parser->s_table, token->value)) {
				symbol_var_T* var = (symbol_var_T*)symbol_table_get(parser->s_table, token->value);
				//log_debug(parser->debug, "<%s, %s>\n", token->value, var->type->name);
				res = ast_new_value(token, var->type);
				consume(parser, T_POINTER);
				break;
			} else {
				log_error(token->loc, 1, "Use of '%s', before declaration.\n", token->value);
			}
		case T_IDENT:
			if (symbol_table_contains(parser->s_table, token->value)) {
				symbol_var_T* var = (symbol_var_T*)symbol_table_get(parser->s_table, token->value);
				//log_debug(parser->debug, "<%s, %s>\n", token->value, var->type->name);
				res = ast_new_value(token, var->type);
				consume(parser, T_IDENT);
				break;
			} else {
				log_error(token->loc, 1, "Use of '%s', before declaration.\n", token->value);
			}
		case T_INTEGER:
			{
				//printf("<%s, %s>\n", token->value, "int");
				char* type;
				if (atoi(token->value) < 255) type = "i8";
				else if (atoi(token->value) < 65535) type = "i16";
				else type = "i32";

				res = ast_new_value(token, symbol_table_get(parser->s_table, type));
				consume(parser, T_INTEGER);
			}
			break;
		case T_STRING:
			data_table_put(parser->data_table, token, "string");
			//printf("<%s, %s>\n", token->value, "string");
			res = ast_new_value(token, symbol_table_get(parser->s_table, "string"));
			consume(parser, T_STRING);
			break;

		case T_CHAR: 
			res = ast_new_value(token, symbol_table_get(parser->s_table, "i8"));
			consume(parser, T_CHAR);
			break;

		default:
			log_error(token->loc, 1, "Invalid token type for value. Found: %s.\n", token_get_name(token->type));
	}

	return res;
}

// op: (PLUS | MINUS | MULTIPLY | DIVIDE);
ast_node_T* op(parser_T* parser) {
	log_debug(parser->debug, "Parse op\n");
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_PLUS:
			res = ast_new_op(token);
			consume(parser, T_PLUS);
			break;
		case T_MINUS:
			res = ast_new_op(token);
			consume(parser, T_MINUS);
			break;
		case T_MULTIPLY:
			res = ast_new_op(token);
			consume(parser, T_MULTIPLY);
			break;
		case T_DIVIDE:
			res = ast_new_op(token);
			consume(parser, T_DIVIDE);
			break;
		case T_MODULUS:
			res = ast_new_op(token);
			consume(parser, T_MODULUS);
			break;

		default:
			log_error(token->loc, 1, "Invalid token type for op. Found: %s.\n", token_get_name(token->type));
	}

	return res;
}


// bin_op: (value | prop | array_element) op (bin_op | value | prop | array_element) ;
ast_node_T* bin_op(parser_T* parser, ast_node_T* lhs) {
	log_debug(parser->debug, "Parse bin op\n");
	token_T* current = parser->tokens[parser->t_index];
	symbol_type_T* lhs_type;
	if (lhs->type == AST_PROP) {
		ast_prop_T* v = (ast_prop_T*)lhs;
		symbol_var_T* prop_sym = (symbol_var_T*) symbol_table_get(parser->s_table, v->parent_sym->name);
		lhs_type = (symbol_type_T*)prop_sym->elem_type;
	} else if (lhs->type == AST_ARRAY_ELEMENT) {
		ast_array_element_T* v = (ast_array_element_T*)lhs;
		symbol_var_T* elem_sym = (symbol_var_T*) symbol_table_get(parser->s_table, v->ident->value);
		lhs_type = (symbol_type_T*)elem_sym->elem_type;
	} else {
		ast_value_T* v = (ast_value_T*) lhs;
		lhs_type = (symbol_type_T*)v->type_sym;
	}
	ast_node_T* operation = op(parser);

	ast_node_T* rhs;
	symbol_type_T* type;
	current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) % parser->t_count];

	if ((current->type == T_INTEGER || current->type == T_IDENT) && next->type == T_LSQUARE) {
		rhs = array_element(parser);
		ast_array_element_T* v = (ast_array_element_T*) rhs;
		symbol_var_T* elem_sym = (symbol_var_T*) symbol_table_get(parser->s_table, v->ident->value);
		// && strcmp(lhs_type->base.name, elem_sym->elem_type->name) == 0
		if (lhs_type != NULL && elem_sym->elem_type != NULL) {
			type = lhs_type;
		} 
		/*
		else {
			log_error(current->loc, 1, "Mismatched types for bin op. Found: '%s' and '%s'\n", lhs_type->base.name, elem_sym->elem_type->name);
		}
		*/
	} else if ((current->type == T_INTEGER || current->type == T_IDENT) && next->type == T_DOT) {
		rhs = prop(parser);
		ast_prop_T* v = (ast_prop_T*) rhs;
		symbol_var_T* prop_sym = (symbol_var_T*) symbol_table_get(parser->s_table, v->parent_sym->name);
		// && strcmp(lhs_type->base.name, prop_sym->elem_type->name) == 0
		if (lhs_type != NULL && prop_sym->type != NULL) {
			type = lhs_type;
		} 
		/*
		else {
			log_error(current->loc, 1, "Mismatched types for bin op. Found: '%s' and '%s'\n", lhs_type->base.name, prop_sym->elem_type->name);
		}
		*/
	} else if (current->type == T_INTEGER || current->type == T_IDENT) {
		rhs = value(parser);
		ast_value_T* v = (ast_value_T*) rhs;
		// && strcmp(lhs_type->base.name, v->type_sym->name) == 0
		if (lhs_type != NULL && v->type_sym != NULL) {
			type = lhs_type;
		} 
		/*
		else {
			log_error(current->loc, 1, "Mismatched types for bin op. Found: '%s' and '%s'\n", lhs_type->base.name, v->type_sym->name);
		}
		*/
	} else {
		log_error(current->loc, 1, "Invalid token type for rhs in bin_op. Found: %s, expected an identifier, value or bin_op\n", token_get_name(current->type));
	}

	current = parser->tokens[parser->t_index];
	if (token_is_op(current)) {
		rhs = bin_op(parser, rhs);
		ast_bin_op_T* b = (ast_bin_op_T*) rhs;
		// && strcmp(lhs_type->base.name, b->type_sym->name) == 0
		if (lhs_type != NULL && b->type_sym != NULL) {
			type = lhs_type;
		} 
		/*
		else {
			log_error(current->loc, 1, "Mismatched types for bin op. Found: '%s' and '%s'\n", lhs_type->base.name, b->type_sym->name);
		}
		*/
	}

	return ast_new_bin_op((ast_node_T*)lhs, operation, rhs, (symbol_T*)type);
}

// array_element : ID LSQUARE (array_element | IDENT | INTEGER | bin_op | prop) RSQUARE ;
ast_node_T* array_element(parser_T* parser) {
	log_debug(parser->debug, "Parse array element\n");
	token_T* ident = parser->tokens[parser->t_index];

	consume(parser, T_IDENT);
	consume(parser, T_LSQUARE);

	ast_node_T* offset;

	token_T* next = parser->tokens[(parser->t_index+1) % parser->t_count];
	if (next->type == T_LSQUARE) {
		offset = array_element(parser);
	} else if (next->type == T_DOT) {
		offset = prop(parser);
	} else {
		offset = value(parser);
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		offset = bin_op(parser, offset);
	}

	consume(parser, T_RSQUARE);

	return ast_new_array_element(ident, offset);
}

// dump: DUMP (bin_op | value | array_element | prop);
ast_node_T* dump(parser_T* parser) {
	log_debug(parser->debug, "Parse dump\n");
	consume(parser, T_DUMP);

	ast_node_T* val;
	token_T* next = parser->tokens[(parser->t_index+1) % parser->t_count];
	if (next->type == T_LSQUARE) {
		val = array_element(parser);
	} else if (next->type == T_DOT) {
		val = prop(parser);
	} else {
		val = value(parser);
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		val = bin_op(parser, val);
	} 

	return ast_new_dump(val);
}

// cond_op : (EQUALS | NOT_EQUALS | LESS | GREATER);
ast_node_T* cond_op(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_EQUALS:
			res = ast_new_cond_op(token);
			consume(parser, T_EQUALS);
			break;
		case T_NOT_EQUALS:
			res = ast_new_cond_op(token);
			consume(parser, T_NOT_EQUALS);
			break;
		case T_LESS:
			res = ast_new_cond_op(token);
			consume(parser, T_LESS);
			break;
		case T_GREATER:
			res = ast_new_cond_op(token);
			consume(parser, T_GREATER);
			break;

		default:
			log_error(token->loc, 1, "Invalid token type for cond_op. Found: %s.\n", token_get_name(token->type));
	}

	return res;
}

// logical_op : (AND | OR) ;
ast_node_T* logical_op(parser_T* parser) {
	ast_node_T* res;
	token_T* token = parser->tokens[parser->t_index];

	switch (token->type) {
		case T_AND:
			res = ast_new_logical_op(token);
			consume(parser, T_AND);
			break;
		case T_OR:
			res = ast_new_logical_op(token);
			consume(parser, T_OR);
			break;

		default:
			log_error(token->loc, 1, "Invalid token type for cond_op. Found: %s.\n", token_get_name(token->type));
	}

	return res;
}

// conditional : (array_element | value | bin_op | prop) cond_op (value | bin_op | prop | array_element) (logical_op conditional)* ;
ast_node_T* conditional(parser_T* parser) {
	ast_node_T* lhs;
	token_T* next = parser->tokens[(parser->t_index + 1) % parser->t_count];
	if (next->type == T_DOT) {
		lhs = prop(parser);
	} else if (next->type == T_LSQUARE) {
		lhs = array_element(parser);
	} else {
		lhs = value(parser);
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		lhs = bin_op(parser, lhs);
	}

	ast_node_T* operation = cond_op(parser);
	ast_node_T* rhs;
	next = parser->tokens[(parser->t_index + 1) % parser->t_count];
	if (next->type == T_DOT) {
		rhs = prop(parser);
	} else if (next->type == T_LSQUARE) {
		rhs = array_element(parser);
	} else {
		rhs = value(parser);
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		rhs = bin_op(parser, rhs);
	}
	
	next = parser->tokens[parser->t_index];
	if (token_is_logical(next)) {
		ast_node_T* logical = logical_op(parser);
		ast_node_T* c = conditional(parser);

		return ast_new_cond(lhs, operation, rhs, logical, c);
	}

	return ast_new_cond(lhs, operation, rhs, NULL, NULL);
}

// block : LCURLY (expr)* RCURLY ;
ast_node_T* block(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T** expressions = malloc(sizeof(ast_node_T*));
	size_t count = 0;

	if (token->type == T_LCURLY) {
		consume(parser, T_LCURLY);

		while (token->type != T_RCURLY) {
			expressions[count++] = expr(parser);
			//count += 1;

			expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T*));

			token = parser->tokens[parser->t_index];
		} 
		consume(parser, T_RCURLY);
	}

	return ast_new_block(expressions, count);
}
//else : ELSE (if | block) ;
ast_node_T* else_block(parser_T* parser, size_t index) {
	consume(parser, T_ELSE);
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
			log_error(next->loc, 1, "Invalid token type for else_block. Found: %s.\n", token_get_name(next->type));
	}

	return ast_new_else(index, b);
}

// while : WHILE conditional block ;
ast_node_T* while_block(parser_T* parser) {
	size_t index = ++parser->if_count;
	consume(parser, T_WHILE);
	ast_node_T* cond = conditional(parser);
	ast_node_T* b = block(parser);

	return ast_new_while(index, cond, b);
}

// if : IF conditional block (else)? ;
ast_node_T* if_block(parser_T* parser) {
	size_t index = parser->if_count++;
	consume(parser, T_IF);
	ast_node_T* cond = conditional(parser);
	ast_node_T* b = block(parser);
	ast_node_T* elze = NULL;

	if (parser->tokens[parser->t_index]->type == T_ELSE) {
		elze = else_block(parser, index);
	}

	return ast_new_if(index, cond, b, elze);
}

// array : ID LSQUARE INTEGER RSQUARE ;
ast_node_T* array(parser_T* parser) {
	token_T* ident = parser->tokens[parser->t_index];
	symbol_T* elem_type = symbol_table_get(parser->s_table, ident->value);
	symbol_T* type = symbol_table_get(parser->s_table, "array");

	if (elem_type == NULL || elem_type->type != SYM_VAR_TYPE) {
		log_error(ident->loc, 1, "Unexpected type of array, found: '%s'.\n", ident->value);
	}

	consume(parser, T_IDENT);
	consume(parser, T_LSQUARE);

	token_T* size = parser->tokens[parser->t_index];

	consume(parser, T_INTEGER);
	consume(parser, T_RSQUARE);
	return ast_new_array(type, elem_type, size);
}

// attribute : ID COLON (value | array_element | prop | bin_op | array) COMMA?
ast_node_T* attribute(parser_T* parser, symbol_type_T* type) {
	token_T* ident = parser->tokens[parser->t_index];
	if(!symbol_is_prop((symbol_T *)type, ident->value)) {
		log_error(ident->loc, 1, "Symbol '%s' is not a propery of type '%s'.\n", ident->value, type->base.name);
	}
	symbol_T* prop_type = symbol_get_prop_type((symbol_T*)type, ident->value);
	consume(parser, T_IDENT);
	consume(parser, T_COLON);

	symbol_T* val_sym = symbol_table_get(parser->s_table, parser->tokens[parser->t_index]->value);
	ast_node_T* val;
	symbol_T* val_type;
	token_T* next = parser->tokens[(parser->t_index + 1) % parser->t_count];
	if (next->type == T_DOT) {
		val = prop(parser);
		ast_prop_T* p = (ast_prop_T*) val;
		symbol_var_T* var = (symbol_var_T*)val_sym;
		val_type = (symbol_T*) symbol_get_prop_type(var->type, p->prop->value);
	} else if (val_sym != NULL && val_sym->type == SYM_VAR_TYPE && next->type == T_LSQUARE) {
		val = array(parser);
		ast_array_T* arr = (ast_array_T*) val;
		val_type = arr->type;
	} else if (val_sym != NULL && val_sym->type == SYM_VAR && next->type == T_LSQUARE) {
		val = array_element(parser);
		symbol_var_T* var = (symbol_var_T*)val_sym;
		val_type = var->elem_type;
	} else {
		val = value(parser);
		ast_value_T* v = (ast_value_T*) val;
		val_type = v->type_sym;
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		val = bin_op(parser, val);
		ast_bin_op_T* v = (ast_bin_op_T*) val;
		val_type = v->type_sym;
	}

	if (val_type == NULL) {
		log_error(ident->loc, 1, "Value in assignment didnt have a type. Expected %s.\n", prop_type->name);
	}
	/*
	if (val_type == NULL || strcmp(val_type->name, prop_type->name) != 0) {
		log_error(ident->loc, 1, "Mismatched types in struct initialization. Found %s and expected %s.\n", val_type->name, prop_type->name);
	}
	*/

	return ast_new_attribute(ident, val);
}

// struct_init : ID LCURLY (attribute)* RCURLY ;
ast_node_T* struct_init(parser_T* parser) {
	log_debug(parser->debug, "Parse struct initializing\n");
	token_T* ident = parser->tokens[parser->t_index];
	if (!symbol_table_contains(parser->s_table, ident->value)) {
		log_error(ident->loc, 1, "Unknown symbol '%s'.\n", ident->value);
	}

	symbol_T* id = symbol_table_get(parser->s_table, ident->value);
	if (id->type != SYM_VAR_TYPE) {
		log_error(ident->loc, 1, "Attempt at initializing a variable of type '%s'. '%s' is not a type.", ident->value, ident->value);
	}
	consume(parser, T_IDENT);
	consume(parser, T_LCURLY);
	symbol_type_T* type = (symbol_type_T*) id;

	ast_node_T** attributes = malloc(sizeof(ast_node_T*));
	size_t attr_count = 0;
	token_T* token = parser->tokens[parser->t_index];
	while (token->type != T_RCURLY) {
		attributes[attr_count++] = attribute(parser, type);
		attributes = realloc(attributes, (attr_count+1) * sizeof(ast_node_T*));

		if (parser->tokens[parser->t_index]->type == T_COMMA) consume(parser, T_COMMA);

		token = parser->tokens[parser->t_index];
	}
	consume(parser, T_RCURLY);

	return ast_new_struct_init(attributes, attr_count, ident->value);
}


// assign : (ID | array_element | prop) ASSIGN (syscall | func_call | value | bin_op | array | array_element | prop | struct_init) ;
ast_node_T* assign(parser_T* parser, ast_node_T* lhs) {
	token_T* ident;
	symbol_T* parent_sym = NULL;
	switch (lhs->type) {
		case AST_ARRAY_ELEMENT:
			{
				ast_array_element_T* e = (ast_array_element_T*) lhs;
				ident = e->ident;
			}
			break;

		case AST_VALUE:
			{
				ast_value_T* e = (ast_value_T*) lhs;
				ident = e->t;
			}
			break;

		case AST_PROP:
			{
				ast_prop_T* e = (ast_prop_T*) lhs;
				ident = e->prop;
				symbol_var_T* parent = (symbol_var_T*)e->parent_sym;
				parent_sym = parent->type;
			}
			break;
				
		default:
			log_error(lhs->loc, 1, "Invalid left hand side of assignment. Cannot assign to %s.\n", ast_get_name(lhs->type));
	}

	symbol_var_T* symbol;
	if (symbol_table_contains(parser->s_table, ident->value)) {
		symbol = (symbol_var_T*)symbol_table_get(parser->s_table, ident->value);
		if (symbol->is_const) {
			log_error(ident->loc, -1, "Cannot reassign constant identifier '%s'\n", ident->value);
			location_T* loc = symbol->base.loc;
			log_info("Constant identifier '%s' first defined here: '%s:%d:%d'\n", ident->value, loc->filePath, loc->row, loc->col);
			exit(1);
		} else if (!symbol->is_mut && symbol->is_assigned && lhs->type != AST_ARRAY_ELEMENT) {
			log_error(ident->loc, -1, "Cannot reassign immutable variable '%s'\n", ident->value);
			location_T* loc = symbol->base.loc;
			log_info("Consider adding 'mut' to declaration of '%s' here: '%s:%d:%d'\n", ident->value, loc->filePath, loc->row, loc->col);
			exit(1);
		}
	} else if (parent_sym != NULL && ident != NULL && symbol_is_prop(parent_sym, ident->value)) {
		symbol = (symbol_var_T*)symbol_table_get(parser->s_table, parent_sym->name);
		if (symbol->is_const) {
			log_error(ident->loc, -1, "Cannot assign to property of constant identifier '%s'\n", ident->value);
			location_T* loc = symbol->base.loc;
			log_info("Constant identifier '%s' defined here: '%s:%d:%d'\n", ident->value, loc->filePath, loc->row, loc->col);
			exit(1);
		} else if (!symbol->is_mut && symbol->is_assigned && lhs->type != AST_ARRAY_ELEMENT) {
			log_error(ident->loc, -1, "Cannot assign to property of immutable variable '%s'\n", ident->value);
			location_T* loc = symbol->base.loc;
			log_info("Consider adding 'mut' to declaration of '%s' here: '%s:%d:%d'\n", ident->value, loc->filePath, loc->row, loc->col);
			exit(1);
		}
		symbol = NULL;
	} else {
		log_error(ident->loc, 1, "Use of '%s', before declaration.\n", ident->value);
		return NULL; //unreachable
	}
	consume(parser, T_ASSIGN);

	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) %parser->t_count];
	symbol_T* s = symbol_table_get(parser->s_table, current->value);
	ast_node_T* v;
	if (current->type == T_SYSCALL) {
		v = syscall(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			symbol->type = symbol_table_get(parser->s_table, "i32");
		}
	} else if (next->type == T_LPAREN) {
		v = func_call(parser, NULL);
		ast_func_call_T* f = (ast_func_call_T*)v;
		symbol_T* sym = symbol_table_get(parser->s_table, f->ident->value);
		if (sym == NULL || sym->type != SYM_FUNC) {
			log_error(f->ident->loc, 1, "Symbol '%s' cannot be called as function.\n", f->ident->value);
		}
		symbol_func_T* f_sym = (symbol_func_T*) sym;
		if (!f_sym->ret_type) {
			log_error(f->ident->loc, 1, "Cannot assign with function '%s' that returns void.\n", f->ident->value);
		}
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			symbol->type = f_sym->ret_type;
		}
	} else if (s == NULL){
		v = value(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_value_T* val = (ast_value_T*) v;
			symbol->type = val->type_sym;
		}
	} else if (s->type == SYM_VAR_TYPE && next->type == T_LCURLY) {
		v = struct_init(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_struct_init_T* structure = (ast_struct_init_T*) v;
			symbol->type = symbol_table_get(parser->s_table, structure->struct_name);
		}
	} else if (s->type == SYM_VAR_TYPE && next->type == T_LSQUARE) {
		v = array(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_array_T* arr = (ast_array_T*) v;
			symbol->type = arr->type;
			symbol->elem_type = arr->elem_type;
		}
	} else if (s->type == SYM_VAR && next->type == T_DOT) {
		v = prop(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_prop_T* p = (ast_prop_T*) v;
			symbol_var_T* var = (symbol_var_T*)s;
			symbol->type = symbol_get_prop_type(var->type, p->prop->value);
		}
	} else if (s->type == SYM_VAR && next->type == T_LSQUARE) {
		v = array_element(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			symbol_var_T* var = (symbol_var_T*)s;
			symbol->type = var->elem_type;
		}
	} else {
		v = value(parser);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_value_T* val = (ast_value_T*) v;
			symbol->type = val->type_sym;
		}
	}

	if (token_is_op(parser->tokens[parser->t_index])) {
		v = bin_op(parser, v);
		if (symbol && symbol->type == NULL && !symbol->is_assigned) {
			ast_bin_op_T* val = (ast_bin_op_T*) v;
			symbol->type = val->type_sym;
		}
	}

	if (symbol) symbol->is_assigned = 1;

	return ast_new_assign(ident, lhs, v);

}

// var_decl : LET (MUT)? ID (COLON ID)? assign SEMI ;
ast_node_T* var_decl(parser_T* parser) {
	unsigned char is_mut = 0;
	consume(parser, T_LET);
	if (parser->tokens[parser->t_index]->type == T_MUT) {
		is_mut = 1;
		consume(parser, T_MUT);
	}
	token_T* token = parser->tokens[parser->t_index];
	if (!symbol_table_contains(parser->s_table, token->value)) {
		char* name = token->value;
		symbol_T* s = symbol_new_var(name, token->loc, NULL, is_mut, 0, 0, 0);
		symbol_table_put(parser->s_table, s);
		ast_node_T* lhs = value(parser);

		if (parser->tokens[parser->t_index]->type == T_COLON) {
			symbol_T* type = malloc(sizeof(symbol_type_T));
			symbol_T* elem_type = malloc(sizeof(symbol_type_T));
			type_annotation(parser, type, elem_type);
			symbol_var_T* var = (symbol_var_T*) s;
			var->type = type;
			var->elem_type = elem_type;
		}

		ast_node_T* a = assign(parser, lhs);

		consume(parser, T_SEMI);
		return ast_new_var_decl(a);
	} else {
		log_error(token->loc, 1, "Redeclaration of existing identifier '%s'.\n", token->value);
		return NULL; //unreachable
	}
}

// const_decl : CONST ID ASSIGN value SEMI ;
ast_node_T* const_decl(parser_T* parser) {
	consume(parser, T_CONST);
	token_T* ident = parser->tokens[parser->t_index];
	if (!symbol_table_contains(parser->s_table, ident->value)) {
		consume(parser, T_IDENT);
		consume(parser, T_ASSIGN);

		ast_value_T* val = (ast_value_T*) value(parser);
		
		symbol_var_T* s = (symbol_var_T*) symbol_new_var(ident->value, ident->loc, val->type_sym, 0, 0, 1, val->t->value);
		symbol_table_put(parser->s_table, (symbol_T*)s);

		data_table_put(parser->data_table, val->t, val->type_sym->name);

		consume(parser, T_SEMI);

		return ast_new_const_decl(ident, (ast_node_T*)val, val->type_sym->name);
	} else {
		log_error(ident->loc, 1, "Redeclaration of existing identifier '%s'.\n", ident->value);
		return NULL; //unreachable
	}
}

// sys_arg : (bin_op | value | prop | array_element);
ast_node_T* sys_arg(parser_T* parser) {
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[(parser->t_index + 1) %parser->t_count];
	if (current->type == T_IDENT || current->type == T_POINTER || current->type == T_INTEGER || current->type == T_STRING) { 
		ast_node_T* node;
		if (next->type == T_LSQUARE) {
			node = array_element(parser);
		} else if (next->type == T_DOT) {
			node = prop(parser);
		}else {
			node = value(parser);
		}

		if (token_is_op(next)) {
			node = bin_op(parser, node);
		}

		return node;
	} else {
		log_error(current->loc, 1, "Unexpected token in sys_arg, found: %s.\n", token_get_name(current->type));
		return NULL; //unreachable
	}
}

// syscall : SYSCALL LPAREN (sys_arg (COMMA sys_arg)*)? RPAREN;
ast_node_T* syscall(parser_T* parser) {
	consume(parser, T_SYSCALL);
	if (parser->tokens[parser->t_index]->type == T_LPAREN) {
		consume(parser, T_LPAREN);

		ast_node_T** params = malloc(sizeof(ast_node_T*));
		size_t count = 0;

		while (parser->tokens[parser->t_index]->type != T_RPAREN) {
			params[count++] = sys_arg(parser);
			params = realloc(params, (count + 1) * sizeof(ast_node_T*));
			if (parser->tokens[parser->t_index]->type == T_COMMA) {
				consume(parser, T_COMMA);
			}
		}
		consume(parser, T_RPAREN);

		return ast_new_syscall(params, count);
	}

	log_error(parser->tokens[parser->t_index]->loc, 1, "Expected list of params for syscall.\n");
	return NULL; //unreachable
}

// func_param : (MUT)? ID COLON ID ;
token_T* func_param(parser_T* parser, symbol_func_T* func) {
	unsigned char is_mut = 0;
	if (parser->tokens[parser->t_index]->type == T_MUT) {
		is_mut = 1;
		consume(parser, T_MUT);
	}
	token_T* param = parser->tokens[parser->t_index];
	consume(parser, T_IDENT);
	token_T* param_type = parser->tokens[parser->t_index];

	symbol_T* type_sym = malloc(sizeof(symbol_type_T));
	symbol_T* elem_type = malloc(sizeof(symbol_type_T));

	type_annotation(parser, type_sym, elem_type);

	if (type_sym != NULL) {
		switch (type_sym->type) {
			case SYM_VAR_TYPE:
				{
					symbol_T* var_sym = symbol_new_var(param->value, param->loc, type_sym, is_mut, 1, 0, 0);
					((symbol_var_T*)var_sym)->elem_type = elem_type;
					((symbol_var_T*)var_sym)->is_assigned = 1;
					func_add_param(func, var_sym);
					symbol_table_put(parser->s_table, var_sym);
				}
				break;

			default:
				log_error(param_type->loc, 1, "Unknown type annotation. Found symbol '%s', symbol type: '%s'.\n", type_sym->name, symbol_get_type_string(type_sym->type));
		}
	} else {
		log_error(param->loc, 1, "Could not extract type annotation for parameter %s\n", param->value);
	}

	return param;
}

// func_decl : FUNC ID LPAREN (func_param (COMMA func_param)*)? RPAREN (COLON ID)? block ;
ast_node_T* func_decl(parser_T* parser) {
	consume(parser, T_FUNC);
	token_T* ident = parser->tokens[parser->t_index];
	symbol_T* func_sym = symbol_new_func(ident->value, ident->loc);

	symbol_table_T* func_table = symbol_table_new(ident->value, parser->s_table->level+1, parser->s_table);
	symbol_table_T* parent = parser->s_table;
	parser->s_table = func_table;

	consume(parser, T_IDENT);
	consume(parser, T_LPAREN);

	token_T** params = malloc(sizeof(token_T*));
	size_t count = 0;

	while (parser->tokens[parser->t_index]->type != T_RPAREN) {
		params[count++] = func_param(parser, (symbol_func_T*) func_sym);
		params = realloc(params, (count + 1) * sizeof(token_T*));

		if (parser->tokens[parser->t_index]->type == T_COMMA) {
			consume(parser, T_COMMA);
		}
	}
	consume(parser, T_RPAREN);

	if (parser->tokens[parser->t_index]->type == T_COLON) {
		symbol_T* return_sym = malloc(sizeof(symbol_type_T)); 
		symbol_T* elem_type = malloc(sizeof(symbol_type_T));
		type_annotation(parser, return_sym, elem_type);

		symbol_func_T* f = (symbol_func_T*) func_sym;
		f->ret_type = return_sym;
	} else {
		symbol_func_T* f = (symbol_func_T*) func_sym;
		f->ret_type = NULL;
	}

	ast_node_T* b = block(parser);

	if (parser->debug) symbol_table_print(parser->s_table);

	func_table = parser->s_table;
	parser->s_table = parent;
	symbol_table_put_child(parser->s_table, func_table);
	symbol_table_put(parser->s_table, (symbol_T*) func_sym);

	return ast_new_func_decl(ident, params, count, b);
}

// arg : (value | bin_op | array_element | prop) ;
ast_node_T* arg(parser_T* parser, symbol_var_T* param) {
	token_T* current = parser->tokens[parser->t_index];
	token_T* next = parser->tokens[((parser->t_index + 1) %parser->t_count)];
	if (current->type == T_IDENT || current->type == T_POINTER || current->type == T_INTEGER || current->type == T_STRING || current->type == T_CHAR) { 
		ast_node_T* node;
		if (next->type == T_LSQUARE) {
			node = array_element(parser);
			/*
			ast_array_element_T* val = (ast_array_element_T*)node;
			symbol_var_T* arr = (symbol_var_T*)symbol_table_get(parser->s_table, val->ident->value);
			if (strcmp(arr->elem_type->name, param->type->name) != 0) {
				log_error(current->loc, 1, "Mismatched argument types in function call, found: %s, expected: %s.\n", arr->elem_type->name, param->type->name);
			}
			*/
		} else if (next->type == T_DOT) {
			node = prop(parser);
			/*
			ast_prop_T* p = (ast_prop_T*) node;
			symbol_var_T* var_sym = (symbol_var_T*) p->parent_sym;
			symbol_T* p_type = symbol_get_prop_type(var_sym->type, p->prop->value);
			if (strcmp(p_type->name, param->type->name) != 0) {
				log_error(current->loc, 1, "Mismatched argument types in function call, found: %s, expected: %s.\n", p_type->name, param->type->name);
			}
			*/
		} else {
			node = value(parser);
			/*
			ast_value_T* val = (ast_value_T*)node;
			if (strcmp(val->type_sym->name, param->type->name) != 0) {
				log_error(current->loc, 1, "Mismatched argument types in function call, found: %s, expected: %s.\n", val->type_sym->name, param->type->name);
			}
			*/
		}

		next = parser->tokens[parser->t_index];
		if (token_is_op(next)) {
			node = bin_op(parser, node);
			/*
			ast_bin_op_T* b = (ast_bin_op_T*)n;
			if (strcmp(b->type_sym->name, param->type->name) != 0) {
				log_error(current->loc, 1, "Mismatched argument types in function call, found: %s, expected: %s.\n", b->type_sym->name, param->type->name);
			}
			*/

		} 

		return node;
	} else {
		log_error(current->loc, 1, "Parsing error: Unexpected token in param, found: %s.\n", token_get_name(current->type));
		return NULL; //unreachable
	}
}

// func_call : ID LPAREN (arg (COMMA arg)*)? RPAREN ;
ast_node_T* func_call(parser_T* parser, ast_node_T* arg1) {
	log_debug(parser->debug, "parse func call\n");
	token_T* ident = parser->tokens[parser->t_index];
	symbol_T* func_sym = symbol_table_get(parser->s_table, ident->value);
	if (func_sym == NULL || func_sym->type != SYM_FUNC) {
		log_error(ident->loc, 1, "Unknown function '%s' is attempted to be called.\n", token_get_name(ident->type));
	}
	symbol_func_T* func = (symbol_func_T*) func_sym;
	consume(parser, T_IDENT);
	consume(parser, T_LPAREN);

	ast_node_T** params = calloc(1, sizeof(ast_node_T*));
	size_t count = 0;

	if (arg1 != NULL) {
		params[count] = arg1;
		count += 1;
		params = realloc(params, (count + 1) * sizeof(ast_node_T*));
	}

	while (parser->tokens[parser->t_index]->type != T_RPAREN) {
		if (count >= func->param_count) {
			log_error(parser->tokens[parser->t_index]->loc, 1, "Too many arguments for function %s. Expected %u.\n", func->base.name, count, func->param_count);
		}

		params[count] = arg(parser, (symbol_var_T*)func->params[count]);
		count += 1;
		params = realloc(params, (count + 1) * sizeof(ast_node_T*));
		if (parser->tokens[parser->t_index]->type == T_COMMA) {
			consume(parser, T_COMMA);
		}
	}
	if (count < func->param_count) {
		log_error(parser->tokens[parser->t_index]->loc, 1, "Too few arguments for function %s. Found %u, expected %u.\n", func->base.name, count, func->param_count);
	}
	consume(parser, T_RPAREN);

	return ast_new_func_call(ident, params, count);
}

/*
// array_expr : array_element ( SEMI | ((ASSIGN | op) (array_expr | value | bin_op | array))) ;
ast_node_T* array_expr(parser_T* parser) {
	log_debug(parser->debug, "parse array expr\n");
	ast_node_T* elem = array_element(parser);

	token_T* t = parser->tokens[parser->t_index];
	ast_node_T* operation;
	if (t->type == T_SEMI) {
		return elem;
	} else if (token_is_op(t)) {
		operation = op(parser);
	} else {
		operation = NULL;
		consume(parser);
	}

	token_T* next = parser->tokens[(parser->t_index+1)%parser->t_count];
	ast_node_T* rhs;
	if (token_is_op(next)) {
		//rhs = bin_op(parser);
	} else if (next->type == T_LSQUARE) {
		symbol_T* s = symbol_table_get(parser->s_table, parser->tokens[parser->t_index]->value);
		if (s->type == SYM_VAR_TYPE) {
			rhs = array(parser);
		} else {
			rhs = array_expr(parser);
		}
	} else {
		rhs = value(parser);
	}

	return ast_new_array_expr(elem, operation, rhs);
}
*/

// attribute : ID type_annotation SEMI ;
symbol_T* decl_attribute(parser_T* parser, size_t offset) {

	token_T* ident = parser->tokens[parser->t_index];
	if (ident->value == NULL) log_error(NULL, 1, "Error parsing struct decl attribute\n");
	consume(parser, T_IDENT);
	symbol_T* type_sym = malloc(sizeof(symbol_type_T));
	symbol_T* elem_type = malloc(sizeof(symbol_type_T));

	type_annotation(parser, type_sym, elem_type);

	consume(parser, T_SEMI);

	return symbol_new_prop(ident->value, offset, type_sym, elem_type);
}
 
// struct_decl : STRUCT ID LCURLY (attribute)* RCURLY ;
void struct_decl(parser_T* parser) {
	log_debug(parser->debug, "parser struct_decl\n");
	consume(parser, T_STRUCT);
	token_T* ident = parser->tokens[parser->t_index];
	if (symbol_table_contains(parser->s_table, ident->value)) {
		log_error(ident->loc, 1, "'%s' is already defined\n", ident->value);
	}
	consume(parser, T_IDENT);
	consume(parser, T_LCURLY);

	size_t struct_size = 0;
	symbol_T** props = malloc(sizeof(symbol_T*));
	size_t prop_count = 0;
	token_T* token = parser->tokens[parser->t_index];

	while (token->type != T_RCURLY) {
		symbol_T* attr = decl_attribute(parser, struct_size);
		symbol_prop_T* prop_sym = (symbol_prop_T*) attr;
		symbol_type_T* prop_type = (symbol_type_T*) prop_sym->type;

		props[prop_count++] = attr;
		props = realloc(props, (prop_count+1) * sizeof(symbol_T*));

		struct_size += prop_type->size;

		token = parser->tokens[parser->t_index];
	}

	consume(parser, T_RCURLY);

	symbol_T* type = symbol_new_type(ident->value, ident->loc, struct_size, props, prop_count);
	symbol_table_put(parser->s_table, type);
}

// expr : syscall SEMI | if | while | var_decl | const_decl | array_expr SEMI | assign SEMI | bin_op SEMI | dump SEMI | func_decl | func_call SEMI ;
ast_node_T* expr(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T* child;

	while (1) {
		switch (token->type) {
			case T_SYSCALL:
				child = syscall(parser);
				consume(parser, T_SEMI);
				break;
			case T_IF:
				child = if_block(parser);
				break;
			case T_WHILE:
				child = while_block(parser);
				break;
			case T_DUMP:
				child = dump(parser);
				consume(parser, T_SEMI);
				break;
			case T_LET:
				child = var_decl(parser);
				break;
			case T_CONST:
				child = const_decl(parser);
				break;
			case T_FUNC:
				child = func_decl(parser);
				break;
			case T_STRUCT:
				struct_decl(parser);
				token = parser->tokens[parser->t_index];
				continue;
			case T_IDENT:
			case T_INTEGER:
			case T_POINTER:
				{
					token_T* next = parser->tokens[(parser->t_index + 1) %parser->t_count];
					ast_node_T* lhs;
					if (next->type == T_LPAREN) {
						child = func_call(parser, NULL);
						consume(parser, T_SEMI);
						break;
					} else if (next->type == T_LSQUARE) {
						lhs = array_element(parser);
					} else if (next->type == T_DOT) {
						lhs = prop(parser);
					} else {
						lhs = value(parser);
					}

					if (parser->tokens[parser->t_index]->type == T_SEMI) {
						child = lhs;
						consume(parser, T_SEMI);
						break;
					}

					next = parser->tokens[parser->t_index];
					log_debug(parser->debug, "next type: %s\n", token_get_name(next->type));
					if (next->type == T_ASSIGN) {
						child = assign(parser, lhs);
						consume(parser, T_SEMI);
					} else {
						child = bin_op(parser, lhs);
						consume(parser, T_SEMI);
					}
					break;
				}

			default:
				log_error(token->loc, 1, "Invalid token type in start of expression. %s cannot start an expression.\n", token_get_name(token->type));
		}
		return ast_new_expr(child);
	}
}

// program : (expr)* ;
ast_node_T* program(parser_T* parser) {
	token_T* token = parser->tokens[parser->t_index];
	ast_node_T** expressions = malloc(sizeof(ast_node_T*));
	size_t count = 0;

	symbol_table_init_builtins(parser->s_table);

	while (token->type != T_EOF) {
		expressions[count++] = expr(parser);
		expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T*));

		token = parser->tokens[parser->t_index];
	} 

	if (parser->debug) symbol_table_print(parser->s_table);

	return ast_new_program(expressions, count);
}

ast_node_T* parser_parse(parser_T* parser) {
	return program(parser);
}


