#include <stdio.h>
#include <stdlib.h>
#include "include/lexer.h"
#include "include/parser.h"
#include "include/semantic.h"
#include "include/utils.h"

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
        fprintf(stderr, RED_COLOR "Error reading file %s\n", filename);

        return EXIT_FAILURE;
    }

    TokenList *tokens = lexer_analyze(source_code);

    if (!tokens)
    {
        fprintf(stderr, "Lexer returned null.\n");
        free(source_code);
        return EXIT_FAILURE;
    }

    if (tokens->has_error)
    {
        print_error("Lexical analysis failed with the following errors\n\n");

        for (int i = 0; i < tokens->error_count; ++i)
        {
            LexerError err = tokens->errors[i];

            print_error("Line %d, Column %d] Unexpected character '%c': %s\n", err.line, err.column, err.character, err.message);
        }

        free_tokens(tokens);
        free(source_code);

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}