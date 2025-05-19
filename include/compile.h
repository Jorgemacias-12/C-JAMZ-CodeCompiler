#ifndef COMPILE_H
#define COMPILE_H

#include "parser.h"
#include "utils.h"
void generate_asm(const JAMZASTNode *ast, const char *output_filename);
#endif 