#ifndef LIR_H
#define LIR_H
#include "ast_nodes.h"
#include "symbol_table.h"
typedef struct LESS_IR_STRUCT lir_node_T;

typedef struct LESS_IR_BUILDER_STRUCT {
  symbol_table_T *table;
} lir_builder_T;

typedef enum LESS_IR_TAG {
  LIR_PROGRAM,
  LIR_LABEL,
  LIR_PUSH,
  LIR_LOAD,
  LIR_STORE,
  LIR_CALL,
  LIR_IDENT,
  LIR_VALUE,
  LIR_ADD,
  LIR_SUB,
  LIR_MUL,
  LIR_DIV,
  LIR_MOD,
  LIR_DUMP,
  LIR_TAG_COUNT,
} lir_tag_E;

typedef struct LESS_IR_STRUCT {
  lir_tag_E tag;
  char *value;
  lir_node_T *children;
  size_t count;
} lir_node_T;

lir_builder_T lir_new_builder(symbol_table_T *symbol_table);
lir_node_T lir_build(lir_builder_T *b, ast_node_T *node);
char *lir_to_string(lir_node_T *n);
char *lir_tag_name(lir_node_T *n);

#endif // !LIR_H
