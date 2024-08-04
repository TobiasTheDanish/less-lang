#ifndef SYMBOL_H
#define SYMBOL_H

#include "token.h"
#include <stddef.h>

typedef struct SYMBOL_TABLE_STRUCT symbol_table_T;

typedef enum TYPE_CAT_ENUM {
  VOID = 0,
  BOOL,
  I8,
  I16,
  I32,
  I64,
  ARRAY,
  STRING,
  CUSTOM,
  TYPE_CAT_COUNT,
} type_cat_E;

unsigned char symbol_can_upgrade_type(type_cat_E a, type_cat_E b);

typedef enum SYMBOL_ENUM {
  SYM_VAR,
  SYM_FUNC,
  SYM_VAR_TYPE,
} symbol_E;

typedef struct SYMBOL_BASE_STRUCT symbol_T;

typedef struct SYMBOL_VAR_STRUCT {
  symbol_T *type;
  size_t index;
  unsigned char is_mut;
  unsigned char is_assigned;
  unsigned char is_param;
  unsigned char is_const;
  char *const_val;
} symbol_var_T;

typedef struct SYMBOL_VAR_TYPE_STRUCT {
  type_cat_E type_cat;
  symbol_T *underlying_type;
  symbol_table_T *scope;
  char *operand;
  unsigned char is_comptime;
  unsigned char is_primitive;
  size_t size;
  size_t alignment;
  char *regs[8];
} symbol_type_T;

typedef struct SYMBOL_FUNC_STRUCT {
  symbol_table_T *scope;
  symbol_T *ret_type;
} symbol_func_T;

typedef struct SYMBOL_BASE_STRUCT {
  symbol_E tag;
  char *name;
  location_T *loc;
  union {
    symbol_var_T var;
    symbol_type_T type;
    symbol_func_T func;
  };
} symbol_T;

symbol_T *symbol_upgrade_type(symbol_T *a, symbol_T *b);

symbol_T *symbol_new(char *name, symbol_E type, location_T *loc);

symbol_T symbol_new_type(char *name, location_T *loc,
                         unsigned char is_primitive, size_t size,
                         size_t alignment, symbol_T *underlying,
                         type_cat_E type_cat);

symbol_T symbol_new_var(char *name, location_T *loc, symbol_T type,
                        unsigned char is_mut, unsigned char is_param,
                        unsigned char is_const, char *const_val);

symbol_T symbol_new_func(char *name, symbol_table_T *scope, location_T *loc);

bool symbol_cmp(symbol_T *a, symbol_T *b);

void func_add_param(symbol_func_T *func, symbol_T *param);

char *symbol_to_string(symbol_T *symbol);

char *symbol_get_type_string(symbol_E type);

#endif // !SYMBOL_H
