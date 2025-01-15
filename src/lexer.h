#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

typedef enum {
    TOK_TYPE_SPEC, //
    TOK_TYPE_MOD,  //
    TOK_TYPE_QF,   //
    TOK_ACC_SPEC,  //
    TOK_FUNC,      //
    TOK_COND,      //
    TOK_LOOPS,     //
    TOK_MEM,       //
    TOK_ERROR,     //
    TOK_MODULE,    //
    TOK_IMPORT,    //
    TOK_ASYNC,     //
    TOK_IDENT,     // variable name
    TOK_NUMBER,    // numbers
    TOK_STRING,    // string literal
    TOK_CHAR,      // character literal
    TOK_ASSIGN,    // '='
    TOK_PLUS,      // '+'
    TOK_MINUS,     // '-'
    TOK_ASTERISK,  // '*'
    TOK_AMPERSAND, // '&'
    TOK_AT,        // '@'
    TOK_HASH,      // '#'
    TOK_FSLASH,    // '/'
    TOK_BSLASH,    // '\'
    TOK_DOT,       // '.'
    TOK_COLON,     // ':'
    TOK_SEMI,      // ';'
    TOK_LT,        // '<'
    TOK_GT,        // '>'
    TOK_MOD,       // '%'
    TOK_ARROW,     // '->'
    TOK_EQ,        // '=='
    TOK_NEQ,       // '!='
    TOK_GEQ,       // '>='
    TOK_LEQ,       // '<='
    TOK_ADD_ASN,   // '+='
    TOK_SUB_ASN,   // '-='
    TOK_DIV_ASN,   // '/='
    TOK_MUL_ASN,   // '*='
    TOK_MOD_ASN,   // '%='
    TOK_POW,       // '**'
    TOK_INCR,      // '++'
    TOK_DECR,      // '--'
    TOK_LPAREN,    // '('
    TOK_RPAREN,    // ')'
    TOK_LBRACE,    // '{'
    TOK_RBRACE,    // '}'
    TOK_LBRACKET,  // '['
    TOK_RBRACKET,  // ']'
    TOK_LANGLE,    // '<'
    TOK_RANGLE,    // '>'
    TOK_COMMA,     // ','
    TOK_UNKNOWN,   // invalid or unknown token
    TOK_EOF,       // end of file
} TokenType;

typedef struct {
    TokenType type;
    char value[64];
    int line;
    int col;
} Token;

typedef struct {
    Token *tokens;
    int size;
} TokenList;

typedef struct {
    char *buffer;
    int pos;
    int line;
    int col;
} Lexer;

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param path File name with path.
 * @return
 */
Lexer *make_lexer(const char *path);

/**
 * @brief Scan the source and return the tokens array.
 * @param src Sourcecode file path.
 * @return
 */
TokenList *scan(const char *src);

/**
 * @brief Print formatted token to the terminal.
 * @param token
 */
void print_token(Token token);

/**
 * @brief Checks if at the end of source file.
 * @param token
 * @return
 */
bool token_eof(Token token);

/**
 * @brief Cleanup allocated for lexer and it's `buffer`.
 * @param lexer
 */
void purge_lexer(Lexer *lexer);

/**
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_tokenlist(TokenList *list);

#endif
