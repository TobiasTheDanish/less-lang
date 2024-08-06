#ifndef COMPILER_H
#define COMPILER_H

#include "data_table.h"
#include "file_util.h"
#include "lir.h"
#include "symbol_table.h"
typedef struct COMPILER_STRUCT {
  lir_node_T *program;
  symbol_table_T *s_table;
  data_table_T *data_table;
  file_T *file;
  size_t stack_pointer;
  size_t mem_pointer;
  unsigned char debug;
} compiler_T;

compiler_T *compiler_new(lir_node_T *program, symbol_table_T *s_table,
                         data_table_T *data_table, char *output_file,
                         unsigned char debug);

void compile(compiler_T *c);

#endif // !COMPILER_H
