#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "utils.h"
#include "lexer.h"
#include "cJSON.h"
#include <stdarg.h>
#include <time.h>

static char error_stack[MAX_ERRORS][MAX_ERROR_LEN];
static size_t error_count;
static FILE *log_file = NULL;

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

    for (size_t i = 0; i < error_count; i++)
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
    char *result = safe_malloc(length + 1);
    if (!result)
        return NULL;

    memcpy(result, src, length);
    result[length] = '\0';
    return result;
}

void *safe_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        fprintf(stderr, "[ERROR] Memory allocation failed for size %zu\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_realloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr)
    {
        fprintf(stderr, "[ERROR] Memory reallocation failed for size %zu\n", size);
        free(ptr); // Liberar memoria previa para evitar fugas
        exit(EXIT_FAILURE);
    }
    return new_ptr;
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

    char *content = safe_malloc(length + 1);

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
    case JAMZ_TOKEN_CHAR:
        return "CHAR";
    case JAMZ_TOKEN_EOF:
        return "EOF";
    case JAMZ_TOKEN_UNKNOWN:
        return "UNKNOWN";
    case JAMZ_TOKEN_MAIN:
        return "MAIN FUNCTION";
    default:
        return "UNDEFINED";
    }
}

void print_tokens(const JAMZTokenList *list)
{
    for (size_t i = 0; i < list->count; ++i)
    {
        JAMZToken token = list->tokens[i];
        Color color;
        switch (token.type)
        {
        case JAMZ_TOKEN_INT:
        case JAMZ_TOKEN_FLOAT:
        case JAMZ_TOKEN_CHAR:
            color = JAMZ_COLOR_BLUE;
            break;
        case JAMZ_TOKEN_IDENTIFIER:
            color = JAMZ_COLOR_GREEN;
            break;
        case JAMZ_TOKEN_OPERATOR:
            color = JAMZ_COLOR_YELLOW;
            break;
        case JAMZ_TOKEN_STRING:
            color = JAMZ_COLOR_MAGENTA;
            break;
        case JAMZ_TOKEN_NUMBER:
            color = JAMZ_COLOR_CYAN;
            break;
        default:
            color = JAMZ_COLOR_RED;
            break;
        }
        print_color("[", JAMZ_COLOR_WHITE, false);
        printf("%-3zu", i);
        print_color("]", JAMZ_COLOR_WHITE, false);
        print_color(" ", JAMZ_COLOR_DEFAULT, false);
        print_color(jamz_token_type_to_string(token.type), color, false);
        print_color(" ", JAMZ_COLOR_DEFAULT, false);
        printf("'%s'  (line %d, col %d)\n",
               token.lexeme,
               token.line,
               token.column);
    }
}

void print_ast_node(const JAMZASTNode *node, int indent)
{
    if (!node)
        return;

    for (int i = 0; i < indent; i++)
        print_color("|   ", JAMZ_COLOR_DEFAULT, false);

    switch (node->type)
    {
    case JAMZ_AST_PROGRAM:
        print_color("`-- Program", JAMZ_COLOR_CYAN, false);
        printf(" (line: %d, col: %d)\n", node->line, node->column);
        for (int i = 0; i < node->block.count; i++)
            print_ast_node(node->block.statements[i], indent + 1);
        break;
    case JAMZ_AST_BLOCK:
        print_color("`-- Block", JAMZ_COLOR_MAGENTA, false);
        printf(" (line: %d, col: %d)\n", node->line, node->column);
        for (int i = 0; i < node->block.count; i++)
            print_ast_node(node->block.statements[i], indent + 1);
        break;
    case JAMZ_AST_DECLARATION:
        print_color("`-- Declaration", JAMZ_COLOR_YELLOW, false);
        printf(": %s of type %s (line: %d, col: %d)\n",
               node->declaration.var_name, node->declaration.type_name, node->line, node->column);
        if (node->declaration.initializer)
            print_ast_node(node->declaration.initializer, indent + 1);
        break;
    case JAMZ_AST_RETURN:
        print_color("`-- Return", JAMZ_COLOR_GREEN, false);
        printf(" (line: %d, col: %d)\n", node->line, node->column);
        if (node->return_stmt.value)
            print_ast_node(node->return_stmt.value, indent + 1);
        break;
    case JAMZ_AST_LITERAL:
        print_color("`-- Literal", JAMZ_COLOR_BLUE, false);
        printf(": %s (line: %d, col: %d)\n", node->literal.value, node->line, node->column);
        break;
    case JAMZ_AST_VARIABLE:
        print_color("`-- Variable", JAMZ_COLOR_RED, false);
        printf(": %s (line: %d, col: %d)\n", node->variable.var_name, node->line, node->column);
        break;
    case JAMZ_AST_BINARY:
        print_color("`-- Binary Operation", JAMZ_COLOR_CYAN, false);
        printf(": %s (line: %d, col: %d)\n", node->binary.op, node->line, node->column);
        print_ast_node(node->binary.left, indent + 1);
        print_ast_node(node->binary.right, indent + 1);
        break;
    default:
        print_color("`-- Unknown node type", JAMZ_COLOR_DEFAULT, false);
        printf(" (line: %d, col: %d)\n", node->line, node->column);
        break;
    }
}

void print_ast(const JAMZASTNode *root, int indent)
{
    print_ast_node(root, 0);
}

void free_ast(JAMZASTNode *node)
{
    if (!node)
        return;
    switch (node->type)
    {
    case JAMZ_AST_BLOCK:
        for (size_t i = 0; i < node->block.count; ++i)
        {
            free_ast(node->block.statements[i]);
        }
        free(node->block.statements);
        break;

    case JAMZ_AST_RETURN:
        free_ast(node->return_stmt.value);
        break;

    case JAMZ_AST_IF:
        free_ast(node->if_stmt.condition);
        free_ast(node->if_stmt.then_branch);
        if (node->if_stmt.else_branch)
            free_ast(node->if_stmt.else_branch);
        break;

    case JAMZ_AST_BINARY:
        free_ast(node->binary.left);
        free_ast(node->binary.right);
        break;

    case JAMZ_AST_DECLARATION:
        if (node->declaration.type_name)
            free(node->declaration.type_name);
        if (node->declaration.var_name)
            free(node->declaration.var_name);
        if (node->declaration.initializer)
            free_ast(node->declaration.initializer);
        break;

    case JAMZ_AST_ASSIGNMENT:
        if (node->assignment.var_name)
            free(node->assignment.var_name);
        if (node->assignment.value)
            free_ast(node->assignment.value);
        break;

    case JAMZ_AST_LITERAL:
    case JAMZ_AST_VARIABLE:
        // Nada que liberar manualmente
        break;

    case JAMZ_AST_EXPRESSION:
        // Si decides tener un nodo JAMZ_AST_EXPRESSION que contenga otra expresión, libera su hijo aquí
        break;

    case JAMZ_AST_PROGRAM:
        for (size_t i = 0; i < node->block.count; ++i)
        {
            free_ast(node->block.statements[i]);
        }
        free(node->block.statements);
        break;
    }

    free(node);
}

Keyword *load_keywords(const char *path, int *out_count)
{
    FILE *file = fopen(path, "rb");

    if (!file)
    {
        push_error("[Error] When opening keywords table file\n");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = safe_malloc(length + 1);

    if (fread(content, 1, length, file) != length)
    {
        push_error("[ERROR] Failed to read complete content from %s\n", path);
        fclose(file);
        free(content);
        return NULL;
    }

    content[length] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(content);
    free(content);

    if (!json)
    {
        push_error("[Error] When parsing the keywords table JSON %s\n", cJSON_GetErrorPtr());
        return NULL;
    }

    int size = cJSON_GetArraySize(json);
    Keyword *keywords = safe_malloc(size * sizeof(Keyword));

    for (int i = 0; i < size; i++)
    {
        cJSON *element = cJSON_GetArrayItem(json, i);

        const char *name = cJSON_GetObjectItem(element, "name")->valuestring;
        const char *type = cJSON_GetObjectItem(element, "type")->valuestring;
        const char *category = cJSON_GetObjectItem(element, "category")->valuestring;

        keywords[i].name = strdup(name);
        keywords[i].type = strdup(type);
        keywords[i].category = strdup(category);
    }

    cJSON_Delete(json);

    *out_count = size;
    return keywords;
}

void print_color(const char *text, Color color, bool newline)
{
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;

    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;

    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);
    if (newline)
        printf("\n");

    SetConsoleTextAttribute(hConsole, saved_attributes);
#else
    if (color != JAMZ_COLOR_DEFAULT)
        printf("\033[%dm", color);
    printf("%s", text);
    if (newline)
        printf("\n");
    if (color != JAMZ_COLOR_DEFAULT)
        printf("\033[0m");
#endif
}

void log_debug(const char *format, ...)
{
    if (!log_file)
    {
        log_file = fopen("program.log", "a");
        if (!log_file)
        {
            perror("Error opening log file");
            return;
        }
    }

    // Obtener la fecha y hora actual
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] ",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fflush(log_file);
}
