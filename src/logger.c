#include "include/logger.h"
#include "include/token.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void log_debug(unsigned char debug, const char* format, ...) {
	if (!debug) return;
	printf("[DEBUG]: ");

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
            format++; // Move past '%'
            if (*format == '\0') break; // Format error

            if (*format == 'd' || *format == 'c') {
                int num = va_arg(args, int);
                printf("%d", num);
            } else if (*format == 's') {
                char *str = va_arg(args, char *);
                printf("%s", str);
            } else if (*format == 'f') {
                double num = va_arg(args, double);
                printf("%f", num);
            } else if (*format == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                printf("%u", num);
            } else {
                putchar('%'); // Print the '%' character
                putchar(*format); // Print the character immediately following '%'
            }

        } else {
            putchar(*format);
        }
        format++;
	}

	va_end(args);
}

void log_info(const char* format, ...) {
	printf("[INFO]: ");

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
            format++; // Move past '%'
            if (*format == '\0') break; // Format error

            if (*format == 'd' || *format == 'c') {
                int num = va_arg(args, int);
                printf("%d", num);
            } else if (*format == 's') {
                char *str = va_arg(args, char *);
                printf("%s", str);
            } else if (*format == 'f') {
                double num = va_arg(args, double);
                printf("%f", num);
            } else if (*format == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                printf("%u", num);
            } else {
                putchar('%'); // Print the '%' character
                putchar(*format); // Print the character immediately following '%'
            }

        } else {
            putchar(*format);
        }
        format++;
	}

	va_end(args);
}

void log_warning(const char* format, ...) {
	printf("[WARN]: ");

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
            format++; // Move past '%'
            if (*format == '\0') break; // Format error

            if (*format == 'd' || *format == 'c') {
                int num = va_arg(args, int);
                printf("%d", num);
            } else if (*format == 's') {
                char *str = va_arg(args, char *);
                printf("%s", str);
            } else if (*format == 'f') {
                double num = va_arg(args, double);
                printf("%f", num);
            } else if (*format == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                printf("%u", num);
            } else {
                putchar('%'); // Print the '%' character
                putchar(*format); // Print the character immediately following '%'
            }

        } else {
            putchar(*format);
        }
        format++;
	}

	va_end(args);
}

void log_todo(const char* format, ...) {
	printf("[TODO]: ");

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
            format++; // Move past '%'
            if (*format == '\0') break; // Format error

            if (*format == 'd' || *format == 'c') {
                int num = va_arg(args, int);
                printf("%d", num);
            } else if (*format == 's') {
                char *str = va_arg(args, char *);
                printf("%s", str);
            } else if (*format == 'f') {
                double num = va_arg(args, double);
                printf("%f", num);
            } else if (*format == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                printf("%u", num);
            } else {
                putchar('%'); // Print the '%' character
                putchar(*format); // Print the character immediately following '%'
            }

        } else {
            putchar(*format);
        }
        format++;
	}

	va_end(args);

	exit(1);
}

void log_error(location_T* loc, int exitcode, const char* format, ...) {
	if (loc != NULL) {
		printf("%s:%lu:%lu ", loc->filePath, loc->row, loc->col);
	}
	printf("[ERROR]: ");

	va_list args;
	va_start(args, format);

	while (*format) {
		if (*format == '%') {
            format++; // Move past '%'
            if (*format == '\0') break; // Format error

            if (*format == 'd' || *format == 'c') {
                int num = va_arg(args, int);
                printf("%d", num);
            } else if (*format == 's') {
                char *str = va_arg(args, char *);
                printf("%s", str);
            } else if (*format == 'f') {
                double num = va_arg(args, double);
                printf("%f", num);
            } else if (*format == 'u') {
                unsigned int num = va_arg(args, unsigned int);
                printf("%u", num);
            } else {
                putchar('%'); // Print the '%' character
                putchar(*format); // Print the character immediately following '%'
            }

        } else {
            putchar(*format);
        }
        format++;
	}

	va_end(args);

	if (exitcode != -1) {
		exit(exitcode);
	}
}


