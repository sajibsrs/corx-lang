#ifndef _LEXER_H
#define _LEXER_H

#include <stdbool.h>

// Token types
typedef enum {
    // Type modifiers
    T_TYPE, //

    // Async
    T_ASYNC, //
    T_WAIT,  //

    // Type qualifiers
    T_CONST,  //
    T_ATOMIC, //

    // Access specifiers
    T_EXTERNAL,
    T_INTERNAL,
    T_RESTRICT,

    // Types
    T_ENUM,     //
    T_STRUCT,   //
    T_CONTRACT, //
    T_IDENT,    // Identifier
    T_NUMBER,   // Numbers
    T_STRING,   // String literal
    T_CHAR,     // Character literal

    // Conditions
    T_IF,      //
    T_ELSE,    //
    T_SWITCH,  //
    T_CASE,    //
    T_DEFAULT, //

    T_BREAK,
    T_CONTINUE,

    // Loops
    T_DO,
    T_WHILE,
    T_FOR,
    T_FOREACH,
    T_IN,

    // Module
    T_MODULE, // Group
    T_IMPORT, //
    T_FROM,   //

    // Function
    T_RETURN,

    // Memory operations
    T_NEW,
    T_NULL, // Pointer non-value
    T_SIZEOF,
    T_THIS,
    T_PURGE,

    // Operations
    T_EQ,        // '='
    T_PLUS,      // '+'
    T_MINUS,     // '-'
    T_BANG,      // '!'
    T_TILDE,     // '~'
    T_ASTERISK,  // '*'
    T_AMPERSAND, // '&'
    T_QMARK,     // "?"
    T_PIPE,      // "|"
    T_CARET,     // "^"
    T_AT,        // '@'
    T_HASH,      // '#'
    T_FSLASH,    // '/'
    T_BSLASH,    // '\\'
    T_DOT,       // '.'
    T_COLON,     // ':'
    T_SCOLON,    // ';'
    T_LT,        // '<'
    T_GT,        // '>'
    T_MODULUS,   // '%'
    T_ARROW,     // '->'
    T_EQEQ,      // '=='
    T_NTEQ,      // '!='
    T_GTEQ,      // '>='
    T_LTEQ,      // '<='
    T_PLUSEQ,    // '+='
    T_MINUSEQ,   // '-='
    T_DIVEQ,     // '/='
    T_MULEQ,     // '*='
    T_MODEQ,     // '%='

    T_AND, // '&&'
    T_OR,  // '||'

    T_LSHIFT, // '<<'
    T_RSHIFT, // '>>'

    // Bitwise assign
    T_LSHIFTEQ, // "<<="
    T_RSHIFTEQ, // ">>="
    T_ANDEQ,    // "&="
    T_XOREQ,    // "^="
    T_OREQ,     // "|="

    // Grouping
    T_LPAREN,   // '('
    T_RPAREN,   // ')'
    T_LBRACE,   // '{'
    T_RBRACE,   // '}'
    T_LBRACKET, // '['
    T_RBRACKET, // ']'
    T_LANGLE,   // '<'
    T_RANGLE,   // '>'
    T_COMMA,    // ','

    // Quotes
    T_SQUOTE, // '
    T_DQUOTE, // "
    T_BQUOTE, // `

    T_ERROR,   //
    T_UNKNOWN, // Unknown token
    T_INVALID, //
    T_EOF,     // End of file
} TokenType;

extern const char *ttypestr[];

typedef struct {
    TokenType type;
    char str[64];
    int line;
    int column;
} Token;

typedef struct {
    Token *tokens;
    int count;
} TokenList;

typedef struct {
    char *buffer;
    int pos;
    int line;
    int column;
} Lexer;

/**
 * @brief Scan the source and return the tokens array.
 * @param src Sourcecode file path.
 * @return
 */
TokenList *scan(const char *src);

/**
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_tlist(TokenList *list);

#endif
