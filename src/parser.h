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
} NodeType;

typedef struct Node {
    NodeType type;
    char *value;
    struct Node **nodes; // child nodes
    int count;           // children count
} Node;

typedef struct {
    const TokenList *list; // token array
    int pos;
    Node *node; // root node
} Parser;

extern const char *ntypestr[];

Parser *make_parser(const TokenList *list);
void purge_parser(Parser *parser);

Node *program(Parser *parser);

#endif
