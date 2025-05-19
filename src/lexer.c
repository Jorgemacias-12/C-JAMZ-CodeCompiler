#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define INITIAL_CAPACITY 64

static JAMZToken make_token(JAMZTokenType type, const char *lexeme, int line, int column)
{
    JAMZToken token;
    token.type = type;
    token.lexeme = strdup(lexeme);
    token.line = line;
    token.column = column;
    return token;
}

static void add_token(JAMZTokenList *list, JAMZToken token)
{
    if (list->count >= list->capacity)
    {
        list->capacity *= 2;
        list->tokens = realloc(list->tokens, list->capacity * sizeof(JAMZToken));
    }
    list->tokens[list->count++] = token;
}

static bool is_operator_char(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '=';
}

static bool resolve_keyword(const char *lexeme, JAMZTokenType *out_type)
{
    if (strcmp(lexeme, "int") == 0)
    {
        *out_type = JAMZ_TOKEN_INT;
        return true;
    }
    if (strcmp(lexeme, "char") == 0) // <-- Añadido para soportar 'char' como tipo
    {
        *out_type = JAMZ_TOKEN_CHAR;
        return true;
    }
    if (strcmp(lexeme, "return") == 0)
    {
        *out_type = JAMZ_TOKEN_RETURN;
        return true;
    }
    if (strcmp(lexeme, "main") == 0)
    {
        *out_type = JAMZ_TOKEN_MAIN;
        return true;
    }
    return false;
}

JAMZTokenList *lexer_analyze(const char *source)
{
    JAMZTokenList *list = malloc(sizeof(JAMZTokenList));
    list->tokens = malloc(sizeof(JAMZToken) * INITIAL_CAPACITY);
    list->count = 0;
    list->capacity = INITIAL_CAPACITY;
    list->has_error = false;

    int line = 1;
    int col = 1;
    const char *start = source;
    const char *current = source;

    while (*current != '\0')
    {
        start = current;

        if (isspace(*current))
        {
            if (*current == '\n')
            {
                line++;
                col = 1;
            }
            else
            {
                col++;
            }
            current++;
            continue;
        }

        if (*current == '/' && *(current + 1) == '/')
        {
            // Comentario de una línea
            current += 2;
            while (*current != '\n' && *current != '\0')
            {
                current++;
            }
            continue;
        }

        if (*current == '/' && *(current + 1) == '*')
        {
            // Comentario de múltiples líneas
            current += 2;
            while (!(*current == '*' && *(current + 1) == '/') && *current != '\0')
            {
                if (*current == '\n')
                {
                    line++;
                    col = 1;
                }
                else
                {
                    col++;
                }
                current++;
            }
            if (*current == '*' && *(current + 1) == '/')
            {
                current += 2;
                col += 2;
            }
            continue;
        }

        if (isdigit(*current))
        {
            while (isdigit(*current))
                current++;
            int len = current - start;
            char *lexeme = strndup_impl(start, len);
            add_token(list, make_token(JAMZ_TOKEN_NUMBER, lexeme, line, col));
            free(lexeme);
            col += len;
            continue;
        }

        if (isalpha(*current) || *current == '_')
        {
            while (isalnum(*current) || *current == '_')
                current++;
            int len = current - start;
            char *lexeme = strndup_impl(start, len);

            JAMZTokenType type = JAMZ_TOKEN_IDENTIFIER;
            resolve_keyword(lexeme, &type);

            add_token(list, make_token(type, lexeme, line, col));
            free(lexeme);
            col += len;
            continue;
        }

        if (is_operator_char(*current))
        {
            char op[2] = {*current, '\0'};
            add_token(list, make_token(JAMZ_TOKEN_OPERATOR, op, line, col));
            current++;
            col++;
            continue;
        }

        switch (*current)
        {
        case ';':
            add_token(list, make_token(JAMZ_TOKEN_SEMICOLON, ";", line, col));
            current++;
            col++;
            continue;
        case '(':
            add_token(list, make_token(JAMZ_TOKEN_LPAREN, "(", line, col));
            current++;
            col++;
            continue;
        case ')':
            add_token(list, make_token(JAMZ_TOKEN_RPAREN, ")", line, col));
            current++;
            col++;
            continue;
        case '{':
            add_token(list, make_token(JAMZ_TOKEN_LBRACE, "{", line, col));
            current++;
            col++;
            continue;
        case '}':
            add_token(list, make_token(JAMZ_TOKEN_RBRACE, "}", line, col));
            current++;
            col++;
            continue;
        case '"':
        {
            current++;
            const char *string_start = current;
            int string_col = col + 1;
            while (*current != '"' && *current != '\0' && *current != '\n')
                current++;
            if (*current == '"')
            {
                int len = current - string_start;
                char *lexeme = strndup_impl(string_start, len);
                add_token(list, make_token(JAMZ_TOKEN_STRING, lexeme, line, string_col));
                free(lexeme);
                current++;
                col += len + 2;
            }
            else
            {
                push_error("Line %d, Column %d] Unterminated string literal\n", line, col);
                list->has_error = true;
                col++;
            }
            continue;
        }
        }

        char msg[64];
        snprintf(msg, sizeof(msg), "Unexpected character '%c'", *current);
        push_error("Line %d, Column %d] %s\n", line, col, msg);
        list->has_error = true;
        current++;
        col++;
    }

    add_token(list, make_token(JAMZ_TOKEN_EOF, "", line, col));
    return list;
}

void free_tokens(JAMZTokenList *list)
{
    if (!list)
        return;

    for (size_t i = 0; i < list->count; ++i)
    {
        if (list->tokens[i].lexeme != NULL)
        {
            log_debug("[LOG] Liberando lexema del token %zu: %s\n", i, list->tokens[i].lexeme);
            free(list->tokens[i].lexeme);
            list->tokens[i].lexeme = NULL; // Evitar doble liberación
        }
    }

    log_debug("[LOG] Liberando lista de tokens...\n");
    free(list->tokens);
    list->tokens = NULL; // Evitar doble liberación

    log_debug("[LOG] Liberando estructura de lista de tokens...\n");
    free(list);
}
