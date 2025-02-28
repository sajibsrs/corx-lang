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
Node *parse_func_decl(Parser *prs, Token type, Token token);
Node *parse_var_decl(Parser *prs, Token type, Token token);
Node *parse_for_init(Parser *prs, Token type, Token token);
Node *parse_params(Parser *prs, Token type, Token token);
Node *parse_func_call(Parser *prs, Token token);
Node *parse_do_while(Parser *prs);
Node *parse_stmt(Parser *prs);
Node *parse_expr(Parser *prs, int prec);
Node *parse_factor(Parser *prs);

/*********************************************
 * Data Definitions
 *********************************************/

// Precedence lookup-table
static const int prectab[] = {
    [T_EQ]       = 1,  //
    [T_QMARK]    = 3,  //
    [T_OR]       = 5,  //
    [T_AND]      = 10, //
    [T_EQEQ]     = 30, //
    [T_NTEQ]     = 30, //
    [T_LT]       = 35, //
    [T_LTEQ]     = 35, //
    [T_GT]       = 35, //
    [T_GTEQ]     = 35,
    [T_PLUS]     = 45, //
    [T_MINUS]    = 45, //
    [T_ASTERISK] = 50, //
    [T_FSLASH]   = 50, //
    [T_MODULUS]  = 50  //
};

static inline int precedence(TokType type) {
    int len = sizeof(prectab) / sizeof(prectab[0]);
    return (type < len) ? prectab[type] : 0;
}

/*********************************************
 * Helper Functions
 *********************************************/

static void errexitinfo(Parser *prs, const char *msg) {
    Token next = peek(prs);
    fprintf(
        stderr, "Error: %s at token '%s' (ln: %d, column: %d)\n", msg, next.str, next.line, next.col
    );
    exit(1);
}

Token expect(Parser *prs, TokType type, const char *msg) {
    Token next = peek(prs);
    if (next.type != type) {
        fprintf(
            stderr, "%s at token '%s' (ln: %d, column: %d)\n", msg, next.str, next.line, next.col
        );
        exit(1);
    }
    return advance(prs);
}

/*********************************************
 * Node Functions
 *********************************************/

Parser *make_parser(const TokList *list) {
    Parser *parser = malloc(sizeof(Parser));
    if (!parser) errexit("make_parser allocation failed");

    parser->list = list;
    parser->pos  = -1;
    parser->node = NULL;
    return parser;
}

/*********************************************
 * Operator Handling
 *********************************************/

static inline bool isbinop(TokType type) {
    switch (type) {
    case T_PLUS:
    case T_MINUS:
    case T_ASTERISK:
    case T_FSLASH:
    case T_MODULUS:
    case T_AND:
    case T_OR:
    case T_EQEQ:
    case T_NTEQ:
    case T_LT:
    case T_LTEQ:
    case T_GT:
    case T_GTEQ:
    case T_EQ:       return true;
    default:         return false;
    }
}

static inline bool isunop(TokType type) {
    switch (type) {
    case T_AMPERSAND:
    case T_ASTERISK:
    case T_PLUS:
    case T_MINUS:
    case T_TILDE:
    case T_BANG:      return true;
    default:          return false;
    }
}

static inline bool isasnop(TokType type) {
    switch (type) {
    case T_EQ:
    case T_PLUSEQ:
    case T_MINUSEQ:
    case T_MULEQ:
    case T_DIVEQ:
    case T_LSHIFTEQ:
    case T_RSHIFTEQ:
    case T_ANDEQ:
    case T_XOREQ:
    case T_OREQ:     return true;
    default:         return false;
    }
}

// Token to binary
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
};

// Token to unary
static const UnOps toktoun_table[] = {
    [T_MINUS] = UN_MINUS, //
    [T_PLUS]  = UN_PLUS,  //
    [T_TILDE] = UN_COMPL, //
    [T_BANG]  = UN_NOT,   //
};

BinOps toktobin(TokType type) {
    return toktobin_table[type];
}

UnOps toktoun(TokType type) {
    return toktoun_table[type];
}

/*********************************************
 * Core Parsing Functions
 *********************************************/

static Token peek(Parser *prs) {
    return prs->list->tokens[prs->pos + 1];
}

static Token peekfw(Parser *prs) {
    return prs->list->tokens[prs->pos + 2];
}

static Token advance(Parser *prs) {
    Token *arr = prs->list->tokens;
    Token pos  = arr[++prs->pos];
    prs->token = pos;
    return pos;
}

Node *parse_program(Parser *prs) {
    ProgNode *prog = malloc(sizeof(ProgNode));
    if (!prog) errexit("ProgNode allocation failed");

    prog->base.type = NODE_PROGRAM;
    prog->icount    = 0;
    int capacity    = 2;
    prog->items     = malloc(capacity * sizeof(Node *));

    Token next = peek(prs);
    while (next.type != T_EOF) {
        if (prog->icount >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(prog->items, capacity * sizeof(Node *));
            if (!new_items) errexit("Program items realloc failed");
            prog->items = new_items;
        }

        if (next.type == T_IDENT) {
            Token type                  = advance(prs);
            Token name                  = advance(prs);
            prog->items[prog->icount++] = parse_declaration(prs, type, name);
        } else {
            errexit("Invalid declaration in global scope");
        }
        next = peek(prs);
    }
    return (Node *)prog;
}

Node *parse_declaration(Parser *prs, Token type, Token name) {
    if (type.type != T_IDENT || name.type != T_IDENT)
        errexitinfo(prs, "Invalid declaration tokens");

    return (peek(prs).type == T_LPAREN) ? parse_func_decl(prs, type, name)
                                        : parse_var_decl(prs, type, name);
}

Node *parse_var_decl(Parser *prs, Token type, Token name) {
    VarNode *var   = malloc(sizeof(VarNode));
    var->base.type = NODE_VAR_DECL;
    var->base.line = prs->token.line;
    var->dtype     = strdup(type.str);
    var->name      = strdup(name.str);
    var->init      = NULL;

    if (peek(prs).type == T_EQ) {
        advance(prs);
        var->init = parse_expr(prs, 0);
    }
    expect(prs, T_SCOLON, "Expected ';' after variable declaration");
    return (Node *)var;
}

Node *parse_func_decl(Parser *prs, Token rtype, Token name) {
    expect(prs, T_LPAREN, "Expected '(' after function name");

    Node **params = malloc(2 * sizeof(Node *));
    int pcount = 0, capacity = 2;
    Token next = peek(prs);

    if (next.type == T_IDENT && strcmp(next.str, "void") == 0) {
        advance(prs);
    } else {
        while (peek(prs).type != T_RPAREN) {
            Token ptype = expect(prs, T_IDENT, "Expected parameter type");
            Token pname = expect(prs, T_IDENT, "Expected parameter name");

            if (pcount >= capacity) {
                capacity *= 2;
                Node **new_params = realloc(params, capacity * sizeof(Node *));
                if (!new_params) errexit("Params realloc failed");
                params = new_params;
            }
            params[pcount++] = parse_params(prs, ptype, pname);

            if (peek(prs).type == T_COMMA) advance(prs);
            else break;
        }
    }
    expect(prs, T_RPAREN, "Expected ')' after parameters");

    Node *body = peek(prs).type == T_LBRACE ? parse_block(prs)
                                            : (expect(prs, T_SCOLON, "Expected ';'"), NULL);

    FuncNode *fn  = malloc(sizeof(FuncNode));
    fn->base.type = NODE_FUNC_DECL;
    fn->base.line = prs->token.line;
    fn->dtype     = strdup(rtype.str);
    fn->name      = strdup(name.str);
    fn->params    = params;
    fn->pcount    = pcount;
    fn->body      = body;
    return (Node *)fn;
}

Node *parse_params(Parser *prs, Token type, Token name) {
    VarNode *param   = malloc(sizeof(VarNode));
    param->base.type = NODE_VAR_DECL;
    param->base.line = prs->token.line;
    param->dtype     = strdup(type.str);
    param->name      = strdup(name.str);
    param->init      = NULL;
    return (Node *)param;
}

Node *parse_block(Parser *prs) {
    expect(prs, T_LBRACE, "Expected '{'");

    BlockNode *block = malloc(sizeof(BlockNode));
    block->base.type = NODE_BLOCK;
    block->base.line = prs->token.line;
    block->icount    = 0;

    int capacity = 2;
    Node **items = malloc(capacity * sizeof(Node *));

    while (peek(prs).type != T_RBRACE && peek(prs).type != T_EOF) {
        if (block->icount >= capacity) {
            capacity *= 2;
            Node **new_items = realloc(items, capacity * sizeof(Node *));
            if (!new_items) errexit("Block items realloc failed");
            items = new_items;
        }
        items[block->icount++] = parse_block_item(prs);
    }
    expect(prs, T_RBRACE, "Expected '}'");

    block->items = items;
    return (Node *)block;
}

Node *parse_block_item(Parser *prs) {
    Token next = peek(prs);
    if (next.type == T_IDENT && peekfw(prs).type == T_IDENT) {
        Token type = advance(prs);
        Token name = advance(prs);
        return parse_declaration(prs, type, name);
    }
    return parse_stmt(prs);
}

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

Node *parse_func_call(Parser *prs, Token token) {
    expect(prs, T_LPAREN, "Expected '('");

    int capacity = 2, acount = 0;
    Node **args = malloc(capacity * sizeof(Node *));
    while (peek(prs).type != T_RPAREN) {
        if (acount >= capacity) {
            capacity *= 2;
            Node **new_args = realloc(args, capacity * sizeof(Node *));
            if (!new_args) errexit("Args realloc failed");
            args = new_args;
        }
        args[acount++] = parse_expr(prs, 0);
        if (peek(prs).type == T_COMMA) advance(prs);
        else break;
    }
    expect(prs, T_RPAREN, "Expected ')'");

    ExprNode *call  = malloc(sizeof(ExprNode));
    call->base.type = NODE_EXPRESSION;
    call->base.line = prs->token.line;
    call->type      = EXPR_CALL;

    // Callee
    ExprNode *callee    = malloc(sizeof(ExprNode));
    callee->base.type   = NODE_EXPRESSION;
    callee->base.line   = token.line;
    callee->type        = EXPR_VAR;
    callee->u.name      = strdup(token.str);
    call->u.call.callee = (Node *)callee;

    call->u.call.args   = args;
    call->u.call.acount = acount;
    return (Node *)call;
}

Node *parse_stmt(Parser *prs) {
    StmtNode *stmt  = malloc(sizeof(StmtNode));
    stmt->base.type = NODE_STATEMENT;
    stmt->base.line = prs->token.line;
    Token token     = peek(prs);

    if (token.type == T_IDENT && peekfw(prs).type == T_LPAREN) { // Function call
        Token func          = advance(prs);
        stmt->type          = STMT_CALL;
        stmt->u.simple.expr = parse_func_call(prs, func);
        expect(prs, T_SCOLON, "Expected ';' after call");
    } else if (token.type == T_IDENT && peekfw(prs).type == T_EQ) { // Assignment
        Token var = advance(prs);
        advance(prs); // Consume '='
        stmt->type             = STMT_ASSIGNMENT;
        stmt->u.assignment.lhs = strdup(var.str);
        stmt->u.assignment.rhs = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';' after assignment");
    } else if (token.type == T_RETURN) { // Return
        advance(prs);
        stmt->type          = STMT_RETURN;
        stmt->u.simple.expr = (peek(prs).type != T_SCOLON) ? parse_expr(prs, 0) : NULL;
        expect(prs, T_SCOLON, "Expected ';' after return");
    } else if (token.type == T_IF) { // If
        advance(prs);
        expect(prs, T_LPAREN, "Expected '(' after if");
        stmt->u.if_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')'");
        stmt->u.if_stmt.then_stmt = parse_stmt(prs);
        stmt->type                = STMT_IF;
        if (peek(prs).type == T_ELSE) {
            advance(prs);
            stmt->u.if_stmt.else_stmt = parse_stmt(prs);
        }
    } else if (token.type == T_LBRACE) { // Compound
        stmt->type          = STMT_COMPOUND;
        stmt->u.simple.expr = parse_block(prs);
    } else if (token.type == T_WHILE) { // While
        advance(prs);
        expect(prs, T_LPAREN, "Expected '(' after while");
        stmt->u.while_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')'");
        stmt->u.while_stmt.body = parse_stmt(prs);
        stmt->type              = STMT_WHILE;
    } else if (token.type == T_DO) { // Do-While
        advance(prs);
        stmt->u.while_stmt.body = parse_stmt(prs);
        expect(prs, T_WHILE, "Expected 'while'");
        expect(prs, T_LPAREN, "Expected '('");
        stmt->u.while_stmt.condition = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')'");
        expect(prs, T_SCOLON, "Expected ';'");
        stmt->type = STMT_DO_WHILE;
    } else if (token.type == T_FOR) { // For
        advance(prs);
        expect(prs, T_LPAREN, "Expected '(' after for");
        stmt->u.for_stmt.init = (peek(prs).type != T_SCOLON) ? parse_expr(prs, 0) : NULL;
        expect(prs, T_SCOLON, "Expected ';'");
        stmt->u.for_stmt.condition = (peek(prs).type != T_SCOLON) ? parse_expr(prs, 0) : NULL;
        expect(prs, T_SCOLON, "Expected ';'");
        stmt->u.for_stmt.post = (peek(prs).type != T_RPAREN) ? parse_expr(prs, 0) : NULL;
        expect(prs, T_RPAREN, "Expected ')'");
        stmt->u.for_stmt.body = parse_stmt(prs);
        stmt->type            = STMT_FOR;
    } else if (token.type == T_BREAK || token.type == T_CONTINUE) { // Break/Continue
        stmt->type = (token.type == T_BREAK) ? STMT_BREAK : STMT_CONTINUE;
        advance(prs);
        expect(prs, T_SCOLON, "Expected ';'");
    } else if (token.type == T_SCOLON) { // Null
        stmt->type = STMT_NULL;
        advance(prs);
    } else { // Expression
        stmt->type          = STMT_EXPRESSION;
        stmt->u.simple.expr = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';'");
    }
    return (Node *)stmt;
}

Node *parse_expr(Parser *prs, int prec) {
    Node *left = parse_factor(prs);
    Token next = peek(prs);

    // Handle assignment
    if (next.type == T_EQ && prec <= 1) {
        advance(prs);
        ExprNode *anode         = malloc(sizeof(ExprNode));
        anode->base.type        = NODE_EXPRESSION;
        anode->base.line        = prs->token.line;
        anode->type             = EXPR_ASSIGNMENT;
        anode->u.assignment.lhs = left;
        anode->u.assignment.rhs = parse_expr(prs, 1);
        left                    = (Node *)anode;
        next                    = peek(prs);
    }

    // Binary operators
    while (isbinop(next.type) && precedence(next.type) >= prec) {
        Token op = next;
        advance(prs);
        ExprNode *enode       = malloc(sizeof(ExprNode));
        enode->base.type      = NODE_EXPRESSION;
        enode->base.line      = prs->token.line;
        enode->type           = EXPR_BINARY;
        enode->u.binary.op    = toktobin(op.type);
        enode->u.binary.left  = left;
        enode->u.binary.right = parse_expr(prs, precedence(op.type) + 1);
        left                  = (Node *)enode;
        next                  = peek(prs);
    }

    // Ternary operator
    if (next.type == T_QMARK) {
        advance(prs);
        ExprNode *enode                = malloc(sizeof(ExprNode));
        enode->base.type               = NODE_EXPRESSION;
        enode->base.line               = prs->token.line;
        enode->type                    = EXPR_CONDITIONAL;
        enode->u.conditional.condition = left;
        enode->u.conditional.true_expr = parse_expr(prs, 0);
        expect(prs, T_COLON, "Expected ':'");
        enode->u.conditional.false_expr = parse_expr(prs, 0);
        left                            = (Node *)enode;
    }

    return left;
}

Node *parse_factor(Parser *prs) {
    Token next = peek(prs);
    ExprNode *enode;

    switch (next.type) {
    case T_IDENT: {
        advance(prs);
        enode            = malloc(sizeof(ExprNode));
        enode->base.type = NODE_EXPRESSION;
        enode->base.line = prs->token.line;
        enode->type      = EXPR_VAR;
        enode->u.name    = strdup(next.str);
        return (Node *)enode;
    }
    case T_NUMBER: {
        advance(prs);
        enode            = malloc(sizeof(ExprNode));
        enode->base.type = NODE_EXPRESSION;
        enode->base.line = prs->token.line;
        enode->type      = EXPR_CONSTANT;

        // Check if the token represents a float (has a decimal point)
        if (strchr(next.str, '.') != NULL) {
            enode->u.con.ctype  = CT_FLOAT;
            enode->u.con.u.fval = atof(next.str); // Parse as float
        } else {
            enode->u.con.ctype  = CT_INT;
            enode->u.con.u.ival = strtol(next.str, NULL, 10); // Parse as integer
        }

        return (Node *)enode;
    }
    case T_STRING: {
        advance(prs);
        enode               = malloc(sizeof(ExprNode));
        enode->base.type    = NODE_EXPRESSION;
        enode->base.line    = prs->token.line;
        enode->type         = EXPR_CONSTANT;
        enode->u.con.ctype  = CT_STRING;
        enode->u.con.u.sval = strdup(next.str);
        return (Node *)enode;
    }
    case T_LPAREN: {
        advance(prs);
        Node *expr = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')'");
        return expr;
    }
    default: {
        if (isunop(next.type)) {
            Token op               = advance(prs);
            enode                  = malloc(sizeof(ExprNode));
            enode->base.type       = NODE_EXPRESSION;
            enode->base.line       = prs->token.line;
            enode->type            = EXPR_UNARY;
            enode->u.unary.op      = toktoun(op.type);
            enode->u.unary.operand = parse_factor(prs);
            return (Node *)enode;
        }
        errexitinfo(prs, "Unexpected token in expression");
        return NULL;
    }
    }
}

/*********************************************
 * Cleanup Functions
 *********************************************/

void purge_parser(Parser *prs) {
    if (prs) {
        purge_node(prs->node);
        free(prs);
    }
}

void purge_node(Node *node) {
    if (!node) return;

    switch (node->type) {
    case NODE_PROGRAM: {
        ProgNode *prog = (ProgNode *)node;
        for (int i = 0; i < prog->icount; i++) purge_node(prog->items[i]);
        free(prog->items);
        free(prog);
        break;
    }
    case NODE_VAR_DECL: {
        VarNode *var = (VarNode *)node;
        free(var->dtype);
        free(var->name);
        purge_node(var->init);
        free(var);
        break;
    }
    case NODE_FUNC_DECL: {
        FuncNode *fn = (FuncNode *)node;
        free(fn->dtype);
        free(fn->name);
        for (int i = 0; i < fn->pcount; i++) purge_node(fn->params[i]);
        free(fn->params);
        purge_node(fn->body);
        free(fn);
        break;
    }
    case NODE_BLOCK: {
        BlockNode *blk = (BlockNode *)node;
        for (int i = 0; i < blk->icount; i++) purge_node(blk->items[i]);
        free(blk->items);
        free(blk);
        break;
    }
    case NODE_STATEMENT: {
        StmtNode *stmt = (StmtNode *)node;
        switch (stmt->type) {
        case STMT_RETURN:
        case STMT_EXPRESSION:
        case STMT_CALL:
        case STMT_COMPOUND:   purge_node(stmt->u.simple.expr); break;
        case STMT_ASSIGNMENT:
            free(stmt->u.assignment.lhs);
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
        default: break;
        }
        free(stmt);
        break;
    }
    case NODE_EXPRESSION: {
        ExprNode *expr = (ExprNode *)node;
        switch (expr->type) {
        case EXPR_CONSTANT:
            if (expr->u.con.ctype == CT_STRING && expr->u.con.u.sval) free(expr->u.con.u.sval);
            break;
        case EXPR_VAR:   free(expr->u.name); break;
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
            purge_node(expr->u.assignment.lhs);
            purge_node(expr->u.assignment.rhs);
            break;
        case EXPR_CALL:
            purge_node(expr->u.call.callee);
            for (int i = 0; i < expr->u.call.acount; i++) purge_node(expr->u.call.args[i]);
            free(expr->u.call.args);
            break;
        }
        free(expr);
        break;
    }
    default: free(node); break;
    }
}
