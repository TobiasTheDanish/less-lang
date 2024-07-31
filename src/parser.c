#include "include/parser.h"
#include "include/ast_nodes.h"
#include "include/logger.h"
#include "include/symbol.h"
#include "include/token.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

ast_node_T *func_call(parser_T *parser);
ast_node_T *syscall(parser_T *parser);
ast_node_T *array_expr(parser_T *parser);
ast_node_T *expr(parser_T *parser);
ast_node_T *if_block(parser_T *parser);
ast_node_T *array_element(parser_T *parser);
ast_node_T *value(parser_T *parser);
ast_node_T *statement(parser_T *parser);

parser_T *parser_new(lexer_T *lexer, size_t t_count, unsigned char debug_info) {
  parser_T *parser = malloc(sizeof(parser_T));
  parser->lexer = lexer;
  parser->if_count = 0;
  parser->t_count = t_count;
  parser->t_index = 0;
  parser->tokens = malloc(t_count * sizeof(token_T *));
  parser->debug = debug_info;

  for (size_t i = 0; i < t_count; i++) {
    parser->tokens[i] = lexer_next_token(lexer);
  }

  return parser;
}

void consume(parser_T *parser, token_E expected_type) {
  token_T *t = parser->tokens[parser->t_index];
  log_debug(parser->debug, "Token: (%s, %s)\n", token_get_name(t->type),
            t->value);
  if (t->type != expected_type) {
    log_error(t->loc, 1,
              "Parsing error: Token '%s' of type '%s', is not of the expected "
              "type '%s'\n",
              t->value, token_get_name(t->type), token_get_name(expected_type));
  }

  parser->tokens[parser->t_index] = lexer_next_token(parser->lexer);
  parser->t_index = (parser->t_index + 1) % parser->t_count;
}

// type_annotation : COLON ID (LSQUARE RSQUARE)? ;
ast_node_T *type_annotation(parser_T *parser) {
  log_debug(parser->debug, "parse type annotation\n");
  token_T *annot_token = parser->tokens[parser->t_index];
  consume(parser, T_COLON);

  token_T *type_token = parser->tokens[parser->t_index];
  if (type_token->type != T_IDENT) {
    log_error(type_token->loc, 1, "Non identifier symbol '%s' use as a type\n",
              type_token->value);
  }

  ast_node_T *type = value(parser);

  unsigned char is_array = (parser->tokens[parser->t_index]->type == T_LSQUARE);

  return ast_new_type_annot(annot_token, type, is_array);
}

// value: (POINTER | ID | INT | STRING | CHAR);
ast_node_T *value(parser_T *parser) {
  log_debug(parser->debug, "Parse value\n");
  ast_node_T *res;
  token_T *token = parser->tokens[parser->t_index];

  switch (token->type) {
  case T_POINTER:
    res = ast_new_value(token);
    consume(parser, T_POINTER);
    break;
  case T_IDENT:
    res = ast_new_value(token);
    consume(parser, T_IDENT);
    break;
  case T_INTEGER:
    res = ast_new_value(token);
    consume(parser, T_INTEGER);
    break;
  case T_STRING:
    res = ast_new_value(token);
    consume(parser, T_STRING);
    break;

  case T_CHAR:
    res = ast_new_value(token);
    consume(parser, T_CHAR);
    break;

  default:
    log_error(token->loc, 1, "Invalid token type for value. Found: %s.\n",
              token_get_name(token->type));
  }

  return res;
}

// op: (PLUS | MINUS | MULTIPLY | DIVIDE);
ast_node_T *op(parser_T *parser) {
  log_debug(parser->debug, "Parse op\n");
  ast_node_T *res;
  token_T *token = parser->tokens[parser->t_index];

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
    log_error(token->loc, 1, "Invalid token type for op. Found: %s.\n",
              token_get_name(token->type));
  }

  return res;
}

// factor: (func_call | value | prop | array_element) (('*' | '/´) factor)*
ast_node_T *factor(parser_T *parser) {
  token_T *next = parser->tokens[(parser->t_index + 1) % parser->t_count];
  ast_node_T *res;
  if (next->type == T_LSQUARE) {
    res = array_element(parser);
  } else if (next->type == T_LPAREN) {
    res = func_call(parser);
  } else {
    res = value(parser);
  }

  token_T *current = parser->tokens[parser->t_index];
  if (current->type == T_DOT) {
    consume(parser, T_DOT);
    res = ast_new_prop(current, res, expr(parser));
  }

  current = parser->tokens[parser->t_index];
  if (current->type == T_ASSIGN) {
    consume(parser, T_ASSIGN);
    res = ast_new_assign(current, res, expr(parser));
  }

  while (current->type == T_MULTIPLY || current->type == T_DIVIDE ||
         current->type == T_MODULUS) {
    ast_node_T *op_node = op(parser);
    res = ast_new_bin_op(res, op_node, expr(parser));
    current = parser->tokens[parser->t_index];
  }

  return res;
}

ast_node_T *term(parser_T *parser) {
  ast_node_T *res = factor(parser);

  token_T *current = parser->tokens[parser->t_index];
  while (current->type == T_PLUS || current->type == T_MINUS) {
    ast_node_T *op_node = op(parser);
    res = ast_new_bin_op(res, op_node, factor(parser));
    current = parser->tokens[parser->t_index];
  }

  return res;
}

// array_element : ID LSQUARE (array_element | IDENT | INTEGER | bin_op | prop)
// RSQUARE ;
ast_node_T *array_element(parser_T *parser) {
  log_debug(parser->debug, "Parse array element\n");
  token_T *ident = parser->tokens[parser->t_index];

  consume(parser, T_IDENT);
  consume(parser, T_LSQUARE);

  ast_node_T *offset = expr(parser);

  consume(parser, T_RSQUARE);

  return ast_new_array_element(ident, offset);
}

// dump: DUMP (bin_op | value | array_element | prop);
ast_node_T *dump(parser_T *parser) {
  log_debug(parser->debug, "Parse dump\n");
  consume(parser, T_DUMP);

  return ast_new_dump(expr(parser));
}

// cond_op : (EQUALS | NOT_EQUALS | LESS | GREATER);
ast_node_T *cond_op(parser_T *parser) {
  ast_node_T *res;
  token_T *token = parser->tokens[parser->t_index];

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
    log_error(token->loc, 1, "Invalid token type for cond_op. Found: %s.\n",
              token_get_name(token->type));
  }

  return res;
}

// logical_op : (AND | OR) ;
ast_node_T *logical_op(parser_T *parser) {
  ast_node_T *res;
  token_T *token = parser->tokens[parser->t_index];

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
    log_error(token->loc, 1, "Invalid token type for cond_op. Found: %s.\n",
              token_get_name(token->type));
  }

  return res;
}

// conditional : (array_element | value | bin_op | prop) cond_op (value | bin_op
// | prop | array_element) (logical_op conditional)* ;
ast_node_T *conditional(parser_T *parser) {
  ast_node_T *lhs = expr(parser);

  ast_node_T *operation = cond_op(parser);
  ast_node_T *rhs = expr(parser);

  token_T *next = parser->tokens[parser->t_index];
  if (token_is_logical(next)) {
    ast_node_T *logical = logical_op(parser);
    ast_node_T *c = conditional(parser);

    return ast_new_cond(lhs, operation, rhs, logical, c);
  }

  return ast_new_cond(lhs, operation, rhs, NULL, NULL);
}

// block : LCURLY (expr)* RCURLY ;
ast_node_T *block(parser_T *parser) {
  token_T *token = parser->tokens[parser->t_index];
  ast_node_T **expressions = malloc(sizeof(ast_node_T *));
  size_t count = 0;

  if (token->type == T_LCURLY) {
    consume(parser, T_LCURLY);

    while (token->type != T_RCURLY) {
      expressions[count++] = statement(parser);

      expressions = realloc(expressions, (count + 1) * sizeof(ast_node_T *));

      token = parser->tokens[parser->t_index];
    }
    consume(parser, T_RCURLY);
  } else {
    log_error(token->loc, 1, "Expected '{' to start block, but found '%s'\n",
              token->value);
  }

  return ast_new_block(expressions, count);
}
// else : ELSE (if | block) ;
ast_node_T *else_block(parser_T *parser, size_t index) {
  consume(parser, T_ELSE);
  token_T *next = parser->tokens[parser->t_index];
  ast_node_T *b;

  switch (next->type) {
  case T_IF:
    b = if_block(parser);
    break;
  case T_LCURLY:
    b = block(parser);
    break;

  default:
    log_error(next->loc, 1, "Invalid token type for else_block. Found: %s.\n",
              token_get_name(next->type));
  }

  return ast_new_else(index, b);
}

// while : WHILE conditional block ;
ast_node_T *while_block(parser_T *parser) {
  size_t index = ++parser->if_count;
  consume(parser, T_WHILE);
  ast_node_T *cond = conditional(parser);
  ast_node_T *b = block(parser);

  return ast_new_while(index, cond, b);
}

// if : IF conditional block (else)? ;
ast_node_T *if_block(parser_T *parser) {
  size_t index = parser->if_count++;
  consume(parser, T_IF);
  ast_node_T *cond = conditional(parser);
  ast_node_T *b = block(parser);
  ast_node_T *elze = NULL;

  if (parser->tokens[parser->t_index]->type == T_ELSE) {
    elze = else_block(parser, index);
  }

  return ast_new_if(index, cond, b, elze);
}

// array : ID LSQUARE INTEGER RSQUARE ;
ast_node_T *array(parser_T *parser) {
  token_T *current = parser->tokens[parser->t_index];
  if (current->type != T_IDENT) {
    log_error(current->loc, 1, "Non identifier symbol '%s' use as a type\n",
              current->value);
  }

  ast_node_T *ident = value(parser);
  consume(parser, T_LSQUARE);

  ast_node_T *len = expr(parser);

  consume(parser, T_INTEGER);
  consume(parser, T_RSQUARE);
  return ast_new_array(ident, len);
}

// attribute : ID COLON (value | array_element | prop | bin_op | array) COMMA?
ast_node_T *attribute(parser_T *parser) {
  token_T *ident = parser->tokens[parser->t_index];
  consume(parser, T_IDENT);
  consume(parser, T_COLON);

  ast_node_T *val = expr(parser);

  return ast_new_attribute(ident, val);
}

// struct_init : ID LCURLY (attribute)* RCURLY ;
ast_node_T *struct_init(parser_T *parser) {
  log_debug(parser->debug, "Parse struct initializing\n");
  token_T *ident = parser->tokens[parser->t_index];
  if (ident->type != T_IDENT) {
    log_error(ident->loc, 1, "Non ident symbol '%s' used as a struct.\n",
              ident->value);
  }
  consume(parser, T_IDENT);
  consume(parser, T_LCURLY);

  ast_node_T **attributes = malloc(sizeof(ast_node_T *));
  size_t attr_count = 0;
  token_T *token = parser->tokens[parser->t_index];
  while (token->type != T_RCURLY) {
    attributes[attr_count++] = attribute(parser);
    attributes = realloc(attributes, (attr_count + 1) * sizeof(ast_node_T *));

    if (parser->tokens[parser->t_index]->type == T_COMMA)
      consume(parser, T_COMMA);

    token = parser->tokens[parser->t_index];
  }
  consume(parser, T_RCURLY);

  return ast_new_struct_init(attributes, attr_count, ident);
}

// assign : (ID | array_element | prop) ASSIGN (syscall | func_call | value |
// bin_op | array | array_element | prop | struct_init) ;
ast_node_T *assign(parser_T *parser) {
  log_todo("parse assign not reimplemented");
  return NULL;
}

// var_decl : LET (MUT)? ID (COLON ID)? assign SEMI ;
ast_node_T *var_decl(parser_T *parser) {
  token_T *decl_token = parser->tokens[parser->t_index];
  consume(parser, T_LET);
  if (parser->tokens[parser->t_index]->type == T_MUT) {
    decl_token = parser->tokens[parser->t_index];
    consume(parser, T_MUT);
  }
  ast_node_T **children = (ast_node_T **)malloc(3 * sizeof(ast_node_T *));
  size_t child_count = 0;

  children[child_count++] = value(parser);

  if (parser->tokens[parser->t_index]->type == T_COLON) {
    children[child_count++] = type_annotation(parser);
  }

  consume(parser, T_ASSIGN);

  children[child_count++] = expr(parser);

  return ast_new_decl(decl_token, children, child_count);
}

// const_decl : CONST ID ASSIGN value SEMI ;
ast_node_T *const_decl(parser_T *parser) {
  token_T *decl_token = parser->tokens[parser->t_index];
  consume(parser, T_CONST);
  ast_node_T **children = (ast_node_T **)malloc(3 * sizeof(ast_node_T *));
  size_t child_count = 0;

  children[child_count++] = value(parser);

  if (parser->tokens[parser->t_index]->type == T_COLON) {
    children[child_count++] = type_annotation(parser);
  }

  children[child_count++] = expr(parser);

  return ast_new_decl(decl_token, children, child_count);
}

// sys_arg : (bin_op | value | prop | array_element);
ast_node_T *sys_arg(parser_T *parser) {
  token_T *current = parser->tokens[parser->t_index];
  if (current->type == T_IDENT || current->type == T_POINTER ||
      current->type == T_INTEGER || current->type == T_STRING) {
    return expr(parser);
  } else {
    log_error(current->loc, 1, "Unexpected token in sys_arg, found: %s.\n",
              token_get_name(current->type));
    return NULL; // unreachable
  }
}

// syscall : SYSCALL LPAREN (sys_arg (COMMA sys_arg)*)? RPAREN;
ast_node_T *syscall(parser_T *parser) {
  consume(parser, T_SYSCALL);
  if (parser->tokens[parser->t_index]->type == T_LPAREN) {
    consume(parser, T_LPAREN);

    ast_node_T **params = malloc(sizeof(ast_node_T *));
    size_t count = 0;

    while (parser->tokens[parser->t_index]->type != T_RPAREN) {
      params[count++] = sys_arg(parser);
      params = realloc(params, (count + 1) * sizeof(ast_node_T *));
      if (parser->tokens[parser->t_index]->type == T_COMMA) {
        consume(parser, T_COMMA);
      }
    }
    consume(parser, T_RPAREN);

    return ast_new_syscall(params, count);
  }

  log_error(parser->tokens[parser->t_index]->loc, 1,
            "Expected list of params for syscall.\n");
  return NULL; // unreachable
}

// func_param : (MUT)? ID COLON ID ;
ast_node_T *func_param(parser_T *parser, symbol_func_T *func) {
  unsigned char is_mut = 0;
  if (parser->tokens[parser->t_index]->type == T_MUT) {
    is_mut = 1;
    consume(parser, T_MUT);
  }
  token_T *param = parser->tokens[parser->t_index];
  if (param->type != T_IDENT) {
    log_error(param->loc, 1,
              "Non identifier symbol '%s' use as function parameter\n",
              param->value);
  }
  ast_node_T *ident = value(parser);

  ast_node_T *type = type_annotation(parser);

  return ast_new_func_param(ident, type, is_mut);
}

// LPAREN (func_param (COMMA func_param)*)? RPAREN
ast_node_T *func_param_list(parser_T *parser) {
  location_T *start_loc = parser->tokens[parser->t_index]->loc;
  consume(parser, T_LPAREN);

  ast_node_T **params = malloc(sizeof(ast_node_T *));
  size_t count = 0;

  while (parser->tokens[parser->t_index]->type != T_RPAREN) {
    params[count++] = func_param(parser, NULL);
    params = realloc(params, (count + 1) * sizeof(token_T *));

    if (parser->tokens[parser->t_index]->type == T_COMMA) {
      consume(parser, T_COMMA);
    }
  }
  consume(parser, T_RPAREN);

  return ast_new_param_list(start_loc, params, count);
}

// func_decl : FUNC ID func_param_list (COLON
// ID)? block ;
ast_node_T *func_decl(parser_T *parser) {
  token_T *decl_token = parser->tokens[parser->t_index];
  consume(parser, T_FUNC);
  ast_node_T **children = (ast_node_T **)malloc(4 * sizeof(ast_node_T *));
  size_t child_count = 0;

  children[child_count++] = value(parser);

  children[child_count++] = func_param_list(parser);

  if (parser->tokens[parser->t_index]->type == T_COLON) {
    children[child_count++] = type_annotation(parser);
  }

  children[child_count++] = block(parser);

  return ast_new_decl(decl_token, children, child_count);
}

// arg : (value | bin_op | array_element | prop) ;
ast_node_T *arg(parser_T *parser) {
  token_T *current = parser->tokens[parser->t_index];
  if (current->type == T_IDENT || current->type == T_POINTER ||
      current->type == T_INTEGER || current->type == T_STRING ||
      current->type == T_CHAR) {
    return expr(parser);
  } else {
    log_error(current->loc, 1,
              "Parsing error: Unexpected token in param, found: %s.\n",
              token_get_name(current->type));
    return NULL; // unreachable
  }
}

// func_call : ID LPAREN (arg (COMMA arg)*)? RPAREN ;
ast_node_T *func_call(parser_T *parser) {
  log_debug(parser->debug, "parse func call\n");
  token_T *ident = parser->tokens[parser->t_index];
  if (ident->type != T_IDENT) {
    log_error(ident->loc, 1, "Non identifier symbol '%s' used as struct name\n",
              ident->value);
  }
  consume(parser, T_LPAREN);

  ast_node_T **params = calloc(1, sizeof(ast_node_T *));
  size_t count = 0;

  while (parser->tokens[parser->t_index]->type != T_RPAREN) {
    params[count] = arg(parser);
    count += 1;
    params = realloc(params, (count + 1) * sizeof(ast_node_T *));
  }
  consume(parser, T_RPAREN);

  return ast_new_func_call(ident, params, count);
}

// attribute : ID type_annotation SEMI ;
ast_node_T *decl_attribute(parser_T *parser, size_t offset) {

  token_T *ident = parser->tokens[parser->t_index];
  if (ident->value == NULL)
    log_error(NULL, 1, "Error parsing struct decl attribute\n");
  consume(parser, T_IDENT);

  ast_node_T *type = type_annotation(parser);

  consume(parser, T_SEMI);

  return ast_new_attribute(ident, type);
}

// struct_decl : STRUCT ID LCURLY (attribute)* RCURLY ;
ast_node_T *struct_decl(parser_T *parser) {
  log_debug(parser->debug, "parser struct_decl\n");
  token_T *decl_token = parser->tokens[parser->t_index];
  consume(parser, T_STRUCT);

  token_T *ident = parser->tokens[parser->t_index];
  if (ident->type != T_IDENT) {
    log_error(decl_token->loc, 1,
              "Non identifier symbol '%s' used as struct name\n",
              decl_token->value);
  }
  ast_node_T **children = (ast_node_T **)malloc(2 * sizeof(ast_node_T *));
  size_t child_count = 0;

  children[child_count++] = value(parser);

  if (parser->tokens[parser->t_index]->type == T_COLON) {
    children[child_count++] = type_annotation(parser);
  }

  children[child_count++] = expr(parser);

  consume(parser, T_LCURLY);

  size_t struct_size = 0;
  ast_node_T **props = malloc(sizeof(ast_node_T *));
  size_t prop_count = 0;
  token_T *token = parser->tokens[parser->t_index];

  while (token->type != T_RCURLY) {
    ast_node_T *attr = decl_attribute(parser, struct_size);

    props[prop_count++] = attr;
    props = realloc(props, (prop_count + 1) * sizeof(ast_node_T *));

    token = parser->tokens[parser->t_index];
  }

  consume(parser, T_RCURLY);

  return ast_new_decl(decl_token, props, prop_count);
}

// expr : syscall SEMI | if | while |
//  bin_op SEMI | func_call SEMI ;
ast_node_T *expr(parser_T *parser) {
  token_T *token = parser->tokens[parser->t_index];

  switch (token->type) {
  case T_SYSCALL:
    return syscall(parser);
    break;
  case T_IF:
    return if_block(parser);
    break;
  case T_IDENT:
  case T_INTEGER:
  case T_POINTER: {
    return term(parser);
    break;
  }

  default:
    log_error(token->loc, 1,
              "Invalid token type in start of expression. '%s' cannot start an "
              "expression.\n",
              token_get_name(token->type));
  }
  return NULL;
}

// var_decl | const_decl | assign SEMI | dump SEMI | func_decl |
ast_node_T *statement(parser_T *parser) {
  token_T *token = parser->tokens[parser->t_index];
  ast_node_T *child;

  switch (token->type) {
  case T_DUMP:
    child = dump(parser);
    consume(parser, T_SEMI);
    break;
  case T_LET:
    child = var_decl(parser);
    consume(parser, T_SEMI);
    break;
  case T_CONST:
    child = const_decl(parser);
    consume(parser, T_SEMI);
    break;
  case T_SYSCALL:
    child = syscall(parser);
    consume(parser, T_SEMI);
    break;
  case T_FUNC:
    child = func_decl(parser);
    break;
  case T_STRUCT:
    child = struct_decl(parser);
    break;
  case T_WHILE:
    child = while_block(parser);
    break;
  case T_IF:
    child = if_block(parser);
    break;
  case T_IDENT:
    child = expr(parser);
    consume(parser, T_SEMI);
    break;

  default:
    log_error(token->loc, 1, "Statement cannot begin with token '%s'\n",
              token_get_name(token->type));
  }

  return child;
}

// program : (statement)* ;
ast_node_T *program(parser_T *parser) {
  token_T *token = parser->tokens[parser->t_index];
  ast_node_T **stmts = malloc(sizeof(ast_node_T *));
  size_t count = 0;

  while (token->type != T_EOF) {
    stmts[count++] = statement(parser);
    stmts = realloc(stmts, (count + 1) * sizeof(ast_node_T *));

    token = parser->tokens[parser->t_index];
  }

  return ast_new_program(stmts, count);
}

ast_node_T *parser_parse(parser_T *parser) { return program(parser); }
