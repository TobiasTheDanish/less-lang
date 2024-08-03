#include "include/type_check.h"
#include "include/ast_nodes.h"
#include "include/logger.h"
#include "include/symbol.h"
#include "include/symbol_table.h"
#include "include/token.h"
#include <stdlib.h>
#include <string.h>
void check_conditional(type_check_t *t, ast_node_T *node);
void check_if(type_check_t *t, ast_node_T *node);
void check_block(type_check_t *t, ast_node_T *node);
void check_value(type_check_t *t, ast_node_T *node);
void check_bin_op(type_check_t *t, ast_node_T *node);
void check_type_annot(type_check_t *t, ast_node_T *node);
void check_expr(type_check_t *t, ast_node_T *node);
void check_statement(type_check_t *t, ast_node_T *node);
void check_program(type_check_t *t, ast_node_T *node);

ast_node_T *shift(ast_node_T ***array) {
  ast_node_T *res = **array;
  *array = *array + 1;
  return res;
}

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
    log_debug(t->debug, "Checking POINTER or IDENT\n");
    symbol_type_T *type;
    if (symbol_table_contains(t->table, value->t->value)) {
      symbol_var_T *var =
          (symbol_var_T *)symbol_table_get(t->table, value->t->value);
      type = (symbol_type_T *)var->type;
    } else {
      type = (symbol_type_T *)symbol_table_get(t->table, "undefined");
    }
    node->symbol_type = type;
  } break;
  case T_INTEGER: {
    log_debug(t->debug, "Checking INTEGER\n");
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
    log_debug(t->debug, "Checking STRING\n");
    data_table_put(t->data, value->t, "string");
    // printf("<%s, %s>\n", value->t->value, "string");
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "string");
    break;

  case T_CHAR:
    log_debug(t->debug, "Checking CHAR\n");
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "i8");
    break;

  default:
    log_error(value->t->loc, 1, "Invalid token type for value. Found: %s.\n",
              token_get_name(value->t->type));
  }

  log_debug(t->debug, "Finished checking value\n");
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

void check_func_call(type_check_t *t, ast_node_T *node) {
  ast_func_call_T *func_node = (ast_func_call_T *)node;

  symbol_T *ident_symbol = symbol_table_get(t->table, func_node->ident->value);
  if (ident_symbol->type != SYM_FUNC) {
    log_error(func_node->ident->loc, 1, "Symbol '%s' is not callable\n",
              func_node->ident->value);
  }
  symbol_func_T *func_symbol = (symbol_func_T *)ident_symbol;

  size_t func_param_count = 0;
  symbol_T **func_params =
      symbol_table_get_params(func_symbol->scope, &func_param_count);

  if (func_node->param_count < func_param_count) {
    log_error(func_node->ident->loc, 1,
              "Too few argmuments passed to function '%s'. Expected '%d', but "
              "found '%d'\n",
              func_node->ident->value, func_param_count,
              func_node->param_count);
  } else if (func_node->param_count > func_param_count) {
    log_error(func_node->ident->loc, 1,
              "Too many argmuments passed to function '%s'. Expected '%d', but "
              "found '%d'\n",
              func_node->ident->value, func_param_count,
              func_node->param_count);
  }

  for (size_t i = 0; i < func_node->param_count; i++) {
    symbol_var_T *param_symbol = (symbol_var_T *)func_params[i];
    ast_node_T *arg = func_node->params[i];
    check_expr(t, arg);
    if (strcmp(arg->symbol_type->base.name, param_symbol->type->name) != 0 &&
        !symbol_can_upgrade_type(
            arg->symbol_type->type_cat,
            ((symbol_type_T *)param_symbol->type)->type_cat)) {
      log_error(arg->loc, 1,
                "Cannot use expression of type '%s' as argument of type '%s'\n",
                arg->symbol_type->base.name, param_symbol->type->name);
    }
  }

  node->symbol_type = (symbol_type_T *)func_symbol->ret_type;
}

void check_array_init(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check array element\n");
  ast_array_element_T *array_node = (ast_array_element_T *)node;
  symbol_type_T *array_elem_symbol =
      (symbol_type_T *)symbol_table_get(t->table, array_node->ident->value);

  check_expr(t, array_node->offset);
  if (array_node->offset->symbol_type->type_cat < I8 ||
      array_node->offset->symbol_type->type_cat > I64) {
    log_error(array_node->offset->loc, 1, "Array length must be an integer\n");
  }

  node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "array");
  node->elem_type = array_elem_symbol;

  log_todo("check_array_init not implemented yet\n");
}

void check_array_element(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check array element\n");
  ast_array_element_T *elem_node = (ast_array_element_T *)node;
  symbol_T *array_symbol = symbol_table_get(t->table, elem_node->ident->value);

  if (array_symbol->type != SYM_VAR && array_symbol->type != SYM_VAR_TYPE) {
    log_error(array_symbol->loc, 1, "Symbol '%s' cannot be used as array\n",
              array_symbol->name);
  }

  if (array_symbol->type == SYM_VAR_TYPE) {
    return check_array_init(t, node);
  }

  symbol_var_T *array_var_symbol = (symbol_var_T *)array_symbol;

  if (strcmp(array_var_symbol->type->name, "array") != 0) {
    log_error(array_symbol->loc, 1,
              "Symbol of '%s' cannot be used as an array\n",
              array_var_symbol->type->name);
  }

  log_todo("check_array_element not implemented yet\n");
}

void check_expr(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check expression\n");
  log_debug(t->debug, "expr ast node '%s'\n", ast_get_name(node->type));

  switch (node->type) {
  case AST_BIN_OP:
    check_bin_op(t, node);
    break;
  case AST_CONDITIONAL:
    check_conditional(t, node);
    break;
  case AST_SYSCALL:
    log_todo("check_syscall not implemented yet\n");
    break;
  case AST_IF:
    check_if(t, node);
    break;
  case AST_VALUE:
    check_value(t, node);
    break;
  case AST_FUNC_CALL:
    check_func_call(t, node);
    break;
  case AST_ARRAY_ELEMENT:
    check_array_element(t, node);
    break;

  case AST_RETURN:
  case AST_WHILE:
  case AST_DUMP:
  case AST_ASSIGN:
  case AST_DECL:
  case AST_VAR_DECL:
  case AST_FUNC_DECL:
  case AST_CONST_DECL:
  case AST_STRUCT_INIT:
  case AST_ATTRIBUTE:
  case AST_ARRAY:
  case AST_PROP:
  case AST_ELSE:
  case AST_BLOCK:
  case AST_EXPR:
  case AST_OP:
  case AST_NO_OP:
  case AST_PROGRAM:
  case AST_COND_OP:
  case AST_LOGICAL_OP:
  case AST_TYPE_ANNOT:
  case AST_FUNC_PARAM_LIST:
  case AST_FUNC_PARAM:
    log_error(node->loc, 1, "Unexpected node in expr, found: %s.\n",
              ast_get_name(node->type));
    break;
  }
}

void check_dump(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check dump\n");
  ast_dump_T *dump = (ast_dump_T *)node;

  check_expr(t, dump->value);
}

void check_type_annot(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check type annotation\n");
  ast_type_annot_T *type_annot = (ast_type_annot_T *)node;
  ast_value_T *value = (ast_value_T *)type_annot->type;
  symbol_type_T *type;
  if (symbol_table_contains(t->table, value->t->value)) {
    symbol_T *symbol = symbol_table_get(t->table, value->t->value);
    if (symbol->type != SYM_VAR_TYPE) {
      log_error(value->t->loc, 1, "Identifier '%s' is not a type\n",
                value->t->value);
    } else {
      type = (symbol_type_T *)symbol;
    }
  } else {
    type = (symbol_type_T *)symbol_table_get(t->table, "undefined");
  }
  if (type_annot->is_array) {
    node->elem_type = type;
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "array");
  } else {
    node->symbol_type = type;
  }
}

void check_param_list(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check function parameter list\n");
  ast_param_list_T *list = (ast_param_list_T *)node;
  for (size_t i = 0; i < list->child_count; i++) {
    ast_node_T *param_node = list->children[i];

    if (param_node->type != AST_FUNC_PARAM) {
      log_error(param_node->loc, 1,
                "Unexpected node in parameter list '%s', expected parameter\n",
                ast_get_name(param_node->type));
    }

    ast_func_param_T *param = (ast_func_param_T *)param_node;

    ast_value_T *ident = (ast_value_T *)param->ident;
    check_type_annot(t, param->type_annot);
    ast_value_T *annot = (ast_value_T *)param->type_annot;

    if (strcmp(annot->base.symbol_type->base.name, "undefined") == 0) {
      log_error(param->type_annot->loc, 1,
                "Undefined symbol '%s' used as type in function parameter\n",
                annot->t->value);
    }

    symbol_T *param_symbol = symbol_new_var(ident->t->value, ident->t->loc,
                                            (symbol_T *)annot->base.symbol_type,
                                            param->is_mut, 1, 0, NULL);

    symbol_table_put(t->table, param_symbol);
  }
}

void check_func_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check function declaration\n");
  ast_decl_T *decl = (ast_decl_T *)node;

  if (decl->child_count == 0 || decl->children[0]->type != AST_VALUE) {
    log_error(decl->token->loc, 1,
              "Missing function name in function declaration\n");
  }
  ast_node_T *name = shift(&decl->children);
  decl->child_count--;
  check_value(t, name);
  ast_value_T *v = (ast_value_T *)name;

  if (name->symbol_type == NULL) {
    log_error(v->t->loc, 1,
              "Type for symbol '%s' was null after call to check_value\n",
              v->t->value);
  }

  if (strcmp(name->symbol_type->base.name, "undefined") != 0) {
    log_error(name->loc, 1, "Redefinition of symbol '%s'\n", v->t->value);
  }

  symbol_table_T *parent_scope = t->table;
  symbol_table_T *func_scope =
      symbol_table_new(v->t->value, t->table->level + 1, parent_scope);

  symbol_T *func_symbol = symbol_new_func(v->t->value, func_scope, v->t->loc);
  symbol_table_put(t->table, func_symbol);
  t->table = func_scope;

  if (decl->child_count == 0) {
    log_error(decl->token->loc, 1,
              "Missing parameter list in function declaration\n");
  } else if (decl->children[0]->type != AST_FUNC_PARAM_LIST) {
    log_error(decl->children[0]->loc, 1,
              "Expected parameter list, found '%s'\n",
              ast_get_name(decl->children[0]->type));
  }

  check_param_list(t, shift(&decl->children));
  decl->child_count--;

  if (decl->child_count == 0) {
    log_error(decl->token->loc, 1,
              "Missing code block in function declaration\n");
  }

  if (decl->children[0]->type == AST_TYPE_ANNOT) {
    ast_node_T *type = shift(&decl->children);
    decl->child_count--;
    check_type_annot(t, type);
    if (strcmp(type->symbol_type->base.name, "undefined") == 0) {
      ast_value_T *v = (ast_value_T *)type;
      log_error(
          type->loc, 1,
          "Undefined symbol '%s' used as return type in function declaration\n",
          v->t->value);
    }
    node->symbol_type = type->symbol_type;
    node->elem_type = type->elem_type;
  } else {
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "void");
  }

  if (decl->child_count == 0 || decl->children[0]->type != AST_BLOCK) {
    log_error(decl->token->loc, 1,
              "Missing code block in function declaration\n");
  }

  check_block(t, decl->children[0]);
  ((symbol_func_T *)func_symbol)->ret_type =
      (symbol_T *)decl->children[0]->symbol_type;
  node->symbol_type = decl->children[0]->symbol_type;

  t->table = parent_scope;

  if (t->debug) {
    symbol_table_print(func_scope);
  }
}

void check_block(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check block\n");
  ast_block_T *block = (ast_block_T *)node;

  for (size_t i = 0; i < block->count; i++) {
    log_debug(t->debug, "stmt #%d: '%s'\n", i,
              ast_get_name(block->expressions[i]->type));
    check_statement(t, block->expressions[i]);
    if (block->expressions[i]->type == AST_RETURN) {
      symbol_type_T *ret_type = block->expressions[i]->symbol_type;
      if (strcmp(ret_type->base.name, "undefined") == 0) {
        log_error(block->expressions[i]->loc, 1,
                  "Cannot use undefined symbol as return statement\n");
      }

      if (node->symbol_type == NULL ||
          strcmp(node->symbol_type->base.name, ret_type->base.name) == 0) {
        node->symbol_type = ret_type;
      } else {
        log_error(block->expressions[i]->loc, 1,
                  "Cannot return multiple types: '%s' and '%s'\n",
                  node->symbol_type->base.name, ret_type->base.name);
      }
    }
  }

  if (node->symbol_type == NULL) {
    node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "void");
  }
}

void check_const_decl(type_check_t *t, ast_node_T *node) {
  log_todo("check_const_decl not implemented yet\n");
}

void check_var_decl(type_check_t *t, ast_node_T *node) {
  ast_decl_T *decl = (ast_decl_T *)node;
  unsigned char is_mut = decl->token->type == T_MUT;

  if (decl->child_count == 0 || decl->children[0]->type != AST_VALUE) {
    log_error(decl->token->loc, 1,
              "Missing variable name in variable declaration\n");
  }
  ast_node_T *name = shift(&decl->children);
  decl->child_count--;
  check_value(t, name);
  ast_value_T *v = (ast_value_T *)name;

  if (name->symbol_type == NULL) {
    log_error(v->t->loc, 1,
              "Type for symbol '%s' was null after call to check_value\n",
              v->t->value);
  }

  if (strcmp(name->symbol_type->base.name, "undefined") != 0) {
    log_error(name->loc, 1, "Redefinition of symbol '%s'\n", v->t->value);
  }

  if (decl->child_count == 0) {
    log_error(name->loc, 1, "Missing value in declaration of symbol '%s'\n",
              v->t->value);
  }

  ast_node_T *type_annot = NULL;
  if (decl->children[0]->type == AST_TYPE_ANNOT) {
    type_annot = shift(&decl->children);
    decl->child_count--;
    check_type_annot(t, type_annot);
    if (strcmp(type_annot->symbol_type->base.name, "undefined") == 0) {
      ast_value_T *type_v = (ast_value_T *)type_annot;
      log_error(type_annot->loc, 1,
                "Undefined symbol '%s' used as type annotation in variable "
                "declaration\n",
                type_v->t->value);
    }
    node->symbol_type = type_annot->symbol_type;
  }

  ast_node_T *value = shift(&decl->children);
  decl->child_count--;

  check_expr(t, value);

  if (strcmp(value->symbol_type->base.name, "undefined") == 0) {
    log_error(name->loc, 1,
              "Cannot use undefined symbol as value for variable '%s'\n",
              v->t->value);
  }

  if (type_annot != NULL) {
    if (strcmp(type_annot->symbol_type->base.name,
               value->symbol_type->base.name) != 0 &&
        !symbol_can_upgrade_type(value->symbol_type->type_cat,
                                 type_annot->symbol_type->type_cat)) {
      log_error(
          value->loc, 1,
          "Cannot assign value of type '%s', to variable '%s' of type '%s'\n",
          value->symbol_type->base.name, v->t->value,
          type_annot->symbol_type->base.name);
    }
  } else {
    node->symbol_type = value->symbol_type;
    node->elem_type = value->elem_type;
  }

  symbol_T *var_symbol = symbol_new_var(
      v->t->value, v->t->loc, (symbol_T *)node->symbol_type, is_mut, 0, 0, 0);
  ((symbol_var_T *)var_symbol)->elem_type = (symbol_T *)node->elem_type;
  symbol_table_put(t->table, var_symbol);
}

void check_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check declaration\n");
  ast_decl_T *decl = (ast_decl_T *)node;
  switch (decl->token->type) {
  case T_LET:
  case T_MUT:
    check_var_decl(t, node);
    break;
  case T_CONST:
    check_const_decl(t, node);
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

void check_conditional(type_check_t *t, ast_node_T *node) {
  ast_cond_T *cond = (ast_cond_T *)node;

  check_expr(t, cond->lhs);
  check_expr(t, cond->rhs);

  if (strcmp(cond->lhs->symbol_type->base.name,
             cond->rhs->symbol_type->base.name) != 0 &&
      !symbol_can_upgrade_type(cond->lhs->symbol_type->type_cat,
                               cond->rhs->symbol_type->type_cat)) {
    ast_cond_op_T *op = (ast_cond_op_T *)cond->op;

    log_error(cond->op->loc, 1,
              "Cannot do boolean operation '%s' between types '%s' and '%s\n",
              op->t->value, cond->lhs->symbol_type->base.name,
              cond->rhs->symbol_type->base.name);
  }

  node->symbol_type = (symbol_type_T *)symbol_table_get(t->table, "bool");
}

void check_else(type_check_t *t, ast_node_T *node) {
  ast_else_T *elze = (ast_else_T *)node;
  if (elze->block->type == AST_BLOCK) {
    check_block(t, elze->block);
  } else {
    check_statement(t, elze->block);
  }

  node->symbol_type = elze->block->symbol_type;
}

void check_if(type_check_t *t, ast_node_T *node) {
  ast_if_T *if_node = (ast_if_T *)node;

  check_expr(t, if_node->cond);

  if (strcmp(if_node->cond->symbol_type->base.name, "bool") != 0) {
    log_error(if_node->cond->loc, 1,
              "If condition must be of type 'bool', found '%s'\n",
              if_node->cond->symbol_type->base.name);
  }

  check_block(t, if_node->block);
  node->symbol_type = if_node->block->symbol_type;

  if (if_node->elze != NULL) {
    check_else(t, if_node->elze);
    if (strcmp(node->symbol_type->base.name,
               if_node->elze->symbol_type->base.name) != 0) {
      log_error(node->loc, 1,
                "Cannot return multiple values from 'if'/'else if'/'else' "
                "block. Found '%s' and '%s\n",
                node->symbol_type->base.name,
                if_node->elze->symbol_type->base.name);
    }
  }
}

void check_return(type_check_t *t, ast_node_T *node) {
  ast_return_T *ret_node = (ast_return_T *)node;

  check_expr(t, ret_node->value);
  node->symbol_type = ret_node->value->symbol_type;
}

void check_assign(type_check_t *t, ast_node_T *node) {
  ast_assign_T *assign = (ast_assign_T *)node;

  check_expr(t, assign->lhs);
  symbol_type_T *lhs_type = assign->lhs->symbol_type;
  if (strcmp(lhs_type->base.name, "undefined") == 0) {
    log_error(assign->lhs->loc, 1, "Cannot assign to undefined variable\n");
  }

  check_expr(t, assign->value);
  symbol_type_T *rhs_type = assign->value->symbol_type;

  if (strcmp(rhs_type->base.name, "undefined") == 0) {
    log_error(assign->value->loc, 1, "Use of undefined symbol\n");
  } else if (strcmp(rhs_type->base.name, lhs_type->base.name) != 0 &&
             !symbol_can_upgrade_type(lhs_type->type_cat, rhs_type->type_cat)) {
    log_error(assign->ident->loc, 1,
              "Cannot assign value of type '%s' to symbol of type '%s'\n",
              rhs_type->base.name, lhs_type->base.name);
  } else if (lhs_type->type_cat == ARRAY &&
             strcmp(assign->lhs->elem_type->base.name,
                    assign->value->elem_type->base.name) != 0) {
    log_error(assign->ident->loc, 1,
              "Cannot assign type '%s array' to symbol of type '%s array'\n",
              assign->value->elem_type->base.name,
              assign->lhs->elem_type->base.name);
  }

  assign->lhs->symbol_type = symbol_upgrade_type(lhs_type, rhs_type);
  node->symbol_type = assign->lhs->symbol_type;
  node->elem_type = assign->lhs->elem_type;
}

void check_while(type_check_t *t, ast_node_T *node) {
  ast_while_T *while_node = (ast_while_T *)node;

  check_expr(t, while_node->cond);
  if (strcmp(while_node->cond->symbol_type->base.name, "bool") != 0) {
    log_error(while_node->cond->loc, 1,
              "If condition must be of type 'bool', found '%s'\n",
              while_node->cond->symbol_type->base.name);
  }

  check_block(t, while_node->block);
  node->symbol_type = while_node->block->symbol_type;
}

void check_statement(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check statement\n");
  switch (node->type) {
  case AST_SYSCALL:
    log_todo("check_syscall not implemented yet\n");
    break;
  case AST_IF:
    check_if(t, node);
    break;
  case AST_ASSIGN:
    check_assign(t, node);
    break;
  case AST_WHILE:
    check_while(t, node);
    break;
  case AST_DUMP:
    check_dump(t, node);
    break;
  case AST_DECL:
    check_decl(t, node);
    break;
  case AST_RETURN:
    check_return(t, node);
    break;
  default:
    log_error(node->loc, 1, "Unexpected node in statement, found: %s.\n",
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

  if (t->debug) {
    symbol_table_print(t->table);
  }

  log_todo("Static analysis is not implemented yet\n");
}
