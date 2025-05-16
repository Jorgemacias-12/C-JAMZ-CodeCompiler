#include <stdio.h>
#include <stdlib.h>
#include "include/lexer.h"
#include "include/parser.h"
#include "include/semantic.h"
#include "include/utils.h"

int main(int argc, char *argv[])
{
    init_error_stack();

    char *source_code = NULL;
    TokenList *tokens = NULL;
    int exit_code = EXIT_SUCCESS;

    if (argc != 2)
    {
        push_error("Usage: %s <source_file.c>\n", argv[0]);
        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

    const char *extension = get_filename_ext(argv[1]);

    if (extension != "c")
    {
        push_error("The file type you provided is not a valid C language type\n");
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

        for (int i = 0; i < tokens->error_count; ++i)
        {
            LexerError err = tokens->errors[i];
            push_error("Line %d, Column %d] Unexpected character '%c': %s\n",
                       err.line, err.column, err.character, err.message);
        }

        exit_code = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    if (tokens != NULL)
        free_tokens(tokens);

    if (source_code != NULL)
        free(source_code);

    if (get_error_count() > 0)
    {
        print_error_stack();
        clear_error_stack();
    }

    return exit_code;
}