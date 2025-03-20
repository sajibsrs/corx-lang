#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "utils.h"

/*********************************************
 * Function Declarations
 *********************************************/

static int precedence(TokType type);

static Type *parse_type_specifier(Parser *prs);
static TypeKind tok_to_type_kind(TokType type);
static StorageClass tok_to_sc(TokType type);
static DeclaratorInfo process_declarator(Parser *prs, Type *base_type);
static Decl *parse_declaration(Parser *prs);
static ConstType tok_to_const_type(TokType type);

static UnOp tok_to_un_op(TokType type);
static BinOp tok_to_bin_op(TokType type);

static Token *peek(Parser *prs);
static Token *peek_next(Parser *prs);
static Token *advance(Parser *prs);
static Token *expect(Parser *prs, TokType type, const char *msg);

static Block *parse_block(Parser *prs);
static Stmt *parse_stmt(Parser *prs);

static Expr *parse_expr(Parser *prs, int min_prec);
static Expr *parse_primary_expr(Parser *prs);
static Expr *create_const_expr(ConstType const_type, Token *tok);
static Expr *create_var_expr(const char *name, int line);
static Expr *create_unary_expr(UnOp op, Expr *u, int line);
static Expr *create_binary_expr(BinOp op, Expr *left, Expr *right, int line);
static Expr *create_call_expr(Expr *func, Expr **args, unsigned arg_count, int line);

static bool isbinop(TokType type);
static bool isunop(TokType type);
static bool isasnop(TokType type);
static bool istypetok(TokType type);
static bool isacctok(TokType type);

static void errexitinfo(Parser *prs, const char *msg);

/*********************************************
 * Data Definitions
 *********************************************/

static const int precedence_table[] = {
    [T_EQ]       = 1,  //
    [T_QMARK]    = 3,  //
    [T_OR]       = 5,  //
    [T_AND]      = 10, //
    [T_EQEQ]     = 30, //
    [T_NTEQ]     = 30, //
    [T_LT]       = 35, //
    [T_LTEQ]     = 35, //
    [T_GT]       = 35, //
    [T_GTEQ]     = 35, //
    [T_PLUS]     = 45, //
    [T_MINUS]    = 45, //
    [T_ASTERISK] = 50, //
    [T_FSLASH]   = 50, //
    [T_MODULUS]  = 50, //
};

static const UnOp unop_table[] = {
    [T_MINUS]     = UOP_NEG,  //
    [T_BANG]      = UOP_NOT,  //
    [T_AMPERSAND] = UOP_ADDR, //
    [T_ASTERISK]  = UOP_DEREF //
};

static const BinOp binop_table[] = {
    [T_PLUS]     = BOP_ADD,  //
    [T_MINUS]    = BOP_SUB,  //
    [T_ASTERISK] = BOP_MUL,  //
    [T_FSLASH]   = BOP_DIV,  //
    [T_MODULUS]  = BOP_MOD,  //
    [T_EQEQ]     = BOP_EQ,   //
    [T_NTEQ]     = BOP_NEQ,  //
    [T_LT]       = BOP_LT,   //
    [T_LTEQ]     = BOP_LTEQ, //
    [T_GT]       = BOP_GT,   //
    [T_GTEQ]     = BOP_GTEQ, //
    [T_AND]      = BOP_AND,  //
    [T_OR]       = BOP_OR    //
};

/*********************************************
 * Helper Functions
 *********************************************/

static void errexitinfo(Parser *prs, const char *msg) {
    Token *next = peek(prs);
    if (next) {
        fprintf(stderr, "Error: %s at '%s' (line %d)\n", msg, ttypestr[next->type], next->line);
    } else {
        fprintf(stderr, "Error: %s at end of input\n", msg);
    }
    exit(1);
}

static Token *expect(Parser *prs, TokType type, const char *msg) {
    Token *next = peek(prs);
    if (!next || next->type != type) {
        errexitinfo(prs, msg);
    }
    return advance(prs);
}

/*********************************************
 * Parser Initialization
 *********************************************/

Parser *make_parser(const TokList *list) {
    Parser *prs  = malloc(sizeof(Parser));
    prs->list    = list;
    prs->pos     = -1;
    prs->current = NULL;
    return prs;
}

static Token *peek(Parser *prs) {
    return (prs->pos + 1 < prs->list->count) ? prs->list->tokens[prs->pos + 1] : NULL;
}

static Token *peek_next(Parser *prs) {
    return (prs->pos + 2 < prs->list->count) ? prs->list->tokens[prs->pos + 2] : NULL;
}

static Token *advance(Parser *prs) {
    if (prs->pos < prs->list->count - 1) {
        prs->current = prs->list->tokens[++prs->pos];
    } else {
        prs->current = NULL;
    }
    return prs->current;
}

/*********************************************
 * Type Parsing
 *********************************************/

static Type *parse_type_specifier(Parser *prs) {
    if (!istypetok(peek(prs)->type)) {
        errexitinfo(prs, "Expected type specifier");
    }

    Token *tok      = peek(prs);
    Type *type      = malloc(sizeof(Type));
    type->base.type = NODE_TYPE;
    type->base.line = tok->line;
    type->kind      = tok_to_type_kind(tok->type);
    advance(prs); // Consume the type token
    return type;
}

/*********************************************
 * Declarator Processing
 *********************************************/

static DeclaratorInfo process_declarator(Parser *prs, Type *base_type) {
    // Handle pointers
    if (peek(prs) && peek(prs)->type == T_ASTERISK) {
        advance(prs);
        Type *ptr_type      = malloc(sizeof(Type));
        ptr_type->base.type = NODE_TYPE;
        ptr_type->base.line = prs->current ? prs->current->line : 0;
        ptr_type->kind      = TY_PTR;
        ptr_type->inner     = base_type;
        return process_declarator(prs, ptr_type);
    }

    // Handle grouping parentheses
    if (peek(prs) && peek(prs)->type == T_LPAREN) {
        advance(prs);
        DeclaratorInfo inner = process_declarator(prs, base_type);
        expect(prs, T_RPAREN, "Expected ')' after declarator");
        return inner;
    }

    // Handle identifier (must come after pointers/grouping)
    DeclaratorInfo info = {0};
    if (peek(prs) && peek(prs)->type == T_IDENT) {
        info.name = peek(prs)->value ? strdup(peek(prs)->value) : NULL;
        info.type = base_type;
        advance(prs);
    } else {
        errexitinfo(prs, "Expected identifier in declarator");
    }

    // Handle function parameters AFTER identifier
    while (peek(prs) && peek(prs)->type == T_LPAREN) {
        advance(prs);

        Type **param_types   = NULL;
        char **param_names   = NULL;
        unsigned param_count = 0;

        while (peek(prs) && peek(prs)->type != T_RPAREN) {
            // Parse parameter type
            Type *param_base          = parse_type_specifier(prs);
            DeclaratorInfo param_info = process_declarator(prs, param_base);

            if (param_info.type->kind == TY_FUNC) {
                fprintf(
                    stderr, "Error: Function parameters not supported (line %d)\n",
                    param_info.type->base.line
                );
                exit(1);
            }

            param_types              = realloc(param_types, (param_count + 1) * sizeof(Type *));
            param_names              = realloc(param_names, (param_count + 1) * sizeof(char *));
            param_types[param_count] = param_info.type;
            param_names[param_count] = param_info.name;
            param_count++;

            if (peek(prs)->type != T_COMMA) break;

            advance(prs);
        }

        expect(prs, T_RPAREN, "Expected ')' after parameters");

        // Create function type wrapping previous type
        Type *func_type        = malloc(sizeof(Type));
        func_type->base.type   = NODE_TYPE;
        func_type->base.line   = prs->current->line;
        func_type->kind        = TY_FUNC;
        func_type->ret         = info.type;
        func_type->params      = param_types;
        func_type->param_count = param_count;

        // Update declarator info
        info.type        = func_type;
        info.param_names = param_names;
        info.param_count = param_count;
    }

    return info;
}

/*********************************************
 * Declaration Parsing
 *********************************************/

static Decl *parse_declaration(Parser *prs) {
    Token *next     = peek(prs);
    StorageClass sc = SC_NONE; // Parse storage class

    // TODO: Handle storage class

    // Parse base type
    Type *base_type = parse_type_specifier(prs);

    // Process declarator
    DeclaratorInfo decl_info = process_declarator(prs, base_type);

    // Build declaration
    Decl *decl      = malloc(sizeof(Decl));
    decl->base.type = NODE_DECL;
    decl->base.line = decl_info.type->base.line;
    decl->name      = decl_info.name;
    decl->type      = decl_info.type;
    decl->storage   = sc;

    if (decl_info.type->kind == TY_FUNC) {
        // Create parameters
        decl->params = malloc(decl_info.param_count * sizeof(Decl *));
        for (unsigned i = 0; i < decl_info.param_count; i++) {
            Decl *param      = malloc(sizeof(Decl));
            param->base.type = NODE_DECL;
            param->base.line = decl_info.type->base.line;
            param->name      = decl_info.param_names ? decl_info.param_names[i] : NULL;
            param->type      = decl_info.param_types ? decl_info.param_types[i] : NULL;
            param->storage   = SC_NONE;
            decl->params[i]  = param;
        }
        decl->param_count = decl_info.param_count;

        // Parse function body
        if (peek(prs)->type == T_LBRACE) {
            decl->body = parse_block(prs);
        } else {
            expect(prs, T_SCOLON, "Expected ';' after function declaration");
        }
    } else {
        // Variable initialization
        if (peek(prs)->type == T_EQ) {
            advance(prs);
            decl->init = parse_expr(prs, 0);
        }
        expect(prs, T_SCOLON, "Expected ';' after declaration");
    }
    return decl;
}

/*********************************************
 * Block Parsing
 *********************************************/

static Block *parse_block(Parser *prs) {
    Block *block      = malloc(sizeof(Block));
    block->base.type  = NODE_BLOCK;
    block->base.line  = prs->current->line;
    block->items      = NULL;
    block->item_count = 0;

    expect(prs, T_LBRACE, "Expected '{'");

    while (peek(prs) && peek(prs)->type != T_RBRACE) {
        block->items = realloc(block->items, (block->item_count + 1) * sizeof(Node *));
        if (istypetok(peek(prs)->type)) {
            block->items[block->item_count++] = (Node *)parse_declaration(prs);
        } else {
            block->items[block->item_count++] = (Node *)parse_stmt(prs);
        }
    }
    expect(prs, T_RBRACE, "Expected '}'");
    return block;
}

/*********************************************
 * Statement Parsing
 *********************************************/

static Stmt *parse_stmt(Parser *prs) {
    Token *next     = peek(prs);
    Stmt *stmt      = malloc(sizeof(Stmt));
    stmt->base.type = NODE_STMT;
    stmt->base.line = next->line;

    switch (next->type) {
    case T_LBRACE:
        stmt->stmt_type      = STMT_COMPOUND;
        stmt->compound.block = parse_block(prs);
        break;
    case T_RETURN: {
        advance(prs);
        stmt->stmt_type    = STMT_RETURN;
        stmt->_return.expr = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';' after return");
        break;
    }
    default:
        stmt->stmt_type = STMT_EXPR;
        stmt->expr      = parse_expr(prs, 0);
        expect(prs, T_SCOLON, "Expected ';' after expression");
        break;
    }
    return stmt;
}

/*********************************************
 * Expression Parsing
 *********************************************/

static Expr *parse_expr(Parser *prs, int min_prec) {
    Expr *left  = parse_primary_expr(prs);
    Token *next = peek(prs);

    while (isbinop(next->type) && precedence(next->type) >= min_prec) {
        advance(prs); // consume operator
        Expr *right = parse_expr(prs, precedence(next->type) + 1);
        left        = create_binary_expr(tok_to_bin_op(next->type), left, right, next->line);
        next        = peek(prs);
    }

    return left;
}

static Expr *parse_primary_expr(Parser *prs) {
    Token *next = peek(prs);

    if (next->type == T_IDENT) {
        // printf("%s:%s\n", ttypestr[next->type], next->value);
        Expr *var = create_var_expr(next->value, next->line);
        advance(prs);
        if (peek(prs) && peek(prs)->type == T_LPAREN) {
            advance(prs);
            Expr **args        = NULL;
            unsigned arg_count = 0;

            while (peek(prs) && peek(prs)->type != T_RPAREN) {
                args              = realloc(args, (arg_count + 1) * sizeof(Expr *));
                args[arg_count++] = parse_expr(prs, 0);
                if (peek(prs)->type != T_COMMA) break;
                advance(prs);
            }
            expect(prs, T_RPAREN, "Expected ')'");
            return create_call_expr(var, args, arg_count, next->line);
        }
        return var;
    }

    else if (next->type == T_INT_LIT || next->type == T_FLOAT_LIT || next->type == T_STRING_LIT) {
        Expr *c = create_const_expr(tok_to_const_type(next->type), next);
        advance(prs);

        return c;
    }

    else if (next->type == T_LPAREN) {
        advance(prs);
        Expr *inner = parse_expr(prs, 0);
        expect(prs, T_RPAREN, "Expected ')'");

        return inner;
    }

    else if (isunop(next->type)) {
        UnOp op = tok_to_un_op(next->type);
        advance(prs);
        Expr *operand = parse_primary_expr(prs);

        return create_unary_expr(op, operand, next->line);
    }
    errexitinfo(prs, "Unexpected token in expression");
    exit(1);
}

/*********************************************
 * Expression Creation
 *********************************************/

static Expr *create_const_expr(ConstType const_type, Token *tok) {
    Expr *expr          = malloc(sizeof(Expr));
    expr->base.type     = NODE_EXPR;
    expr->base.line     = tok->line;
    expr->type          = EXPR_CONST;
    expr->constant.type = const_type;

    switch (const_type) {
    case CONST_INT:   expr->constant.ival = atoi(tok->value); break;
    case CONST_FLOAT: expr->constant.fval = atof(tok->value); break;
    case CONST_STR:   expr->constant.sval = strdup(tok->value); break;
    default:          break;
    }

    return expr;
}

static Expr *create_var_expr(const char *name, int line) {
    Expr *expr          = malloc(sizeof(Expr));
    expr->base.type     = NODE_EXPR;
    expr->base.line     = line;
    expr->type          = EXPR_VAR;
    expr->variable.name = strdup(name);

    return expr;
}

static Expr *create_unary_expr(UnOp op, Expr *operand, int line) {
    Expr *expr       = malloc(sizeof(Expr));
    expr->base.type  = NODE_EXPR;
    expr->base.line  = line;
    expr->type       = EXPR_UNARY;
    expr->unary.op   = op;
    expr->unary.expr = operand;

    return expr;
}

static Expr *create_binary_expr(BinOp op, Expr *left, Expr *right, int line) {
    Expr *expr         = malloc(sizeof(Expr));
    expr->base.type    = NODE_EXPR;
    expr->base.line    = line;
    expr->type         = EXPR_BINARY;
    expr->binary.op    = op;
    expr->binary.left  = left;
    expr->binary.right = right;

    return expr;
}

static Expr *create_call_expr(Expr *func, Expr **args, unsigned arg_count, int line) {
    Expr *expr           = malloc(sizeof(Expr));
    expr->base.type      = NODE_EXPR;
    expr->base.line      = line;
    expr->type           = EXPR_CALL;
    expr->call.func      = func;
    expr->call.args      = args;
    expr->call.arg_count = arg_count;

    return expr;
}

/*********************************************
 * Operator Helpers
 *********************************************/

static TypeKind tok_to_type_kind(TokType type) {
    switch (type) {
    case T_INT:    return TY_INT;
    case T_FLOAT:  return TY_FLOAT;
    case T_CHAR:   return TY_CHAR;
    case T_STRING: return TY_STRING;
    default:       return TY_INT;
    }
}

static ConstType tok_to_const_type(TokType type) {
    switch (type) {
    case T_INT_LIT:    return CONST_INT;
    case T_FLOAT_LIT:  return CONST_FLOAT;
    case T_STRING_LIT: return CONST_STR;
    default:           return CONST_INT;
    }
}

static UnOp tok_to_un_op(TokType type) {
    return unop_table[type];
}

static BinOp tok_to_bin_op(TokType type) {
    return binop_table[type];
}

static int precedence(TokType type) {
    return precedence_table[type];
}

static bool isbinop(TokType type) {
    switch (type) {
    case T_PLUS:
    case T_MINUS:
    case T_ASTERISK:
    case T_FSLASH:
    case T_MODULUS:
    case T_EQEQ:
    case T_NTEQ:
    case T_LT:
    case T_LTEQ:
    case T_GT:
    case T_GTEQ:
    case T_AND:
    case T_OR:       return true;
    default:         return false;
    }
}

static bool isunop(TokType type) {
    switch (type) {
    case T_MINUS:
    case T_BANG:
    case T_AMPERSAND:
    case T_ASTERISK:  return true;
    default:          return false;
    }
}

static bool isasnop(TokType type) {
    return type == T_EQ;
}

static bool istypetok(TokType type) {
    return type == T_INT || type == T_FLOAT || type == T_CHAR || type == T_STRING;
}

static bool isacctok(TokType type) {
    return type == T_STATIC || type == T_EXTERN;
}

static StorageClass tok_to_sc(TokType type) {
    switch (type) {
    case T_STATIC: return SC_STATIC;
    case T_EXTERN: return SC_EXTERN;
    default:       return SC_NONE;
    }
}

/*********************************************
 * Program Parsing
 *********************************************/

Program *parse_program(Parser *prs) {
    Program *prog    = malloc(sizeof(Program));
    prog->base.type  = NODE_PROGRAM;
    prog->base.line  = 0;
    prog->decls      = NULL;
    prog->decl_count = 0;

    while (peek(prs) && peek(prs)->type != T_EOF) {
        prog->decls = realloc(prog->decls, (prog->decl_count + 1) * sizeof(Decl *));
        prog->decls[prog->decl_count++] = parse_declaration(prs);
    }

    return prog;
}

/*********************************************
 * Cleanup Functions
 *********************************************/
void purge_stmt(Stmt *stmt);
void purge_block(Block *block);
void purge_decl(Decl *decl) ;

void purge_expr(Expr *expr) {
    if (!expr) return;

    switch (expr->type) {
    case EXPR_VAR: free(expr->variable.name); break;
    case EXPR_CONST:
        if (expr->constant.type == CONST_STR && expr->constant.sval) {
            free(expr->constant.sval);
        }
        break;
    case EXPR_UNARY: purge_expr(expr->unary.expr); break;
    case EXPR_BINARY:
        purge_expr(expr->binary.left);
        purge_expr(expr->binary.right);
        break;
    case EXPR_CALL:
        purge_expr(expr->call.func);
        for (unsigned i = 0; i < expr->call.arg_count; i++) {
            purge_expr(expr->call.args[i]);
        }
        free(expr->call.args);
        break;
    }
    free(expr);
}

void purge_stmt(Stmt *stmt) {
    if (!stmt) return;

    switch (stmt->stmt_type) {
    case STMT_COMPOUND: purge_block(stmt->compound.block); break;
    case STMT_RETURN:   purge_expr(stmt->_return.expr); break;
    case STMT_EXPR:     purge_expr(stmt->expr); break;
    }
    free(stmt);
}

void purge_block(Block *block) {
    if (!block) return;

    for (unsigned i = 0; i < block->item_count; i++) {
        Node *node = block->items[i];
        switch (node->type) {
        case NODE_DECL: purge_decl((Decl *)node); break;
        case NODE_STMT: purge_stmt((Stmt *)node); break;
        default:        break;
        }
    }
    free(block->items);
    free(block);
}

void purge_type(Type *type) {
    if (!type) return;

    switch (type->kind) {
    case TY_PTR: purge_type(type->inner); break;
    case TY_FUNC:
        purge_type(type->ret);
        for (unsigned i = 0; i < type->param_count; i++) {
            purge_type(type->params[i]);
        }
        free(type->params);
        break;
    default: break;
    }
    free(type);
}

void purge_decl(Decl *decl) {
    if (!decl) return;

    free(decl->name);
    purge_type(decl->type);

    // Free parameters
    for (unsigned i = 0; i < decl->param_count; i++) {
        purge_decl(decl->params[i]);
    }
    free(decl->params);

    purge_expr(decl->init);
    purge_block(decl->body);
    free(decl);
}

void purge_program(Program *prog) {
    if (!prog) return;

    for (unsigned i = 0; i < prog->decl_count; i++) {
        purge_decl(prog->decls[i]);
    }
    free(prog->decls);
    free(prog);
}

void purge_parser(Parser *prs) {
    if (prs) {
        // TODO: Free any parser-specific resources
        free(prs);
    }
}

/*********************************************
 * AST Print helper functions
 *********************************************/

void print_ast(Node *node);

static void print_node(Node *node, int indent);
static void print_indent(int indent);
static void print_program(Program *prog, int indent);
static void print_decl(Decl *decl, int indent);
static void print_type(Type *type, int indent);
static void print_block(Block *block, int indent);
static void print_stmt(Stmt *stmt, int indent);
static void print_expr(Expr *expr, int indent);

static const char *typekind_str(TypeKind kind);
static const char *stmttype_str(StmtType type);
static const char *binop_str(BinOp op);
static const char *unop_str(UnOp op);
static const char *consttype_str(ConstType type);

void print_ast(Node *node) {
    print_node(node, 0);
}

static void print_node(Node *node, int indent) {
    if (!node) return;

    switch (node->type) {
    case NODE_PROGRAM: print_program((Program *)node, indent); break;
    case NODE_DECL:    print_decl((Decl *)node, indent); break;
    case NODE_BLOCK:   print_block((Block *)node, indent); break;
    case NODE_STMT:    print_stmt((Stmt *)node, indent); break;
    case NODE_EXPR:
    case NODE_TYPE:
    default:           fprintf(stderr, "Unknown node type: %d\n", node->type);
    }
}

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void print_program(Program *prog, int indent) {
    print_indent(indent);
    printf("Program:\n");
    for (unsigned i = 0; i < prog->decl_count; i++) {
        print_node((Node *)prog->decls[i], indent + 1);
    }
}

static void print_decl(Decl *decl, int indent) {
    print_indent(indent);
    printf("Declaration: %s\n", decl->name ? decl->name : "(anonymous)");

    // Print type information with null check
    print_indent(indent + 1);
    printf("Type:\n");
    if (decl->type) {
        print_type(decl->type, indent + 2);
    } else {
        print_indent(indent + 2);
        printf("NULL TYPE\n");
    }

    // Print initialization only for variables
    if (decl->type && decl->type->kind != TY_FUNC && decl->init) {
        print_indent(indent + 1);
        printf("Initializer:\n");
        print_expr(decl->init, indent + 2);
    }

    // Print parameters with type validation
    if (decl->param_count > 0) {
        print_indent(indent + 1);
        printf("Parameters (%u):\n", decl->param_count);
        for (unsigned i = 0; i < decl->param_count; i++) {
            if (decl->params[i] && decl->params[i]->type) {
                print_decl(decl->params[i], indent + 2);
            } else {
                print_indent(indent + 2);
                printf("INVALID PARAMETER\n");
            }
        }
    }

    // Print function body if exists
    if (decl->body) {
        print_indent(indent + 1);
        printf("Body:\n");
        print_block(decl->body, indent + 2);
    }
}

static void print_type(Type *type, int indent) {
    print_indent(indent);
    if (!type) {
        printf("NULL TYPE\n");
        return;
    }

    printf("%s", typekind_str(type->kind));

    switch (type->kind) {
    case TY_PTR:
        printf(" to:\n");
        print_type(type->inner, indent + 1);
        break;
    case TY_FUNC:
        printf(" returning:\n");
        print_type(type->ret, indent + 1);
        if (type->param_count > 0) {
            print_indent(indent);
            printf("Parameters (%u):\n", type->param_count);
            for (unsigned i = 0; i < type->param_count; i++) {
                print_type(type->params[i], indent + 1);
            }
        }
        break;
    default: printf("\n");
    }
}

static void print_block(Block *block, int indent) {
    print_indent(indent);
    printf("Block (%u items):\n", block->item_count);
    for (unsigned i = 0; i < block->item_count; i++) {
        print_node(block->items[i], indent + 1);
    }
}

static void print_stmt(Stmt *stmt, int indent) {
    print_indent(indent);
    printf("Statement (%s):\n", stmttype_str(stmt->stmt_type));

    switch (stmt->stmt_type) {
    case STMT_COMPOUND: print_block(stmt->compound.block, indent + 1); break;
    case STMT_RETURN:
        print_indent(indent + 1);
        printf("Return:\n");
        print_expr(stmt->_return.expr, indent + 2);
        break;
    case STMT_EXPR: print_expr(stmt->expr, indent + 1); break;
    }
}

static void print_expr(Expr *expr, int indent) {
    print_indent(indent);
    printf("Expression: ");

    switch (expr->type) {
    case EXPR_CONST:
        printf("%s: ", consttype_str(expr->constant.type));
        switch (expr->constant.type) {
        case CONST_INT:   printf("%d\n", expr->constant.ival); break;
        case CONST_FLOAT: printf("%.4f\n", expr->constant.fval); break;
        case CONST_STR:
            if (expr->constant.sval) {
                printf("\"%s\"\n", expr->constant.sval);
            } else {
                printf("(null string)\n");
            }
            break;
        }
        break;

    case EXPR_VAR: printf("Variable: %s\n", expr->variable.name); break;

    case EXPR_UNARY:
        printf("Unary %s:\n", unop_str(expr->unary.op));
        print_expr(expr->unary.expr, indent + 1);
        break;

    case EXPR_BINARY:
        printf("Binary %s:\n", binop_str(expr->binary.op));
        print_expr(expr->binary.left, indent + 1);
        print_expr(expr->binary.right, indent + 1);
        break;

    case EXPR_CALL:
        printf("Call:\n");
        print_indent(indent + 1);
        printf("Function:\n");
        print_expr(expr->call.func, indent + 2);
        print_indent(indent + 1);
        printf("Arguments (%u):\n", expr->call.arg_count);
        for (unsigned i = 0; i < expr->call.arg_count; i++) {
            print_expr(expr->call.args[i], indent + 2);
        }
        break;
    }
}

static const char *typekind_str(TypeKind kind) {
    switch (kind) {
    case TY_INT:    return "int";
    case TY_FLOAT:  return "float";
    case TY_CHAR:   return "char";
    case TY_STRING: return "string";
    case TY_PTR:    return "pointer";
    case TY_FUNC:   return "function";
    default:        return "unknown";
    }
}

static const char *stmttype_str(StmtType type) {
    switch (type) {
    case STMT_COMPOUND: return "compound";
    case STMT_RETURN:   return "return";
    case STMT_EXPR:     return "expression";
    default:            return "unknown";
    }
}

static const char *binop_str(BinOp op) {
    switch (op) {
    case BOP_ADD:  return "+";
    case BOP_SUB:  return "-";
    case BOP_MUL:  return "*";
    case BOP_DIV:  return "/";
    case BOP_MOD:  return "%";
    case BOP_EQ:   return "==";
    case BOP_NEQ:  return "!=";
    case BOP_LT:   return "<";
    case BOP_LTEQ: return "<=";
    case BOP_GT:   return ">";
    case BOP_GTEQ: return ">=";
    case BOP_AND:  return "&&";
    case BOP_OR:   return "||";
    default:       return "?";
    }
}

static const char *unop_str(UnOp op) {
    switch (op) {
    case UOP_NEG:   return "-";
    case UOP_NOT:   return "!";
    case UOP_ADDR:  return "&";
    case UOP_DEREF: return "*";
    default:        return "?";
    }
}

static const char *consttype_str(ConstType type) {
    switch (type) {
    case CONST_INT:   return "int";
    case CONST_FLOAT: return "float";
    case CONST_STR:   return "string";
    default:          return "unknown";
    }
}
