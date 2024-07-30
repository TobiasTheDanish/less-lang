#ifndef TYPE_CHECK_H
#define TYPE_CHECK_H

#include "ast_nodes.h"
#include "data_table.h"
#include "symbol_table.h"
typedef struct TYPE_CHECK_STRUCT {
  ast_node_T *program;
  symbol_table_T *table;
  data_table_T *data;
  unsigned char debug;
} type_check_t;

type_check_t *type_check_new(ast_node_T *program, symbol_table_T *table,
                             data_table_T *data, unsigned char debug);

ast_node_T *type_check(type_check_t *t);

#endif // !TYPE_CHECK_H
