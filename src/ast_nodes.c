#include "include/ast_nodes.h"
#include "include/logger.h"
#include "include/token.h"
#include <stdlib.h>

ast_node_T ast_new(ast_node_E type, location_T *loc) {
  ast_node_T base = {
      .type = type,
      .loc = loc,
      .symbol_type = NULL,
  };

  return base;
}

ast_node_T *ast_new_program(ast_node_T **expressions, size_t count) {
  ast_node_T base = ast_new(AST_PROGRAM, NULL);

  ast_program_T *program = malloc(sizeof(ast_program_T));
  program->base = base;
  program->expressions = expressions;
  program->count = count;

  return (ast_node_T *)program;
}

ast_node_T *ast_new_block(ast_node_T **expressions, size_t count) {
  ast_node_T base = ast_new(AST_BLOCK, NULL);

  ast_block_T *block = malloc(sizeof(ast_block_T));
  block->base = base;
  block->expressions = expressions;
  block->count = count;

  return (ast_node_T *)block;
}

ast_node_T *ast_new_syscall(ast_node_T **params, size_t count) {
  ast_node_T base = ast_new(AST_SYSCALL, NULL);

  ast_syscall_T *syscall = malloc(sizeof(ast_syscall_T));
  syscall->base = base;
  syscall->params = params;
  syscall->count = count;

  return (ast_node_T *)syscall;
}

ast_node_T *ast_new_func_decl(token_T *ident, token_T **params,
                              size_t param_count, ast_node_T *block) {
  ast_node_T base = ast_new(AST_FUNC_DECL, ident->loc);

  ast_func_decl_T *func_decl = malloc(sizeof(ast_func_decl_T));
  func_decl->base = base;
  func_decl->params = params;
  func_decl->param_count = param_count;
  func_decl->block = block;
  func_decl->ident = ident;

  return (ast_node_T *)func_decl;
}

ast_node_T *ast_new_func_call(token_T *ident, ast_node_T **params,
                              size_t param_count) {
  ast_node_T base = ast_new(AST_FUNC_CALL, ident->loc);

  ast_func_call_T *func_call = malloc(sizeof(ast_func_call_T));
  func_call->base = base;
  func_call->params = params;
  func_call->param_count = param_count;
  func_call->ident = ident;

  return (ast_node_T *)func_call;
}

ast_node_T *ast_new_struct_init(ast_node_T **attributes, size_t attr_count,
                                token_T *ident) {
  ast_node_T base = ast_new(AST_STRUCT_INIT, NULL);

  ast_struct_init_T *node = malloc(sizeof(ast_struct_init_T));
  node->base = base;
  node->attributes = attributes;
  node->attr_count = attr_count;
  node->ident = ident;

  return (ast_node_T *)node;
}

ast_node_T *ast_new_attribute_list(location_T *loc, ast_node_T **attribs,
                                   size_t count) {
  ast_attribute_list_T *node = malloc(sizeof(ast_attribute_list_T));
  node->base = ast_new(AST_ATTRIBUTE_LIST, loc);
  node->children = attribs;
  node->child_count = count;

  return (ast_node_T *)node;
}

ast_node_T *ast_new_attribute(token_T *name, ast_node_T *value) {
  ast_node_T base = ast_new(AST_ATTRIBUTE, name->loc);

  ast_attribute_T *node = malloc(sizeof(ast_attribute_T));
  node->base = base;
  node->name = name;
  node->value = value;

  return (ast_node_T *)node;
}

ast_node_T *ast_new_param_list(location_T *loc, ast_node_T **children,
                               size_t child_count) {
  ast_param_list_T *node = malloc(sizeof(ast_param_list_T));
  node->base = ast_new(AST_FUNC_PARAM_LIST, loc);
  node->children = children;
  node->child_count = child_count;

  return (ast_node_T *)node;
}

ast_node_T *ast_new_func_param(ast_node_T *ident, ast_node_T *type,
                               unsigned char is_mut) {
  ast_func_param_T *node = malloc(sizeof(ast_func_param_T));
  node->base = ast_new(AST_FUNC_PARAM, ident->loc);
  node->ident = ident;
  node->type_annot = type;
  node->is_mut = is_mut;

  return (ast_node_T *)node;
}

ast_node_T *ast_new_type_annot(token_T *token, ast_node_T *type,
                               unsigned char is_array) {
  ast_type_annot_T *annot = malloc(sizeof(ast_type_annot_T));
  annot->base = ast_new(AST_TYPE_ANNOT, token->loc);
  annot->token = token;
  annot->type = type;
  annot->is_array = is_array;

  return (ast_node_T *)annot;
}

ast_node_T *ast_new_decl(token_T *token, ast_node_T **children,
                         size_t child_count) {
  ast_node_T base = ast_new(AST_DECL, NULL);

  ast_decl_T *decl = malloc(sizeof(ast_decl_T));
  decl->base = base;
  decl->token = token;
  decl->children = children;
  decl->child_count = child_count;

  return (ast_node_T *)decl;
}

ast_node_T *ast_new_var_decl(ast_node_T *assign) {
  ast_node_T base = ast_new(AST_VAR_DECL, NULL);

  ast_var_decl_T *decl = malloc(sizeof(ast_var_decl_T));
  decl->base = base;
  decl->assign = assign;

  return (ast_node_T *)decl;
}

ast_node_T *ast_new_const_decl(token_T *ident, ast_node_T *value, char *type) {
  ast_node_T base = ast_new(AST_CONST_DECL, ident->loc);

  ast_const_decl_T *decl = malloc(sizeof(ast_const_decl_T));
  decl->base = base;
  decl->ident = ident;
  decl->value = value;
  decl->type = type;

  return (ast_node_T *)decl;
}

ast_node_T *ast_new_assign(token_T *ident, ast_node_T *lhs, ast_node_T *value) {
  ast_node_T base = ast_new(AST_ASSIGN, ident->loc);

  ast_assign_T *assign = malloc(sizeof(ast_assign_T));
  assign->base = base;
  assign->ident = ident;
  assign->lhs = lhs;
  assign->value = value;

  return (ast_node_T *)assign;
}

ast_node_T *ast_new_while(size_t index, ast_node_T *cond, ast_node_T *block) {
  ast_node_T base = ast_new(AST_WHILE, NULL);

  ast_while_T *w = malloc(sizeof(ast_while_T));
  w->base = base;
  w->index = index;
  w->cond = cond;
  w->block = block;

  return (ast_node_T *)w;
}

ast_node_T *ast_new_if(size_t index, ast_node_T *cond, ast_node_T *block,
                       ast_node_T *elze) {
  ast_node_T base = ast_new(AST_IF, NULL);

  ast_if_T *if_node = malloc(sizeof(ast_if_T));
  if_node->base = base;
  if_node->index = index;
  if_node->cond = cond;
  if_node->block = block;
  if_node->elze = elze;

  return (ast_node_T *)if_node;
}

ast_node_T *ast_new_else(size_t index, ast_node_T *block) {
  ast_node_T base = ast_new(AST_ELSE, NULL);

  ast_else_T *elze = malloc(sizeof(ast_else_T));
  elze->base = base;
  elze->index = index;
  elze->block = block;

  return (ast_node_T *)elze;
}

ast_node_T *ast_new_cond(ast_node_T *lhs, ast_node_T *op, ast_node_T *rhs) {
  ast_node_T base = ast_new(AST_CONDITIONAL, op->loc);

  ast_cond_T *cond = malloc(sizeof(ast_cond_T));
  cond->base = base;
  cond->lhs = lhs;
  cond->rhs = rhs;
  cond->op = op;

  return (ast_node_T *)cond;
}

ast_node_T *ast_new_cond_op(token_T *t) {
  ast_node_T base = ast_new(AST_COND_OP, t->loc);

  ast_cond_op_T *op = malloc(sizeof(ast_cond_op_T));
  op->base = base;
  op->t = t;
  return (ast_node_T *)op;
}

ast_node_T *ast_new_logical_op(token_T *t) {
  ast_node_T base = ast_new(AST_LOGICAL_OP, t->loc);

  ast_logical_op_T *op = malloc(sizeof(ast_logical_op_T));
  op->base = base;
  op->t = t;
  return (ast_node_T *)op;
}

ast_node_T *ast_new_bin_op(ast_node_T *lhs, ast_node_T *op, ast_node_T *rhs) {
  ast_node_T base = ast_new(AST_BIN_OP, NULL);

  ast_bin_op_T *bin_op = malloc(sizeof(ast_bin_op_T));
  bin_op->base = base;
  bin_op->lhs = lhs;
  bin_op->rhs = rhs;
  bin_op->op = op;
  bin_op->type_sym = NULL;

  return (ast_node_T *)bin_op;
}

ast_node_T *ast_new_dump(ast_node_T *value) {
  ast_node_T base = ast_new(AST_DUMP, NULL);

  ast_dump_T *dump = malloc(sizeof(ast_dump_T));
  dump->base = base;
  dump->value = value;

  return (ast_node_T *)dump;
}

ast_node_T *ast_new_op(token_T *t) {
  ast_node_T base = ast_new(AST_OP, t->loc);

  ast_op_T *op = malloc(sizeof(ast_op_T));
  op->base = base;
  op->t = t;
  return (ast_node_T *)op;
}

ast_node_T *ast_new_value(token_T *t) {
  ast_node_T base = ast_new(AST_VALUE, t->loc);

  ast_value_T *value = malloc(sizeof(ast_value_T));
  value->base = base;
  value->t = t;
  value->type_sym = NULL;
  return (ast_node_T *)value;
}

ast_node_T *ast_new_array(ast_node_T *ident, ast_node_T *len) {
  ast_node_T base = ast_new(AST_ARRAY, len->loc);

  ast_array_T *arr = malloc(sizeof(ast_array_T));
  arr->base = base;
  arr->ident = ident;
  arr->len = len;

  return (ast_node_T *)arr;
}

ast_node_T *ast_new_array_element(token_T *ident, ast_node_T *offset) {
  ast_node_T base = ast_new(AST_ARRAY_ELEMENT, ident->loc);

  ast_array_element_T *e = malloc(sizeof(ast_array_element_T));
  e->base = base;
  e->ident = ident;
  e->offset = offset;

  return (ast_node_T *)e;
}

ast_node_T *ast_new_prop(token_T *dot, ast_node_T *lhs, ast_node_T *rhs,
                         token_T *rhs_token) {
  ast_node_T base = ast_new(AST_PROP, dot->loc);

  ast_prop_T *p = malloc(sizeof(ast_prop_T));
  p->base = base;
  p->lhs = lhs;
  p->dot = dot;
  p->rhs = rhs;
  p->rhs_token = rhs_token;

  return (ast_node_T *)p;
}

ast_node_T *ast_new_return_stmt(token_T *token, ast_node_T *value) {
  log_debug(1, "token->loc: %p\n", token->loc);
  log_debug(1, "ast_new_return_stmt: token: %p, value: %p\n", token, value);
  ast_return_T *ret = malloc(sizeof(ast_return_T));
  ret->base = ast_new(AST_RETURN, token->loc);
  ret->token = token;
  ret->value = value;

  return (ast_node_T *)ret;
}

char *ast_get_name(ast_node_E type) {
  char *names[] = {"Program",
                   "Block",
                   "Expression",
                   "Syscall",
                   "Declaration",
                   "Type annotation",
                   "Variable declaration",
                   "Constant declaration",
                   "Function declaration",
                   "Function parameter list",
                   "Function parameter",
                   "Function call",
                   "Struct initialization",
                   "Struct property list",
                   "Struct property",
                   "Assign",
                   "While",
                   "If",
                   "Else",
                   "Conditional",
                   "Conditional operater",
                   "Logical operater",
                   "Binary operation",
                   "Operation",
                   "Value",
                   "Array",
                   "Array element",
                   "Property",
                   "Dump",
                   "Return",
                   "No operation"};

  return names[type];
}
