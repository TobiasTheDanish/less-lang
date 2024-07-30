#include "include/type_check.h"
#include "include/ast_nodes.h"
#include "include/logger.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stdlib.h>
void check_value(type_check_t *t, ast_node_T *node);
void check_bin_op(type_check_t *t, ast_node_T *node);
void check_expr(type_check_t *t, ast_node_T *node);
void check_program(type_check_t *t, ast_node_T *node);

type_check_t *type_check_new(ast_node_T *program, symbol_table_T *table,
                             data_table_T *data, unsigned char debug) {
  type_check_t *t = malloc(sizeof(type_check_t));
  t->program = program;
  t->debug = debug;
  t->table = table;
  t->data = data;

  return t;
}

ast_node_T *type_check(type_check_t *t) {
  symbol_table_init_builtins(t->table);

  check_program(t, t->program);

  return t->program;
}

void check_value(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check value\n");
  ast_value_T *value = (ast_value_T *)node;

  switch (value->t->type) {
  case T_POINTER:
  case T_IDENT: {
    symbol_var_T *var;
    if (symbol_table_contains(t->table, value->t->value)) {
      var = (symbol_var_T *)symbol_table_get(t->table, value->t->value);
    } else {
      var = (symbol_var_T *)symbol_table_get(t->table, "undefined");
    }
    node->symbol_type = (symbol_type_T *)var->type;
  } break;
  case T_INTEGER: {
    // printf("<%s, %s>\n", value->t->value, "int");
    char *type;
    if (atoi(value->t->value) < 255)
      type = "i8";
    else if (atoi(value->t->value) < 65535)
      type = "i16";
    else
      type = "i32";

    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, type);
  } break;
  case T_STRING:
    data_table_put(t->data, value->t, "string");
    // printf("<%s, %s>\n", value->t->value, "string");
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "string");
    break;

  case T_CHAR:
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "i8");
    break;

  default:
    log_error(value->t->loc, 1, "Invalid token type for value. Found: %s.\n",
              token_get_name(value->t->type));
  }
}

void check_bin_op(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check bin op\n");
  ast_bin_op_T *bin_op = (ast_bin_op_T *)node;

  check_expr(t, bin_op->rhs);
  check_expr(t, bin_op->lhs);

  if (!symbol_cmp(&bin_op->rhs->symbol_type->base,
                  &bin_op->lhs->symbol_type->base)) {
    log_error(bin_op->op->loc, 1,
              "Cannot perform '%s' for types '%s' and '%s'\n",
              ast_get_name(bin_op->op->type),
              symbol_get_type_string(bin_op->lhs->symbol_type->base.type),
              symbol_get_type_string(bin_op->rhs->symbol_type->base.type));
  }

  node->symbol_type = bin_op->lhs->symbol_type;
}

void check_expr(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check expression\n");
  ast_expr_T *expr = (ast_expr_T *)node;

  switch (expr->child->type) {
  case AST_BIN_OP:
    check_bin_op(t, expr->child);
    break;
  case AST_SYSCALL:
    break;
  case AST_IF:
    break;
  case AST_VALUE:
    check_value(t, expr->child);

  case AST_WHILE:
  case AST_DUMP:
  case AST_ASSIGN:
  case AST_DECL:
  case AST_VAR_DECL:
  case AST_FUNC_DECL:
  case AST_FUNC_CALL:
  case AST_CONST_DECL:
  case AST_STRUCT_INIT:
  case AST_ATTRIBUTE:
  case AST_ARRAY:
  case AST_PROP:
  case AST_ARRAY_ELEMENT:
  case AST_ELSE:
  case AST_BLOCK:
  case AST_EXPR:
  case AST_OP:
  case AST_NO_OP:
  case AST_PROGRAM:
  case AST_CONDITIONAL:
  case AST_COND_OP:
  case AST_LOGICAL_OP:
  case AST_TYPE_ANNOT:
  case AST_FUNC_PARAM_LIST:
  case AST_FUNC_PARAM:
    log_error(expr->child->loc, 1, "Unexpected node in expr, found: %s.\n",
              ast_get_name(expr->child->type));
    break;
  }
}

void check_dump(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check dump\n");
  ast_dump_T *dump = (ast_dump_T *)node;

  check_expr(t, dump->value);
}

void check_func_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check function declaration\n");
  log_todo("check_func_decl not implemented yet\n");
}

void check_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check declaration\n");
  ast_decl_T *decl = (ast_decl_T *)node;
  switch (decl->token->type) {
  case T_LET:
  case T_CONST:
    log_todo("check_decl not implemented yet\n");
    break;
  case T_FUNC:
    check_func_decl(t, node);
    break;

  default:
    log_error(decl->token->loc, 1,
              "Unexpected keyword in declaration, found: %s.\n",
              decl->token->value);
  }
}

void check_statement(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check statement\n");
  switch (node->type) {
  case AST_SYSCALL:
    break;
  case AST_IF:
    break;
  case AST_ASSIGN:
    break;
  case AST_WHILE:
    break;
  case AST_DUMP:
    check_dump(t, node);
  case AST_DECL:
    check_decl(t, node);
    break;
  default:
    log_error(node->loc, 1, "Unexpected node in expr, found: %s.\n",
              ast_get_name(node->type));
    break;
  }
}

void check_program(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check program\n");
  if (node->type == AST_PROGRAM) {
    ast_program_T *program = (ast_program_T *)node;

    for (size_t i = 0; i < program->count; i++) {
      log_debug(t->debug, "Program statement #%u type: %s\n", i,
                ast_get_name(program->expressions[i]->type));
      check_statement(t, program->expressions[i]);
    }
  } else {
    log_error(node->loc, 1,
              "Unexpected node to start program.\n Found: %s, expects: %s.\n",
              ast_get_name(node->type), ast_get_name(AST_PROGRAM));
  }
}
