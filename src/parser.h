#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"

typedef enum {
    N_PROGRAM,
    N_FUNCTION,
    N_RETURN,
    N_UNARY,
    N_BINARY,
    N_ASSIGNMENT,
    N_IDENTIFIER,
    N_INTEGER,
    N_BLOCK,
    N_DECLARATION,
    N_EXPRESSION,
    N_CONDITIONAL, //
    N_IF,          // Represents an if statement
    N_ELSE,        //
    N_FOR,         //
    N_BREAK,       //
    N_CONTINUE,    //
    N_EMPTY,       // Represents an empty statement (;)
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

Node *parse_program(Parser *parser);

#endif
