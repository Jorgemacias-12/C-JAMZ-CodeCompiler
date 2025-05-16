#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include "parser.h"

#define COLOR_RESET "\033[0m"
#define COLOR_BLUE "\033[94m"
#define COLOR_GREEN "\033[92m"
#define COLOR_YELLOW "\033[93m"
#define COLOR_MAGENTA "\033[95m"
#define COLOR_CYAN "\033[96m"
#define COLOR_BOLD "\033[1m"
#define COLOR_RED "\033[31m"

#ifdef _WIN32
#include <windows.h>
#endif

char *strndup_impl(const char *src, size_t length);
char *read_file(const char *filename);

Token *token_list_to_array(TokenList *list, int *out_count);

void print_error(const char *format, ...);
void print_ast(ASTNode *node, int indent);
void print_ast_ascii(ASTNode *node, const char *prefix, bool is_last);
void set_console_color(WORD color);
void reset_console_color();

#endif