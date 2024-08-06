#include "include/lir.h"
#include "include/ast_nodes.h"
#include "include/logger.h"
#include "include/symbol.h"
#include "include/token.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lir_node_T lir_expr(lir_builder_T *b, ast_node_T *node);
lir_node_T lir_statement(lir_builder_T *b, ast_node_T *node);

lir_builder_T lir_new_builder(symbol_table_T *symbol_table) {
  return (lir_builder_T){
      .table = symbol_table,
  };
}

lir_node_T lir_dump(lir_builder_T *b, ast_dump_T *dump) {
  lir_node_T *children = malloc(sizeof(lir_node_T));
  children[0] = lir_expr(b, dump->value);

  return (lir_node_T){
      .tag = LIR_DUMP,
      .value = "dump",
      .children = children,
      .count = 1,
  };
}

lir_node_T lir_value(lir_builder_T *b, ast_value_T *value) {
  lir_tag_E tag;

  switch (value->t->type) {
  case T_POINTER:
  case T_IDENT: {
    tag = LIR_IDENT;
  } break;
  case T_INTEGER:
  case T_STRING:
  case T_CHAR: {
    tag = LIR_VALUE;
  } break;

  default:
    log_error(value->base.loc, 1, "Invalid ast node '%s' in value\n",
              ast_get_name(value->base.type));
  }

  return (lir_node_T){
      .tag = tag,
      .value = value->t->value,
      .children = NULL,
      .count = 0,
  };
}

lir_node_T lir_bin_op(lir_builder_T *b, ast_bin_op_T *bin_op) {
  lir_node_T *children = malloc(2 * sizeof(lir_node_T));

  ast_op_T *op_node = (ast_op_T *)bin_op->op;

  lir_tag_E tag;
  switch (op_node->t->type) {
  case T_PLUS:
    tag = LIR_ADD;
    break;
  case T_MINUS:
    tag = LIR_SUB;
    break;
  case T_MODULUS:
    tag = LIR_MOD;
    break;
  case T_MULTIPLY:
    tag = LIR_MUL;
    break;
  case T_DIVIDE:
    tag = LIR_DIV;
    break;

  default:
    log_error(op_node->base.loc, 1, "Invalid operation '%s' in binary op\n",
              ast_get_name(op_node->base.type));
  }

  children[0] = lir_expr(b, bin_op->lhs);
  children[1] = lir_expr(b, bin_op->rhs);

  return (lir_node_T){
      .tag = tag,
      .value = op_node->t->value,
      .children = children,
      .count = 2,
  };
}

lir_node_T lir_expr(lir_builder_T *b, ast_node_T *node) {
  switch (node->type) {
  case AST_BIN_OP:
    return lir_bin_op(b, (ast_bin_op_T *)node);
  case AST_VALUE:
    return lir_value(b, (ast_value_T *)node);

  case AST_PROGRAM:
  case AST_BLOCK:
  case AST_EXPR:
  case AST_SYSCALL:
  case AST_DECL:
  case AST_TYPE_ANNOT:
  case AST_VAR_DECL:
  case AST_CONST_DECL:
  case AST_FUNC_DECL:
  case AST_FUNC_PARAM_LIST:
  case AST_FUNC_PARAM:
  case AST_FUNC_CALL:
  case AST_STRUCT_INIT:
  case AST_ATTRIBUTE_LIST:
  case AST_ATTRIBUTE:
  case AST_ASSIGN:
  case AST_WHILE:
  case AST_IF:
  case AST_ELSE:
  case AST_CONDITIONAL:
  case AST_COND_OP:
  case AST_LOGICAL_OP:
  case AST_OP:
  case AST_ARRAY:
  case AST_ARRAY_ELEMENT:
  case AST_PROP:
  case AST_DUMP:
  case AST_RETURN:
  case AST_NO_OP:
    log_error(node->loc, 1, "Unexpected node in start of statement '%s'\n",
              ast_get_name(node->type));
  }

  return (lir_node_T){0};
}

lir_node_T *lir_block(lir_builder_T *b, ast_block_T *block, size_t *count) {
  size_t num = 0;
  lir_node_T *res = malloc(sizeof(lir_node_T));

  while (num < block->count) {
    res[num] = lir_statement(b, block->expressions[num]);
    num += 1;

    res = realloc(res, (num + 1) * sizeof(lir_node_T));
  }

  *count = num;
  return res;
}

lir_node_T lir_func_decl(lir_builder_T *b, ast_decl_T *func_decl) {
  ast_value_T *name = (ast_value_T *)func_decl->children[0];

  size_t count = 0;
  lir_node_T *children = lir_block(
      b, (ast_block_T *)func_decl->children[func_decl->child_count - 1],
      &count);

  return (lir_node_T){
      .tag = LIR_LABEL,
      .value = name->t->value,
      .children = children,
      .count = count,
  };
}

lir_node_T lir_decl(lir_builder_T *b, ast_decl_T *decl) {
  switch (decl->token->type) {
  case T_FUNC:
    return lir_func_decl(b, decl);
  case T_MUT:
  case T_LET:
  case T_CONST:
  case T_STRUCT:
    log_todo("decl not fully implemented yet\n");
    return (lir_node_T){0};

  default:
    log_error(decl->token->loc, 1, "Unexpected token in declaration '%s'\n",
              token_get_name(decl->token->type));
    return (lir_node_T){0};
  }
}

lir_node_T lir_statement(lir_builder_T *b, ast_node_T *node) {
  switch (node->type) {
  case AST_DUMP:
    return lir_dump(b, (ast_dump_T *)node);
  case AST_DECL:
    return lir_decl(b, (ast_decl_T *)node);

  case AST_PROGRAM:
  case AST_BLOCK:
  case AST_EXPR:
  case AST_SYSCALL:
  case AST_TYPE_ANNOT:
  case AST_VAR_DECL:
  case AST_CONST_DECL:
  case AST_FUNC_DECL:
  case AST_FUNC_PARAM_LIST:
  case AST_FUNC_PARAM:
  case AST_FUNC_CALL:
  case AST_STRUCT_INIT:
  case AST_ATTRIBUTE_LIST:
  case AST_ATTRIBUTE:
  case AST_ASSIGN:
  case AST_WHILE:
  case AST_IF:
  case AST_ELSE:
  case AST_CONDITIONAL:
  case AST_COND_OP:
  case AST_LOGICAL_OP:
  case AST_BIN_OP:
  case AST_OP:
  case AST_VALUE:
  case AST_ARRAY:
  case AST_ARRAY_ELEMENT:
  case AST_PROP:
  case AST_RETURN:
  case AST_NO_OP:
    log_error(node->loc, 1, "Unexpected node in start of statement '%s'\n",
              ast_get_name(node->type));
  }

  return (lir_node_T){0};
}

lir_node_T lir_build(lir_builder_T *b, ast_node_T *node) {
  lir_node_T program = {
      .tag = LIR_PROGRAM,
      .value = "program",
      .children = malloc(sizeof(lir_node_T)),
      .count = 0,
  };

  if (node->type != AST_PROGRAM) {
    log_error(node->loc, 1, "Invalid ast node type '%s' to start program\n",
              ast_get_name(node->type));
  }

  ast_program_T *ast = (ast_program_T *)node;

  for (size_t i = 0; i < ast->count; i++) {
    program.children[program.count++] = lir_statement(b, ast->expressions[i]);
    program.children =
        realloc(program.children, (program.count + 1) * sizeof(lir_node_T));
  }

  return program;
}

char *lir_to_string(lir_node_T *n) {
  char *tag_name = lir_tag_name(n);
  char *start = calloc(strlen(tag_name) + strlen(n->value) + 4, sizeof(char));
  sprintf(start, "%s %s%s", tag_name, n->value, n->count > 0 ? "(" : "");

  for (size_t i = 0; i < n->count; i++) {
    char *child = lir_to_string(&n->children[i]);
    char *tmp = calloc(strlen(start) + strlen(child), sizeof(char));
    sprintf(tmp, "%s%s%s", start, child, i < n->count - 1 ? " " : "");
    start = tmp;
  }

  char *final = calloc(strlen(start) + 2, sizeof(char));
  sprintf(final, "%s%s", start, n->count > 0 ? ")" : "");

  return final;
}

char *lir_tag_name(lir_node_T *n) {
  assert(LIR_TAG_COUNT == 14);

  char *names[LIR_TAG_COUNT] = {
      "Program", "Label", "Push", "Load", "Store", "Call", "Ident",
      "Value",   "Add",   "Sub",  "Mul",  "Div",   "Mod",  "Dump",
  };

  return names[n->tag];
}
