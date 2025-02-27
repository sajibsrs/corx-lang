#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "parser.h"

/*********************************************
 * Declarations
 *********************************************/

static inline int precedence(TokType type);
static inline bool isbinop(TokType type);
static inline bool isunop(TokType type);
static inline bool isasnop(TokType type);

UnOps toktoun(TokType type);
BinOps toktobin(TokType type);

static void errexitinfo(Parser *prs, const char *msg);

static Token peek(Parser *prs);
static Token peekfw(Parser *prs);
static Token advance(Parser *prs);

Node *parse_program(Parser *prs);
Node *parse_block(Parser *prs);
Node *parse_block_item(Parser *prs);
Node *parse_declaration(Parser *prs, Token type, Token token);
Node *parse_var_decl(Parser *prs, Token type, Token token);
Node *parse_func_decl(Parser *prs, Token type, Token token);
Node *parse_func_call(Parser *prs, Token token);
Node *parse_params(Parser *prs, Token type, Token token);
Node *parse_for_init(Parser *prs, Token type, Token token);
Node *parse_do_while(Parser *prs);
Node *parse_stmt(Parser *prs);
Node *parse_expr(Parser *prs, int prec);
Node *parse_factor(Parser *prs);

/*********************************************
 * Data Definitions
 *********************************************/

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
static inline int precedence(TokType type) {
    int len = sizeof(prectable) / sizeof(prectable[0]);
    return (type < len) ? prectable[type] : 0;
}

/*********************************************
 * Helper Functions
 *********************************************/

/**
 * @brief Prints error message with current token line and column information and exits.
 * @param prs
 * @param msg
 */
static void errexitinfo(Parser *prs, const char *msg) {
    Token next = peek(prs);

    fprintf(
        stderr, "Error: %s at token '%s' (ln: %d, column: %d)\n", //
        msg, next.str, next.line, next.col
    );
    exit(1);
}

/**
 * @brief Compare expected token type against next token type. Consume on
 * success and exit on failure.
 *
 * @param prs Parser to check.
 * @param type Expected token type.
 * @param msg Error message to be displayed.
 */
Token expect(Parser *prs, TokType type, const char *msg) {
    Token next = peek(prs);
    if (next.type != type) {
        fprintf(
            stderr, "%s at token '%s' (ln: %d, column: %d)\n", //
            msg, next.str, next.line, next.col
        );
        exit(1);
    }

    return advance(prs);
}

/*********************************************
 * Node Functions
 *********************************************/

/**
 * @brief Make a parser object.
 * @param list Token list source.
 * @return
 */
Parser *make_parser(const TokList *list) {
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) errexit("make_parser allocation failed");

    parser->list = list;
    parser->pos  = -1;
    parser->node = NULL;

    return parser;
}

/*********************************************
 * Helper Functions
 *********************************************/

/**
 * @brief Checks if it's a binary operator.
 * @param type
 * @return
 */
static inline bool isbinop(TokType type) {
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
static inline bool isunop(TokType type) {
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
static inline bool isasnop(TokType type) {
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

// Lookup table for binary operators.
static const BinOps toktobin_table[] = {
    [T_PLUS]     = BIN_ADD,  //
    [T_MINUS]    = BIN_SUB,  //
    [T_ASTERISK] = BIN_MUL,  //
    [T_FSLASH]   = BIN_DIV,  //
    [T_MODULUS]  = BIN_MOD,  //
    [T_EQEQ]     = BIN_EQ,   //
    [T_NTEQ]     = BIN_NEQ,  //
    [T_LT]       = BIN_LT,   //
    [T_LTEQ]     = BIN_LTEQ, //
    [T_GT]       = BIN_GT,   //
    [T_GTEQ]     = BIN_GTEQ, //
    // Other tokens (like T_TILDE, T_BANG) remain 0 (BIN_INVALID)
};

// Lookup table for unary operators.
static const UnOps toktoun_table[] = {
    [T_MINUS] = UN_MINUS, //
    [T_PLUS]  = UN_PLUS,  //
    [T_TILDE] = UN_COMPL, //
    [T_BANG]  = UN_NOT,   //
    // Other tokens default to 0 (UN_INVALID)
};

/**
 * @brief Converts token to binary operator.
 * @param type
 * @return
 */
BinOps toktobin(TokType type) {
    // TODO: Bound check
    return toktobin_table[type];
}

/**
 * @brief Converts token to unary operator.
 * @param type
 * @return
 */
UnOps toktoun(TokType type) {
    // TODO: Bound check
    return toktoun_table[type];
}

/*********************************************
 * Parsing Functions
 *********************************************/

/**
 * @brief Peek next token without consumption.
 * @param prs
 * @return
 */
static Token peek(Parser *prs) {
    return prs->list->tokens[prs->pos + 1];
}

/**
 * @brief Peek forward, one token ahead of 'peek' and two token ahead of
 * the current token, without consumption.
 * @param prs Pointer to current parser.
 * @return
 */
static Token peekfw(Parser *prs) {
    return prs->list->tokens[prs->pos + 2];
}

/**
 * @brief Look one token behind of the current token.
 * @param prs
 * @param pos
 * @return
 */
static Token backtrack(Parser *prs, int pos) {
    return prs->list->tokens[prs->pos - pos];
}

/**
 * @brief Consume next token and move parsers current position forward.
 * @param prs
 * @return Consumed token.
 */
static Token advance(Parser *prs) {
    Token *arr = prs->list->tokens;
    Token pos  = arr[++prs->pos];

    prs->token = pos;
    return pos;
}

/**
 * @brief Parses the program.
 * @param prs
 * @return
 */
Node *parse_program(Parser *prs) {
    ProgNode *prog = malloc(sizeof(ProgNode));
    if (!prog) errexit("Memory allocation failed for ProgNode");

    prog->base.type = NODE_PROGRAM;
    prog->icount    = 0;

    int capacity = 2;
    prog->items  = malloc(capacity * sizeof(Node *));
    if (!prog->items) errexit("Memory allocation failed for program items");

    Token next = peek(prs);
    while (next.type != T_EOF) {
        Node *decl = NULL;

        if (next.type == T_IDENT) {
            // We assume a declaration starts with two consecutive identifiers:
            // one for the type and one for the variable or function name
            advance(prs);              // Consume first identifier (assumed type)
            Token name = advance(prs); // Consume second identifier (name)

            // Call parse_declaration with the type token and name token
            decl = parse_declaration(prs, next, name);
        } else {
            errexit("Invalid declaration in global scope");
        }

        if (!decl) errexit("Invalid declaration in global scope");

        // Resize items array if needed
        if (prog->icount >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(prog->items, capacity * sizeof(Node *));
            if (!new_items) {
                free(prog->items);
                errexit("Memory allocation failed during resize in parse_program");
            }
            prog->items = new_items;
        }
        prog->items[prog->icount++] = decl;
        next                        = peek(prs);
    }

    return (Node *)prog;
}

/**
 * @brief Parses declarations.
 * @param prs
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_declaration(Parser *prs, Token type, Token name) {
    if (type.type != T_IDENT || name.type != T_IDENT) {
        errexitinfo(prs, "Incorrect token types in declaration");
    }

    if (peek(prs).type == T_LPAREN) {
        return parse_func_decl(prs, type, name);
    }

    return parse_var_decl(prs, type, name);
}

/**
 * @brief Parses variable declarations.
 * @param prs
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_var_decl(Parser *prs, Token type, Token name) {
    VarNode *var = malloc(sizeof(VarNode));
    if (!var) errexit("Memory allocation failed for VarNode");

    var->base.type = NODE_VAR_DECL;
    var->base.line = prs->token.line;
    var->dtype     = strdup(type.str); // Duplicate string if necessary
    var->name      = strdup(name.str);
    var->init      = NULL;

    // Optional initializer:
    if (peek(prs).type == T_EQ) {
        advance(prs); // Consume '='
        // Use parse_expr (not parse_expr) to correctly parse the initializer
        var->init = parse_expr(prs, 0);
        if (!var->init) errexitinfo(prs, "Failed to parse initializer expression");
    }

    expect(prs, T_SCOLON, "Expected ';' after variable declaration");
    return (Node *)var;
}

/**
 * @brief Parses function declarations.
 * @param prs
 * @param rtype Return type.
 * @param name Identifier.
 * @return
 */
Node *parse_func_decl(Parser *prs, Token rtype, Token name) {
    expect(prs, T_LPAREN, "Expected '(' after function name");

    Node **params = malloc(2 * sizeof(Node *));
    if (!params) errexit("Memory allocation failed for function parameters");

    int pcount = 0, capacity = 2;
    Token next = peek(prs);

    if (next.type == T_IDENT && strcmp(next.str, "void") == 0) {
        // Parameter list is 'void'; consume it.
        advance(prs);
    } else {
        while (peek(prs).type != T_RPAREN) {

            Token param_type = expect(prs, T_IDENT, "Expected parameter type");
            Token param_name = expect(prs, T_IDENT, "Expected parameter identifier");
            Node *param      = parse_params(prs, param_type, param_name);

            if (pcount >= capacity) {
                capacity *= 2;
                Node **new_params = realloc(params, capacity * sizeof(Node *));
                if (!new_params) {
                    free(params);
                    errexit("Memory reallocation failed for function parameters");
                }
                params = new_params;
            }
            params[pcount++] = param;

            if (peek(prs).type == T_COMMA) advance(prs); // Consume ','
            else break;
        }
    }
    expect(prs, T_RPAREN, "Expected ')' after parameter list");

    Node *body = NULL;

    if (peek(prs).type == T_LBRACE) {
        body = parse_block(prs);
    } else {
        expect(prs, T_SCOLON, "Expected ';' after function declaration");
    }

    FuncNode *fn = malloc(sizeof(FuncNode));
    if (!fn) errexit("Memory allocation failed for FuncNode");

    fn->base.type = NODE_FUNC_DECL;
    fn->base.line = prs->token.line;
    fn->dtype     = strdup(rtype.str);
    fn->name      = strdup(name.str);
    fn->params    = params;
    fn->pcount    = pcount;
    fn->body      = body;

    return (Node *)fn;
}

/**
 * @brief Parses parameters.
 * @param prs
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_params(Parser *prs, Token type, Token name) {
    VarNode *param = malloc(sizeof(VarNode));
    if (!param) errexit("Memory allocation failed for parameter");

    param->base.type = NODE_VAR_DECL; // You may define a separate NODE_PARAMETER if desired
    param->base.line = prs->token.line;
    param->dtype     = strdup(type.str);
    param->name      = strdup(name.str);
    param->init      = NULL; // Parameters do not have initializers

    return (Node *)param;
}

/**
 * @brief Parses blocks.
 * @param prs
 * @return
 */
Node *parse_block(Parser *prs) {
    expect(prs, T_LBRACE, "Expected '{' to start block");

    BlockNode *block = malloc(sizeof(BlockNode));
    if (!block) errexit("Memory allocation failed for BlockNode");

    block->base.type = NODE_BLOCK;
    block->base.line = prs->token.line;
    block->icount    = 0;

    int capacity = 2, count = 0;
    Node **items = malloc(capacity * sizeof(Node *));
    if (!items) errexit("Memory allocation failed in parse_block");

    while (peek(prs).type != T_RBRACE && peek(prs).type != T_EOF) {
        Node *item = parse_block_item(prs);

        if (!item) errexit("Failed to parse block item");

        if (count >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(items, capacity * sizeof(Node *));
            if (!new_items) errexit("Memory reallocation failed in parse_block");

            items = new_items;
        }
        items[count++] = item;
    }
    expect(prs, T_RBRACE, "Expected '}' at end of block");

    block->items  = items;
    block->icount = count;

    return (Node *)block;
}

/**
 * @brief Parses block items.
 * @param prs
 * @return
 */
Node *parse_block_item(Parser *prs) {
    Token next = peek(prs);

    // If the next two tokens are both identifiers, assume a declaration
    if (next.type == T_IDENT && peekfw(prs).type == T_IDENT) {
        Token type = advance(prs); // Consume type
        Token name = advance(prs); // Consume name

        return parse_declaration(prs, type, name);
    }

    return parse_stmt(prs);
}

/**
 * @brief Parses statements.
 * @param prs
 * @return
 */
Node *parse_stmt(Parser *prs) {
    Token token    = peek(prs);
    StmtNode *stmt = malloc(sizeof(StmtNode));
    if (!stmt) errexit("Memory allocation failed for StmtNode");

    stmt->base.type = NODE_STATEMENT;
    stmt->base.line = prs->token.line;

    // Check for function call statement: identifier followed by '('
    if (token.type == T_IDENT && peekfw(prs).type == T_LPAREN) {
        Token funcToken = advance(prs); // Consume function name
        Node *callExpr  = parse_func_call(prs, funcToken);

        // Enforce that a semicolon terminates the function call statement
        expect(prs, T_SCOLON, "Expected ';' after function call statement");

        stmt->type          = STMT_CALL;
        stmt->u.simple.expr = callExpr;

        return (Node *)stmt;
    }

    // Handle assignment statement: identifier followed by '='
    else if (token.type == T_IDENT && peekfw(prs).type == T_EQ) {
        Token var_token = advance(prs); // Consume identifier
        advance(prs);                   // Consume '='
        Node *rhs = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';' after assignment");
        stmt->type             = STMT_ASSIGNMENT;
        stmt->u.assignment.lhs = strdup(var_token.str);
        stmt->u.assignment.rhs = rhs;
        return (Node *)stmt;
    }

    // Handle return statement
    else if (token.type == T_RETURN) {
        advance(prs); // Consume 'return'
        stmt->type          = STMT_RETURN;
        stmt->u.simple.expr = (peek(prs).type != T_SCOLON) ? parse_expr(prs, 0) : NULL;
        expect(prs, T_SCOLON, "Expected ';' after return statement");
    }

    // Handle if statement
    else if (token.type == T_IF) {
        advance(prs); // Consume 'if'
        expect(prs, T_LPAREN, "Expected '(' after if");
        stmt->u.if_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')' after if condition");
        stmt->u.if_stmt.then_stmt = parse_stmt(prs);

        if (peek(prs).type == T_ELSE) {
            advance(prs); // Consume 'else'
            stmt->u.if_stmt.else_stmt = parse_stmt(prs);
        } else {
            stmt->u.if_stmt.else_stmt = NULL;
        }
        stmt->type = STMT_IF;
    }

    // Handle compound statement (block)
    else if (token.type == T_LBRACE) {
        stmt->u.simple.expr = parse_block(prs);
        stmt->type          = STMT_COMPOUND;
    }

    // Handle while statement
    else if (token.type == T_WHILE) {
        advance(prs); // Consume 'while'
        expect(prs, T_LPAREN, "Expected '(' after while");
        stmt->u.while_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')' after while condition");
        stmt->u.while_stmt.body = parse_stmt(prs);
        stmt->type              = STMT_WHILE;
    }

    // Handle do-while statement
    else if (token.type == T_DO) {
        advance(prs); // Consume 'do'
        stmt->u.while_stmt.body = parse_stmt(prs);
        expect(prs, T_WHILE, "Expected 'while' after do");
        expect(prs, T_LPAREN, "Expected '(' after while in do-while");
        stmt->u.while_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')' after condition in do-while");
        expect(prs, T_SCOLON, "Expected ';' after do-while");
        stmt->type = STMT_DO_WHILE;
    }

    // Handle for statement.
    else if (token.type == T_FOR) {
        advance(prs); // Consume 'for'
        expect(prs, T_LPAREN, "Expected '(' after for");

        Node *init      = NULL;
        Node *condition = NULL;
        Node *post      = NULL;
        Token next      = peek(prs);

        if (next.type != T_SCOLON) {
            Token lookahead = peekfw(prs);

            if (next.type == T_IDENT && lookahead.type == T_IDENT) {
                Token typeTok = advance(prs); // Consume type identifier
                Token nameTok = advance(prs); // Consume variable name
                init          = parse_for_init(prs, typeTok, nameTok);
            } else {
                init = parse_expr(prs, 0);
            }
        }
        expect(prs, T_SCOLON, "Expected ';' after for initializer");

        next = peek(prs);
        if (next.type != T_SCOLON) {
            condition = parse_expr(prs, 0);
        }
        expect(prs, T_SCOLON, "Expected ';' after for condition");

        next = peek(prs);
        if (next.type != T_RPAREN) {
            post = parse_expr(prs, 0);
        }
        expect(prs, T_RPAREN, "Expected ')' after for clauses");

        Node *body                 = parse_stmt(prs);
        stmt->u.for_stmt.init      = init;
        stmt->u.for_stmt.condition = condition;
        stmt->u.for_stmt.post      = post;
        stmt->u.for_stmt.body      = body;
        stmt->type                 = STMT_FOR;

        return (Node *)stmt;
    }

    // Handle break statement
    else if (token.type == T_BREAK) {
        advance(prs);
        stmt->type = STMT_BREAK;
        expect(prs, T_SCOLON, "Expected ';' after break");
    }

    // Handle continue statement
    else if (token.type == T_CONTINUE) {
        advance(prs);
        stmt->type = STMT_CONTINUE;
        expect(prs, T_SCOLON, "Expected ';' after continue");
    }

    // Handle null statement
    else if (token.type == T_SCOLON) {
        advance(prs);
        stmt->type = STMT_NULL;
        // Note: We now return a null statement node, which will be printed.
    }

    // Otherwise, treat as an expression statement
    else {
        stmt->u.simple.expr = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';' after expression statement");
        stmt->type = STMT_EXPRESSION;
    }

    return (Node *)stmt;
}

/**
 * @brief Parses for loop initializers.
 * @param prs
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_for_init(Parser *prs, Token type, Token name) {
    VarNode *var = malloc(sizeof(VarNode));
    if (!var) errexit("Memory allocation failed for VarNode (for initializer)");

    var->base.type = NODE_VAR_DECL;
    var->base.line = prs->token.line;
    var->dtype     = strdup(type.str); // TODO:
    var->name      = strdup(name.str);
    var->init      = NULL;

    if (peek(prs).type == T_EQ) {
        advance(prs); // Consume '='
        var->init = parse_expr(prs, 0);
        if (!var->init) errexitinfo(prs, "Failed to parse initializer expression in for-loop");
    }

    // Do not expect a semicolon here
    return (Node *)var;
}

/**
 * @brief Parses expressions.
 * @param prs
 * @param prec Minimum precedence.
 * @return
 */
Node *parse_expr(Parser *prs, int prec) {
    Node *left = parse_factor(prs);
    Token next = peek(prs);

    // Check for assignment operator (lowest precedence, e.g., precedence value 1)
    if (next.type == T_EQ && prec <= 1) {
        advance(prs);                     // Consume '='.
        Node *right = parse_expr(prs, 1); // Recursively parse right-hand side

        // Assignment node
        ExprNode *anode = malloc(sizeof(ExprNode));
        if (!anode) errexit("Memory allocation failed for assignment ExprNode");

        anode->base.type        = NODE_EXPRESSION;
        anode->base.line        = prs->token.line;
        anode->type             = EXPR_ASSIGNMENT;
        anode->u.assignment.lhs = left;
        anode->u.assignment.rhs = right;

        return (Node *)anode;
    }

    // Process binary operators using precedence climbing
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        Token optoken = next;
        advance(prs); // Consume operator
        Node *right = parse_expr(prs, precedence(optoken.type) + 1);

        // Expression node
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for binary ExprNode");

        enode->base.type      = NODE_EXPRESSION;
        enode->base.line      = prs->token.line;
        enode->type           = EXPR_BINARY;
        enode->u.binary.op    = toktobin(optoken.type);
        enode->u.binary.left  = left;
        enode->u.binary.right = right;
        left                  = (Node *)enode;
        next                  = peek(prs);
    }

    // Handle ternary conditional operator: condition ? true_expr : false_expr
    if (next.type == T_QMARK) {
        advance(prs); // Consume '?'

        Node *true_expr = parse_expr(prs, 0);
        expect(prs, T_COLON, "Expected ':' in ternary operator");
        Node *false_expr = parse_expr(prs, 0);

        // Expression node
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for conditional ExprNode");

        enode->base.type                = NODE_EXPRESSION;
        enode->base.line                = prs->token.line;
        enode->type                     = EXPR_CONDITIONAL;
        enode->u.conditional.condition  = left;
        enode->u.conditional.true_expr  = true_expr;
        enode->u.conditional.false_expr = false_expr;
        left                            = (Node *)enode;
    }

    return left;
}

/**
 * @brief Parses do-while loop.
 * @param prs
 * @return
 */
Node *parse_do_while(Parser *prs) {
    StmtNode *stmt = malloc(sizeof(StmtNode));
    if (!stmt) errexit("Memory allocation failed for StmtNode");

    stmt->base.type = NODE_STATEMENT;
    stmt->base.line = prs->node->line;
    stmt->type      = STMT_DO_WHILE;

    advance(prs); // Consume 'do'
    // Parse the loop body
    stmt->u.while_stmt.body = parse_stmt(prs);

    // Expect 'while' and '('
    expect(prs, T_WHILE, "Expected 'while' after do");
    expect(prs, T_LPAREN, "Expected '(' after while");

    // Parse the condition
    stmt->u.while_stmt.condition = parse_expr(prs, 0);

    // Expect ')' and ';'
    expect(prs, T_RPAREN, "Expected ')' after condition");
    expect(prs, T_SCOLON, "Expected ';' after do-while");

    return (Node *)stmt;
}

/**
 * @brief Parses factors.
 * @param prs
 * @return
 */
Node *parse_factor(Parser *prs) {
    Token next = peek(prs);

    if (next.type == T_IDENT) {
        advance(prs); // Consume the identifier

        // Return a variable expression
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for variable expression");

        enode->base.type = NODE_EXPRESSION;
        enode->base.line = prs->token.line;
        enode->type      = EXPR_VAR;
        enode->u.name    = strdup(next.str);

        return (Node *)enode;
    }

    if (next.type == T_NUMBER) {
        advance(prs); // Consume number

        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for constant expression");

        enode->base.type = NODE_EXPRESSION;
        enode->base.line = prs->token.line;
        enode->type      = EXPR_CONSTANT;
        enode->u.value   = atoi(next.str);

        return (Node *)enode;
    }

    if (next.type == T_STRING) {
        advance(prs); // Consume string literal

        ExprNode *snode = malloc(sizeof(ExprNode));
        if (!snode) errexit("Memory allocation failed for StrNode");

        snode->base.type = NODE_EXPRESSION;
        snode->base.line = next.line;
        snode->type      = EXPR_STRING;
        snode->u.str     = next.str;

        return (Node *)snode;
    }

    if (isunop(next.type)) {
        Token op      = advance(prs); // Consume unary operator
        Node *operand = parse_factor(prs);

        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for unary expression");

        enode->base.type       = NODE_EXPRESSION;
        enode->base.line       = prs->token.line;
        enode->type            = EXPR_UNARY;
        enode->u.unary.op      = toktoun(op.type);
        enode->u.unary.operand = operand;

        return (Node *)enode;
    }

    if (next.type == T_LPAREN) {
        advance(prs); // Consume '('

        Node *expr = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')' after expression");

        return expr;
    }

    errexitinfo(prs, "Invalid factor syntax");
    return NULL;
}

/**
 * @brief Parses function call.
 * @param prs
 * @param token
 * @return
 */
Node *parse_func_call(Parser *prs, Token token) {
    // Expect '(' after function name.
    expect(prs, T_LPAREN, "Expected '(' after function name in function call");

    int capacity = 2, arg_count = 0;
    Node **args = malloc(capacity * sizeof(Node *));
    if (!args) errexit("Memory allocation failed for function call arguments");

    // Parse argument list until ')'
    while (peek(prs).type != T_RPAREN) {
        if (arg_count >= capacity) {
            capacity *= 2;
            Node **new_args = realloc(args, capacity * sizeof(Node *));

            if (!new_args) {
                free(args);
                errexit("Memory reallocation failed for function call arguments");
            }
            args = new_args;
        }
        args[arg_count++] = parse_expr(prs, 0);

        if (peek(prs).type == T_COMMA) {
            advance(prs); // Consume comma.
        } else {
            break;
        }
    }
    expect(prs, T_RPAREN, "Expected ')' after function call arguments");

    // Create an expression node for the function call
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (!expr) errexit("Memory allocation failed for function call expression");

    expr->base.type = NODE_EXPRESSION;
    expr->base.line = prs->token.line;
    expr->type      = EXPR_CALL;

    // Build the callee expression node (a variable node) for the function name.
    ExprNode *callee = malloc(sizeof(ExprNode));
    if (!callee) errexit("Memory allocation failed for function call callee");

    callee->base.type = NODE_EXPRESSION;
    callee->base.line = prs->token.line;
    callee->type      = EXPR_VAR;
    callee->u.name    = strdup(token.str);

    expr->u.call.callee = (Node *)callee;
    expr->u.call.args   = args;
    expr->u.call.acount = arg_count;

    return (Node *)expr;
}

/*********************************************
 * Cleanup Functions
 *********************************************/

/**
 * @brief Cleans up memory allocated for parser.
 * @param prs
 */
void purge_parser(Parser *prs) {
    if (!prs) return;

    purge_node(prs->node);
    free(prs);
}

/**
 * @brief Cleans up memory for node and its sub-nodes.
 * @param node
 */
void purge_node(Node *node) {
    if (!node) return;

    switch (node->type) {
    case NODE_PROGRAM: {
        ProgNode *prog = (ProgNode *)node;
        for (int i = 0; i < prog->icount; i++) {
            purge_node(prog->items[i]);
        }
        free(prog->items);
        free(prog);
        break;
    }

    case NODE_VAR_DECL: {
        VarNode *var = (VarNode *)node;
        if (var->dtype) free(var->dtype);
        if (var->name) free(var->name);
        purge_node(var->init);
        free(var);
        break;
    }

    case NODE_FUNC_DECL: {
        FuncNode *fn = (FuncNode *)node;
        if (fn->dtype) free(fn->dtype);
        if (fn->name) free(fn->name);
        for (int i = 0; i < fn->pcount; i++) {
            purge_node(fn->params[i]);
        }
        free(fn->params);
        purge_node(fn->body);
        free(fn);
        break;
    }

    case NODE_BLOCK: {
        BlockNode *blk = (BlockNode *)node;
        for (int i = 0; i < blk->icount; i++) {
            purge_node(blk->items[i]);
        }
        free(blk->items);
        free(blk);
        break;
    }

    case NODE_STATEMENT: {
        StmtNode *stmt = (StmtNode *)node;
        switch (stmt->type) {
        // For these types, the associated node is stored in u.simple.expr.
        case STMT_RETURN:
        case STMT_EXPRESSION:
        case STMT_CALL:
        case STMT_COMPOUND:   purge_node(stmt->u.simple.expr); break;

        case STMT_ASSIGNMENT:
            if (stmt->u.assignment.lhs) free(stmt->u.assignment.lhs);
            purge_node(stmt->u.assignment.rhs);
            break;

        case STMT_IF:
            purge_node(stmt->u.if_stmt.condition);
            purge_node(stmt->u.if_stmt.then_stmt);
            purge_node(stmt->u.if_stmt.else_stmt);
            break;

        case STMT_WHILE:
        case STMT_DO_WHILE:
            purge_node(stmt->u.while_stmt.condition);
            purge_node(stmt->u.while_stmt.body);
            break;

        case STMT_FOR:
            purge_node(stmt->u.for_stmt.init);
            purge_node(stmt->u.for_stmt.condition);
            purge_node(stmt->u.for_stmt.post);
            purge_node(stmt->u.for_stmt.body);
            break;

        case STMT_BREAK:
        case STMT_CONTINUE:
        case STMT_NULL:
            // Nothing to free.
            break;

        default: break;
        }
        free(stmt);
        break;
    }

    case NODE_EXPRESSION: {
        ExprNode *expr = (ExprNode *)node;
        switch (expr->type) {
        case EXPR_CONSTANT:
            // No dynamic memory in a constant.
            break;

        case EXPR_STRING:
            if (expr->u.str) free(expr->u.str);
            break;

        case EXPR_VAR:
            if (expr->u.name) free(expr->u.name);
            break;

        case EXPR_UNARY: purge_node(expr->u.unary.operand); break;

        case EXPR_BINARY:
            purge_node(expr->u.binary.left);
            purge_node(expr->u.binary.right);
            break;

        case EXPR_CONDITIONAL:
            purge_node(expr->u.conditional.condition);
            purge_node(expr->u.conditional.true_expr);
            purge_node(expr->u.conditional.false_expr);
            break;

        case EXPR_ASSIGNMENT:
            // In expression assignments both LHS and RHS are nodes.
            purge_node(expr->u.assignment.lhs);
            purge_node(expr->u.assignment.rhs);
            break;

        case EXPR_CALL:
            purge_node(expr->u.call.callee);
            if (expr->u.call.args) {
                for (int i = 0; i < expr->u.call.acount; i++) {
                    purge_node(expr->u.call.args[i]);
                }
                free(expr->u.call.args);
            }
            break;

        default: break;
        }
        free(expr);
        break;
    }

    case NODE_TYPE:
        // If NODE_TYPE ever allocates additional dynamic memory, free it here.
        free(node);
        break;

    default: free(node); break;
    }
}
