#ifndef LOGGER_H
#define LOGGER_H

#include "stdarg.h"
#include "token.h"

void log_info(const char* format, ...);
void log_warning(const char* format, ...);
void log_todo(const char* format, ...);
void log_error(location_T* loc, int exitcode, const char* format, ...);

#endif // !LOGGER_H
