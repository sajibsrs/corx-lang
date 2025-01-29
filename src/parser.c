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

Node *block(Parser *parser);

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
    [NOD_CONDITIONAL] = "NOD_CONDITIONAL", //
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
    case TOK_QUESTION: // "?"
        return 3;      //
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
    default:           // No match
        return 0;      //
    }
}

/*********************************************
 * Helper Functions
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
        msg, next.str, next.line, next.column
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
            msg, next.str, next.line, next.column
        );
        exit(1);
    }

    advance(parser);
}

/*********************************************
 * Node Functions
 *********************************************/

Parser *make_parser(const TokenList *list) {
    Parser *parser = malloc(sizeof(Parser));

    parser->list = list;
    parser->pos  = -1; // Uninitialized state
    parser->node = NULL;

    return parser;
}

Node *make_node(NodeType type, const char *str) {
    Node *node = malloc(sizeof(Node));
    if (!node) errexit("make_node memory allocation");

    node->type  = type;
    node->str   = str ? strdup(str) : NULL;
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
    case TOK_AMPERSAND: // "&"
    case TOK_ASTERISK:  // "*"
    case TOK_PLUS:      // "+"
    case TOK_MINUS:     // "-"
    case TOK_TILDE:     // "~"
    case TOK_BANG:      // "!"
        return true;    //
    default:            //
        return false;   //
    }
}

/**
 * @brief Checks if it's an assignment operator.
 * @param type
 * @return
 */
static bool isasnop(TokenType type) {
    switch (type) {
    case TOK_ASSIGN:        // "="
    case TOK_ADD_ASSIGN:    // "+="
    case TOK_SUB_ASSIGN:    // "-="
    case TOK_MUL_ASSIGN:    // "*="
    case TOK_DIV_ASSIGN:    // "/="
    case TOK_LSHIFT_ASSIGN: // "<<="
    case TOK_RSHIFT_ASSIGN: // ">>="
    case TOK_AND_ASSIGN:    // "&="
    case TOK_XOR_ASSIGN:    // "^="
    case TOK_OR_ASSIGN:     // "|="
        return true;        //
    default:                //
        return false;       //
    }
}

/**
 * @brief Peek next token without consuming.
 * @param parser
 * @return
 */
static Token peek(Parser *parser) {
    Token *tokens = parser->list->tokens;
    return (parser->pos == -1) ? tokens[0] : tokens[parser->pos + 1];
}

/**
 * @brief Consume next token and move forward.
 * @param parser
 * @return Consumed token.
 */
static Token advance(Parser *parser) {
    Token *tokens = parser->list->tokens;

    if (parser->pos == -1) {
        parser->pos = 0;
        return tokens[0];
    }

    return tokens[parser->pos++];
}

/**
 * @brief Parse program.
 * @param parser
 * @return
 */
Node *program(Parser *parser) {
    Node *func = function(parser);
    Node *prog = make_node(NOD_PROGRAM, "program");

    add_child(prog, func);
    return prog;
}

/**
 * @brief Parse function.
 * @param parser
 * @return
 */
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
        Node *bnode = block(parser);
        add_child(body, bnode);
    }

    expect(parser, TOK_RBRACE, "expected '}' in the end of function body");

    Node *func = make_node(NOD_FUNCTION, "function");
    add_child(func, ident);
    add_child(func, body);

    return func;
}

Node *block(Parser *parser) {
    Token next = peek(parser);

    // Handle variable declarations
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
    // Handle statements (e.g., return, assignments)
    else {
        Node *stmt = statement(parser);
        expect(parser, TOK_SEMI, "expected ';' after statement");
        return stmt;
    }

    errexitinfo(parser, "malformed block statement");
    return NULL;
}

/**
 * @brief Parse statement.
 * @param parser
 * @return
 */
Node *statement(Parser *parser) {
    Token next = peek(parser);

    // Handle return statement
    if (next.type == TOK_RETURN) {
        advance(parser); // Consume 'return'

        Node *expr = expression(parser, 0);
        Node *stmt = make_node(NOD_RETURN, "return");
        add_child(stmt, expr);

        return stmt;
    }
    // Handle identifier (assignments)
    else if (next.type == TOK_IDENT) {
        Node *ident = identifier(parser);
        next        = peek(parser);

        if (isasnop(next.type)) {
            Token asnop = next; // Save operator
            advance(parser);

            Node *expr = expression(parser, 0);
            Node *stmt = make_node(NOD_ASSIGNMENT, asnop.str);
            add_child(stmt, ident);
            add_child(stmt, expr);

            return stmt;
        }
    }

    errexitinfo(parser, "malformed statement");
    return NULL;
}

/**
 * @brief Parse expression.
 * @param parser
 * @param prec Minimum precedence.
 * @return
 */
Node *expression(Parser *parser, int prec) {
    Node *left = factor(parser);
    Token next = peek(parser);

    // Precedence climbing loop for binary operators
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        advance(parser);

        // Handle binary operation
        Node *right = expression(parser, precedence(next.type) + 1);
        Node *node  = make_node(NOD_BINARY, next.str);
        add_child(node, left);
        add_child(node, right);

        left = node; // Update left for next iteration
        next = peek(parser);
    }

    // Handle ternary conditional operator
    if (next.type == TOK_QUESTION) {
        advance(parser); // Consume '?'

        // Parse the true expression
        Node *true_expr = expression(parser, 0);

        // Expect the colon separator
        expect(parser, TOK_COLON, "expected ':' in ternary operator");

        // Parse the false expression
        Node *false_expr = expression(parser, precedence(next.type));

        // Create a conditional node
        Node *node = make_node(NOD_CONDITIONAL, "conditional");
        add_child(node, left);       // Condition
        add_child(node, true_expr);  // True expression
        add_child(node, false_expr); // False expression

        left = node; // Update left for next iteration
    }

    return left;
}

/**
 * @brief Parse factor.
 * @param parser
 * @return
 */
Node *factor(Parser *parser) {
    Token next = peek(parser);

    // Handle integer constants
    if (next.type == TOK_NUMBER) {
        return integer(parser);

    }
    // Handle identifier
    else if (next.type == TOK_IDENT) {

        return identifier(parser);

    }
    // Handle unary expression
    else if (isunop(next.type)) {
        advance(parser);

        Node *inode = factor(parser);
        Node *unode = make_node(NOD_UNARY, next.str);
        add_child(unode, inode);

        return unode;

    }
    // Handle group expression
    else if (next.type == TOK_LPAREN) {
        advance(parser);

        Node *inode = expression(parser, 0);
        expect(parser, TOK_RPAREN, "expected ')' after expression");

        return inode;
    }

    errexitinfo(parser, "malformed factor");
    return NULL;
}

/**
 * @brief Parse identifier.
 * @param parser
 * @return
 */
Node *identifier(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(NOD_IDENTIFIER, next.str);
}

/**
 * @brief Parse integer.
 * @param parser
 * @return
 */
Node *integer(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(NOD_INTEGER, next.str);
}

/*********************************************
 * Cleanup Functions
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
    free(node->str);
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
