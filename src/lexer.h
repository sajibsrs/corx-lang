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
    T_CONST,    //
    T_VOLATILE, //
    T_ATOMIC,   //

    // Access specifiers
    T_PUBLIC,
    T_PROTECTED,
    T_PRIVATE,

    // Storage class
    T_STATIC,
    T_EXTERN,
    T_THREAD,

    // Types
    T_INT,      //
    T_FLOAT,    //
    T_CHAR,     //
    T_STRING,   //
    T_ENUM,     //
    T_STRUCT,   //
    T_CONTRACT, //
    T_IDENT,    //

    // Values
    T_INT_LIT,    //
    T_FLOAT_LIT,  //
    T_CHAR_LIT,   //
    T_STRING_LIT, //

    // Conditions
    T_IF,      //
    T_ELSE,    //
    T_SWITCH,  //
    T_CASE,    //
    T_DEFAULT, //

    T_BREAK,    //
    T_CONTINUE, //

    // Loops
    T_DO,      //
    T_WHILE,   //
    T_FOR,     //
    T_FOREACH, //
    T_IN,      //

    // Module
    T_MODULE, // Group
    T_IMPORT, //
    T_FROM,   //

    // Function
    T_RETURN, //

    // Memory operations
    T_NEW,    //
    T_NULL,   // Pointer non-value
    T_SIZEOF, //
    T_THIS,   //
    T_PURGE,  //

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
    T_EOF,     // End of file
} TokType;

extern const char *ttypestr[];

typedef struct {
    TokType type;
    char *value;
    int line;
    int col;
} Token;

typedef struct {
    Token **tokens;
    int count;
} TokList;

typedef struct {
    char *buffer;
    int pos;
    int line;
    int col;
} Lexer;

extern const char *ttypestr[];

/**
 * @brief Scan the source and return the tokens array.
 * @param src Sourcecode file path.
 * @return
 */
TokList *scan(const char *src);

/**
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_toklist(TokList *list);
void print_toklist(const TokList *list);

#endif
