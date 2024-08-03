#include "include/symbol.h"
#include "include/logger.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char upgrade_table[TYPE_CAT_COUNT][TYPE_CAT_COUNT] = {
  //          VOID BOOL I8 I16 I32 I64 ARRAY STRING CUSTOM
  /*VOID  */  {1,   0,  0,  0,  0,  0,  0,  0,  0,},
  /*BOOL  */  {0,   1,  0,  0,  0,  0,  0,  0,  0,},
  /*I8    */  {0,   0,  1,  1,  1,  1,  0,  0,  0,},
  /*i16   */  {0,   0,  0,  1,  1,  1,  0,  0,  0,}, 
  /*i32   */  {0,   0,  0,  0,  1,  1,  0,  0,  0,}, 
  /*i64   */  {0,   0,  0,  0,  0,  1,  0,  0,  0,}, 
  /*ARRAY */  {0,   0,  0,  0,  0,  0,  1,  0,  0,}, 
  /*STRING*/  {0,   0,  0,  0,  0,  0,  0,  1,  0,}, 
  /*CUSTOM*/  {0,   0,  0,  0,  0,  0,  0,  0,  1,}, 
};

unsigned char symbol_can_upgrade_type(type_cat_E a, type_cat_E b) {
  return upgrade_table[a][b] || upgrade_table[b][a];
}

symbol_type_T *symbol_upgrade_type(symbol_type_T *a, symbol_type_T *b) {
  if (a->type_cat > b->type_cat) {
    return a;
  } else {
    return b;
  }
}

symbol_T *symbol_new(char *name, symbol_E type, location_T *loc) {
  symbol_T *s = malloc(sizeof(symbol_T));

  s->name = name;
  s->tag = type;
  s->loc = loc;

  return s;
}

symbol_T symbol_new_type(char *name, location_T *loc,
                          unsigned char is_primitive, size_t size, size_t alignment,
                          symbol_T *underlying, type_cat_E type_cat) {
  symbol_type_T s = {
    .type_cat = type_cat,
    .underlying_type = underlying,
    .is_primitive = is_primitive,
    .size = size,
    .alignment = alignment,
  };

  if (!is_primitive) {
    s.operand = "QWORD";
    s.regs[0] = "rdi";
    s.regs[1] = "rsi";
    s.regs[2] = "rdx";
    s.regs[3] = "rcx";
    s.regs[4] = "r8";
    s.regs[5] = "r9";
    s.regs[6] = "rax";
    s.regs[7] = "r10";
  } else {
    switch (size) {
    case 1:
      s.operand = "BYTE";
      s.regs[0] = "dil";
      s.regs[1] = "sil";
      s.regs[2] = "dl";
      s.regs[3] = "cl";
      s.regs[4] = "r8b";
      s.regs[5] = "r9b";
      s.regs[6] = "al";
      s.regs[7] = "r10b";
      break;
    case 2:
      s.operand = "WORD";
      s.regs[0] = "di";
      s.regs[1] = "si";
      s.regs[2] = "dx";
      s.regs[3] = "cx";
      s.regs[4] = "r8w";
      s.regs[5] = "r9w";
      s.regs[6] = "ax";
      s.regs[7] = "r10w";
      break;
    case 4:
      s.operand = "DWORD";
      s.regs[0] = "edi";
      s.regs[1] = "esi";
      s.regs[2] = "edx";
      s.regs[3] = "ecx";
      s.regs[4] = "r8d";
      s.regs[5] = "r9d";
      s.regs[6] = "eax";
      s.regs[7] = "r10d";
      break;
    default:
      s.operand = "QWORD";
      s.regs[0] = "rdi";
      s.regs[1] = "rsi";
      s.regs[2] = "rdx";
      s.regs[3] = "rcx";
      s.regs[4] = "r8";
      s.regs[5] = "r9";
      s.regs[6] = "rax";
      s.regs[7] = "r10";
      break;
    }
  }

  return (symbol_T) {
    .name = name,
    .loc = loc,
    .tag = SYM_VAR_TYPE,
    .type = s,
  };
}

symbol_T symbol_new_var(char *name, location_T *loc, symbol_T *type,
                         unsigned char is_mut, unsigned char is_param,
                         unsigned char is_const, char *const_val) {
  symbol_var_T var = {
    .type = type,
    .index = -1,
    .is_mut = is_mut,
    .is_assigned = 0,
    .is_param = is_param,
    .is_const = is_const,
    .const_val = const_val,
  };

  return (symbol_T){
    .name = name,
    .loc = loc,
    .tag = SYM_VAR,
    .var = var,
  };
}

symbol_T symbol_new_func(char *name, symbol_table_T *scope, location_T *loc) {
  symbol_func_T func = {
  .scope = scope,
  .ret_type = NULL,
};

  return (symbol_T){
    .name = name,
    .loc = loc,
    .tag = SYM_FUNC,
    .func = func,
  };
}

bool symbol_cmp(symbol_T *a, symbol_T *b) {
  if (a->tag != b->tag) {
    return false;
  }

  return strcmp(a->name, a->name) == 0;
}


char *symbol_to_string(symbol_T *symbol) {
  char *s = calloc(100, sizeof(char));

  switch (symbol->tag) {
  case SYM_VAR_TYPE: {
    sprintf(s, "<type:%s", symbol->name);
      if (symbol->type.underlying_type != NULL) {
        sprintf(s, " of %s", symbol_to_string(symbol->type.underlying_type));
      }
      sprintf(s, ">");
  } break;

  case SYM_VAR: {
    sprintf(s, "<var:%s, type:%s>", symbol->name, symbol_to_string(symbol->var.type));
    // sprintf(s, "<%s:%s:%zu>", var->base.name, var->type->name var->index);
  } break;

  case SYM_FUNC: {
    sprintf(s, "<func: %s, returns: %s>", symbol->name, symbol_to_string(symbol->func.ret_type));
  } break;
  }
  return s;
}

char *symbol_get_type_string(symbol_E type) {
  char *names[] = {
      "Variable",
      "Function",
      "Type",
  };

  return names[type];
}
