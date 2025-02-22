#ifndef _UTILS_H
#define _UTILS_H

#include "lexer.h"
#include "parser.h"

void errexit(const char *msg);
void errwarn(const char *msg);

unsigned hashfnv(const char *str, const int size);

void print_ast(Node *node, int indent);
void print_tlist(const TokList *list);

#endif
