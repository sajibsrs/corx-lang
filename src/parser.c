#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

static Token peek(Parser *parser);
static Token advance(Parser *parser);

Node *parse_program(Parser *parser);
Node *parse_function(Parser *parser);
Node *parse_statement(Parser *parser);
Node *parse_expression(Parser *parser, int prec);
Node *parse_factor(Parser *parser);
Node *parse_block(Parser *parser);
Node *parse_block_item(Parser *parser);
Node *parse_identifier(Parser *parser);
Node *parse_integer(Parser *parser);

const char *ntypestr[] = {
    [N_PROGRAM]     = "N_PROGRAM",     //
    [N_FUNCTION]    = "N_FUNCTION",    //
    [N_RETURN]      = "N_RETURN",      //
    [N_UNARY]       = "N_UNARY",       //
    [N_BINARY]      = "N_BINARY",      //
    [N_ASSIGNMENT]  = "N_ASSIGNMENT",  //
    [N_IDENTIFIER]  = "N_IDENTIFIER",  //
    [N_INTEGER]     = "N_INTEGER",     //
    [N_BLOCK]       = "N_BLOCK",       //
    [N_DECLARATION] = "N_DECLARATION", //
    [N_EXPRESSION]  = "N_EXPRESSION",  //
    [N_CONDITIONAL] = "N_CONDITIONAL", //
    [N_IF]          = "N_IF",          //
    [N_ELSE]        = "N_ELSE",        //
    [N_EMPTY]       = "N_EMPTY",       //
};

/**
 * @brief Get precedence by token type.
 * @param type
 * @return
 */
static int precedence(TokenType type) {
    switch (type) {
    case T_EQ:       // "="
        return 1;    //
    case T_QMARK:    // "?"
        return 3;    //
    case T_OR:       // "||"
        return 5;    //
    case T_AND:      // "&&"
        return 10;   //
    case T_EQEQ:     // "=="
    case T_NTEQ:     // "!="
        return 30;   //
    case T_LT:       // "<"
    case T_LTEQ:     // "<="
    case T_GT:       // ">"
    case T_GTEQ:     // "=>"
        return 35;   //
    case T_PLUS:     // "+"
    case T_MINUS:    // "-"
        return 45;   //
    case T_ASTERISK: // "*"
    case T_FSLASH:   // "/"
    case T_MODULUS:  // "%"
        return 50;   //
    default:         // No match
        return 0;    //
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
    case T_PLUS:     // "+"
    case T_MINUS:    // "-"
    case T_ASTERISK: // "*"
    case T_FSLASH:   // "/"
    case T_MODULUS:  // "%"
    case T_AND:      // "&&"
    case T_OR:       // "||"
    case T_EQEQ:     // "=="
    case T_NTEQ:     // "!="
    case T_LT:       // "<"
    case T_LTEQ:     // "<="
    case T_GT:       // ">"
    case T_GTEQ:     // ">="
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
    case T_AMPERSAND: // "&"
    case T_ASTERISK:  // "*"
    case T_PLUS:      // "+"
    case T_MINUS:     // "-"
    case T_TILDE:     // "~"
    case T_BANG:      // "!"
        return true;  //
    default:          //
        return false; //
    }
}

/**
 * @brief Checks if it's an assignment operator.
 * @param type
 * @return
 */
static bool isasnop(TokenType type) {
    switch (type) {
    case T_EQ:        // "="
    case T_PLUSEQ:    // "+="
    case T_MINUSEQ:   // "-="
    case T_MULEQ:     // "*="
    case T_DIVEQ:     // "/="
    case T_LSHIFTEQ:  // "<<="
    case T_RSHIFTEQ:  // ">>="
    case T_ANDEQ:     // "&="
    case T_XOREQ:     // "^="
    case T_OREQ:      // "|="
        return true;  //
    default:          //
        return false; //
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
Node *parse_program(Parser *parser) {
    Node *func = parse_function(parser);
    Node *prog = make_node(N_PROGRAM, "program");

    add_child(prog, func);
    return prog;
}

/**
 * @brief Parse function.
 * @param parser
 * @return
 */
Node *parse_function(Parser *parser) {
    Token next = peek(parser);
    if (next.type != T_INT) errexit("expected a type");

    advance(parser);

    Node *ident = parse_identifier(parser);

    expect(parser, T_LPAREN, "expected '(' after function name");
    expect(parser, T_VOID, "expected 'void' in the parameter list");
    expect(parser, T_RPAREN, "expected ')' after 'void'");

    Node *body = parse_block(parser);
    Node *func = make_node(N_FUNCTION, "function");
    add_child(func, ident);
    add_child(func, body);

    return func;
}

Node *parse_block(Parser *parser) {
    expect(parser, T_LBRACE, "expected '{' after before block");
    Node *nblock = make_node(N_BLOCK, "block");

    while (peek(parser).type != T_RBRACE && peek(parser).type != T_EOF) {
        Node *stmt = parse_block_item(parser);
        add_child(nblock, stmt);
    }

    expect(parser, T_RBRACE, "expected '}' after the block");

    return nblock;
}

Node *parse_block_item(Parser *parser) {
    Token next = peek(parser);

    // Handle variable declarations
    if (next.type == T_INT) {
        advance(parser);

        Node *ident = parse_identifier(parser);
        Node *decl  = make_node(N_DECLARATION, "declaration");
        add_child(decl, ident);

        if (peek(parser).type == T_EQ) {
            advance(parser);

            Node *expr = parse_expression(parser, 0);
            add_child(decl, expr);
        }

        expect(parser, T_SCOLON, "expected ';' after declaration");
        return decl;
    }
    // Handle statements (e.g., return, assignments)
    else {
        Node *stmt = parse_statement(parser);
        return stmt;
    }

    errexitinfo(parser, "malformed block-sitem statement");
    return NULL;
}

/**
 * @brief Parse statement.
 * @param parser
 * @return
 */
Node *parse_statement(Parser *parser) {
    Token next = peek(parser);

    // Handle return statement
    if (next.type == T_RETURN) {
        advance(parser); // Consume 'return'

        Node *expr = parse_expression(parser, 0);

        expect(parser, T_SCOLON, "expected ';' semicolon after return statement");

        Node *stmt = make_node(N_RETURN, "return");
        add_child(stmt, expr);

        return stmt;
    }

    // Handle if statement
    else if (next.type == T_IF) {
        advance(parser); // Consume 'if'

        expect(parser, T_LPAREN, "expected '(' after if statement");
        Node *expr = parse_expression(parser, 0);
        expect(parser, T_RPAREN, "expected ')' after if statement");

        Node *ibody; // If statement body

        if (peek(parser).type == T_LBRACE) {
            ibody = parse_block(parser);
        } else {
            ibody = parse_statement(parser); // Single statement or expression
        }

        Node *inode = make_node(N_IF, "if-stmt");
        add_child(inode, expr);
        add_child(inode, ibody);

        // Handle optional else statement
        next = peek(parser);

        if (next.type == T_ELSE) {
            advance(parser); // Consume 'else'

            Node *ebody;

            if (peek(parser).type == T_LBRACE) {
                ebody = parse_block(parser);
            } else {
                ebody = parse_statement(parser);
            }

            Node *enode = make_node(N_ELSE, "else-stmt");
            add_child(enode, ebody);
            add_child(inode, enode);
        }

        return inode;
    }

    // Handle identifier (assignments)
    else if (next.type == T_IDENT) {
        Node *ident = parse_identifier(parser);

        next = peek(parser);

        if (isasnop(next.type)) {
            Token asnop = next; // Save assignment operator
            advance(parser);

            Node *expr = parse_expression(parser, 0);
            expect(parser, T_SCOLON, "expected ';' after assignment statement");

            Node *stmt = make_node(N_ASSIGNMENT, asnop.str);
            add_child(stmt, ident);
            add_child(stmt, expr);

            return stmt;
        }
    }

    // Handle other statements or report error
    errexitinfo(parser, "malformed statement");
    return NULL;
}

/**
 * @brief Parse expression.
 * @param parser
 * @param prec Minimum precedence.
 * @return
 */
Node *parse_expression(Parser *parser, int prec) {
    Node *left = parse_factor(parser);
    Token next = peek(parser);

    // Precedence climbing loop for binary operators
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        advance(parser);

        // Handle binary operation
        Node *right = parse_expression(parser, precedence(next.type) + 1);
        Node *node  = make_node(N_BINARY, next.str);
        add_child(node, left);
        add_child(node, right);

        left = node; // Update left for next iteration
        next = peek(parser);
    }

    // Handle ternary conditional operator
    if (next.type == T_QMARK) {
        advance(parser); // Consume '?'

        // Parse the true expression
        Node *leftexpr = parse_expression(parser, 0);

        // Expect the colon separator
        expect(parser, T_COLON, "expected ':' in ternary operator");

        // Parse the false expression
        Node *rightexpr = parse_expression(parser, precedence(next.type));

        // Create a conditional node
        Node *node = make_node(N_CONDITIONAL, "cond-expr");
        add_child(node, left);      // Condition
        add_child(node, leftexpr);  // True expression
        add_child(node, rightexpr); // False expression

        left = node; // Update left for next iteration
    }

    return left;
}

/**
 * @brief Parse factor.
 * @param parser
 * @return
 */
Node *parse_factor(Parser *parser) {
    Token next = peek(parser);

    // Handle integer constants
    if (next.type == T_NUMBER) {
        return parse_integer(parser);

    }
    // Handle identifier
    else if (next.type == T_IDENT) {

        return parse_identifier(parser);

    }
    // Handle unary expression
    else if (isunop(next.type)) {
        advance(parser);

        Node *inode = parse_factor(parser);
        Node *unode = make_node(N_UNARY, next.str);
        add_child(unode, inode);

        return unode;

    }
    // Handle group expression
    else if (next.type == T_LPAREN) {
        advance(parser);

        Node *inode = parse_expression(parser, 0);
        expect(parser, T_RPAREN, "expected ')' after expression");

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
Node *parse_identifier(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(N_IDENTIFIER, next.str);
}

/**
 * @brief Parse integer.
 * @param parser
 * @return
 */
Node *parse_integer(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(N_INTEGER, next.str);
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
