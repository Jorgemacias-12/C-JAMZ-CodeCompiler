#include "lexer.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static const int INITIAL_ERROR_CAPACITY = 4;

static void add_lexer_error(TokenList *list, int line, int column, char character, const char *message)
{
    if (list->error_count >= list->error_capacity)
    {
        int new_capacity = (list->error_capacity == 0) ? INITIAL_ERROR_CAPACITY : list->error_capacity * 2;

        LexerError *resized = realloc(list->errors, sizeof(LexerError) * new_capacity);
        if (!resized)
            return;

        list->errors = resized;
        list->error_capacity = new_capacity;
    }

    LexerError err = {
        .line = line,
        .column = column,
        .character = character,
        .message = strdup(message)};

    list->errors[list->error_count++] = err;
    list->has_error = true;
}

static TokenNode *create_token(TokenType type, const char *lexeme, size_t length)
{
    TokenNode *node = malloc(sizeof(TokenNode));
    if (!node)
        return NULL;

    node->token.type = type;
    node->token.lexeme = strndup_impl(lexeme, length);
    node->next = NULL;

    return node;
}

static void append_token(TokenList *list, TokenNode *node)
{
    if (!list->head)
    {
        list->head = list->tail = node;
    }
    else
    {
        list->tail->next = node;
        list->tail = node;
    }
}

TokenList *lexer_analyze(const char *source_code)
{
    TokenList *tokens = malloc(sizeof(TokenList));

    tokens->head = tokens->tail = NULL;
    tokens->error_count = 0;
    tokens->error_capacity = INITIAL_ERROR_CAPACITY;
    tokens->errors = malloc(tokens->error_capacity * sizeof(LexerError));
    tokens->has_error = false;

    const char *curr = source_code;
    int line = 1;
    int column = 1;

    while (*curr)
    {
        if (isspace(*curr))
        {
            if (*curr == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
            ++curr;
            continue;
        }

        if (*curr == '/' && *(curr + 1) == '/')
        {
            while (*curr && *curr != '\n')
            {
                ++curr;
                column++;
            }
            continue;
        }

        if (*curr == '/' && *(curr + 1) == '*')
        {
            curr += 2;
            column += 2;
            while (*curr && (*curr != '*' || *(curr + 1) != '/'))
            {
                ++curr;
                column++;
            }
            curr += 2;
            column += 2;
            continue;
        }

        if (isalpha(*curr) || *curr == '_')
        {
            const char *start = curr;
            while (isalnum(*curr) || *curr == '_')
            {
                ++curr;
                column++;
            }
            TokenNode *node = create_token(TOKEN_IDENTIFIER, start, curr - start);
            append_token(tokens, node);
            continue;
        }

        if (*curr == '-' && isdigit(*(curr + 1)))
        {
            const char *start = curr;
            ++curr;
            column++;
            while (isdigit(*curr))
            {
                ++curr;
                column++;
            }
            if (*curr == '.')
            {
                ++curr;
                column++;
                while (isdigit(*curr))
                {
                    ++curr;
                    column++;
                }
            }
            TokenNode *node = create_token(TOKEN_NUMBER, start, curr - start);
            append_token(tokens, node);
            continue;
        }

        if (isdigit(*curr))
        {
            const char *start = curr;
            while (isdigit(*curr))
            {
                ++curr;
                column++;
            }
            if (*curr == '.')
            {
                ++curr;
                column++;
                while (isdigit(*curr))
                {
                    ++curr;
                    column++;
                }
            }
            TokenNode *node = create_token(TOKEN_NUMBER, start, curr - start);
            append_token(tokens, node);
            continue;
        }

        if (*curr == '(')
        {
            TokenNode *node = create_token(TOKEN_LPAREN, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == ')')
        {
            TokenNode *node = create_token(TOKEN_RPAREN, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == '{')
        {
            TokenNode *node = create_token(TOKEN_LBRACE, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == '}')
        {
            TokenNode *node = create_token(TOKEN_RBRACE, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }

        if (*curr == ';')
        {
            TokenNode *node = create_token(TOKEN_SEMICOLON, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }

        if (*curr == '"')
        {
            const char *start = curr;
            ++curr;
            column++;
            while (*curr && *curr != '"')
            {
                ++curr;
                column++;
            }
            if (*curr == '"')
            {
                ++curr;
                column++;
                TokenNode *node = create_token(TOKEN_STRING, start, curr - start);
                append_token(tokens, node);
            }
            else
            {
                add_lexer_error(tokens, line, column, *curr, "Unterminated string literal");
            }
            continue;
        }

        if (strncmp(curr, "int", 3) == 0 && !isalnum(curr[3]))
        {
            TokenNode *node = create_token(TOKEN_INT, curr, 3);
            append_token(tokens, node);
            curr += 3;
            column += 3;
            continue;
        }
        if (strncmp(curr, "return", 6) == 0 && !isalnum(curr[6]))
        {
            TokenNode *node = create_token(TOKEN_RETURN, curr, 6);
            append_token(tokens, node);
            curr += 6;
            column += 6;
            continue;
        }

        if (*curr == '+')
        {
            TokenNode *node = create_token(TOKEN_OPERATOR, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == '-')
        {
            TokenNode *node = create_token(TOKEN_OPERATOR, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == '*')
        {
            TokenNode *node = create_token(TOKEN_OPERATOR, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }
        if (*curr == '/')
        {
            TokenNode *node = create_token(TOKEN_OPERATOR, curr, 1);
            append_token(tokens, node);
            ++curr;
            column++;
            continue;
        }

        if (*curr == '=' && *(curr + 1) == '=')
        {
            TokenNode *node = create_token(TOKEN_OPERATOR, curr, 2);
            append_token(tokens, node);
            curr += 2;
            column += 2;
            continue;
        }

        add_lexer_error(tokens, line, column, *curr, "Invalid character");
        ++curr;
        column++;
    }

    return tokens;
}

void free_tokens(TokenList *tokens)
{
    TokenNode *node = tokens->head;

    while (node)
    {
        TokenNode *next = node->next;

        free(node->token.lexeme);
        free(node);

        node = next;
    }

    for (int i = 0; i < tokens->error_count; ++i)
    {
        free(tokens->errors[i].message);
    }

    free(tokens->errors);
    free(tokens);
}