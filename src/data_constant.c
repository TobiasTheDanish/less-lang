#include "include/data_constant.h"
#include <stdlib.h>

data_const_T* data_const_new(token_T* t, char* type) {
	data_const_T* d = malloc(sizeof(data_const_T));
	d->t = t;
	d->type = type;

	return d;
}
