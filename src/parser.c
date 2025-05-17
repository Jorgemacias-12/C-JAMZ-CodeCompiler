#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

static inline JAMZToken current_token(JAMZParser *parser)
{
    return parser->tokens->tokens[parser->current];
}

static inline JAMZToken advance(JAMZParser *parser)
{
    if (parser->current < parser->tokens->count)
        parser->current++;
    return parser->tokens->tokens[parser->current - 1];
}

static inline bool at_end(JAMZParser *parser)
{
    return parser->current >= parser->tokens->count ||
           current_token(parser).type == JAMZ_TOKEN_EOF;
}

static inline bool check(JAMZParser *parser, JAMZTokenType type)
{
    return !at_end(parser) && current_token(parser).type == type;
}

static inline bool match(JAMZParser *parser, JAMZTokenType type)
{
    if (check(parser, type))
    {
        advance(parser);
        return true;
    }
    return false;
}

// Forward declarations
static JAMZASTNode *parse_declaration(JAMZParser *parser);
static JAMZASTNode *parse_block(JAMZParser *parser);
static JAMZASTNode *parse_program_node(JAMZParser *parser);

JAMZASTNode *parser_parse(JAMZTokenList *tokens)
{
    JAMZParser *parser = malloc(sizeof(JAMZParser));
    if (!parser)
    {
        push_error("Out of memory creating parser.");
        return NULL;
    }
    parser->tokens = tokens;
    parser->current = 0;
    parser->had_error = false;

    JAMZASTNode *program = parse_program_node(parser);
    free(parser);
    return program;
}

static JAMZASTNode *parse_program_node(JAMZParser *parser)
{
    if (!match(parser, JAMZ_TOKEN_INT))
    {
        push_error("Expected 'int' at start of program (main declaration).\n");
        return NULL;
    }

    if (!match(parser, JAMZ_TOKEN_MAIN))
    {
        push_error("Expected 'main' after 'int'.\n");
        return NULL;
    }

    if (!match(parser, JAMZ_TOKEN_LPAREN))
    {
        push_error("Expected '(' after 'main'.\n");
        return NULL;
    }

    if (!match(parser, JAMZ_TOKEN_RPAREN))
    {
        push_error("Expected ')' after 'main('.\n");
        return NULL;
    }

    JAMZASTNode *main_block = parse_block(parser);
    if (!main_block)
    {
        push_error("Expected '{...}' block after 'main()'.\n");
        return NULL;
    }

    JAMZASTNode *program = malloc(sizeof(JAMZASTNode));
    if (!program)
    {
        push_error("Out of memory creating program node.");
        free_ast(main_block);
        return NULL;
    }

    program->type = JAMZ_AST_PROGRAM;
    program->line = 1; // Línea y columna fija o podrías obtener del primer token
    program->column = 1;
    program->block.count = 1;
    program->block.statements = malloc(sizeof(JAMZASTNode *));
    if (!program->block.statements)
    {
        push_error("Out of memory creating program statements array.");
        free_ast(main_block);
        free(program);
        return NULL;
    }
    program->block.statements[0] = main_block;
    return program;
}

static JAMZASTNode *parse_block(JAMZParser *parser)
{
    if (!match(parser, JAMZ_TOKEN_LBRACE))
    {
        push_error("Expected '{' to start block.");
        return NULL;
    }

    size_t capacity = 8;
    size_t count = 0;

    JAMZASTNode **stmts = malloc(capacity * sizeof(JAMZASTNode *));
    if (!stmts)
    {
        push_error("Out of memory parsing block.");
        return NULL;
    }
    while (!check(parser, JAMZ_TOKEN_RBRACE) && !at_end(parser))
    {
        JAMZASTNode *decl = parse_declaration(parser);
        if (!decl)
            break;
        if (count >= capacity)
        {
            capacity *= 2;
            JAMZASTNode **new = realloc(stmts, capacity * sizeof(JAMZASTNode *));
            if (!new)
            {
                push_error("Out of memory expanding block.");
                for (size_t i = 0; i < count; i++)
                    free_ast(stmts[i]);
                free(stmts);
                return NULL;
            }
            stmts = new;
        }
        stmts[count++] = decl;
    }
    if (!match(parser, JAMZ_TOKEN_RBRACE))
    {
        push_error("Expected '}' to close block.");
        for (size_t i = 0; i < count; i++)
            free_ast(stmts[i]);
        free(stmts);
        return NULL;
    }
    JAMZASTNode *node = malloc(sizeof(JAMZASTNode));
    if (!node)
    {
        push_error("Out of memory creating block node.");
        for (size_t i = 0; i < count; i++)
            free_ast(stmts[i]);
        free(stmts);
        return NULL;
    }
    node->type = JAMZ_AST_BLOCK;
    node->line = current_token(parser).line;
    node->column = current_token(parser).column;
    node->block.statements = stmts;
    node->block.count = count;
    return node;
}

static JAMZASTNode *parse_declaration(JAMZParser *parser)
{
    if (match(parser, JAMZ_TOKEN_RETURN))
    {
        JAMZToken return_token = parser->tokens->tokens[parser->current - 1];
        // Por ahora no parseamos expresión, solo NULL
        JAMZASTNode *node = malloc(sizeof(JAMZASTNode));
        if (!node)
        {
            push_error("Out of memory creating return node.");
            return NULL;
        }
        node->type = JAMZ_AST_RETURN;
        node->line = return_token.line;
        node->column = return_token.column;
        node->return_stmt.value = NULL;
        return node;
    }

    JAMZToken tok = advance(parser);
    JAMZASTNode *lit = malloc(sizeof(JAMZASTNode));
    if (!lit)
    {
        push_error("Out of memory creating literal node.");
        return NULL;
    }
    lit->type = JAMZ_AST_LITERAL;
    lit->line = tok.line;
    lit->column = tok.column;
    lit->literal.value = tok;
    return lit;
}