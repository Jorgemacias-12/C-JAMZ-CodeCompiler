#include <string.h>
#include <stdbool.h>
#include "semantic.h"
#include "parser.h"
#include "utils.h"

static bool is_keyword_of_category(const char *name, const char *category, Keyword *keywords, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(keywords[i].name, name) == 0 && strcmp(keywords[i].category, category) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool is_valid_type(const char *name, Keyword *keywords, int count)
{
    return is_keyword_of_category(name, "type", keywords, count);
}

static bool is_control_keyword(const char *name, Keyword *keywords, int count)
{
    return is_keyword_of_category(name, "control", keywords, count);
}

static void analyze_node(JAMZASTNode *ast, Keyword *keywords, int keyword_count)
{
    if (!ast)
        return;

    switch (ast->type)
    {
    case JAMZ_AST_LITERAL:
        if (!is_valid_type(ast->literal.value.lexeme, keywords, keyword_count) &&
            !is_control_keyword(ast->literal.value.lexeme, keywords, keyword_count) &&
            strcmp(ast->literal.value.lexeme, ";") != 0)
        {
            push_error("Unknown literal or keyword: '%s' (line %d, col %d)\n",
                       ast->literal.value.lexeme, ast->line, ast->column);
        }
        break;
    case JAMZ_AST_RETURN:
        // if (ast-> == 0 || strcmp(node->children[0]->value, "0") != 0)
        // {
        //     push_error("Return statement must return a value (line %d, col %d)\n",
        //                node->line, node->column);
        // }

        break;
    case JAMZ_AST_BLOCK:
    case JAMZ_AST_PROGRAM:
        break;
    }
}