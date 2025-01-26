#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static Token peek(Parser *parser);
static Token advance(Parser *parser);

Node *program(Parser *parser);
Node *function(Parser *parser);
Node *statement(Parser *parser);
Node *expression(Parser *parser, int prec);
Node *factor(Parser *parser);

Node *blockitem(Parser *parser);

Node *identifier(Parser *parser);
Node *integer(Parser *parser);

const char *ntypestr[] = {
    [NOD_PROGRAM]     = "NOD_PROGRAM",     //
    [NOD_FUNCTION]    = "NOD_FUNCTION",    //
    [NOD_RETURN]      = "NOD_RETURN",      //
    [NOD_UNARY]       = "NOD_UNARY",       //
    [NOD_BINARY]      = "NOD_BINARY",      //
    [NOD_ASSIGNMENT]  = "NOD_ASSIGNMENT",  //
    [NOD_IDENTIFIER]  = "NOD_IDENTIFIER",  //
    [NOD_INTEGER]     = "NOD_INTEGER",     //
    [NOD_BLOCK]       = "NOD_BLOCK",       //
    [NOD_DECLARATION] = "NOD_DECLARATION", //
    [NOD_EXPRESSION]  = "NOD_EXPRESSION",  //
};

/**
 * @brief Get precedence by token type.
 * @param type
 * @return
 */
static int precedence(TokenType type) {
    switch (type) {
    case TOK_ASSIGN:   // "="
        return 1;      //
    case TOK_OR:       // "||"
        return 5;      //
    case TOK_AND:      // "&&"
        return 10;     //
    case TOK_EQ:       // "=="
    case TOK_NEQ:      // "!="
        return 30;     //
    case TOK_LT:       // "<"
    case TOK_LEQ:      // "<="
    case TOK_GT:       // ">"
    case TOK_GEQ:      // "=>"
        return 35;     //
    case TOK_PLUS:     // "+"
    case TOK_MINUS:    // "-"
        return 45;     //
    case TOK_ASTERISK: // "*"
    case TOK_FSLASH:   // "/"
    case TOK_MOD:      // "%"
        return 50;     //
    default:           // no match
        return 0;      //
    }
}

/*********************************************
 * Helper functions
 *********************************************/

/**
 * @brief Prints error message and exits.
 * @param msg
 */
void errexit(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

/**
 * @brief Prints error message with current token line and column information and exits.
 * @param parser
 * @param msg
 */
static void errexitinfo(Parser *parser, const char *msg) {
    Token next = peek(parser);

    fprintf(
        stderr, "Error: %s at token '%s' (ln: %d, column: %d)\n", //
        msg, next.value, next.line, next.column
    );
    exit(1);
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
            stderr, "%s at token '%s' (ln: %d, column: %d)\n", //
            msg, next.value, next.line, next.column
        );
        exit(1);
    }

    advance(parser);
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
    if (!node) errexit("make_node memory allocation");

    node->type  = type;
    node->value = value ? strdup(value) : NULL;
    node->nodes = NULL;
    node->count = 0;

    return node;
}

void add_child(Node *parent, Node *child) {
    parent->nodes = (Node **)realloc(parent->nodes, sizeof(Node *) * (parent->count + 1));
    if (!parent->nodes) errexit("add_child memory allocation");

    parent->nodes[parent->count] = child;
    parent->count++;
}

/*********************************************
 * Parsing functions
 *********************************************/

/**
 * @brief Checks if it's a binary operator.
 * @param type
 * @return
 */
static bool isbinop(TokenType type) {
    switch (type) {
    case TOK_PLUS:     // "+"
    case TOK_MINUS:    // "-"
    case TOK_ASTERISK: // "*"
    case TOK_FSLASH:   // "/"
    case TOK_MOD:      // "%"
    case TOK_AND:      // "&&"
    case TOK_OR:       // "||"
    case TOK_EQ:       // "=="
    case TOK_NEQ:      // "!="
    case TOK_LT:       // "<"
    case TOK_LEQ:      // "<="
    case TOK_GT:       // ">"
    case TOK_GEQ:      // ">="
        return true;
    default: //
        return false;
    }
}

/**
 * @brief Checks if it's a unary operator.
 * @param type
 * @return
 */
static bool isunop(TokenType type) {
    switch (type) {
    case TOK_MINUS:   //
    case TOK_TILDE:   //
        return true;  //
    default:          //
        return false; //
    }
}

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

Node *program(Parser *parser) {
    Node *func = function(parser);
    Node *prog = make_node(NOD_PROGRAM, "program");

    add_child(prog, func);
    return prog;
}

Node *function(Parser *parser) {
    Token next = peek(parser);
    if (next.type != TOK_INT) errexit("expected a type");

    advance(parser);

    Node *ident = identifier(parser);

    expect(parser, TOK_LPAREN, "expected '(' after function name");
    expect(parser, TOK_VOID, "expected 'void' in the parameter list");
    expect(parser, TOK_RPAREN, "expected ')' after 'void'");
    expect(parser, TOK_LBRACE, "expected '{' in the start of function body");

    Node *body = make_node(NOD_BLOCK, "body");
    while (peek(parser).type != TOK_RBRACE) {
        Node *bnode = blockitem(parser);
        add_child(body, bnode);
    }

    expect(parser, TOK_RBRACE, "expected '}' in the end of function body");

    Node *func = make_node(NOD_FUNCTION, "function");
    add_child(func, ident);
    add_child(func, body);

    return func;
}

Node *blockitem(Parser *parser) {
    Token next = peek(parser);

    if (next.type == TOK_INT) {
        advance(parser);

        Node *ident = identifier(parser);
        Node *decl  = make_node(NOD_DECLARATION, "declaration");
        add_child(decl, ident);

        if (peek(parser).type == TOK_ASSIGN) {
            advance(parser);
            Node *expr = expression(parser, 0);
            add_child(decl, expr);
        }

        expect(parser, TOK_SEMI, "expected ';' after declaration");
        return decl;
    }

    return statement(parser);
}

Node *statement(Parser *parser) {
    Token next = peek(parser);

    // handle return statement
    if (next.type == TOK_RETURN) {
        advance(parser); // consume 'return'

        Node *expr = expression(parser, 0);
        expect(parser, TOK_SEMI, "expected ';' after return");

        Node *stmt = make_node(NOD_RETURN, "return");
        add_child(stmt, expr);

        return stmt;

    }
    // handle identifier
    else if (next.type == TOK_IDENT) {
        Node *indent = identifier(parser);

        next = peek(parser);
        if (next.type == TOK_ASSIGN) {
            advance(parser);

            Node *expr = expression(parser, 0);
            expect(parser, TOK_SEMI, "expected ';' after assignment");

            Node *stmt = make_node(NOD_ASSIGNMENT, "=");
            add_child(stmt, indent);
            add_child(stmt, expr);

            return stmt;
        }
    }

    errexitinfo(parser, "unexpected statement");
    return NULL;
}

Node *expression(Parser *parser, int prec) {
    Node *left = factor(parser);
    Token next = peek(parser);

    Node *node, *right;

    // precedence climbing loop
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        advance(parser);

        if (next.type == TOK_ASSIGN) {
            right = expression(parser, precedence(next.type));
            node  = make_node(NOD_ASSIGNMENT, next.value);
        } else {
            right = expression(parser, precedence(next.type) + 1);
            node  = make_node(NOD_BINARY, next.value);
        }

        add_child(node, left);
        add_child(node, right);

        left = node; // update left for next iteration
        next = peek(parser);
    }

    return left;
}

Node *factor(Parser *parser) {
    Token next = peek(parser);

    // handle integer constants
    if (next.type == TOK_NUMBER) {
        return integer(parser);

    }
    // handle identifier
    else if (next.type == TOK_IDENT) {

        return identifier(parser);

    }
    // handle unary expression
    else if (isunop(next.type)) {
        advance(parser);

        Node *inode = factor(parser);
        Node *unode = make_node(NOD_UNARY, next.value);
        add_child(unode, inode);

        return unode;

    }
    // handle group expression
    else if (next.type == TOK_LPAREN) {
        advance(parser);

        Node *inode = expression(parser, 0);
        expect(parser, TOK_RPAREN, "expected ')' after expression");

        return inode;
    }

    errexitinfo(parser, "malformed factor");
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

    return make_node(NOD_INTEGER, next.value);
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
