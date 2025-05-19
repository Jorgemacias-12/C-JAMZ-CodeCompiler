#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
static JAMZASTNode *parse_expression(JAMZParser *parser);
static JAMZASTNode *parse_assignment(JAMZParser *parser);
static JAMZASTNode *parse_primary(JAMZParser *parser);
static JAMZASTNode *parse_binary_expression(JAMZParser *parser, int min_prec);

// Tabla de precedencia simple
static int get_precedence(JAMZTokenType type)
{
    switch (type)
    {
    case JAMZ_TOKEN_OPERATOR:
        // Solo + y * por ahora
        // Puedes mejorar esto para más operadores
        return 1;
    default:
        return 0;
    }
}

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
    if ((check(parser, JAMZ_TOKEN_INT)) ||
        (check(parser, JAMZ_TOKEN_CHAR)) ||
        (check(parser, JAMZ_TOKEN_IDENTIFIER) && (strcmp(current_token(parser).lexeme, "int") == 0 ||
                                                  strcmp(current_token(parser).lexeme, "char") == 0)))
    {
        JAMZToken type_token = advance(parser);
        char *type_name = strdup(type_token.lexeme);
        // Soporte para punteros: si hay '*', concatenar al tipo
        if (check(parser, JAMZ_TOKEN_OPERATOR) && strcmp(current_token(parser).lexeme, "*") == 0)
        {
            advance(parser); // Ignora el '*'
            char *ptr_type = malloc(strlen(type_name) + 2);
            sprintf(ptr_type, "%s*", type_name);
            free(type_name);
            type_name = ptr_type;
        }
        if (!check(parser, JAMZ_TOKEN_IDENTIFIER))
        {
            push_error("Expected identifier after type in declaration.");
            free(type_name);
            return NULL;
        }
        JAMZToken name_token = advance(parser);
        JAMZASTNode *initializer = NULL;
        if (check(parser, JAMZ_TOKEN_OPERATOR) && strcmp(current_token(parser).lexeme, "=") == 0)
        {
            advance(parser);
            initializer = parse_expression(parser);
        }
        if (!match(parser, JAMZ_TOKEN_SEMICOLON))
        {
            push_error("Expected ';' after declaration.");
            if (initializer)
                free_ast(initializer);
            free(type_name);
            return NULL;
        }
        JAMZASTNode *decl = calloc(1, sizeof(JAMZASTNode));
        decl->type = JAMZ_AST_DECLARATION;
        decl->line = type_token.line;
        decl->column = type_token.column;
        decl->declaration.type_name = type_name;
        decl->declaration.var_name = strdup(name_token.lexeme);
        decl->declaration.initializer = initializer;
        return decl;
    }
    // Asignación: nombre = expr ;
    if (check(parser, JAMZ_TOKEN_IDENTIFIER))
    {
        JAMZToken name_token = advance(parser);
        if (check(parser, JAMZ_TOKEN_OPERATOR) && strcmp(current_token(parser).lexeme, "=") == 0)
        {
            advance(parser);
            JAMZASTNode *value = parse_expression(parser);
            if (!match(parser, JAMZ_TOKEN_SEMICOLON))
            {
                push_error("Expected ';' after assignment.");
                if (value)
                    free_ast(value);
                return NULL;
            }
            JAMZASTNode *assign = calloc(1, sizeof(JAMZASTNode));
            assign->type = JAMZ_AST_ASSIGNMENT;
            assign->line = name_token.line;
            assign->column = name_token.column;
            assign->assignment.var_name = strdup(name_token.lexeme);
            assign->assignment.value = value;
            return assign;
        }
        else
        {
            push_error("Expected '=' after identifier for assignment.");
            return NULL;
        }
    }
    // Return
    if (match(parser, JAMZ_TOKEN_RETURN))
    {
        JAMZToken return_token = parser->tokens->tokens[parser->current - 1];
        JAMZASTNode *value = NULL;
        if (!check(parser, JAMZ_TOKEN_SEMICOLON))
        {
            value = parse_expression(parser);
        }
        if (!match(parser, JAMZ_TOKEN_SEMICOLON))
        {
            push_error("Expected ';' after return statement.");
            if (value)
                free_ast(value);
            return NULL;
        }
        JAMZASTNode *node = malloc(sizeof(JAMZASTNode));
        node->type = JAMZ_AST_RETURN;
        node->line = return_token.line;
        node->column = return_token.column;
        node->return_stmt.value = value;
        return node;
    }
    // Si nada coincide, no crear nodo Unknown, solo error
    push_error("Unknown or invalid statement/declaration.");
    return NULL;
}

static JAMZASTNode *parse_expression(JAMZParser *parser)
{
    return parse_assignment(parser);
}

static JAMZASTNode *parse_assignment(JAMZParser *parser)
{
    JAMZASTNode *left = parse_binary_expression(parser, 0);
    if (left && check(parser, JAMZ_TOKEN_OPERATOR) && strcmp(current_token(parser).lexeme, "=") == 0)
    {
        advance(parser);
        JAMZASTNode *value = parse_assignment(parser);
        JAMZASTNode *assign = malloc(sizeof(JAMZASTNode));
        assign->type = JAMZ_AST_ASSIGNMENT;
        assign->assignment.var_name = strdup(left->variable.var_name);
        assign->assignment.value = value;
        assign->line = left->line;
        assign->column = left->column;
        free_ast(left);
        return assign;
    }
    return left;
}

static JAMZASTNode *parse_binary_expression(JAMZParser *parser, int min_prec)
{
    JAMZASTNode *left = parse_primary(parser);
    while (check(parser, JAMZ_TOKEN_OPERATOR))
    {
        JAMZToken op_token = current_token(parser);
        int prec = get_precedence(op_token.type);
        if (prec < min_prec)
            break;
        advance(parser);
        JAMZASTNode *right = parse_primary(parser);
        JAMZASTNode *bin = malloc(sizeof(JAMZASTNode));
        bin->type = JAMZ_AST_BINARY;
        bin->binary.left = left;
        bin->binary.op = strdup(op_token.lexeme);
        bin->binary.right = right;
        bin->line = op_token.line;
        bin->column = op_token.column;
        left = bin;
    }
    return left;
}

static JAMZASTNode *parse_primary(JAMZParser *parser)
{
    if (check(parser, JAMZ_TOKEN_IDENTIFIER))
    {
        JAMZToken tok = advance(parser);
        JAMZASTNode *var = malloc(sizeof(JAMZASTNode));
        var->type = JAMZ_AST_VARIABLE;
        var->variable.var_name = strdup(tok.lexeme);
        var->line = tok.line;
        var->column = tok.column;
        return var;
    }
    if (check(parser, JAMZ_TOKEN_NUMBER) || check(parser, JAMZ_TOKEN_STRING) || check(parser, JAMZ_TOKEN_CHAR))
    {
        JAMZToken tok = advance(parser);
        JAMZASTNode *lit = malloc(sizeof(JAMZASTNode));
        lit->type = JAMZ_AST_LITERAL;
        lit->literal.value = strdup(tok.lexeme);
        lit->literal.token_type = tok.type; // Guardar tipo de token original
        lit->line = tok.line;
        lit->column = tok.column;
        return lit;
    }
    push_error("Unexpected token in expression.");
    return NULL;
}