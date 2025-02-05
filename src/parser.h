#ifndef _PARSER_H_
#define _PARSER_H_

#include "lexer.h"

// Node types
typedef enum {
    N_PROGRAM,
    N_VAR_DECL,
    N_VAR_DEF,
    N_FUNC_DECL,
    N_FUNC_DEF,
    N_RETURN,
    N_UNARY,
    N_BINARY,
    N_ASSIGNMENT,
    N_IDENTIFIER,
    N_NUMBER,
    N_FLOAT,
    N_BLOCK,
    N_EXPRESSION,
    N_CONDITIONAL, //
    N_IF,          // If statement
    N_ELSE,        //
    N_FOR,         //
    N_BREAK,       //
    N_CONTINUE,    //
    N_PARAM_LIST,  //
    N_TYPE,        //
    N_EMPTY,       // Empty statement (;)
} NodeType;

typedef struct Node {
    NodeType type;
    char *dtype;
    char *value;
    struct Node **cnodes;   // Child nodes
    struct Node **inherits; //
    struct Node **fulfills; //
    unsigned ccount;        // Child count
    unsigned icount;        // Inheritance count
    unsigned fcount;        // Fulfillment count
} Node;

typedef struct {
    const TokenList *list; // Token array
    int pos;               // Current position
    Node *node;            // Root node
} Parser;

extern const char *ntypestr[];

Parser *make_parser(const TokenList *list);
void purge_parser(Parser *parser);
Node *parse_prog(Parser *parser);

#endif
