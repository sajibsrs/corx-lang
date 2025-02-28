#ifndef _PARSER_H
#define _PARSER_H

#include "lexer.h"

typedef enum {
    CT_INT,   // Integer constant (e.g., 42)
    CT_FLOAT, // Floating-point constant (e.g., 3.14)
    CT_CHAR,  // Character constant (e.g., 'A')
    CT_STRING // String literal (e.g., "hello")
} ConstType;

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
    EXPR_CONSTANT, // All constants (int, float, string, etc.)
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
} UnOps;

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
} BinOps;

// Base node structure.
typedef struct {
    NodeType type;
    int line;
} Node;

// Program node.
typedef struct {
    Node base;
    Node **items;
    int icount;
} ProgNode;

// Variable node.
typedef struct {
    Node base;   // Base node
    char *dtype; // Data type
    char *name;  // Identifier
    Node *init;  // Optional initializer
} VarNode;

// Function node.
typedef struct {
    Node base;     // Base node type.
    char *dtype;   // Return type
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
        char *name; // Variable name

        struct {             // For constant value
            ConstType ctype; // Type of the constant
            union {
                int ival;   // Integer
                float fval; // Float
                char cval;  // Character
                char *sval; // String (stored in data section)
            } u;
        } con;

        struct {
            UnOps op;
            Node *operand;
        } unary;

        struct {
            BinOps op;
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

void purge_node(Node *node);

// Parser.
typedef struct {
    const TokList *list; // Token array
    int pos;             // Current position
    Token token;         // Current token
    Node *node;          // Root node
} Parser;

extern const char *ntypestr[];

Parser *make_parser(const TokList *list);
Node *parse_program(Parser *parser);

void purge_parser(Parser *parser);

#endif
