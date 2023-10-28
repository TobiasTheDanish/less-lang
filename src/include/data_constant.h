#ifndef DATA_CONSTANT_H
#define DATA_CONSTANT_H

#include "token.h"
typedef struct DATA_CONSTANT_STRUCT {
	token_T* t;
	char* type;
} data_const_T;

data_const_T* data_const_new(token_T* t, char* type);

#endif // !DATA_CONSTANT_H
