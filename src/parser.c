#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "parser.h"

/*********************************************
 * Declarations
 *********************************************/

static Token peek(Parser *parser);
static Token peekfw(Parser *parser);
static Token advance(Parser *parser);

Node *parse_program(Parser *parser);
Node *parse_block(Parser *parser);
Node *parse_block_item(Parser *parser);
Node *parse_declaration(Parser *parser, Token type, Token name);
Node *parse_var_decl(Parser *parser, Token type, Token name);
Node *parse_func_decl(Parser *parser, Token type, Token name);
Node *parse_func_call(Parser *parser, Token funcToken);
Node *parse_params(Parser *parser, Token type, Token name);
Node *parse_for_init(Parser *parser, Token type, Token name);
Node *parse_stmt(Parser *parser);
Node *parse_expr(Parser *parser, int prec);
Node *parse_factor(Parser *parser);

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
static inline int precedence(TokenType type) {
    int len = sizeof(prectable) / sizeof(prectable[0]);
    return (type < len) ? prectable[type] : 0;
}

/*********************************************
 * Helper Functions
 *********************************************/

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
Token expect(Parser *parser, TokenType type, const char *msg) {
    Token next = peek(parser);
    if (next.type != type) {
        fprintf(
            stderr, "%s at token '%s' (ln: %d, column: %d)\n", //
            msg, next.str, next.line, next.column
        );
        exit(1);
    }

    return advance(parser);
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

// Lookup table for binary operators.
static const BinaryOperator token_binop_table[] = {
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
static const UnaryOperator token_unop_table[] = {
    [T_MINUS] = UN_MINUS, //
    [T_PLUS]  = UN_PLUS,  //
    [T_TILDE] = UN_COMPL, //
    [T_BANG]  = UN_NOT,   //
    // Other tokens default to 0 (UN_INVALID)
};

BinaryOperator token_to_binop(TokenType type) {
    // TODO: Bound check
    return token_binop_table[type];
}

UnaryOperator token_to_unop(TokenType type) {
    // TODO: Bound check
    return token_unop_table[type];
}

/*********************************************
 * Parsing Functions
 *********************************************/

/**
 * @brief Peek next token without consumption.
 * @param parser
 * @return
 */
static Token peek(Parser *parser) {
    return parser->list->tokens[parser->pos + 1];
}

/**
 * @brief Peek forward, one token ahead of 'peek' and two token ahead of
 * the current token, without consumption.
 * @param parser Pointer to current parser.
 * @return
 */
static Token peekfw(Parser *parser) {
    return parser->list->tokens[parser->pos + 2];
}

/**
 * @brief Look one token behind of the current token.
 * @param parser
 * @param pos
 * @return
 */
static Token backtrack(Parser *parser, int pos) {
    return parser->list->tokens[parser->pos - pos];
}

/**
 * @brief Consume next token and move parsers current position forward.
 * @param parser
 * @return Consumed token.
 */
static Token advance(Parser *parser) {
    Token *arr = parser->list->tokens;
    return arr[++parser->pos];
}

/**
 * @brief Parses the program.
 * @param parser
 * @return
 */
Node *parse_program(Parser *parser) {
    ProgNode *program = malloc(sizeof(ProgNode));
    if (!program) errexit("Memory allocation failed for ProgNode");

    program->base.type = NODE_PROGRAM;
    program->icount    = 0;

    int capacity   = 2;
    program->items = malloc(capacity * sizeof(Node *));
    if (!program->items) errexit("Memory allocation failed for program items");

    Token next = peek(parser);
    while (next.type != T_EOF) {
        Node *decl = NULL;

        if (next.type == T_IDENT) {
            // We assume a declaration starts with two consecutive identifiers:
            // one for the type and one for the variable or function name
            advance(parser);              // Consume first identifier (assumed type)
            Token name = advance(parser); // Consume second identifier (name)

            // Call parse_declaration with the type token and name token
            decl = parse_declaration(parser, next, name);
        } else {
            errexit("Invalid declaration in global scope");
        }

        if (!decl) errexit("Invalid declaration in global scope");

        // Resize items array if needed
        if (program->icount >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(program->items, capacity * sizeof(Node *));
            if (!new_items) {
                free(program->items);
                errexit("Memory allocation failed during resize in parse_program");
            }
            program->items = new_items;
        }
        program->items[program->icount++] = decl;
        next                              = peek(parser);
    }

    return (Node *)program;
}

/**
 * @brief Parses declarations.
 * @param parser
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_declaration(Parser *parser, Token type, Token name) {
    if (type.type != T_IDENT || name.type != T_IDENT) {
        errexitinfo(parser, "Incorrect token types in declaration");
    }

    if (peek(parser).type == T_LPAREN) {
        return parse_func_decl(parser, type, name);
    }

    return parse_var_decl(parser, type, name);
}

/**
 * @brief Parses variable declarations.
 * @param parser
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_var_decl(Parser *parser, Token type, Token name) {
    VarNode *var = malloc(sizeof(VarNode));
    if (!var) errexit("Memory allocation failed for VarNode");

    var->base.type = NODE_VAR_DECL;
    var->type      = strdup(type.str); // Duplicate string if necessary
    var->name      = strdup(name.str);
    var->init      = NULL;

    // Optional initializer:
    if (peek(parser).type == T_EQ) {
        advance(parser); // Consume '='
        // Use parse_expr (not parse_expr) to correctly parse the initializer
        var->init = parse_expr(parser, 0);
        if (!var->init) errexitinfo(parser, "Failed to parse initializer expression");
    }

    expect(parser, T_SCOLON, "Expected ';' after variable declaration");
    return (Node *)var;
}

/**
 * @brief Parses function declarations.
 * @param parser
 * @param rtype Return type.
 * @param name Identifier.
 * @return
 */
Node *parse_func_decl(Parser *parser, Token rtype, Token name) {
    expect(parser, T_LPAREN, "Expected '(' after function name");

    Node **params = malloc(2 * sizeof(Node *));
    if (!params) errexit("Memory allocation failed for function parameters");

    int pcount = 0, capacity = 2;
    Token next = peek(parser);

    if (next.type == T_IDENT && strcmp(next.str, "void") == 0) {
        // Parameter list is 'void'; consume it.
        advance(parser);
    } else {
        while (peek(parser).type != T_RPAREN) {

            Token param_type = expect(parser, T_IDENT, "Expected parameter type");
            Token param_name = expect(parser, T_IDENT, "Expected parameter identifier");
            Node *param      = parse_params(parser, param_type, param_name);

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

            if (peek(parser).type == T_COMMA) advance(parser); // Consume ','
            else break;
        }
    }
    expect(parser, T_RPAREN, "Expected ')' after parameter list");

    Node *body = NULL;

    if (peek(parser).type == T_LBRACE) {
        body = parse_block(parser);
    } else {
        expect(parser, T_SCOLON, "Expected ';' after function declaration");
    }

    FuncNode *fn = malloc(sizeof(FuncNode));
    if (!fn) errexit("Memory allocation failed for FuncNode");

    fn->base.type = NODE_FUNC_DECL;
    fn->type      = strdup(rtype.str);
    fn->name      = strdup(name.str);
    fn->params    = params;
    fn->pcount    = pcount;
    fn->body      = body;

    return (Node *)fn;
}

/**
 * @brief Parses parameters.
 * @param parser
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_params(Parser *parser, Token type, Token name) {
    VarNode *param = malloc(sizeof(VarNode));
    if (!param) errexit("Memory allocation failed for parameter");
    param->base.type = NODE_VAR_DECL; // You may define a separate NODE_PARAMETER if desired
    param->type      = strdup(type.str);
    param->name      = strdup(name.str);
    param->init      = NULL; // Parameters do not have initializers
    return (Node *)param;
}

/**
 * @brief Parses blocks.
 * @param parser
 * @return
 */
Node *parse_block(Parser *parser) {
    expect(parser, T_LBRACE, "Expected '{' to start block");
    BlockNode *block = malloc(sizeof(BlockNode));

    if (!block) errexit("Memory allocation failed for BlockNode");
    block->base.type = NODE_BLOCK;
    block->icount    = 0;

    int capacity = 2, count = 0;
    Node **items = malloc(capacity * sizeof(Node *));
    if (!items) errexit("Memory allocation failed in parse_block");

    while (peek(parser).type != T_RBRACE && peek(parser).type != T_EOF) {
        Node *item = parse_block_item(parser);

        if (!item) errexit("Failed to parse block item");

        if (count >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(items, capacity * sizeof(Node *));
            if (!new_items) {
                free(items);
                free(block);
                errexit("Memory reallocation failed in parse_block");
            }
            items = new_items;
        }
        items[count++] = item;
    }
    expect(parser, T_RBRACE, "Expected '}' at end of block");

    block->items  = items;
    block->icount = count;

    return (Node *)block;
}

/**
 * @brief Parses block items.
 * @param parser
 * @return
 */
Node *parse_block_item(Parser *parser) {
    Token next = peek(parser);

    // If the next two tokens are both identifiers, assume a declaration
    if (next.type == T_IDENT && peekfw(parser).type == T_IDENT) {
        Token type = advance(parser); // Consume type
        Token name = advance(parser); // Consume name

        return parse_declaration(parser, type, name);
    }

    return parse_stmt(parser);
}

/**
 * @brief Parses statements.
 * @param parser
 * @return
 */
Node *parse_stmt(Parser *parser) {
    Token token    = peek(parser);
    StmtNode *stmt = malloc(sizeof(StmtNode));

    if (!stmt) errexit("Memory allocation failed for StmtNode");
    stmt->base.type = NODE_STATEMENT;

    // Check for function call statement: identifier followed by '('
    if (token.type == T_IDENT && peekfw(parser).type == T_LPAREN) {
        Token funcToken = advance(parser); // Consume function name
        Node *callExpr  = parse_func_call(parser, funcToken);

        // Enforce that a semicolon terminates the function call statement
        expect(parser, T_SCOLON, "Expected ';' after function call statement");

        stmt->type          = STMT_CALL;
        stmt->u.simple.expr = callExpr;

        return (Node *)stmt;
    }

    // Handle assignment statement: identifier followed by '='
    else if (token.type == T_IDENT && peekfw(parser).type == T_EQ) {
        Token var_token = advance(parser); // Consume identifier
        advance(parser);                   // Consume '='
        Node *rhs = parse_expr(parser, 0);
        expect(parser, T_SCOLON, "Expected ';' after assignment");
        stmt->type             = STMT_ASSIGNMENT;
        stmt->u.assignment.lhs = strdup(var_token.str);
        stmt->u.assignment.rhs = rhs;
        return (Node *)stmt;
    }

    // Handle return statement
    else if (token.type == T_RETURN) {
        advance(parser); // Consume 'return'
        stmt->type          = STMT_RETURN;
        stmt->u.simple.expr = (peek(parser).type != T_SCOLON) ? parse_expr(parser, 0) : NULL;
        expect(parser, T_SCOLON, "Expected ';' after return statement");
    }

    // Handle if statement
    else if (token.type == T_IF) {
        advance(parser); // Consume 'if'
        expect(parser, T_LPAREN, "Expected '(' after if");
        stmt->u.if_stmt.condition = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "Expected ')' after if condition");
        stmt->u.if_stmt.then_stmt = parse_stmt(parser);

        if (peek(parser).type == T_ELSE) {
            advance(parser); // Consume 'else'
            stmt->u.if_stmt.else_stmt = parse_stmt(parser);
        } else {
            stmt->u.if_stmt.else_stmt = NULL;
        }
        stmt->type = STMT_IF;
    }

    // Handle compound statement (block)
    else if (token.type == T_LBRACE) {
        stmt->u.simple.expr = parse_block(parser);
        stmt->type          = STMT_COMPOUND;
    }

    // Handle while statement
    else if (token.type == T_WHILE) {
        advance(parser); // Consume 'while'
        expect(parser, T_LPAREN, "Expected '(' after while");
        stmt->u.while_stmt.condition = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "Expected ')' after while condition");
        stmt->u.while_stmt.body = parse_stmt(parser);
        stmt->type              = STMT_WHILE;
    }

    // Handle do-while statement
    else if (token.type == T_DO) {
        advance(parser); // Consume 'do'
        stmt->u.while_stmt.body = parse_stmt(parser);
        expect(parser, T_WHILE, "Expected 'while' after do");
        expect(parser, T_LPAREN, "Expected '(' after while in do-while");
        stmt->u.while_stmt.condition = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "Expected ')' after condition in do-while");
        expect(parser, T_SCOLON, "Expected ';' after do-while");
        stmt->type = STMT_DO_WHILE;
    }

    // Handle for statement.
    else if (token.type == T_FOR) {
        advance(parser); // Consume 'for'
        expect(parser, T_LPAREN, "Expected '(' after for");

        Node *init      = NULL;
        Node *condition = NULL;
        Node *post      = NULL;
        Token next      = peek(parser);

        if (next.type != T_SCOLON) {
            Token lookahead = peekfw(parser);

            if (next.type == T_IDENT && lookahead.type == T_IDENT) {
                Token typeTok = advance(parser); // Consume type identifier
                Token nameTok = advance(parser); // Consume variable name
                init          = parse_for_init(parser, typeTok, nameTok);
            } else {
                init = parse_expr(parser, 0);
            }
        }
        expect(parser, T_SCOLON, "Expected ';' after for initializer");

        next = peek(parser);
        if (next.type != T_SCOLON) {
            condition = parse_expr(parser, 0);
        }
        expect(parser, T_SCOLON, "Expected ';' after for condition");

        next = peek(parser);
        if (next.type != T_RPAREN) {
            post = parse_expr(parser, 0);
        }
        expect(parser, T_RPAREN, "Expected ')' after for clauses");

        Node *body                 = parse_stmt(parser);
        stmt->u.for_stmt.init      = init;
        stmt->u.for_stmt.condition = condition;
        stmt->u.for_stmt.post      = post;
        stmt->u.for_stmt.body      = body;
        stmt->type                 = STMT_FOR;

        return (Node *)stmt;
    }

    // Handle break statement
    else if (token.type == T_BREAK) {
        advance(parser);
        stmt->type = STMT_BREAK;
        expect(parser, T_SCOLON, "Expected ';' after break");
    }

    // Handle continue statement
    else if (token.type == T_CONTINUE) {
        advance(parser);
        stmt->type = STMT_CONTINUE;
        expect(parser, T_SCOLON, "Expected ';' after continue");
    }

    // Handle null statement
    else if (token.type == T_SCOLON) {
        advance(parser);
        stmt->type = STMT_NULL;
        // Note: We now return a null statement node, which will be printed.
    }

    // Otherwise, treat as an expression statement
    else {
        stmt->u.simple.expr = parse_expr(parser, 0);
        expect(parser, T_SCOLON, "Expected ';' after expression statement");
        stmt->type = STMT_EXPRESSION;
    }

    return (Node *)stmt;
}

/**
 * @brief Parses for loop initializers.
 * @param parser
 * @param type Data type.
 * @param name Identifier.
 * @return
 */
Node *parse_for_init(Parser *parser, Token type, Token name) {
    VarNode *var = malloc(sizeof(VarNode));
    if (!var) errexit("Memory allocation failed for VarNode (for initializer)");

    var->base.type = NODE_VAR_DECL;
    var->type      = strdup(type.str);
    var->name      = strdup(name.str);
    var->init      = NULL;

    if (peek(parser).type == T_EQ) {
        advance(parser); // Consume '='
        var->init = parse_expr(parser, 0);
        if (!var->init) errexitinfo(parser, "Failed to parse initializer expression in for-loop");
    }

    // Do not expect a semicolon here
    return (Node *)var;
}

/**
 * @brief Parses expressions.
 * @param parser
 * @param prec Minimum precedence.
 * @return
 */
Node *parse_expr(Parser *parser, int prec) {
    Node *left = parse_factor(parser);
    Token next = peek(parser);

    // Check for assignment operator (lowest precedence, e.g., precedence value 1)
    if (next.type == T_EQ && prec <= 1) {
        advance(parser);                     // Consume '='.
        Node *right = parse_expr(parser, 1); // Recursively parse right-hand side

        // Assignment node
        ExprNode *anode = malloc(sizeof(ExprNode));
        if (!anode) errexit("Memory allocation failed for assignment ExprNode");

        anode->base.type        = NODE_EXPRESSION;
        anode->type             = EXPR_ASSIGNMENT;
        anode->u.assignment.lhs = left;
        anode->u.assignment.rhs = right;

        return (Node *)anode;
    }

    // Process binary operators using precedence climbing
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        Token optoken = next;
        advance(parser); // Consume operator
        Node *right = parse_expr(parser, precedence(optoken.type) + 1);

        // Expression node
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for binary ExprNode");

        enode->base.type      = NODE_EXPRESSION;
        enode->type           = EXPR_BINARY;
        enode->u.binary.op    = token_to_binop(optoken.type);
        enode->u.binary.left  = left;
        enode->u.binary.right = right;
        left                  = (Node *)enode;
        next                  = peek(parser);
    }

    // Handle ternary conditional operator: condition ? true_expr : false_expr
    if (next.type == T_QMARK) {
        advance(parser); // Consume '?'

        Node *true_expr = parse_expr(parser, 0);
        expect(parser, T_COLON, "Expected ':' in ternary operator");
        Node *false_expr = parse_expr(parser, 0);

        // Expression node
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for conditional ExprNode");

        enode->base.type                = NODE_EXPRESSION;
        enode->type                     = EXPR_CONDITIONAL;
        enode->u.conditional.condition  = left;
        enode->u.conditional.true_expr  = true_expr;
        enode->u.conditional.false_expr = false_expr;
        left                            = (Node *)enode;
    }

    return left;
}

/**
 * @brief Parses factors.
 * @param parser
 * @return
 */
Node *parse_factor(Parser *parser) {
    Token next = peek(parser);

    if (next.type == T_IDENT) {
        advance(parser); // Consume the identifier

        // Return a variable expression
        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for variable expression");

        enode->base.type = NODE_EXPRESSION;
        enode->type      = EXPR_VAR;
        enode->u.name    = strdup(next.str);

        return (Node *)enode;
    }

    if (next.type == T_NUMBER) {
        advance(parser); // Consume number

        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for constant expression");

        enode->base.type = NODE_EXPRESSION;
        enode->type      = EXPR_CONSTANT;
        enode->u.value   = atoi(next.str);

        return (Node *)enode;
    }

    if (isunop(next.type)) {
        Token op      = advance(parser); // Consume unary operator
        Node *operand = parse_factor(parser);

        ExprNode *enode = malloc(sizeof(ExprNode));
        if (!enode) errexit("Memory allocation failed for unary expression");

        enode->base.type       = NODE_EXPRESSION;
        enode->type            = EXPR_UNARY;
        enode->u.unary.op      = token_to_unop(op.type);
        enode->u.unary.operand = operand;

        return (Node *)enode;
    }

    if (next.type == T_LPAREN) {
        advance(parser); // Consume '('

        Node *expr = parse_expr(parser, 0);
        expect(parser, T_RPAREN, "Expected ')' after expression");

        return expr;
    }

    errexitinfo(parser, "Invalid factor syntax");
    return NULL;
}

/**
 * @brief
 * @param parser
 * @param funcToken
 * @return
 */
Node *parse_func_call(Parser *parser, Token funcToken) {
    // Expect '(' after function name.
    expect(parser, T_LPAREN, "Expected '(' after function name in function call");

    int capacity = 2, arg_count = 0;
    Node **args = malloc(capacity * sizeof(Node *));
    if (!args) errexit("Memory allocation failed for function call arguments");

    // Parse argument list until ')'
    while (peek(parser).type != T_RPAREN) {
        if (arg_count >= capacity) {
            capacity *= 2;
            Node **new_args = realloc(args, capacity * sizeof(Node *));

            if (!new_args) {
                free(args);
                errexit("Memory reallocation failed for function call arguments");
            }
            args = new_args;
        }
        args[arg_count++] = parse_expr(parser, 0);

        if (peek(parser).type == T_COMMA) {
            advance(parser); // Consume comma.
        } else {
            break;
        }
    }
    expect(parser, T_RPAREN, "Expected ')' after function call arguments");

    // Create an expression node for the function call
    ExprNode *expr = malloc(sizeof(ExprNode));
    if (!expr) errexit("Memory allocation failed for function call expression");

    expr->base.type = NODE_EXPRESSION;
    expr->type      = EXPR_CALL;

    // Build the callee expression node (a variable node) for the function name.
    ExprNode *callee = malloc(sizeof(ExprNode));
    if (!callee) errexit("Memory allocation failed for function call callee");

    callee->base.type = NODE_EXPRESSION;
    callee->type      = EXPR_VAR;
    callee->u.name    = strdup(funcToken.str);

    expr->u.call.callee = (Node *)callee;
    expr->u.call.args   = args;
    expr->u.call.acount = arg_count;

    return (Node *)expr;
}

/*********************************************
 * Cleanup Functions
 *********************************************/

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
        if (var->type) free(var->type);
        if (var->name) free(var->name);
        purge_node(var->init);
        free(var);
        break;
    }

    case NODE_FUNC_DECL: {
        FuncNode *fn = (FuncNode *)node;
        if (fn->type) free(fn->type);
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

/**
 * @brief Cleans up memory allocated for parser.
 * @param parser
 */
void purge_parser(Parser *parser) {
    if (!parser) return;

    purge_node(parser->node);
    free(parser);
}
