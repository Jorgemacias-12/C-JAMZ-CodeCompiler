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

void print_ast(ASTNode *node, int indent)
{
    if (!node)
        return;

    for (int i = 0; i < indent; i++)
        printf("  ");

    switch (node->type)
    {
    case AST_PROGRAM:
        printf(COLOR_BOLD COLOR_MAGENTA "Program\n" COLOR_RESET);
        for (int i = 0; i < node->program.count; i++)
            print_ast(node->program.statements[i], indent + 1);
        break;

    case AST_DECLARATION:
        printf(COLOR_BOLD COLOR_YELLOW "Declaration: " COLOR_RESET "%s %s\n",
               node->declaration.var_type, node->declaration.identifier);
        print_ast(node->declaration.value, indent + 1);
        break;

    case AST_NUMBER:
        printf(COLOR_BOLD COLOR_GREEN "Number: " COLOR_RESET "%d\n",
               node->number.value);
        break;

    case AST_IDENTIFIER:
        printf(COLOR_BOLD COLOR_CYAN "Identifier: " COLOR_RESET "%s\n",
               node->identifier.name);
        break;

    case AST_BINARY_OP:
        printf(COLOR_BOLD COLOR_BLUE "Binary Operation: " COLOR_RESET "%s\n",
               node->binary_op.op);
        print_ast(node->binary_op.left, indent + 1);
        print_ast(node->binary_op.right, indent + 1);
        break;

    default:
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("Unknown AST node type\n");
        break;
    }
}

void print_ast_ascii(ASTNode *node, const char *indent, bool is_last)
{
    if (!node)
        return;

    printf("%s", indent);
    printf(is_last ? "\\-- " : "|-- ");

    char new_indent[1024];
    snprintf(new_indent, sizeof(new_indent), "%s%s", indent, is_last ? "    " : "│   ");

    switch (node->type)
    {
    case AST_PROGRAM:
        set_console_color(13); // MAGENTA
        printf("Program\n");
        reset_console_color();

        for (int i = 0; i < node->program.count; i++)
        {
            print_ast_ascii(node->program.statements[i], new_indent, i == node->program.count - 1);
        }
        break;

    case AST_DECLARATION:
        set_console_color(9); // BLUE
        printf("Declaration: %s %s\n", node->declaration.var_type, node->declaration.identifier);
        reset_console_color();

        print_ast_ascii(node->declaration.value, new_indent, true);
        break;

    case AST_NUMBER:
        set_console_color(10); // GREEN
        printf("Number: %d\n", node->number.value);
        reset_console_color();
        break;

    case AST_IDENTIFIER:
        set_console_color(14); // YELLOW
        printf("Identifier: %s\n", node->identifier.name);
        reset_console_color();
        break;

    case AST_BINARY_OP:
        set_console_color(11); // CYAN
        printf("BinaryOp: %s\n", node->binary_op.op);
        reset_console_color();

        print_ast_ascii(node->binary_op.left, new_indent, false);
        print_ast_ascii(node->binary_op.right, new_indent, true);
        break;

    default:
        printf("Unknown Node\n");
        break;
    }
}

Token *token_list_to_array(TokenList *list, int *out_count)
{
    int count = 0;
    for (TokenNode *node = list->head; node != NULL; node = node->next)
    {
        count++;
    }

    Token *array = malloc(sizeof(Token) * count);
    int i = 0;
    for (TokenNode *node = list->head; node != NULL; node = node->next)
    {
        array[i++] = node->token;
    }

    if (out_count)
        *out_count = count;

    return array;
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
