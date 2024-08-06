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
  log_info("Type checker initialized\n");
  type_check_t *t = malloc(sizeof(type_check_t));
  t->program = program;
  t->debug = debug;
  t->table = table;
  t->data = data;

  return t;
}

ast_node_T *type_check(type_check_t *t) {
  log_info("Type checker started\n");
  symbol_table_init_builtins(t->table);
  log_info("Symbol table initialized\n");

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
    symbol_T *type;
    if (symbol_table_contains(t->table, value->t->value)) {
      symbol_T *sym = symbol_table_get(t->table, value->t->value);
      log_debug(t->debug, "symbol of '%s': %p\n", value->t->value, sym);
      if (sym->tag == SYM_VAR) {
        type = sym->var.type;
      } else if (sym->tag == SYM_VAR_TYPE) {
        type = sym;
      } else {
        type = sym->func.ret_type;
      }
    } else {
      type = symbol_table_get(t->table, "undefined");
    }
    log_debug(t->debug, "type of '%s': %p\n", value->t->value, type);
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

    symbol_T *type_symbol = symbol_table_get(t->table, type);
    type_symbol->type.is_comptime = 1;
    node->symbol_type = type_symbol;
  } break;
  case T_STRING: {
    log_debug(t->debug, "Checking STRING\n");
    data_table_put(t->data, value->t, "string");
    // printf("<%s, %s>\n", value->t->value, "string");

    symbol_T *type_symbol = symbol_table_get(t->table, "string");
    type_symbol->type.is_comptime = 1;
    node->symbol_type = type_symbol;
  } break;
  case T_CHAR: {
    log_debug(t->debug, "Checking CHAR\n");

    symbol_T *type_symbol = symbol_table_get(t->table, "i8");
    type_symbol->type.is_comptime = 1;
    node->symbol_type = type_symbol;
  } break;

  default:
    log_error(value->t->loc, 1, "Invalid token type for value. Found: %s.\n",
              token_get_name(value->t->type));
  }

  log_debug(t->debug, "Value type symbol after check: '%s'\n",
            symbol_to_string(node->symbol_type));
  log_debug(t->debug, "Finished checking value\n");
}

void check_bin_op(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check bin op\n");
  ast_bin_op_T *bin_op = (ast_bin_op_T *)node;

  check_expr(t, bin_op->rhs);
  check_expr(t, bin_op->lhs);

  if (!symbol_cmp(bin_op->rhs->symbol_type, bin_op->lhs->symbol_type)) {
    ast_op_T *op = (ast_op_T *)bin_op->op;

    log_error(bin_op->op->loc, 1,
              "Cannot perform '%s' for types '%s' and '%s'\n", op->t->value,
              (bin_op->lhs->symbol_type->name),
              (bin_op->rhs->symbol_type->name));
  }

  node->symbol_type = bin_op->lhs->symbol_type;
  node->symbol_type->type.is_comptime =
      bin_op->lhs->symbol_type->type.is_comptime &&
      bin_op->rhs->symbol_type->type.is_comptime;
}

void check_func_call(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "check_func_call\n");
  ast_func_call_T *func_node = (ast_func_call_T *)node;

  symbol_T *ident_symbol = symbol_table_get(t->table, func_node->ident->value);
  if (ident_symbol == NULL) {
    log_error(func_node->ident->loc, 1,
              "Attempt to call undefined identifier '%s'\n",
              func_node->ident->value);
  }

  if (ident_symbol->tag != SYM_FUNC) {
    log_error(func_node->ident->loc, 1, "Symbol '%s' is not callable\n",
              func_node->ident->value);
  }
  symbol_func_T func_symbol = ident_symbol->func;

  size_t func_param_count = 0;
  symbol_T *func_params =
      symbol_table_get_params(func_symbol.scope, &func_param_count);

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
    symbol_T param_symbol = func_params[i];
    if (param_symbol.tag != SYM_VAR) {
      log_error(param_symbol.loc, 1,
                "Symbol '%s' cannot be used as a function parameter\n",
                param_symbol.name);
    }
    ast_node_T *arg = func_node->params[i];

    if (arg == NULL) {
      log_error(func_node->base.loc, 1,
                "argument #%d of function call was null, expected to find "
                "'%d' arguments\n",
                (i + 1), func_node->param_count);
    }

    if (arg->symbol_type == NULL) {
      check_expr(t, arg);
      if (arg->symbol_type == NULL) {
        log_error(arg->loc, 1, "Symbol type of arg has not been set!\n");
      }
    }

    if (arg->symbol_type->tag != SYM_VAR_TYPE) {
      log_error(arg->symbol_type->loc, 1,
                "Identifier '%s' cannot be used as a type\n",
                arg->symbol_type->name);
    }

    if (strcmp(arg->symbol_type->name, param_symbol.var.type->name) != 0 &&
        !symbol_can_upgrade_type(arg->symbol_type->type.type_cat,
                                 param_symbol.var.type->type.type_cat)) {
      log_error(arg->loc, 1,
                "Cannot use expression of type '%s' as argument of type '%s'\n",
                arg->symbol_type->name, param_symbol.var.type->name);
    }
  }

  node->symbol_type = func_symbol.ret_type;
}

void check_array_init(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check array init\n");
  ast_array_element_T *array_node = (ast_array_element_T *)node;

  check_expr(t, array_node->offset);
  if (array_node->offset->symbol_type->type.type_cat < I8 ||
      array_node->offset->symbol_type->type.type_cat > I64) {
    log_error(array_node->offset->loc, 1, "Array length must be an integer\n");
  }

  symbol_T *res = malloc(sizeof(symbol_T));
  memcpy(res, symbol_table_get(t->table, "array"), sizeof(symbol_T));
  res->type.underlying_type = malloc(sizeof(symbol_T));
  memcpy(res->type.underlying_type,
         symbol_table_get(t->table, array_node->offset->symbol_type->name),
         sizeof(symbol_T));

  node->symbol_type = res;
}

void check_array_element(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check array element\n");
  ast_array_element_T *elem_node = (ast_array_element_T *)node;
  symbol_T *array_symbol = symbol_table_get(t->table, elem_node->ident->value);

  if (array_symbol->tag != SYM_VAR && array_symbol->tag != SYM_VAR_TYPE) {
    log_error(array_symbol->loc, 1, "Symbol '%s' cannot be used as array\n",
              array_symbol->name);
  }

  if (array_symbol->tag == SYM_VAR_TYPE) {
    return check_array_init(t, node);
  }

  if (strcmp(array_symbol->var.type->name, "array") != 0) {
    log_error(array_symbol->loc, 1,
              "Symbol of '%s' cannot be used as an array\n",
              array_symbol->name);
  }

  if (array_symbol->var.type->type.underlying_type == NULL ||
      array_symbol->var.type->type.underlying_type->tag != SYM_VAR_TYPE) {
    log_error(array_symbol->loc, 1, "Symbol of '%s' has no underlying type\n",
              array_symbol->name);
  }

  node->symbol_type = array_symbol->var.type->type.underlying_type;
}

void check_syscall(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check syscall\n");
  ast_syscall_T *syscall_node = (ast_syscall_T *)node;

  if (syscall_node->count == 0) {
    log_error(node->loc, 1, "Syscalls require atleast 1 parameter\n");
  }

  check_expr(t, syscall_node->params[0]);

  if (syscall_node->params[0]->symbol_type->type.type_cat < I8 ||
      syscall_node->params[0]->symbol_type->type.type_cat > I64) {
    log_error(node->loc, 1,
              "First parameter of syscall must be an integer, found '%s'\n",
              syscall_node->params[0]->symbol_type->name);
  }

  node->symbol_type = symbol_table_get(t->table, "i64");
}

void check_prop(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "check_prop\n");
  ast_prop_T *prop_node = (ast_prop_T *)node;

  check_expr(t, prop_node->lhs);

  log_debug(t->debug, "lhs type scope: %p\n",
            prop_node->lhs->symbol_type->type.scope);

  if (!symbol_table_contains(prop_node->lhs->symbol_type->type.scope,
                             prop_node->rhs_token->value)) {
    log_error(prop_node->rhs->loc, 1, "'%s' is not a property of type '%s'\n",
              prop_node->rhs_token->value, prop_node->lhs->symbol_type->name);
  }

  symbol_table_T *parent_scope = t->table;
  t->table = prop_node->lhs->symbol_type->type.scope;
  t->table->parent = parent_scope;

  if (prop_node->rhs->type == AST_FUNC_CALL) {
    ast_func_call_T *rhs = (ast_func_call_T *)prop_node->rhs;

    ast_node_T **new_params =
        malloc((rhs->param_count + 1) * sizeof(ast_node_T *));
    new_params[0] = prop_node->lhs;

    for (size_t i = 0; i < rhs->param_count; i++) {
      log_debug(t->debug,
                "Param #%d of original params for prop func call: %p\n",
                (i + 1), rhs->params[i]);
      new_params[i + 1] = rhs->params[i];
    }
    rhs->params = new_params;
    rhs->param_count += 1;

    for (size_t i = 0; i < rhs->param_count; i++) {
      log_debug(t->debug, "Param #%d of new params for prop func call: %p\n",
                (i + 1), rhs->params[i]);
    }
  }
  check_expr(t, prop_node->rhs);

  if (strcmp(prop_node->rhs->symbol_type->name, "undefined") == 0) {
    log_error(prop_node->rhs->loc, 1, "'%s' is not a property of type '%s'\n",
              prop_node->rhs_token->value, prop_node->lhs->symbol_type->name);
  }

  t->table = parent_scope;

  node->symbol_type = prop_node->rhs->symbol_type;
}

void check_struct_init_attrib(type_check_t *t, symbol_table_T *struct_scope,
                              ast_node_T *node) {
  log_debug(t->debug, "type check struct init attribute\n");
  ast_attribute_T *attrib_node = (ast_attribute_T *)node;

  if (!symbol_table_contains(struct_scope, attrib_node->name->value)) {
    log_error(attrib_node->name->loc, 1, "'%s' is not a part of struct '%s'\n",
              attrib_node->name->value, struct_scope->name);
  }

  check_expr(t, attrib_node->value);

  if (strcmp(attrib_node->value->symbol_type->name, "undefined") == 0) {
    log_error(attrib_node->value->loc, 1,
              "Cannot use undefined symbol as value for property '%s'\n",
              attrib_node->name->value);
  }

  symbol_T *attrib_symbol =
      symbol_table_get(struct_scope, attrib_node->name->value);

  if (strcmp(attrib_symbol->var.type->name,
             attrib_node->value->symbol_type->name) != 0 &&
      !symbol_can_upgrade_type(
          attrib_symbol->var.type->type.type_cat,
          attrib_node->value->symbol_type->type.type_cat)) {
    log_error(attrib_node->value->loc, 1,
              "Cannot use value of type '%s' in attribute '%s' of type '%s'\n",
              attrib_node->value->symbol_type->name, attrib_node->name->value,
              attrib_symbol->var.type->name);
  } else if (attrib_node->value->symbol_type->type.type_cat >
             attrib_symbol->var.type->type.type_cat) {
    log_error(attrib_node->value->loc, 1,
              "Type '%s' does not fit in struct attribute '%s' of type '%s'\n",
              attrib_node->value->symbol_type->name, attrib_node->name->value,
              attrib_symbol->var.type->name);
  }

  node->symbol_type = symbol_upgrade_type(attrib_symbol->var.type,
                                          attrib_node->value->symbol_type);

  log_debug(t->debug, "struct attrib: '%s' of type '%s'\n",
            attrib_node->name->value, attrib_node->value->symbol_type->name);
}

void check_struct_init(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check struct init\n");
  ast_struct_init_T *struct_node = (ast_struct_init_T *)node;

  symbol_T *struct_symbol =
      symbol_table_get(t->table, struct_node->ident->value);

  if (struct_symbol->tag != SYM_VAR_TYPE) {
    log_error(struct_node->ident->loc, 1,
              "Symbol '%s' cannot be used as a type\n",
              struct_node->ident->value);
  }

  for (size_t i = 0; i < struct_node->attr_count; i++) {
    check_struct_init_attrib(t, struct_symbol->type.scope,
                             struct_node->attributes[i]);
  }

  node->symbol_type = struct_symbol;
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
    check_syscall(t, node);
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
  case AST_PROP:
    check_prop(t, node);
    break;
  case AST_STRUCT_INIT:
    check_struct_init(t, node);
    break;

  case AST_ATTRIBUTE_LIST:
  case AST_RETURN:
  case AST_WHILE:
  case AST_DUMP:
  case AST_ASSIGN:
  case AST_DECL:
  case AST_VAR_DECL:
  case AST_FUNC_DECL:
  case AST_CONST_DECL:
  case AST_ATTRIBUTE:
  case AST_ARRAY:
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
  node->symbol_type = symbol_table_get(t->table, "void");
}

void check_type_annot(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check type annotation\n");
  ast_type_annot_T *type_annot = (ast_type_annot_T *)node;
  ast_value_T *value = (ast_value_T *)type_annot->type;
  symbol_T *type;
  if (symbol_table_contains(t->table, value->t->value)) {
    symbol_T *symbol = symbol_table_get(t->table, value->t->value);
    if (symbol->tag != SYM_VAR_TYPE) {
      log_error(value->t->loc, 1, "Identifier '%s' is not a type\n",
                value->t->value);
    } else {
      type = symbol;
    }
  } else {
    type = symbol_table_get(t->table, "undefined");
  }
  if (type_annot->is_array) {
    if (strcmp(type->name, "undefined") == 0) {
      log_error(value->t->loc, 1, "Cannot create array of type 'undefined'\n");
    }

    symbol_T *res = malloc(sizeof(symbol_T));
    memcpy(res, symbol_table_get(t->table, "array"), sizeof(symbol_T));
    res->type.underlying_type = malloc(sizeof(symbol_T));
    memcpy(res->type.underlying_type, type, sizeof(symbol_T));

    node->symbol_type = res;
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

    if (strcmp(annot->base.symbol_type->name, "undefined") == 0) {
      log_error(param->type_annot->loc, 1,
                "Undefined symbol '%s' used as type in function parameter\n",
                annot->t->value);
    }

    symbol_table_put(t->table, symbol_new_var(ident->t->value, ident->t->loc,
                                              *annot->base.symbol_type,
                                              param->is_mut, 1, 0, NULL));
  }
}

void check_func_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check function declaration\n");
  node->symbol_type = malloc(sizeof(symbol_T));
  ast_decl_T *decl = (ast_decl_T *)node;
  size_t start_child_count = decl->child_count;
  ast_node_T **start_child_ptr = decl->children;

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

  if (strcmp(name->symbol_type->name, "undefined") != 0) {
    log_error(name->loc, 1, "Redefinition of symbol '%s'\n", v->t->value);
  }

  symbol_table_T *parent_scope = t->table;
  symbol_table_T *func_scope =
      symbol_table_new(v->t->value, t->table->level + 1, parent_scope);

  symbol_T *func_symbol = malloc(sizeof(symbol_T));
  *func_symbol = symbol_new_func(v->t->value, func_scope, v->t->loc);
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

  size_t count = 0;
  symbol_T *params = symbol_table_get_params(func_scope, &count);

  if (decl->child_count == 0) {
    log_error(decl->token->loc, 1,
              "Missing code block in function declaration\n");
  }

  if (decl->children[0]->type == AST_TYPE_ANNOT) {
    ast_node_T *type = shift(&decl->children);
    decl->child_count--;
    check_type_annot(t, type);
    if (strcmp(type->symbol_type->name, "undefined") == 0) {
      ast_value_T *v = (ast_value_T *)type;
      log_error(
          type->loc, 1,
          "Undefined symbol '%s' used as return type in function declaration\n",
          v->t->value);
    }
    *node->symbol_type = *type->symbol_type;
  } else {
    *node->symbol_type = *symbol_table_get(t->table, "void");
  }

  log_debug(t->debug, "func decl type after type annotation: '%s'\n",
            symbol_to_string(node->symbol_type));

  if (decl->child_count == 0 || decl->children[0]->type != AST_BLOCK) {
    log_error(decl->token->loc, 1,
              "Missing code block in function declaration\n");
  }

  check_block(t, decl->children[0]);
  if (decl->children[0]->symbol_type == NULL) {
    log_error(decl->children[0]->loc, 1,
              "Checking block in func declaration failed, and block type was "
              "not set\n");
  }

  log_debug(t->debug, "func decl block type: '%s'\n",
            symbol_to_string(decl->children[0]->symbol_type));

  if (!symbol_cmp(node->symbol_type, decl->children[0]->symbol_type)) {
    log_error(decl->children[0]->loc, 1,
              "Return type '%s' does not match declared function return "
              "type '%s'\n",
              decl->children[0]->symbol_type, node->symbol_type->name);
  }

  func_symbol->func.ret_type = malloc(sizeof(symbol_T));
  *func_symbol->func.ret_type = *node->symbol_type;

  log_debug(t->debug, "func symbol ret_type: '%s'\n",
            symbol_to_string(func_symbol->func.ret_type));
  log_debug(t->debug, "node symbol_type: '%s'\n",
            symbol_to_string(node->symbol_type));

  if (count > 0) {
    log_debug(t->debug, "Adding '%s' to scope of type '%s'\n",
              func_symbol->name, params[0].var.type->name);
    symbol_table_put(params[0].var.type->type.scope, *func_symbol);
    symbol_table_update(t->table, *params[0].var.type);
    symbol_table_print(params[0].var.type->type.scope);
  }
  t->table = parent_scope;
  symbol_table_put(t->table, *func_symbol);

  if (t->debug) {
    symbol_table_print(func_scope);
  }
  decl->children = start_child_ptr;
  decl->child_count = start_child_count;
}

void check_block(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check block\n");
  ast_block_T *block = (ast_block_T *)node;

  for (size_t i = 0; i < block->count; i++) {
    log_debug(t->debug, "stmt #%d: '%s'\n", i,
              ast_get_name(block->expressions[i]->type));
    check_statement(t, block->expressions[i]);
    if (block->expressions[i]->type == AST_RETURN) {
      symbol_T *ret_type = block->expressions[i]->symbol_type;
      if (strcmp(ret_type->name, "undefined") == 0) {
        log_error(block->expressions[i]->loc, 1,
                  "Cannot use undefined symbol as return statement\n");
      }

      if (node->symbol_type == NULL ||
          strcmp(node->symbol_type->name, ret_type->name) == 0) {
        node->symbol_type = ret_type;
      } else {
        log_error(block->expressions[i]->loc, 1,
                  "Cannot return multiple types: '%s' and '%s'\n",
                  node->symbol_type->name, ret_type->name);
      }
    }
  }

  if (node->symbol_type == NULL) {
    node->symbol_type = symbol_table_get(t->table, "void");
  }
}

void check_const_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check const decl\n");
  ast_decl_T *decl = (ast_decl_T *)node;
  size_t start_child_count = decl->child_count;
  ast_node_T **start_child_ptr = decl->children;

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

  if (strcmp(name->symbol_type->name, "undefined") != 0) {
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
    if (strcmp(type_annot->symbol_type->name, "undefined") == 0) {
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

  if (strcmp(value->symbol_type->name, "undefined") == 0) {
    log_error(name->loc, 1,
              "Cannot use undefined symbol as value for variable '%s'\n",
              v->t->value);
  }

  if (!value->symbol_type->type.is_comptime) {
    log_error(value->loc, 1,
              "Value for const declaration must be known at compile time\n");
  }

  if (type_annot != NULL) {
    if (strcmp(type_annot->symbol_type->name, value->symbol_type->name) != 0 &&
        !symbol_can_upgrade_type(value->symbol_type->type.type_cat,
                                 type_annot->symbol_type->type.type_cat)) {
      log_error(
          value->loc, 1,
          "Cannot assign value of type '%s', to variable '%s' of type '%s'\n",
          value->symbol_type->name, v->t->value, type_annot->symbol_type->name);
    }
  } else {
    node->symbol_type = value->symbol_type;
  }

  symbol_T var_symbol = symbol_new_var(
      v->t->value, v->t->loc, *node->symbol_type, 0, 0, 1, v->t->value);

  decl->children = start_child_ptr;
  decl->child_count = start_child_count;
  symbol_table_put(t->table, var_symbol);
}

void check_var_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check var decl\n");
  ast_decl_T *decl = (ast_decl_T *)node;
  unsigned char is_mut = decl->token->type == T_MUT;
  size_t start_child_count = decl->child_count;
  ast_node_T **start_child_ptr = decl->children;

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

  if (strcmp(name->symbol_type->name, "undefined") != 0) {
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
    if (strcmp(type_annot->symbol_type->name, "undefined") == 0) {
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

  if (strcmp(value->symbol_type->name, "undefined") == 0) {
    log_error(name->loc, 1,
              "Cannot use undefined symbol as value for variable '%s'\n",
              v->t->value);
  }

  if (type_annot != NULL) {
    if (strcmp(type_annot->symbol_type->name, value->symbol_type->name) != 0 &&
        !symbol_can_upgrade_type(value->symbol_type->type.type_cat,
                                 type_annot->symbol_type->type.type_cat)) {
      log_error(
          value->loc, 1,
          "Cannot assign value of type '%s', to variable '%s' of type '%s'\n",
          value->symbol_type->name, v->t->value, type_annot->symbol_type->name);
    }
  } else {
    node->symbol_type = value->symbol_type;
  }

  symbol_T var_symbol = symbol_new_var(v->t->value, v->t->loc,
                                       *node->symbol_type, is_mut, 0, 0, 0);

  decl->children = start_child_ptr;
  decl->child_count = start_child_count;
  symbol_table_put(t->table, var_symbol);
}

void check_attrib_list(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check attribute list\n");
  size_t alignment = 0;

  ast_attribute_list_T *list = (ast_attribute_list_T *)node;
  for (size_t i = 0; i < list->child_count; i++) {
    ast_node_T *param_node = list->children[i];

    if (param_node->type != AST_ATTRIBUTE) {
      log_error(param_node->loc, 1,
                "Unexpected node in attribute list '%s', expected attribute\n",
                ast_get_name(param_node->type));
    }

    ast_attribute_T *attrib = (ast_attribute_T *)param_node;

    check_type_annot(t, attrib->value);
    ast_value_T *annot = (ast_value_T *)attrib->value;

    if (strcmp(annot->base.symbol_type->name, "undefined") == 0) {
      log_error(attrib->value->loc, 1,
                "Undefined symbol '%s' used as type in struct attribute\n",
                annot->t->value);
    } else if (strcmp(annot->base.symbol_type->name, "void") == 0) {
      log_error(attrib->value->loc, 1,
                "Cannot use '%s' with type void, in struct attribute\n",
                annot->t->value);
    }

    if (annot->base.symbol_type->type.alignment > alignment) {
      alignment = annot->base.symbol_type->type.alignment;
    }

    symbol_table_put(t->table,
                     symbol_new_var(attrib->name->value, attrib->name->loc,
                                    *annot->base.symbol_type, 0, 0, 0, NULL));
  }
}

void check_struct_decl(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check struct declaration\n");
  node->symbol_type = malloc(sizeof(symbol_T));
  ast_decl_T *decl = (ast_decl_T *)node;
  size_t start_child_count = decl->child_count;
  ast_node_T **start_child_ptr = decl->children;

  if (decl->child_count == 0 || decl->children[0]->type != AST_VALUE) {
    log_error(decl->token->loc, 1,
              "Missing struct name in struct declaration\n");
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

  if (strcmp(name->symbol_type->name, "undefined") != 0) {
    log_error(name->loc, 1, "Redefinition of symbol '%s'\n", v->t->value);
  }

  symbol_table_T *parent_scope = t->table;
  symbol_table_T *struct_scope =
      symbol_table_new(v->t->value, parent_scope->level, parent_scope);

  t->table = struct_scope;

  ast_node_T *attrib_list_node = shift(&decl->children);
  if (attrib_list_node->type != AST_ATTRIBUTE_LIST) {
    log_error(attrib_list_node->loc, 1, "Unexpected node '%s', expected '%s'\n",
              ast_get_name(attrib_list_node->type),
              ast_get_name(AST_ATTRIBUTE_LIST));
  }
  ast_attribute_list_T *attrib_list = (ast_attribute_list_T *)attrib_list_node;

  check_attrib_list(t, attrib_list_node);
  decl->child_count--;

  t->table = parent_scope;

  symbol_T struct_symbol =
      symbol_new_type(v->t->value, v->t->loc, 0,
                      (attrib_list->alignment * attrib_list->child_count),
                      attrib_list->alignment, NULL, CUSTOM);
  struct_symbol.type.scope = struct_scope;

  symbol_table_put(t->table, struct_symbol);

  *node->symbol_type = struct_symbol;

  decl->children = start_child_ptr;
  decl->child_count = start_child_count;
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
  case T_STRUCT:
    check_struct_decl(t, node);
    break;

  default:
    log_error(decl->token->loc, 1,
              "Unexpected keyword in declaration, found: %s.\n",
              decl->token->value);
  }
}

void check_conditional(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check conditional\n");
  ast_cond_T *cond = (ast_cond_T *)node;

  check_expr(t, cond->lhs);
  check_expr(t, cond->rhs);

  if (strcmp(cond->lhs->symbol_type->name, cond->rhs->symbol_type->name) != 0 &&
      !symbol_can_upgrade_type(cond->lhs->symbol_type->type.type_cat,
                               cond->rhs->symbol_type->type.type_cat)) {
    ast_cond_op_T *op = (ast_cond_op_T *)cond->op;

    log_error(cond->op->loc, 1,
              "Cannot do boolean operation '%s' between types '%s' and '%s\n",
              op->t->value, cond->lhs->symbol_type->name,
              cond->rhs->symbol_type->name);
  }

  node->symbol_type = symbol_table_get(t->table, "bool");
}

void check_else(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check else\n");
  ast_else_T *elze = (ast_else_T *)node;
  if (elze->block->type == AST_BLOCK) {
    check_block(t, elze->block);
  } else {
    check_statement(t, elze->block);
  }

  node->symbol_type = elze->block->symbol_type;
}

void check_if(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check if\n");
  ast_if_T *if_node = (ast_if_T *)node;

  check_expr(t, if_node->cond);

  if (strcmp(if_node->cond->symbol_type->name, "bool") != 0) {
    log_error(if_node->cond->loc, 1,
              "If condition must be of type 'bool', found '%s'\n",
              if_node->cond->symbol_type->name);
  }

  check_block(t, if_node->block);
  node->symbol_type = if_node->block->symbol_type;

  if (if_node->elze != NULL) {
    check_else(t, if_node->elze);
    if (strcmp(node->symbol_type->name, if_node->elze->symbol_type->name) !=
        0) {
      log_error(node->loc, 1,
                "Cannot return multiple values from 'if'/'else if'/'else' "
                "block. Found '%s' and '%s\n",
                node->symbol_type->name, if_node->elze->symbol_type->name);
    }
  }
}

void check_return(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check return\n");
  ast_return_T *ret_node = (ast_return_T *)node;

  check_expr(t, ret_node->value);
  node->symbol_type = ret_node->value->symbol_type;
}

void check_assign(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check assign\n");
  ast_assign_T *assign = (ast_assign_T *)node;

  check_expr(t, assign->lhs);
  symbol_T *lhs_type = assign->lhs->symbol_type;
  if (strcmp(lhs_type->name, "undefined") == 0) {
    log_error(assign->lhs->loc, 1, "Cannot assign to undefined variable\n");
  }

  if (assign->lhs->type == AST_VALUE) {
    ast_value_T *v = (ast_value_T *)assign->lhs;
    if (v->t->type == T_IDENT) {
      symbol_T *lhs_symbol = symbol_table_get(t->table, v->t->value);
      if (lhs_symbol->tag == SYM_VAR && !lhs_symbol->var.is_mut) {
        log_error(lhs_type->loc, 1, "Cannot reassign immutable variable '%s'\n",
                  lhs_type->name);
      }
    }
  }

  check_expr(t, assign->value);
  symbol_T *rhs_type = assign->value->symbol_type;

  if (strcmp(rhs_type->name, "undefined") == 0) {
    log_error(assign->value->loc, 1, "Use of undefined symbol\n");
  } else if (!symbol_cmp(lhs_type, rhs_type)) {
    log_error(assign->ident->loc, 1,
              "Cannot assign value of type '%s' to symbol of type '%s'\n",
              rhs_type->name, lhs_type->name);
  }

  assign->lhs->symbol_type = symbol_upgrade_type(lhs_type, rhs_type);
  node->symbol_type = assign->lhs->symbol_type;
}

void check_while(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check while\n");
  ast_while_T *while_node = (ast_while_T *)node;

  check_expr(t, while_node->cond);
  if (strcmp(while_node->cond->symbol_type->name, "bool") != 0) {
    log_error(while_node->cond->loc, 1,
              "If condition must be of type 'bool', found '%s'\n",
              while_node->cond->symbol_type->name);
  }

  check_block(t, while_node->block);
  node->symbol_type = while_node->block->symbol_type;
}

void check_statement(type_check_t *t, ast_node_T *node) {
  log_debug(t->debug, "type check statement\n");
  switch (node->type) {
  case AST_SYSCALL:
    check_syscall(t, node);
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
  case AST_PROP:
    check_prop(t, node);
    break;
  case AST_FUNC_CALL:
    check_func_call(t, node);
    break;
  case AST_ATTRIBUTE_LIST:
  case AST_PROGRAM:
  case AST_BLOCK:
  case AST_EXPR:
  case AST_TYPE_ANNOT:
  case AST_VAR_DECL:
  case AST_CONST_DECL:
  case AST_FUNC_DECL:
  case AST_FUNC_PARAM_LIST:
  case AST_FUNC_PARAM:
  case AST_STRUCT_INIT:
  case AST_ATTRIBUTE:
  case AST_ELSE:
  case AST_CONDITIONAL:
  case AST_COND_OP:
  case AST_LOGICAL_OP:
  case AST_BIN_OP:
  case AST_OP:
  case AST_VALUE:
  case AST_ARRAY:
  case AST_ARRAY_ELEMENT:
  case AST_NO_OP:
    log_error(node->loc, 1, "Unexpected node in statement, found: %s.\n",
              ast_get_name(node->type));
    break;
  }

  log_debug(t->debug, "Statement type symbol after check '%s'\n",
            symbol_to_string(node->symbol_type));
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
}
