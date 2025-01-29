#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"

typedef enum {
    NOD_PROGRAM,
    NOD_FUNCTION,
    NOD_RETURN,
    NOD_UNARY,
    NOD_BINARY,
    NOD_ASSIGNMENT,
    NOD_IDENTIFIER,
    NOD_INTEGER,
    NOD_BLOCK,
    NOD_DECLARATION,
    NOD_EXPRESSION,
    NOD_CONDITIONAL,
} NodeType;

typedef struct Node {
    NodeType type;
    char *str;
    struct Node **nodes; // Child nodes
    int count;           // Children count
} Node;

typedef struct {
    const TokenList *list; // Token array
    int pos;
    Node *node; // Root node
} Parser;

extern const char *ntypestr[];

Parser *make_parser(const TokenList *list);
void purge_parser(Parser *parser);

Node *program(Parser *parser);

#endif
