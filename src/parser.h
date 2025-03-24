#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"
#include <stdlib.h>

/* -------------------- Pre declaration -------------------- */
typedef struct Node Node;
typedef struct Type Type;
typedef struct Decl Decl;
typedef struct Stmt Stmt;
typedef struct Expr Expr;
typedef struct Block Block;
typedef struct Program Program;
typedef struct DeclInfo DeclInfo;
typedef struct Parser Parser;

/* -------------------- Base AST Structure -------------------- */
typedef enum {
    NODE_PROGRAM,
    NODE_DECL,
    NODE_BLOCK,
    NODE_STMT,
    NODE_EXPR,
    NODE_TYPE,
} NodeType;

struct Node {
    NodeType node_type;
};

/* -------------------- Type System -------------------- */
typedef enum {
    TY_VOID,
    TY_INT,
    TY_FLOAT,
    TY_CHAR,
    TY_STRING,
    TY_PTR,
    TY_FUNC,
} TypeKind;

struct Type {
    Node base;
    TypeKind type_kind;
    union {
        struct { // TY_PTR
            Type *ref;
        } ptr;
        struct { // TY_FUNC
            Type *ret;
            Type **params;
            unsigned param_count;
        } func;
    };
};

/* -------------------- Declarations -------------------- */

typedef enum {
    SC_NONE,
    SC_STATIC,
    SC_EXTERN,
    SC_THREAD,
} StgClass;

// Declarator node
struct Decl {
    Node base;      // Base node
    char *name;     // Declaration name/identifier
    Type *type;     // Declaration type
    StgClass class; // Storage class
    union {
        struct { // Function declaration
            Decl **params;
            Block *body;
            unsigned param_count;
        } func;
        struct { // Variable declaration
            Expr *init;
        } var;
    };
};

/* -------------------- Statements -------------------- */
typedef enum {
    STMT_RETURN,
    STMT_IF,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_COMPOUND,
    STMT_EXPR
} StmtType;

struct Stmt {
    Node base;
    StmtType stmt_type;
    union {
        struct { // Return
            Expr *expr;
        } _return;
        struct { // If
            Expr *cond;
            Stmt *then;
            Stmt *else_;
        } _if;
        struct { // While/Do-while
            Expr *cond;
            Stmt *body;
        } _while;
        struct { // For
            Decl *init;
            Expr *cond;
            Expr *post;
            Stmt *body;
        } _for;
        struct { // Compound
            Block *block;
        } compound;
        Expr *expr; // Expression statement
    };
};

/* -------------------- Expressions -------------------- */
typedef enum {
    EXPR_CONST,
    EXPR_VAR,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_CALL,
    EXPR_ASSIGN,
    EXPR_TERNARY,
    EXPR_CAST,
} ExprType;

typedef enum {
    CONST_INT,
    CONST_FLOAT,
    CONST_STR,
} ConstType;

typedef enum {
    BOP_ADD,    // +
    BOP_SUB,    // -
    BOP_MUL,    // *
    BOP_DIV,    // /
    BOP_MOD,    // %
    BOP_AND,    // &&
    BOP_OR,     // ||
    BOP_ASSIGN, // =
    BOP_EQ,     // ==
    BOP_NEQ,    // !=
    BOP_LT,     // <
    BOP_LTEQ,   // <=
    BOP_GT,     // >
    BOP_GTEQ,   // >=
} BinOp;

typedef enum {
    UOP_NEG,
    UOP_NOT,
    UOP_ADDR,
    UOP_DEREF,
} UnOp;

struct Expr {
    Node base;
    ExprType expr_type;
    union {
        struct { // Constant
            ConstType const_type;
            union {
                int ival;
                double fval;
                char *sval;
            };
        } constant;
        struct { // Variable
            char *name;
        } variable;
        struct { // Unary
            UnOp op;
            Expr *expr;
        } unary;
        struct { // Binary
            BinOp op;
            Expr *left;
            Expr *right;
        } binary;
        struct { // Call
            Expr *func;
            Expr **args;
            unsigned arg_count;
        } call;
        struct { // Assignment
            Expr *left;
            Expr *right;
        } assignment;
        struct { // conditional
            Expr *middle;
            Expr *left;
            Expr *right;
        } conditional;
        struct { // Cast
            Type *type;
            Expr *expr;
        } cast;
    };
};

/* -------------------- Block Structure -------------------- */
struct Block {
    Node base;
    Node **items;
    unsigned item_count;
};

/* -------------------- Program Structure -------------------- */
struct Program {
    Node base;
    Decl **decls;
    unsigned decl_count;
};

/* -------------------- Parser State -------------------- */
struct Parser {
    const TokList *list; // Token list
    int pos;             // Current position
    Token *token;        // Current token
};

struct DeclInfo {
    char *name;
    Type *type;
    struct { // For function parameters
        char **names;
        Type **types;
        unsigned count;
    } params;
};

// Parser interface
Parser *make_parser(const TokList *list);
void purge_parser(Parser *prs);

Program *parse_program(Parser *parser);

void print_ast(Node *node);

#endif
