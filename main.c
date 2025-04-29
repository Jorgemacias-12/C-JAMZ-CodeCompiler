#include <stdio.h>
#include <stdlib.h>
#include "src/lexer.h"
#include "src/parser.h"
#include "src/semantic.h"
#include "src/utils.h"

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <source_file.c>\n", argv[0]);

        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    char *source_code = read_file(filename);

    if (!source_code)
    {
        fprintf(stderr, "Error reading file %s\n", filename);

        return EXIT_FAILURE;
    }

    TokenList *tokens = lexer_analyze(source_code);

    if (!tokens || tokens->has_error)
    {
        fprintf(stderr, "Lexical analisis failed.\n");

        free(source_code);

        return EXIT_FAILURE;
    }

    ASTNode *ast = parse(tokens);

    if (!ast)
    {
        fprintf(stderr, "Syntax analysis failed.\n");

        free_tokens(tokens);

        free(source_code);

        return EXIT_FAILURE;
    }

    if (!semantic_check(ast))
    {
        fprintf(stderr, "Semantic analysis failed\n");

        free_ast(ast);

        free_tokens(tokens);

        free(source_code);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}