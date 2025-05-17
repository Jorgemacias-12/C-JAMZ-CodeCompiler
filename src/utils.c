#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "utils.h"
#include "lexer.h"

static char error_stack[MAX_ERRORS][MAX_ERROR_LEN];
static size_t error_count;

#ifdef _WIN32
#include <windows.h>
#endif

void init_error_stack(void)
{
    error_count = 0;

    for (size_t i = 0; i < MAX_ERRORS; i++)
        error_stack[i][0] = '\0';
}

void check_for_errors(void)
{
    if (get_error_count() == 0)
        return;

    print_error_stack();
    clear_error_stack();
}

void push_error(const char *format, ...)
{
    if (error_count >= MAX_ERRORS)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vsnprintf(error_stack[error_count], MAX_ERROR_LEN, format, args);
    va_end(args);

    error_count++;
}

void print_error_stack(void)
{
    print_error("The JAMZ compiler encountered the following error you must check:\n\n");

    for (int i = 0; i < error_count; i++)
    {
        print_error("Compiler error %zu: %s\n", i + 1, error_stack[i]);
    }

    print_error("Compiler finished with errors! quantity: %d\n", get_error_count());
}

void clear_error_stack(void)
{
    error_count = 0;
}

size_t get_error_count(void)
{
    return error_count;
}

char *strndup_impl(const char *src, size_t length)
{
    char *result = malloc(length + 1);
    if (!result)
        return NULL;

    memcpy(result, src, length);
    result[length] = '\0';
    return result;
}

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (!file)
    {
        push_error("Failed to open file: %s\n", filename);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        push_error("Failed to seek to end of file: %s\n", filename);
        return NULL;
    }

    long length = ftell(file);

    if (length <= 0)
    {
        push_error("The source code file %s has no readable size.\n", filename);
        fclose(file);
        return NULL;
    }

    rewind(file);

    char *content = malloc(length + 1);

    if (!content)
    {
        fclose(file);
        push_error("Memory allocation failed for reading file: %s\n");
        return NULL;
    }

    size_t read = fread(content, 1, length, file);
    content[read] = '\0';

    fclose(file);
    return content;
}

void print_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_ERROR_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;

    GetConsoleScreenBufferInfo(console, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    SetConsoleTextAttribute(console, FOREGROUND_RED | FOREGROUND_INTENSITY);
    vfprintf(stderr, format, args);
    SetConsoleTextAttribute(console, saved_attributes);
#else
    fprintf(stderr, "\x1b[31m");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\x1b[0m");
#endif

    va_end(args);
}

void set_console_color(WORD color)
{
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(console, color);
}

void reset_console_color()
{
    set_console_color(7);
}

const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return "";
    return dot + 1;
}

const char *jamz_token_type_to_string(JAMZTokenType type)
{
    switch (type)
    {
    case JAMZ_TOKEN_INT:
        return "INT";
    case JAMZ_TOKEN_RETURN:
        return "RETURN";
    case JAMZ_TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case JAMZ_TOKEN_NUMBER:
        return "NUMBER";
    case JAMZ_TOKEN_OPERATOR:
        return "OPERATOR";
    case JAMZ_TOKEN_SEMICOLON:
        return "SEMICOLON";
    case JAMZ_TOKEN_LPAREN:
        return "LPAREN";
    case JAMZ_TOKEN_RPAREN:
        return "RPAREN";
    case JAMZ_TOKEN_LBRACE:
        return "LBRACE";
    case JAMZ_TOKEN_RBRACE:
        return "RBRACE";
    case JAMZ_TOKEN_STRING:
        return "STRING";
    case JAMZ_TOKEN_EOF:
        return "EOF";
    case JAMZ_TOKEN_UNKNOWN:
        return "UNKNOWN";
    default:
        return "UNDEFINED";
    }
}

void print_tokens(const JAMZTokenList *list)
{
    for (size_t i = 0; i < list->count; ++i)
    {
        JAMZToken token = list->tokens[i];
        printf("[%-3zu] %-12s '%s'  (line %d, col %d)\n",
               i,
               jamz_token_type_to_string(token.type),
               token.lexeme,
               token.line,
               token.column);
    }
}