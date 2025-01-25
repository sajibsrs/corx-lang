#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/**
 * @brief Prints error message and exits.
 * @param msg
 */
void error_exit(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

const char *ntypestr[] = {
    [NOD_PROGRAM]    = "NOD_PROGRAM",    //
    [NOD_FUNCTION]   = "NOD_FUNCTION",   //
    [NOD_RETURN]     = "NOD_RETURN",     //
    [NOD_UNARY]      = "NOD_UNARY",      //
    [NOD_BINARY]     = "NOD_BINARY",     //
    [NOD_IDENTIFIER] = "NOD_IDENTIFIER", //
    [NOD_INT]        = "NOD_INT",        //
};

static int precedence(TokenType type) {
    switch (type) {
    case TOK_ASSIGN:     //
        return 1;        // assignment
    case TOK_ADD_ASSIGN: //
    case TOK_SUB_ASSIGN: //
    case TOK_MUL_ASSIGN: //
    case TOK_DIV_ASSIGN: //
    case TOK_MOD_ASSIGN: //
        return 2;        // compound assignment
    case TOK_EQ:         //
    case TOK_NEQ:        //
    case TOK_LT:         //
    case TOK_LEQ:        //
    case TOK_GT:         //
    case TOK_GEQ:        //
        return 3;        // comparisons
    case TOK_PLUS:       //
    case TOK_MINUS:      //
        return 4;        // addition, subtraction
    case TOK_ASTERISK:   //
    case TOK_FSLASH:     //
    case TOK_MOD:        //
        return 5;        // multiplication, division, modulus
    case TOK_TILDE:      //
    case TOK_BANG:       //
    case TOK_AMPERSAND:  // address-of
    case TOK_AT:         // pass-by-reference
    case TOK_BSLASH:     //
        return 6;        // unary operators (e.g., pointer `*p` and dereference `p*`)
    case TOK_ARROW:      // struct member access via pointer
    case TOK_DOT:        // struct member access
        return 7;        // member access
    default: return 0;   // default
    }
}

/*********************************************
 * Node functions
 *********************************************/

Parser *make_parser(const TokenList *list) {
    Parser *parser = malloc(sizeof(Parser));

    parser->list = list;
    parser->pos  = -1; // uninitialized state
    parser->node = NULL;

    return parser;
}

Node *make_node(NodeType type, const char *value) {
    Node *node = malloc(sizeof(Node));
    if (!node) error_exit("make_node memory allocation");

    node->type  = type;
    node->value = value ? strdup(value) : NULL;
    node->nodes = NULL;
    node->count = 0;

    return node;
}

void add_child(Node *parent, Node *child) {
    parent->nodes = (Node **)realloc(parent->nodes, sizeof(Node *) * (parent->count + 1));
    if (!parent->nodes) error_exit("add_child memory allocation");

    parent->nodes[parent->count] = child;
    parent->count++;
}

/*********************************************
 * Parsing functions
 *********************************************/

static Token peek(Parser *parser) {
    Token *tokens = parser->list->tokens;
    return (parser->pos == -1) ? tokens[0] : tokens[parser->pos + 1];
}

static Token advance(Parser *parser) {
    Token *tokens = parser->list->tokens;

    if (parser->pos == -1) {
        parser->pos = 0;
        return tokens[0];
    }

    return tokens[parser->pos++];
}

/**
 * @brief Compare expected token type against next token type. Consume on
 * success and exit on failure.
 *
 * @param parser Parser to check.
 * @param type Expected token type.
 * @param msg Error message to be displayed.
 */
static void expect(Parser *parser, TokenType type, const char *msg) {
    Token next = peek(parser);

    if (next.type != type) {
        fprintf(
            stderr, "%s at token '%s' (ln: %d, column: %d)\n", msg, next.value, next.line,
            next.column
        );
        exit(1);
    }

    advance(parser);
}

Node *program(Parser *parser);
Node *function(Parser *parser);
Node *statement(Parser *parser);
Node *expression(Parser *parser);

Node *identifier(Parser *parser);
Node *integer(Parser *parser);

Node *program(Parser *parser) {
    Node *func = function(parser);
    Node *prog = make_node(NOD_PROGRAM, "program");

    add_child(prog, func);
    return prog;
}

Node *function(Parser *parser) {
    Token next = peek(parser);
    if (next.type != TOK_INT) error_exit("expected a type");

    advance(parser);

    Node *indent = identifier(parser);

    expect(parser, TOK_LPAREN, "expected '(' after function name");
    expect(parser, TOK_VOID, "expected 'void' in the parameter list");
    expect(parser, TOK_RPAREN, "expected ')' after 'void'");
    expect(parser, TOK_LBRACE, "expected '{' in the start of function body");

    Node *body = statement(parser);

    expect(parser, TOK_RBRACE, "expected '}' in the end of function body");

    Node *func = make_node(NOD_FUNCTION, "function");
    add_child(func, indent);
    add_child(func, body);

    return func;
}

Node *statement(Parser *parser) {
    Token next = peek(parser);
    if (next.type != TOK_RETURN) error_exit("expected 'return' ");
    advance(parser);

    Node *expr = expression(parser);

    expect(parser, TOK_SEMI, "expected ';' after return");

    Node *stmt = make_node(NOD_RETURN, "return");
    add_child(stmt, expr);

    return stmt;
}

Node *expression(Parser *parser) {
    Token next = peek(parser);

    if (next.type == TOK_NUMBER) {
        return integer(parser);
    }

    if (next.type == TOK_MINUS || next.type == TOK_TILDE) {
        advance(parser);

        Node *inode = expression(parser);
        Node *unode = make_node(NOD_UNARY, next.value);
        add_child(unode, inode);

        return unode;
    }

    if (next.type == TOK_LPAREN) {
        advance(parser);

        Node *inode = expression(parser);
        expect(parser, TOK_RPAREN, "expected ')' after expression");

        return inode;
    }

    error_exit("malformed expression");
    return NULL;
}

Node *identifier(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(NOD_IDENTIFIER, next.value);
}

Node *integer(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(NOD_INT, next.value);
}

/*********************************************
 * Cleanup functions
 *********************************************/

/**
 * @brief
 * @param node
 */
void purge_node(Node *node) {
    if (!node) return;

    for (int i = 0; i < node->count; i++) {
        purge_node(node->nodes[i]);
    }
    free(node->nodes);
    free(node->value);
    free(node);
}

/**
 * @brief
 * @param parser
 */
void purge_parser(Parser *parser) {
    if (!parser) return;

    purge_node(parser->node);
    free(parser);
}
