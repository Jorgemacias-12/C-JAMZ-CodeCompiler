#ifndef UTILS_H
#define UTILS_H

#define RED_COLOR "\x1b[31m"
#define COLOR_RESET "\x1b[0m"

#include <stddef.h>

char *strndup_impl(const char *src, size_t length);
char *read_file(const char *filename);

void print_error(const char *format, ...);

#endif