#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

typedef enum {
    TOK_IDENTIFIER, // variable name
    TOK_NUMBER,     // numbers
    TOK_ASSIGN,     // '='
    TOK_PLUS,       // '+'
    TOK_MINUS,      // '-'
    TOK_STAR,       // '*'
    TOK_FSLASH,     // '/'
    TOK_BSLASH,     // '\'
    TOK_DOT,        // '.'
    TOK_SEMICOLON,  // ';'
    TOK_KEYWORD,    // keywords like if, else etc.
    TOK_EOF,        // end of file
} TokenType;

typedef struct {
    TokenType type;
    char value[32]; // for storing identifier or numbers
} Token;

extern const char *input; // input string
extern int position;      // current position in the input

/**
 * @brief Returns current input character.
 * @return
 */
char current_char();

/**
 * @brief Move to the next input character.
 */
void advance();

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
