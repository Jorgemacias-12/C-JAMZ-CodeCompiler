#include <stdio.h>
#include <stdlib.h>
#include "include/lexer.h"
#include "include/parser.h"
#include "include/semantic.h"
#include "include/utils.h"
#include <locale.h>

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    init_error_stack();

    int keyword_count = 0;
    Keyword *keywords = NULL;

    char *source_code = NULL;
    JAMZTokenList *tokens = NULL;
    int exit_code = EXIT_SUCCESS;

    print_color("\nJAMZ C Compiler v0.0.1\n", JAMZ_COLOR_CYAN, true);

    if (argc != 2)
    {
        push_error("Usage: %s <source_file.c>\n", argv[0]);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    const char *extension = get_filename_ext(argv[1]);

    if (strcmp(extension, "c") != 0)
    {
        push_error("The file type you provided is not a valid C language type\nFilname: %s\n", argv[1]);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    const char *filename = argv[1];

    source_code = read_file(filename);

    if (!source_code)
    {
        push_error("Error reading file %s\n", filename);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    tokens = lexer_analyze(source_code);

    if (!tokens)
    {
        push_error("Lexer analysis returned null.\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    if (tokens->has_error)
    {
        push_error("Lexical analysis failed with the following errors:\n\n");

        for (size_t i = 0; i < tokens->error_count; ++i)
        {
            JAMZLexerError err = tokens->errors[i];
            push_error("Line %d, Column %d] Unexpected character '%c': %s\n",
                       err.line, err.column, err.character, err.message);
        }

        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    print_color("\nLexer analysis has ecountered the following tokens:\n\n", JAMZ_COLOR_YELLOW, true);

    print_tokens(tokens);

    print_color("\nThe parser has the following AST:\n\n", JAMZ_COLOR_MAGENTA, true);

    JAMZASTNode *ast = parser_parse(tokens);

    if (!ast)
    {
        push_error("[ERROR] The parser result was null.\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    print_ast(ast, 0);

    print_color("\n\nThe semantic analysis: \n\n", JAMZ_COLOR_YELLOW, true);

    keywords = load_keywords("data/keywords.json", &keyword_count);

    if (!keywords)
    {
        push_error("[ERROR] Getting keywords for the semantic analysis.\n");
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    printf("\nKeywords loaded globally.\n\n");

    for (int i = 0; i < keyword_count; i++)
    {
        printf("Name: %s, Type: %s, Category: %s\n",
               keywords[i].name, keywords[i].type, keywords[i].category);
    }

    analyze_semantics(ast, keywords, keyword_count);

    if (get_error_count() == 0)
    {
        printf("\n[OK] Semantic analysis completed without errors\n");
    }

cleanup:
    if (tokens != NULL)
    {
        log_debug("[LOG] Liberando memoria de tokens...\n");

        for (size_t i = 0; i < tokens->count; i++)
        {
            log_debug("[LOG] Liberando lexema del token %d: %s\n", i, tokens->tokens[i].lexeme);
            free(tokens->tokens[i].lexeme);
        }

        log_debug("[LOG] Memoria de tokens liberada.\n");
        free_tokens(tokens);
    }

    if (source_code != NULL)
    {
        log_debug("[LOG] Memoria de source_code liberada.\n");
        free(source_code);
    }

    if (ast != NULL)
        free_ast(ast);

    if (get_error_count() > 0)
    {
        print_error_stack();
        log_debug("[LOG] Pila de errores limpiada.\n");
        clear_error_stack();
    }

    if (keywords != NULL && keyword_count > 0)
    {
        for (int i = 0; i < keyword_count; i++)
        {
            free(keywords[i].name);
            free(keywords[i].type);
            free(keywords[i].category);
        }
        free(keywords);
    }

    return exit_code;
}