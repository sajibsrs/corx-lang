#ifndef _LEXER_H_
#define _LEXER_H_

#include <stdbool.h>

typedef enum {
    // type modifiers
    TOK_TYPE, //

    // async
    TOK_ASYNC, //
    TOK_WAIT,  //

    // type qualifiers
    TOK_CONST,  //
    TOK_ATOMIC, //

    // access specifiers
    TOK_EXTERNAL,
    TOK_INTERNAL,
    TOK_RESTRICT,

    // type specifiers
    TOK_INT,
    TOK_FLOAT,

    TOK_BOOL,     //
    TOK_ENUM,     //
    TOK_STRUCT,   //
    TOK_CONTRACT, //
    TOK_NUMBER,   // numbers
    TOK_STRING,   // string literal
    TOK_CHAR,     // character literal
    TOK_VOID,     // type non-value

    TOK_IDENT, // identifier

    // conditions
    TOK_IF,      //
    TOK_ELSE,    //
    TOK_SWITCH,  //
    TOK_CASE,    //
    TOK_DEFAULT, //

    TOK_CONTINUE,

    // loops
    TOK_DO,
    TOK_WHILE,
    TOK_FOR,
    TOK_FOREACH,
    TOK_IN,

    // module
    TOK_MODULE, // group
    TOK_IMPORT, //
    TOK_FROM,   //

    // function
    TOK_RETURN,

    // memory operations
    TOK_NEW,
    TOK_NULL, // pointer non-value
    TOK_SIZEOF,
    TOK_THIS,
    TOK_PURGE,

    // operations
    TOK_ASSIGN,     // '='
    TOK_PLUS,       // '+'
    TOK_MINUS,      // '-'
    TOK_BANG,       // '!'
    TOK_TILDE,      // '~'
    TOK_ASTERISK,   // '*'
    TOK_AMPERSAND,  // '&'
    TOK_QUESTION,   // "?"
    TOK_PIPE,       // "|"
    TOK_CARET,      // "^"
    TOK_AT,         // '@'
    TOK_HASH,       // '#'
    TOK_FSLASH,     // '/'
    TOK_BSLASH,     // '\\'
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

    TOK_AND, // '&&'
    TOK_OR,  // '||'

    TOK_LSHIFT, // '<<'
    TOK_RSHIFT, // '>>'

    // bitwise assign
    TOK_LSHIFT_ASSIGN, // "<<="
    TOK_RSHIFT_ASSIGN, // ">>="
    TOK_AND_ASSIGN,    // "&="
    TOK_XOR_ASSIGN,    // "^="
    TOK_OR_ASSIGN,     // "|="

    // grouping
    TOK_LPAREN,   // '('
    TOK_RPAREN,   // ')'
    TOK_LBRACE,   // '{'
    TOK_RBRACE,   // '}'
    TOK_LBRACKET, // '['
    TOK_RBRACKET, // ']'
    TOK_LANGLE,   // '<'
    TOK_RANGLE,   // '>'
    TOK_COMMA,    // ','

    // quotes
    TOK_SQUOTE, // '
    TOK_DQUOTE, // "
    TOK_BQUOTE, // `

    TOK_ERROR,   //
    TOK_UNKNOWN, // unknown token
    TOK_INVALID,
    TOK_EOF, // end of file
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

typedef struct KWMap {
    char token[32];
    TokenType type;
    struct KWMap *next; // pointer for chaining
} KWMap;

/**
 * @brief Initialize keyword hashmap.
 */
void init_kwmap();

/**
 * @brief Removes and cleanups hashmap.
 */
void purge_kwmap();

/**
 * @brief Finds a keyword from the hashmap.
 * @param key Key to search.
 * @return
 */
KWMap *search_kwmap(const char *name);

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
 * @param list
 */
void print_tokenlist(const TokenList *list);

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
