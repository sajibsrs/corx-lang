#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

// Defines node types.
typedef enum {
    NODE_PROGRAM,
    NODE_VAR_DECL,
    NODE_FUNC_DECL,
    NODE_BLOCK,
    NODE_BLOCK_ITEM,
    NODE_FOR_INIT,
    NODE_EXPRESSION,
    NODE_STATEMENT,
    NODE_TYPE,
} NodeType;

// Defines statement types.
typedef enum {
    STMT_RETURN,
    STMT_EXPRESSION,
    STMT_ASSIGNMENT,
    STMT_CALL,
    STMT_IF,
    STMT_COMPOUND,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_NULL,
} StmtType;

// Defines statement types.
typedef enum {
    EXPR_CONSTANT,
    EXPR_VAR,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_CONDITIONAL,
    EXPR_ASSIGNMENT,
    EXPR_CALL,
} ExprType;

// Defines unary operators.
typedef enum {
    UN_COMPL, // ~
    UN_PLUS,  // +
    UN_MINUS, // -
    UN_NOT,   // !
} UnaryOperator;

// Defines binary operators.
typedef enum {
    BIN_ADD,
    BIN_SUB,
    BIN_MUL,
    BIN_DIV,
    BIN_MOD,
    BIN_AND,
    BIN_OR,
    BIN_EQ,
    BIN_NEQ,
    BIN_LT,
    BIN_LTEQ,
    BIN_GT,
    BIN_GTEQ,
} BinaryOperator;

// Base node structure.
typedef struct Node {
    NodeType type;
} Node;

// Program node.
typedef struct {
    Node base;
    Node **items;
    int icount;
} ProgNode;

// Variable node.
typedef struct {
    Node base;  // Base node
    char *type; // Data type
    char *name; // Identifier
    Node *init; // Optional initializer
} VarNode;

// Function node.
typedef struct {
    Node base;     // Base node type.
    char *type;    // Return type
    char *name;    // Identifier
    Node **params; // Parameters
    int pcount;    // Parameter count
    Node *body;    // optional block
} FuncNode;

// Block node.
typedef struct {
    Node base;
    Node **items;
    int icount;
} BlockNode;

// Statement node.
typedef struct {
    Node base;
    StmtType type;

    union {
        struct {
            Node *expr; // return, expression statement, or function call statement
        } simple;

        struct {
            char *lhs; // Variable
            Node *rhs; // Value
        } assignment;

        struct {
            Node *condition;
            Node *then_stmt;
            Node *else_stmt;
        } if_stmt;

        struct {
            Node *init;
            Node *condition;
            Node *post;
            Node *body;
        } for_stmt;

        struct {
            Node *condition;
            Node *body;
        } while_stmt;
    } u;
} StmtNode;

// Expression node.
typedef struct {
    Node base;
    ExprType type;

    union {
        int value;  // Constant value
        char *name; // Variable name

        struct {
            UnaryOperator op;
            Node *operand;
        } unary;

        struct {
            BinaryOperator op;
            Node *left;
            Node *right;
        } binary;

        struct {
            Node *condition;
            Node *true_expr;
            Node *false_expr;
        } conditional;

        struct {
            Node *lhs; // Variable
            Node *rhs;
        } assignment;

        struct {
            Node *callee; // Function being called
            Node **args;
            int acount;
        } call;
    } u;
} ExprNode;

// Parser.
typedef struct {
    const TokenList *list; // Token array
    int pos;               // Current position
    Node *node;            // Root node
} Parser;

extern const char *ntypestr[];

Parser *make_parser(const TokenList *list);
Node *parse_program(Parser *parser);
void purge_parser(Parser *parser);

#endif
