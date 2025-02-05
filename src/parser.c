#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

/*********************************************
 * Declarations
 *********************************************/

static Token peek(Parser *parser);
static Token advance(Parser *parser);

Node *parse_prog(Parser *parser);

Node *parse_decl(Parser *parser);
Node *parse_func_decl(Parser *parser);
Node *parse_var_decl(Parser *parser);

Node *parse_param_list(Parser *parser);

Node *parse_for_expr(Parser *parser);
Node *parse_for_init(Parser *parser);

Node *parse_stmt(Parser *parser);
Node *parse_expr(Parser *parser, int prec);
Node *parse_factor(Parser *parser);
Node *parse_block(Parser *parser);
Node *parse_block_item(Parser *parser);

Node *parse_ident(Parser *parser);
Node *parse_number(Parser *parser);

/*********************************************
 * Data Definitions
 *********************************************/

// Node type to string lookup-table.
const char *ntypestr[] = {
    [N_PROGRAM]     = "N_PROGRAM", //
    [N_VAR_DECL]    = "N_VAR_DECL",
    [N_VAR_DEF]     = "N_VAR_DEF",
    [N_FUNC_DECL]   = "N_FUNC_DECL",   //
    [N_FUNC_DEF]    = "N_FUNC_DEF",    //
    [N_RETURN]      = "N_RETURN",      //
    [N_UNARY]       = "N_UNARY",       //
    [N_BINARY]      = "N_BINARY",      //
    [N_ASSIGNMENT]  = "N_ASSIGNMENT",  //
    [N_IDENTIFIER]  = "N_IDENTIFIER",  //
    [N_NUMBER]      = "N_NUMBER",      //
    [N_BLOCK]       = "N_BLOCK",       //
    [N_EXPRESSION]  = "N_EXPRESSION",  //
    [N_CONDITIONAL] = "N_CONDITIONAL", //
    [N_IF]          = "N_IF",          //
    [N_ELSE]        = "N_ELSE",        //
    [N_BREAK]       = "N_BREAK",       //
    [N_CONTINUE]    = "N_CONTINUE",    //
    [N_FOR]         = "N_FOR",         //
    [N_PARAM_LIST]  = "N_PARAM_LIST",  //
    [N_TYPE]        = "N_TYPE",        //
    [N_EMPTY]       = "N_EMPTY",       //
};

// Precedence lookup-table.
static const int prectable[] = {
    [T_EQ]    = 1, //
    [T_QMARK] = 3, //
    [T_OR]    = 5, //

    [T_AND]  = 10, //
    [T_EQEQ] = 30, //
    [T_NTEQ] = 30, //

    [T_LT]   = 35, //
    [T_LTEQ] = 35, //
    [T_GT]   = 35, //
    [T_GTEQ] = 35, //

    [T_PLUS]  = 45, //
    [T_MINUS] = 45, //

    [T_ASTERISK] = 50, //
    [T_FSLASH]   = 50, //
    [T_MODULUS]  = 50,
};

/**
 * @brief Get precedence by token type.
 * @param type
 * @return
 */
static inline int precedence(TokenType type) {
    int len = sizeof(prectable) / sizeof(prectable[0]);
    return (type < len) ? prectable[type] : 0;
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

/**
 * @brief Make a parser object.
 * @param list Token list source.
 * @return
 */
Parser *make_parser(const TokenList *list) {
    Parser *parser = malloc(sizeof(Parser));

    if (!parser) errexit("make_parser allocation failed");

    parser->list = list;
    parser->pos  = -1;
    parser->node = NULL;

    return parser;
}

/**
 * @brief Create AST node.
 * @param type Node type.
 * @param value String value.
 * @return
 */
Node *make_node(NodeType type, const char *value) {
    Node *node = malloc(sizeof(Node));
    if (!node) errexit("make_node memory allocation");

    node->type     = type;
    node->dtype    = NULL;
    node->value    = (value && *value) ? strdup(value) : NULL;
    node->cnodes   = NULL;
    node->inherits = NULL;
    node->fulfills = NULL;
    node->ccount   = 0;
    node->icount   = 0;
    node->fcount   = 0;

    return node;
}

/**
 * @brief Attach a child node to a parent AST node.
 * @param parent
 * @param child
 */
void add_child(Node *parent, Node *child) {
    static const int CAPACITY = 4;

    if (parent->ccount % CAPACITY == 0) {
        int nsize      = (parent->ccount == 0) ? CAPACITY : parent->ccount + CAPACITY;
        parent->cnodes = realloc(parent->cnodes, sizeof(Node *) * nsize);

        if (!parent->cnodes) errexit("add_child memory allocation");
    }

    parent->cnodes[parent->ccount++] = child;
}

void add_inheritance(Node *current, Node *inherits) {
    static const int CAPACITY = 4;

    if (current->icount % CAPACITY == 0) {
        int nsize         = (current->icount == 0) ? CAPACITY : current->icount + CAPACITY;
        current->inherits = realloc(current->inherits, sizeof(Node *) * nsize);

        if (!current->inherits) errexit("add_inheritance memory allocation");
    }

    current->inherits[current->icount++] = inherits;
}

void add_fulfillment(Node *current, Node *fulfills) {
    static const int CAPACITY = 4;

    if (current->fcount % CAPACITY == 0) {
        int nsize         = (current->fcount == 0) ? CAPACITY : current->fcount + CAPACITY;
        current->fulfills = realloc(current->fulfills, sizeof(Node *) * nsize);

        if (!current->fulfills) errexit("add_inheritance memory allocation");
    }

    current->fulfills[current->fcount++] = fulfills;
}

/*********************************************
 * Helper Functions
 *********************************************/

/**
 * @brief Checks if it's a binary operator.
 * @param type
 * @return
 */
static inline bool isbinop(TokenType type) {
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
    case T_EQ:       // "="
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
static inline bool isunop(TokenType type) {
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
static inline bool isasnop(TokenType type) {
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

/*********************************************
 * Parsing Functions
 *********************************************/

/**
 * @brief Peek next token without consuming.
 * @param parser
 * @return
 */
static Token peek(Parser *parser) {
    return parser->list->tokens[parser->pos + 1];
}

/**
 * @brief Peek forward nth token, without consumption.
 * @param parser Pointer to current parser.
 * @param pos Number of token to look forward.
 * @return
 */
static Token peekfw(Parser *parser, int pos) {
    return parser->list->tokens[parser->pos + pos];
}

/**
 * @brief Look behind current token.
 * @param parser
 * @param pos
 * @return
 */
static Token backtrack(Parser *parser, int pos) {
    return parser->list->tokens[parser->pos - pos];
}

/**
 * @brief Consume next token and move forward.
 * @param parser
 * @return Consumed token.
 */
static Token advance(Parser *parser) {
    return parser->list->tokens[parser->pos++];
}

/**
 * @brief Parse program.
 * @param parser
 * @return
 */
Node *parse_prog(Parser *parser) {
    Node *func = parse_decl(parser);
    Node *prog = make_node(N_PROGRAM, "program");

    add_child(prog, func);
    return prog;
}

/**
 * @brief Parse declaration.
 * Determines declaration without consuming tokens.
 * @param parser
 * @return
 */
Node *parse_decl(Parser *parser) {
    Token next = peek(parser);

    // Look for a type
    if (next.type != T_INT) {
        errexitinfo(parser, "expected a type");
    }

    // TODO: Consume type

    Node *dnode;

    // TODO: Exit if not identifier

    // TODO: Consume identifier

    // Ignore identifier and look forward
    Token lookahead = peekfw(parser, 3);

    if (lookahead.type == T_LPAREN) {
        dnode = parse_func_decl(parser);
    } else {
        dnode = parse_var_decl(parser);
    }

    dnode->dtype = strdup(next.str);

    return dnode;
}

/**
 * @brief Parse declarations.
 * @param parser
 * @return
 */
Node *parse_var_decl(Parser *parser) {
    advance(parser); // Consume 'type'

    Node *ident = parse_ident(parser);
    Node *var   = make_node(N_VAR_DECL, ident->value);
    // add_child(var, ident);

    // Handle assignment
    if (peek(parser).type == T_EQ) {
        var->type = N_VAR_DEF;

        advance(parser); // Consume '='

        Node *expr = parse_expr(parser, 0);
        add_child(var, expr);
    }
    expect(parser, T_SCOLON, "expected ';' after declaration");

    return var;
}

/**
 * @brief Parse function declaration.
 * @param parser
 * @return
 */
Node *parse_func_decl(Parser *parser) {
    advance(parser); // Consume 'type'

    Node *ident = parse_ident(parser);

    expect(parser, T_LPAREN, "expected '(' after function name");

    // Parse parameter-list
    Node *plist = parse_param_list(parser);

    expect(parser, T_RPAREN, "expected ')' after parameter list");

    Node *func = make_node(N_FUNC_DECL, ident->value);
    // add_child(func, ident);

    // Attach parameter list
    if (plist) add_child(func, plist);

    Token next = peek(parser);

    // Parse body
    if (next.type == T_LBRACE) {
        func->type = N_FUNC_DEF;

        Node *body = parse_block(parser);
        add_child(func, body);
    }

    // Parse declaration
    else {
        expect(parser, T_SCOLON, "expected ';' after function declaration");
    }

    return func;
}

/**
 * @brief Parse function parameter list.
 * @param parser
 * @return
 */
Node *parse_param_list(Parser *parser) {
    Token next = peek(parser);

    // If there are no parameters
    if (next.type == T_RPAREN) return NULL;

    Node *plist = make_node(N_PARAM_LIST, "");

    // Handle parameters
    while (peek(parser).type != T_RPAREN) {
        expect(parser, T_INT, "expected 'type' before param");

        if (peek(parser).type == T_IDENT) {
            Node *param  = parse_ident(parser);
            param->dtype = "int";
            add_child(plist, param);
        } else {
            Node *param  = make_node(N_IDENTIFIER, "");
            param->dtype = "int";
            add_child(plist, param);
        }

        // Comsume comma ','
        if (peek(parser).type == T_COMMA) advance(parser);
    }

    return plist;
}

/**
 * @brief Parse a for loop statement.
 * @param parser
 * @return
 */
Node *parse_for_expr(Parser *parser) {
    expect(parser, T_LPAREN, "expected '(' after 'for'"); // Consume '('

    // Handle initialization expression
    Node *init = parse_for_init(parser);

    // Handle optional conditional/test expression
    Node *cond = NULL;
    if (peek(parser).type != T_SCOLON) {
        cond = parse_expr(parser, 0);
    }

    expect(parser, T_SCOLON, "expected ';' after for-loop expression");

    // Handle optional iteration/increment expression
    Node *iter = NULL;
    if (peek(parser).type != T_RPAREN) {
        iter = parse_expr(parser, 0);
    }

    expect(parser, T_RPAREN, "expected ')' after for-loop expression");

    // Parse loop body
    Node *body = parse_stmt(parser);

    Node *fnode = make_node(N_FOR, "");
    add_child(fnode, init);
    add_child(fnode, cond ? cond : make_node(N_EMPTY, ""));
    add_child(fnode, iter ? iter : make_node(N_EMPTY, ""));
    add_child(fnode, body);

    return fnode;
}

/**
 * @brief Parse the initialization part of a for loop.
 * It can be either a variable declaration or an expression.
 * @param parser
 * @return
 */
Node *parse_for_init(Parser *parser) {
    Token next = peek(parser);

    // Parse null statement
    if (next.type == T_SCOLON) {
        advance(parser);

        return make_node(N_EMPTY, "");
    }

    // Parse declaration
    else if (next.type == T_INT) {
        return parse_decl(parser);
    }

    expect(parser, T_SCOLON, "expected ';' after for-loop expression");

    // Otherwise parse expression
    return parse_expr(parser, 0);
}

Node *parse_block(Parser *parser) {
    expect(parser, T_LBRACE, "expected '{' before block");
    Node *node = make_node(N_BLOCK, "");

    while (peek(parser).type != T_RBRACE) {
        Node *stmt = parse_block_item(parser);
        add_child(node, stmt);
    }

    expect(parser, T_RBRACE, "expected '}' after the block");

    return node;
}

/**
 * @brief Parse block item.
 * @param parser
 * @return
 */
Node *parse_block_item(Parser *parser) {
    Token next = peek(parser);

    // Handle variable declarations
    if (next.type == T_INT) {
        return parse_decl(parser);
    }

    // Handle statements (e.g., return, assignments)
    else {
        Node *stmt = parse_stmt(parser);
        return stmt;
    }

    return NULL;
}

/**
 * @brief Parse statement.
 * @param parser
 * @return
 */
Node *parse_stmt(Parser *parser) {
    Token next = peek(parser);

    // Handle return statement
    if (next.type == T_RETURN) {
        advance(parser); // Consume 'return'

        Node *expr = parse_expr(parser, 0);
        expect(parser, T_SCOLON, "expected ';' semicolon after return statement");

        Node *stmt = make_node(N_RETURN, "return");
        add_child(stmt, expr);

        return stmt;
    }

    // Handle if statement
    else if (next.type == T_IF) {
        advance(parser); // Consume 'if'

        expect(parser, T_LPAREN, "expected '(' after if statement");
        Node *expr = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "expected ')' after if statement");

        Node *ibody = parse_stmt(parser);
        Node *inode = make_node(N_IF, "");
        add_child(inode, expr);
        add_child(inode, ibody);

        // Handle optional else statement
        next = peek(parser);

        if (next.type == T_ELSE) {
            advance(parser); // Consume 'else'

            Node *ebody = parse_stmt(parser);
            Node *enode = make_node(N_ELSE, "");
            add_child(enode, ebody);
            add_child(inode, enode);
        }

        return inode;
    }

    // Handle block statement
    else if (next.type == T_LBRACE) {
        return parse_block(parser);
    }

    // Handle identifier (assignments)
    else if (next.type == T_IDENT) {
        Node *ident = parse_ident(parser);

        next = peek(parser);

        if (isasnop(next.type)) {
            Token asnop = next; // Save assignment operator
            advance(parser);

            Node *expr = parse_expr(parser, 0);
            expect(parser, T_SCOLON, "expected ';' after assignment statement");

            Node *stmt = make_node(N_ASSIGNMENT, asnop.str);
            add_child(stmt, ident);
            add_child(stmt, expr);

            return stmt;
        }
    }

    // Handle for loop
    else if (next.type == T_FOR) {
        advance(parser);
        return parse_for_expr(parser);
    }

    // Handle break statement
    else if (next.type == T_BREAK) {
        advance(parser);
        return make_node(N_BREAK, "");
    }

    // Handle continue statement
    else if (next.type == T_CONTINUE) {
        advance(parser);
        return make_node(N_CONTINUE, "");
    }

    // Handle null statement
    else if (next.type == T_SCOLON) {
        Token tok = advance(parser);
        return make_node(N_EMPTY, "");
    }

    // Handle other statements or report error
    errexitinfo(parser, "invalid statement syntax");
    return NULL;
}

/**
 * @brief Parse expression.
 * @param parser
 * @param prec Minimum precedence.
 * @return
 */
Node *parse_expr(Parser *parser, int prec) {
    Node *left = parse_factor(parser);
    Token next = peek(parser);

    // Precedence climbing loop for binary operators
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        advance(parser);

        // Handle binary operation
        Node *right = parse_expr(parser, precedence(next.type) + 1);
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
        Node *leftexpr = parse_expr(parser, 0);

        // Expect the colon separator
        expect(parser, T_COLON, "expected ':' in ternary operator");

        // Parse the false expression
        Node *rightexpr = parse_expr(parser, precedence(next.type));

        // Create a conditional node
        Node *node = make_node(N_CONDITIONAL, "");
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

    // Handle identifier
    if (next.type == T_IDENT) {
        return parse_ident(parser);
    }

    // Handle integer constants
    else if (next.type == T_NUMBER) {
        return parse_number(parser);
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

        Node *inode = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "expected ')' after expression");

        return inode;
    }

    errexitinfo(parser, "invalid factor syntax");
    return NULL;
}

/**
 * @brief Parse identifier.
 * @param parser
 * @return
 */
Node *parse_ident(Parser *parser) {
    Token next = peek(parser);
    advance(parser); // Consume identifier

    return make_node(N_IDENTIFIER, next.str);
}

/**
 * @brief Parse number.
 * @param parser
 * @return
 */
Node *parse_number(Parser *parser) {
    Token next = peek(parser);
    advance(parser);

    return make_node(N_NUMBER, next.str);
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

    // Recursively purge child nodes
    for (int i = 0; i < node->ccount; i++) {
        purge_node(node->cnodes[i]);
    }

    // Recursively purge inheritance nodes
    for (int i = 0; i < node->icount; i++) {
        purge_node(node->inherits[i]);
    }

    // Recursively purge fulfillment nodes
    for (int i = 0; i < node->fcount; i++) {
        purge_node(node->fulfills[i]);
    }

    // Free the arrays
    free(node->cnodes);
    free(node->inherits);
    free(node->fulfills);

    // Free node values
    free(node->dtype);
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
