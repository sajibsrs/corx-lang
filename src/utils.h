#ifndef _UTILS_H_
#define _UTILS_H_

#include "lexer.h"
#include "parser.h"

void errexit(const char *msg);
void errwarn(const char *msg);
unsigned int hashfnv(const char *str, const int size);
void print_ast(const Node *node, int depth);
void print_toklist(const TokenList *list);

#endif
