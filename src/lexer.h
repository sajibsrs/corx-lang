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
    char value[64]; // for storing identifier or numbers
} Token;

extern const char *input; // input string
extern int position;      // current position in the input

/**
 * @brief Load source from a file.
 * @param fname File name.
 */
void loadfile(const char *fname);

/**
 * @brief Returns token name string.
 * @param type Token type.
 * @return
 */
char *tokenstr(const TokenType type);

/**
 * @brief Print formatted token to the terminal.
 * @param token
 */
void token_print(Token token);

/**
 * @brief Returns current input character.
 * @return
 */
char current_char();

/**
 * @brief Move to a certain position in input character.
 * @param n Characters to skip from current position.
 */
void advance(int n);

/**
 * @brief Skip whitespace character and moves to next.
 * It keeps doing so until a non-whitespace character is found.
 */
void skip_whitespace();

/**
 * @brief Returns true if it's a digit false otherwise.
 * @param c Input character.
 * @return
 */
bool is_digit(char c);

/**
 * @brief Returns true if it's a letter false otherwise.
 * @param c Input character.
 * @return
 */
bool is_letter(char c);

/**
 * @brief Recognise the number, tokenize return it.
 * @return
 */
Token get_number();

/**
 * @brief Recognise the identifier, tokenize and return it.
 * @return
 */
Token get_identifier();

/**
 * @brief Recognise next token, tokenize and return it.
 * @return
 */
Token get_next_token();

#endif
