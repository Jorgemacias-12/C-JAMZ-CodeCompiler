#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include "lexer.h"
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

// Error stack configuration
#define MAX_ERRORS 50
#define MAX_ERROR_LEN 512

void init_error_stack(void);
void check_for_errors(void);
void push_error(const char *format, ...);
void print_error_stack(void);
void clear_error_stack(void);
size_t get_error_count(void);

char *strndup_impl(const char *src, size_t length);
char *read_file(const char *filename);

void print_error(const char *format, ...);
void set_console_color(WORD color);
void reset_console_color();

const char *get_filename_ext(const char *filename);

// Lexer utils
const char *jamz_token_type_to_string(JAMZTokenType type);
void print_tokens(const JAMZTokenList *list);

// Parser utils
void print_ast(const JAMZASTNode *node, int indent);

#endif