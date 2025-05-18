#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "utils.h"
#include "lexer.h"
#include "cJSON.h"

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
        printf("[%-3zu] %-12s '%s'  (line %d, col %d)\n",
               i,
               jamz_token_type_to_string(token.type),
               token.lexeme,
               token.line,
               token.column);
    }
}

static void print_indent_ascii(int indent, bool is_last)
{
    for (int i = 0; i < indent - 1; ++i)
    {
        printf("|   ");
    }
    if (indent > 0)
    {
        printf("%s-- ", is_last ? "`" : "|");
    }
}

static void print_node_info(const JAMZASTNode *node, const char *label)
{
    printf("%s (line: %d, col: %d)\n", label, node->line, node->column);
}

static void print_ast_internal(const JAMZASTNode *node, int indent, bool is_last)
{
    if (!node)
        return;

    print_indent_ascii(indent, is_last);

    switch (node->type)
    {
    case JAMZ_AST_PROGRAM:
        print_node_info(node, "Program");
        for (size_t i = 0; i < node->block.count; ++i)
        {
            print_ast_internal(node->block.statements[i], indent + 1, i == node->block.count - 1);
        }
        break;

    case JAMZ_AST_BLOCK:
        print_node_info(node, "Block");
        for (size_t i = 0; i < node->block.count; ++i)
        {
            print_ast_internal(node->block.statements[i], indent + 1, i == node->block.count - 1);
        }
        break;

    case JAMZ_AST_RETURN:
        print_node_info(node, "Return");
        print_ast_internal(node->return_stmt.value, indent + 1, true);
        break;

    case JAMZ_AST_IF:
        print_node_info(node, "If");

        print_indent_ascii(indent + 1, false);
        printf("Condition\n");
        print_ast_internal(node->if_stmt.condition, indent + 2, true);

        print_indent_ascii(indent + 1, node->if_stmt.else_branch == NULL);
        printf("Then\n");
        print_ast_internal(node->if_stmt.then_branch, indent + 2, node->if_stmt.else_branch == NULL);

        if (node->if_stmt.else_branch)
        {
            print_indent_ascii(indent + 1, true);
            printf("Else\n");
            print_ast_internal(node->if_stmt.else_branch, indent + 2, true);
        }
        break;

    case JAMZ_AST_BINARY:
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Binary '%s'", node->binary.op ? node->binary.op : "");
        print_node_info(node, buffer);
        print_ast_internal(node->binary.left, indent + 1, false);
        print_ast_internal(node->binary.right, indent + 1, true);
        break;
    }

    case JAMZ_AST_LITERAL:
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Literal '%s'", node->literal.value ? node->literal.value : "");
        print_node_info(node, buffer);
        break;
    }

    case JAMZ_AST_VARIABLE:
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Variable '%s'", node->variable.var_name ? node->variable.var_name : "");
        print_node_info(node, buffer);
        break;
    }

    default:
        print_node_info(node, "Unknown");
        break;
    }
}

void print_ast(const JAMZASTNode *node, int indent)
{
    print_ast_internal(node, indent, true);
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

    char *content = malloc(length + 1);

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
    Keyword *keywords = malloc(size * sizeof(Keyword));

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