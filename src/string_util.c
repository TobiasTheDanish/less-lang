#include "include/string_util.h"
#include <stdio.h>
#include <string.h>

void decode_escaped_characters(char *str) {
	int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == '\\') {
            if (i + 1 < len) {

                switch (str[i + 1]) {
                    case 'n':
                        str[i] = '\n';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case 't':
                        str[i] = '\t';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case '\\':
                        str[i] = '\\';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case '\"':
                        str[i] = '\"';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case '\'':
                        str[i] = '\'';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case 'v':
                        str[i] = '\v';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case 'r':
                        str[i] = '\r';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case 'b':
                        str[i] = '\b';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case 'f':
                        str[i] = '\f';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    case '0':
                        str[i] = '\0';
                        memmove(&str[i + 1], &str[i + 2], len - i - 1);
                        len--;
                        break;
                    // Add more cases for other escape sequences as needed
                    default:
						printf("Encountered unexpected escaped character when decoding: '%c'", str[i+1]);
                        break;
                }
            }
        }

    }
}
