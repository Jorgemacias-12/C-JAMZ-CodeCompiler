#include "compile.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>

void generate_asm(const JAMZASTNode *ast, const char *input_filename)
{
    // Cambiar la extensión del archivo de entrada a .asm
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s.asm", strtok((char *)input_filename, "."));

    FILE *file = fopen(output_filename, "w"); // Cambiado de "rw" a "w" para sobrescribir el contenido si el archivo ya existe.
    if (!file)
    {
        fprintf(stderr, "Error: No se pudo crear el archivo %s\n", output_filename);
        return;
    }

    // Cargar el diccionario desde un archivo JSON
    FILE *dict_file = fopen("data/dictionary.json", "r");
    if (!dict_file)
    {
        fprintf(stderr, "Error: No se pudo abrir el archivo dictionary.json\n");
        fclose(file);
        return;
    }

    fseek(dict_file, 0, SEEK_END);
    long length = ftell(dict_file);
    fseek(dict_file, 0, SEEK_SET);

    char *dict_content = safe_malloc(length + 1);
    fread(dict_content, 1, length, dict_file);
    dict_content[length] = '\0';
    fclose(dict_file);

    cJSON *dictionary = cJSON_Parse(dict_content);
    free(dict_content);

    if (!dictionary)
    {
        fprintf(stderr, "Error: No se pudo parsear el archivo dictionary.json\n");
        fclose(file);
        return;
    }

    fprintf(file, ".text\n");

    fprintf(file, "main:\n");
    fprintf(file, "    ; Inicio del programa mínimo\n");

    for (size_t i = 0; i < ast->block.count; i++)
    {
        JAMZASTNode *node = ast->block.statements[i];
        if (node->type == JAMZ_AST_DECLARATION)
        {
            if (node->declaration.initializer)
            {
                if (node->declaration.initializer->type == JAMZ_AST_LITERAL)
                {
                    cJSON *declaration_with_literal = cJSON_GetObjectItem(dictionary, "declaration_with_literal");
                    if (declaration_with_literal)
                    {
                        const char *template = cJSON_GetStringValue(declaration_with_literal);
                        fprintf(file, template, node->declaration.var_name, node->declaration.initializer->literal.value);
                    }
                }
                else if (node->declaration.initializer->type == JAMZ_AST_BINARY)
                {
                    cJSON *declaration_with_binary = cJSON_GetObjectItem(dictionary, "declaration_with_binary");
                    if (declaration_with_binary)
                    {
                        const char *template = cJSON_GetStringValue(declaration_with_binary);
                        fprintf(file, template, node->declaration.var_name, node->declaration.initializer->binary.left->literal.value, node->declaration.initializer->binary.right->literal.value);
                    }
                }
            }
        }
        else if (node->type == JAMZ_AST_RETURN)
        {
            cJSON *return_instr = cJSON_GetObjectItem(dictionary, "return");
            if (return_instr)
            {
                const char *template = cJSON_GetStringValue(return_instr);
                fprintf(file, template, node->return_stmt.value->literal.value);
            }
        }
        else if (node->type == JAMZ_AST_PRINT)
        {
            cJSON *print_instr = cJSON_GetObjectItem(dictionary, "print");
            if (print_instr)
            {
                const char *template = cJSON_GetStringValue(print_instr);
                fprintf(file, template, node->literal.value);
            }
            else
            {
                fprintf(stderr, "[ERROR] No se encontró la instrucción para 'print' en el diccionario.\n");
            }
        }
    }

    fprintf(file, "    ; Fin del programa mínimo\n");
    fprintf(file, "    ret\n");

    cJSON_Delete(dictionary);
    fclose(file);
}
