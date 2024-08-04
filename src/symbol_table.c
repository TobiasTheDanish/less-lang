#include "include/symbol_table.h"
#include "include/logger.h"
#include "include/symbol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

symbol_table_T *symbol_table_new(char *name, size_t level,
                                 symbol_table_T *parent) {
  symbol_table_T *t = malloc(sizeof(symbol_table_T));
  t->name = name;
  t->level = level;
  t->count = 0;
  t->symbols = malloc(sizeof(symbol_T));
  t->parent = parent;
  t->children = malloc(sizeof(symbol_table_T *));
  t->child_count = 0;

  return t;
}

void symbol_table_init_builtins(symbol_table_T *table) {
  log_info("Initializing builtin symbols\n");
  symbol_table_put(table,
                   symbol_new_type("undefined", NULL, 1, 0, 0, NULL, VOID));
  symbol_table_put(table, symbol_new_type("void", NULL, 1, 0, 0, NULL, VOID));
  symbol_table_put(table, symbol_new_type("bool", NULL, 1, 1, 1, NULL, BOOL));
  symbol_table_put(table, symbol_new_type("i8", NULL, 1, 1, 1, NULL, I8));
  symbol_table_put(table, symbol_new_type("i16", NULL, 1, 2, 2, NULL, I16));
  symbol_table_put(table, symbol_new_type("i32", NULL, 1, 4, 4, NULL, I32));
  symbol_table_put(table, symbol_new_type("i64", NULL, 1, 8, 8, NULL, I64));
  symbol_T array_type = symbol_new_type("array", NULL, 0, 8, 8, NULL, ARRAY);
  symbol_table_put(
      array_type.type.scope,
      symbol_new_var("len", NULL, *symbol_table_get(table, "i32"), 0, 0, 0, 0));
  symbol_table_put(table, array_type);
  symbol_T str_type = symbol_new_type("string", NULL, 0, 8, 8, NULL, STRING);
  symbol_table_put(
      str_type.type.scope,
      symbol_new_var("len", NULL, *symbol_table_get(table, "i32"), 0, 0, 0, 0));
  symbol_table_put(table, str_type);

  for (size_t i = 0; i < table->count; i++) {
    table->symbols[i].type.scope->parent = table;
    table->symbols[i].type.scope->level = table->level + 1;
  }
}

symbol_T *symbol_table_get(symbol_table_T *table, char *name) {
  for (size_t i = 0; i < table->count; i++) {
    symbol_T current = table->symbols[i];
    if (strcmp(current.name, name) == 0) {
      return &table->symbols[i];
    }
  }

  if (table->parent != NULL) {
    return symbol_table_get(table->parent, name);
  }

  return NULL;
}

symbol_T *symbol_table_get_params(symbol_table_T *table, size_t *count) {
  symbol_T *res = malloc(table->count * sizeof(symbol_T));
  size_t param_count = 0;
  for (size_t i = 0; i < table->count; i++) {
    symbol_T current = table->symbols[i];
    if (current.tag == SYM_VAR && current.var.is_param) {
      res[param_count++] = table->symbols[i];
    }
  }
  *count = param_count;

  return res;
}

size_t symbol_table_calc_index(symbol_table_T *table) {
  if (table->parent != NULL) {
    return table->count + symbol_table_calc_index(table->parent);
  }

  return table->count;
}

void symbol_table_put(symbol_table_T *table, symbol_T symbol) {
  if (!symbol_table_contains(table, symbol.name)) {
    if (symbol.tag == SYM_VAR) {
      symbol.var.index = symbol_table_calc_index(table);
    }
    table->symbols[table->count++] = symbol;
    table->symbols =
        realloc(table->symbols, (table->count + 1) * sizeof(*table->symbols));
  }
}

void symbol_table_update(symbol_table_T *table, symbol_T symbol) {
  for (size_t i = 0; i < table->count; i++) {
    symbol_T current = table->symbols[i];
    if (strcmp(current.name, symbol.name) == 0) {
      table->symbols[i] = symbol;
      return;
    }
  }

  if (table->parent != NULL) {
    return symbol_table_update(table->parent, symbol);
  }

  symbol_table_put(table, symbol);
}

bool symbol_table_contains(symbol_table_T *table, char *name) {
  for (size_t i = 0; i < table->count; i++) {
    symbol_T current = table->symbols[i];
    if (strcmp(current.name, name) == 0) {
      return true;
    }
  }

  if (table->parent != NULL) {
    return symbol_table_contains(table->parent, name);
  }

  return false;
}

symbol_table_T *symbol_table_get_child(symbol_table_T *table, char *name) {
  for (size_t i = 0; i < table->child_count; i++) {
    symbol_table_T *current = table->children[i];
    if (strcmp(current->name, name) == 0) {
      return current;
    }
  }

  return NULL;
}

void symbol_table_put_child(symbol_table_T *table, symbol_table_T *child) {
  table->children[table->child_count++] = child;
  table->children = realloc(table->children, (table->child_count + 1) *
                                                 sizeof(symbol_table_T *));
}

void symbol_table_print(symbol_table_T *table) {
  printf("\nScope '%s', level %zu\n", table->name, table->level);
  if (table->parent != NULL) {
    printf("Parent: '%s'\n", table->parent->name);
  } else {
    printf("Enclosing scope: None\n");
  }
  for (size_t i = 0; i < table->count; i++) {
    printf("\tSymbol #%lu: %s\n", (i + 1),
           symbol_to_string(&table->symbols[i]));
  }
}
