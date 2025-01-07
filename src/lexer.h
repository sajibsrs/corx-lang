#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

typedef enum {
    TOK_KEYWORD,    // keywords like if, else, etc.
    TOK_IDENT,      // variable name
    TOK_NUMBER,     // numbers
    TOK_STRING,     // string literal
    TOK_CHAR,       // character literal
    TOK_ASSIGN,     // '='
    TOK_PLUS,       // '+'
    TOK_MINUS,      // '-'
    TOK_STAR,       // '*'
    TOK_AMP,        // '&'
    TOK_AT,         // '@'
    TOK_HASH,       // '#'
    TOK_FSLASH,     // '/'
    TOK_BSLASH,     // '\'
    TOK_DOT,        // '.'
    TOK_COLON,      // ':'
    TOK_SEMI,       // ';'
    TOK_LT,         // '<'
    TOK_GT,         // '>'
    TOK_MOD,        // '%'
    TOK_ARROW,      // '->'
    TOK_EQ,         // '=='
    TOK_NEQ,        // '!='
    TOK_GEQ,        // '>='
    TOK_LEQ,        // '<='
    TOK_ADD_ASSIGN, // '+='
    TOK_SUB_ASSIGN, // '-='
    TOK_DIV_ASSIGN, // '/='
    TOK_MUL_ASSIGN, // '*='
    TOK_MOD_ASSIGN, // '%='
    TOK_POW,        // '**'
    TOK_INCR,       // '++'
    TOK_DECR,       // '--'
    TOK_LPAREN,     // '('
    TOK_RPAREN,     // ')'
    TOK_LBRACE,     // '{'
    TOK_RBRACE,     // '}'
    TOK_LBRACKET,   // '['
    TOK_RBRACKET,   // ']'
    TOK_LANGLE,     // '<'
    TOK_RANGLE,     // '>'
    TOK_COMMA,      // ','
    TOK_UNKNOWN,    // invalid or unknown token
    TOK_EOF,        // end of file
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[64];
    int line;
    int col;
} Token;

typedef struct {
    char *buffer;
    int pos;
    int line;
    int col;
} Lexer;

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param fname File name with path.
 * @return
 */
Lexer *create_lexer(const char *fname);

/**
 * @brief Cleanup resources allocated for lexer and it's `buffer`.
 * @param lexer
 */
void remove_lexer(Lexer *lexer);

/**
 * @brief Fetch the next token.
 * @param lexer
 * @return
 */
Token get_next_token(Lexer *lexer);

/**
 * @brief Print formatted token to the terminal.
 * @param token
 */
void print_token(Token token);

#endif
