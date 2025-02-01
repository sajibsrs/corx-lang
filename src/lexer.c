#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

// Token type to token string lookup table.
const char *ttypestr[] = {
    // Type specifiers
    [T_INT]   = "T_INT",
    [T_FLOAT] = "T_FLOAT",

    [T_BOOL]     = "T_BOOL",
    [T_ENUM]     = "T_ENUM",
    [T_STRUCT]   = "T_STRUCT",
    [T_CONTRACT] = "T_CONTRACT",
    [T_NUMBER]   = "T_NUMBER",
    [T_STRING]   = "T_STRING",
    [T_CHAR]     = "T_CHAR",
    [T_VOID]     = "T_VOID",

    // Type modifiers
    [T_TYPE] = "T_TYPE",

    // Async operations
    [T_ASYNC] = "T_ASYNC",
    [T_WAIT]  = "T_WAIT",

    // Type qualifiers
    [T_CONST]  = "T_CONST",
    [T_ATOMIC] = "T_ATOMIC",

    // Access specifiers
    [T_EXTERNAL] = "T_EXTERNAL",
    [T_INTERNAL] = "T_INTERNAL",
    [T_RESTRICT] = "T_RESTRICT",

    // Conditions
    [T_IF]      = "T_IF",
    [T_ELSE]    = "T_ELSE",
    [T_SWITCH]  = "T_SWITCH",
    [T_CASE]    = "T_CASE",
    [T_DEFAULT] = "T_DEFAULT",

    [T_BREAK]    = "T_BREAK",
    [T_CONTINUE] = "T_CONTINUE",

    // Loops
    [T_DO]      = "T_DO",
    [T_WHILE]   = "T_WHILE",
    [T_FOR]     = "T_FOR",
    [T_FOREACH] = "T_FOREACH",
    [T_IN]      = "T_IN",

    // Module
    [T_MODULE] = "T_MODULE",
    [T_IMPORT] = "T_IMPORT",
    [T_FROM]   = "T_FROM",

    // Functions
    [T_RETURN] = "T_RETURN",

    // Memory operations
    [T_NEW]    = "T_NEW",
    [T_NULL]   = "T_NULL",
    [T_SIZEOF] = "T_SIZEOF",
    [T_THIS]   = "T_THIS",
    [T_PURGE]  = "T_PURGE",

    // Identifiers
    [T_IDENT] = "T_IDENT",

    // Operators
    [T_EQ]        = "T_EQ",
    [T_PLUS]      = "T_PLUS",
    [T_MINUS]     = "T_MINUS",
    [T_BANG]      = "T_BANG",      // "!"
    [T_TILDE]     = "T_TILDE",     // "~"
    [T_ASTERISK]  = "T_ASTERISK",  //
    [T_AMPERSAND] = "T_AMPERSAND", //
    [T_QMARK]     = "T_QMARK",     // "?"
    [T_PIPE]      = "T_PIPE",      // "|"
    [T_CARET]     = "T_CARET",     // "^"
    [T_AT]        = "T_AT",
    [T_HASH]      = "T_HASH",
    [T_FSLASH]    = "T_FSLASH",
    [T_BSLASH]    = "T_BSLASH",
    [T_DOT]       = "T_DOT",
    [T_COLON]     = "T_COLON",
    [T_SCOLON]    = "T_SCOLON",
    [T_LT]        = "T_LT",
    [T_GT]        = "T_GT",
    [T_MODULUS]   = "T_MODULUS",
    [T_ARROW]     = "T_ARROW",
    [T_EQEQ]      = "T_EQEQ",
    [T_NTEQ]      = "T_NTEQ",
    [T_GTEQ]      = "T_GTEQ",
    [T_LTEQ]      = "T_LTEQ",
    [T_PLUSEQ]    = "T_PLUSEQ",
    [T_MINUSEQ]   = "T_MINUSEQ",
    [T_DIVEQ]     = "T_DIVEQ",
    [T_MULEQ]     = "T_MULEQ",
    [T_MODEQ]     = "T_MODEQ",

    [T_AND] = "T_AND", // "&&"
    [T_OR]  = "T_OR",  // "||"

    [T_LSHIFT] = "T_LSHIFT", // "<<"
    [T_RSHIFT] = "T_RSHIFT", // ">>"

    // Bitwise assign
    [T_LSHIFTEQ] = "T_LSHIFTEQ", // "<<="
    [T_RSHIFTEQ] = "T_RSHIFTEQ", // ">>="
    [T_ANDEQ]    = "T_ANDEQ",    // "&="
    [T_XOREQ]    = "T_XOREQ",    // "^="
    [T_OREQ]     = "T_OREQ",     // "|="

    // Grouping
    [T_LPAREN]   = "T_LPAREN",
    [T_RPAREN]   = "T_RPAREN",
    [T_LBRACE]   = "T_LBRACE",
    [T_RBRACE]   = "T_RBRACE",
    [T_LBRACKET] = "T_LBRACKET",
    [T_RBRACKET] = "T_RBRACKET",
    [T_LANGLE]   = "T_LANGLE",
    [T_RANGLE]   = "T_RANGLE",
    [T_COMMA]    = "T_COMMA",

    // Quotes
    [T_SQUOTE] = "T_SQUOTE",
    [T_DQUOTE] = "T_DQUOTE",
    [T_BQUOTE] = "T_BQUOTE",

    // Miscellaneous
    [T_ERROR]   = "T_ERROR",
    [T_UNKNOWN] = "T_UNKNOWN",
    [T_INVALID] = "T_INVALID",
    [T_EOF]     = "T_EOF",
};

#define HASH_SIZE 256

/**
 * @brief Returns hash for token string (Fowler–Noll–Vo hash).
 * @param str Token string.
 * @return
 */
unsigned int hashfnv(const char *str) {
    unsigned int hash = 2166136261u; // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)(*str); // XOR with byte
        hash *= 16777619u;             // FNV prime
        str++;
    }
    return hash % HASH_SIZE;
}

// keyword mapper
KWMap *kwmap[HASH_SIZE] = {NULL};

void kwmap_add(TokenType type, const char *token); // forward declaration

/**
 * @brief Initialize keyword hashmap.
 */
void init_kwmap() {
    for (int i = 0; i < HASH_SIZE; i++) {
        kwmap[i] = NULL; // make buckets empty
    }

    // type modifiers
    kwmap_add(T_TYPE, "type");

    // async
    kwmap_add(T_ASYNC, "async");
    kwmap_add(T_WAIT, "wait");

    // type qualifiers
    kwmap_add(T_CONST, "const");
    kwmap_add(T_ATOMIC, "atomic");

    // access specifier
    kwmap_add(T_EXTERNAL, "external");
    kwmap_add(T_INTERNAL, "internal");
    kwmap_add(T_RESTRICT, "restrict");

    // type specifiers
    kwmap_add(T_INT, "int");
    kwmap_add(T_FLOAT, "float");

    kwmap_add(T_BOOL, "bool");
    kwmap_add(T_ENUM, "enum");
    kwmap_add(T_STRUCT, "struct");
    kwmap_add(T_CONTRACT, "contract");
    kwmap_add(T_STRING, "string");
    kwmap_add(T_CHAR, "char");
    kwmap_add(T_VOID, "void");

    // conditions
    kwmap_add(T_IF, "if");
    kwmap_add(T_ELSE, "else");
    kwmap_add(T_SWITCH, "switch");
    kwmap_add(T_CASE, "case");
    kwmap_add(T_DEFAULT, "default");
    kwmap_add(T_BREAK, "break");
    kwmap_add(T_CONTINUE, "continue");

    // loops
    kwmap_add(T_DO, "do");
    kwmap_add(T_WHILE, "while");
    kwmap_add(T_FOR, "for");
    kwmap_add(T_FOREACH, "foreach");
    kwmap_add(T_IN, "in");

    // module
    kwmap_add(T_MODULE, "module");
    kwmap_add(T_IMPORT, "import");
    kwmap_add(T_FROM, "from");

    // function
    kwmap_add(T_RETURN, "return");

    // memory operations
    kwmap_add(T_NEW, "new");
    kwmap_add(T_NULL, "null");
    kwmap_add(T_SIZEOF, "sizeof");
    kwmap_add(T_THIS, "this");
    kwmap_add(T_PURGE, "purge");

    // others
    kwmap_add(T_ERROR, "error");
}

/**
 * @brief Adds new keyword entry to hashmap.
 * @param type Token type.
 * @param token Token string.
 */
void kwmap_add(TokenType type, const char *token) {
    unsigned int idx = hashfnv(token);
    KWMap *entry     = malloc(sizeof(KWMap));
    if (!entry) {
        perror("Memory allocation failed");
        exit(1);
    }

    strncpy(entry->token, token, sizeof(entry->token) - 1);
    entry->token[sizeof(entry->token) - 1] = '\0';

    entry->type = type;

    entry->next = kwmap[idx]; // move current head
    kwmap[idx]  = entry;      // insert at head
}

/**
 * @brief Finds a keyword from the hashmap.
 * @param key Key to search.
 * @return
 */
KWMap *search_kwmap(const char *key) {
    unsigned int idx = hashfnv(key);
    KWMap *current   = kwmap[idx];

    while (current) {
        if (strcmp(current->token, key) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * @brief Removes and cleanups hashmap.
 */
void purge_kwmap() {
    for (int i = 0; i < HASH_SIZE; i++) {
        KWMap *current = kwmap[i];

        while (current) {
            KWMap *temp = current;
            current     = current->next;
            free(temp);
        }
        kwmap[i] = NULL;
    }
}

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param path File name with path.
 * @return
 */
Lexer *make_lexer(const char *path) {
    Lexer *lexer = malloc(sizeof(Lexer));
    if (!lexer) {
        perror("Memory allocation error");
        exit(1);
    }

    lexer->pos    = 0;
    lexer->line   = 1;
    lexer->column = 1;

    FILE *file = fopen(path, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    // get length of the file
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // allocate memory for the file
    lexer->buffer = malloc(fsize + 1);
    if (!lexer->buffer) {
        perror("Memory allocation error");
        exit(1);
    }

    // read the file into buffer
    fread(lexer->buffer, 1, fsize, file);
    lexer->buffer[fsize] = '\0';

    // close file
    fclose(file);

    return lexer;
}

/**
 * @brief Returns current input character.
 * @param lexer
 * @return
 */
static char current(Lexer *lexer) {
    return lexer->buffer[lexer->pos];
}

/**
 * @brief Move to a certain position in input character.
 * @param lexer
 * @param n Characters to skip from current position.
 * @param movecol Should `lexer` column update or not.
 */
static void advance(Lexer *lexer, int n, bool movecol) {
    lexer->pos += n;
    if (movecol) lexer->column += n;
}

/**
 * @brief Peek next input from lexer buffer.
 * @param lexer
 * @return
 */
static char peek(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 1];
}

/**
 * @brief Peek ahead of next input from lexer buffer.
 * @param lexer
 * @return
 */
static char peeknext(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 2];
}

/**
 * @brief Skip space and tab and moves to next.
 * It keeps doing so until other character is found.
 * @param lexer
 */
static void skipblank(Lexer *lexer) {
    char cin = current(lexer);
    while (isblank(cin)) {
        advance(lexer, 1, true);
        cin = current(lexer);
    }
}

/**
 * @brief Recognise the number, tokenize return it.
 * @param lexer
 * @return
 */
static Token number(Lexer *lexer) {
    Token token;
    token.type = T_NUMBER;

    char cin  = current(lexer);
    int idx   = 0;
    int tscol = lexer->column; // token start column

    // Read digits
    while (isdigit(cin) || cin == '.' || cin == '_') {
        token.str[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    token.str[idx] = '\0';

    token.line   = lexer->line;
    token.column = tscol;

    return token;
}

/**
 * @brief Recognise the identifier, tokenize and return it.
 * @param lexer
 * @return
 */
static Token identifier(Lexer *lexer) {
    Token token;
    token.type = T_IDENT; // default is identifier

    char cin  = current(lexer);
    int idx   = 0;
    int tscol = lexer->column; // token start column

    // continue allowing letters, digits and underscores
    while (isalnum(cin) || cin == '_') {
        token.str[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    token.str[idx] = '\0'; // null-terminate string

    // check if it's a keyword
    init_kwmap();
    KWMap *kw = search_kwmap(token.str);
    if (kw) token.type = kw->type;
    purge_kwmap();

    token.line   = lexer->line;
    token.column = tscol; // lexer->column;

    return token;
}

/**
 * @brief Handle string literal.
 * @param lexer
 * @return
 */
static Token string(Lexer *lexer) {
    Token token;
    token.type = T_STRING;

    char cin  = current(lexer);
    int idx   = 0;
    int tscol = lexer->column; // token start column

    advance(lexer, 1, true); // skip opening quote
    cin = current(lexer);

    while (cin != '"' && cin != '\0') { // end of string or EOF
        token.str[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    if (cin == '"') {
        token.str[idx] = '\0';   // null-terminate string
        advance(lexer, 1, true); // consume closing quote
    } else {
        token.type   = T_UNKNOWN;
        token.str[0] = '\0'; // empty str
    }

    token.line   = lexer->line;
    token.column = tscol;

    return token;
}

/**
 * @brief Handle character literal.
 * @param lexer
 * @return
 */
static Token character(Lexer *lexer) {
    Token token;
    token.type = T_CHAR;

    char cin = current(lexer);

    advance(lexer, 1, true);
    cin = current(lexer);

    if (cin != '\'') {
        token.str[0] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);

        if (cin == '\'') {
            advance(lexer, 1, true);
        } else {
            // if there's no closing quote
            token.type   = T_UNKNOWN;
            token.str[0] = '\0';
        }
    } else {
        // if it's just an empty quote
        token.type   = T_UNKNOWN;
        token.str[0] = '\0';
    }

    token.line   = lexer->line;
    token.column = lexer->column - 1;

    return token;
}

/**
 * @brief Skip single and multi-line comments.
 * @param lexer
 */
static void comment(Lexer *lexer) {
    char cin   = current(lexer);
    char ptk   = peek(lexer);
    int buffsz = strlen(lexer->buffer);

    if (cin == '/' && ptk == '*') { // multi-line comment
        advance(lexer, 2, true);    // skip '/*'
        while (lexer->pos < buffsz) {
            if (current(lexer) == '*' && peek(lexer) == '/') {
                advance(lexer, 2, true); // skip '*/'
                return;
            }
            if (current(lexer) == '\n') {
                lexer->line++;
                lexer->column = 1;
                advance(lexer, 1, false);
            } else {
                advance(lexer, 1, true);
            }
        }
    } else if (cin == '/' && ptk == '/') { // single-line comment
        advance(lexer, 2, true);           // skip '//'
        while (lexer->pos < buffsz && current(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    } else if (cin == '#') {     // hash-style comment
        advance(lexer, 1, true); // skip '#'
        while (lexer->pos < buffsz && current(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    }
}

/**
 * @brief Fetch the next token.
 * @param lexer
 * @return
 */
static Token next(Lexer *lexer) {
    skipblank(lexer);

    char cin = current(lexer);

    if (cin == '\n' || cin == '\r') {
        lexer->line++;
        lexer->column = 1; // reset for new line
        advance(lexer, 1, false);
        return next(lexer); // recursively get the next token
    }

    // handle comments
    if ((cin == '/' && (peek(lexer) == '*' || peek(lexer) == '/')) || cin == '#') {
        comment(lexer);     // skip the comment
        return next(lexer); // recursively get the next token after the comment
    }

    if (isalpha(cin) || cin == '_') {
        return identifier(lexer);
    }

    if (isdigit(cin)) {
        return number(lexer);
    }

    if (cin == '"') {
        return string(lexer);
    }

    if (cin == '\'') {
        return character(lexer);
    }

    /*********************************************
     * Compound operators
     *********************************************/

    // "<<="
    if (cin == '<' && peek(lexer) == '<' && peeknext(lexer) == '=') {
        advance(lexer, 3, true);
        return (Token){T_LSHIFTEQ, "<<=", lexer->line, lexer->column - 3};
    }

    // ">>="
    if (cin == '>' && peek(lexer) == '>' && peeknext(lexer) == '=') {
        advance(lexer, 3, true);
        return (Token){T_RSHIFTEQ, ">>=", lexer->line, lexer->column - 3};
    }

    // "=="
    if (cin == '=' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_EQEQ, "==", lexer->line, lexer->column - 2};
    }

    // "!="
    if (cin == '!' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){T_NTEQ, "!=", lexer->line, lexer->column - 2};
    }

    // "<="
    if (cin == '<' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){T_LTEQ, "<=", lexer->line, lexer->column - 2};
    }

    // ">="
    if (cin == '>' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){T_GTEQ, ">=", lexer->line, lexer->column - 2};
    }

    // "+="
    if (cin == '+' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_PLUSEQ, "+=", lexer->line, lexer->column - 2};
    }

    // "-="
    if (cin == '-' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MINUSEQ, "-=", lexer->line, lexer->column - 2};
    }

    // "*="
    if (cin == '*' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MULEQ, "*=", lexer->line, lexer->column - 2};
    }

    // "/="
    if (cin == '/' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_DIVEQ, "/=", lexer->line, lexer->column - 2};
    }

    // "%="
    if (cin == '%' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MODEQ, "%=", lexer->line, lexer->column - 2};
    }

    // "&&"
    if (cin == '&' && peek(lexer) == '&') {
        advance(lexer, 2, true); // consume '&&'
        return (Token){T_AND, "<<", lexer->line, lexer->column - 2};
    }

    // "||"
    if (cin == '|' && peek(lexer) == '|') {
        advance(lexer, 2, true); // consume '||'
        return (Token){T_OR, "||", lexer->line, lexer->column - 2};
    }

    // "<<"
    if (cin == '<' && peek(lexer) == '<') {
        advance(lexer, 2, true); // consume '<<'
        return (Token){T_LSHIFT, "<<", lexer->line, lexer->column - 2};
    }

    // ">>"
    if (cin == '>' && peek(lexer) == '>') {
        advance(lexer, 2, true); // consume '>>'
        return (Token){T_RSHIFT, ">>", lexer->line, lexer->column - 2};
    }

    // "&="
    if (cin == '&' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_ANDEQ, "&=", lexer->line, lexer->column - 2};
    }

    // "^="
    if (cin == '^' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_XOREQ, "^=", lexer->line, lexer->column - 2};
    }

    // "|="
    if (cin == '|' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_OREQ, "|=", lexer->line, lexer->column - 2};
    }

    /*********************************************
     * Single-character Operators
     *********************************************/

    if (cin == '<') {
        advance(lexer, 1, true);
        return (Token){T_LT, "<", lexer->line, lexer->column - 1};
    }

    if (cin == '>') {
        advance(lexer, 1, true);
        return (Token){T_GT, ">", lexer->line, lexer->column - 1};
    }

    if (cin == '=') {
        advance(lexer, 1, true);
        return (Token){T_EQ, "=", lexer->line, lexer->column - 1};
    }

    if (cin == '+') {
        advance(lexer, 1, true);
        return (Token){T_PLUS, "+", lexer->line, lexer->column - 1};
    }

    if (cin == '-') {
        advance(lexer, 1, true);
        return (Token){T_MINUS, "-", lexer->line, lexer->column - 1};
    }

    if (cin == '*') {
        advance(lexer, 1, true);
        return (Token){T_ASTERISK, "*", lexer->line, lexer->column - 1};
    }

    if (cin == '/') {
        advance(lexer, 1, true);
        return (Token){T_FSLASH, "/", lexer->line, lexer->column - 1};
    }

    if (cin == ';') {
        advance(lexer, 1, true);
        return (Token){T_SCOLON, ";", lexer->line, lexer->column - 1};
    }

    if (cin == '\\') {
        advance(lexer, 1, true);
        return (Token){T_BSLASH, "\\", lexer->line, lexer->column - 1};
    }

    if (cin == '&') {
        advance(lexer, 1, true);
        return (Token){T_AMPERSAND, "&", lexer->line, lexer->column - 1};
    }

    if (cin == '?') {
        advance(lexer, 1, true);
        return (Token){T_QMARK, "?", lexer->line, lexer->column - 1};
    }

    if (cin == '|') {
        advance(lexer, 1, true);
        return (Token){T_PIPE, "|", lexer->line, lexer->column - 1};
    }

    if (cin == '^') {
        advance(lexer, 1, true);
        return (Token){T_CARET, "^", lexer->line, lexer->column - 1};
    }

    if (cin == '(') {
        advance(lexer, 1, true);
        return (Token){T_LPAREN, "(", lexer->line, lexer->column - 1};
    }

    if (cin == ')') {
        advance(lexer, 1, true);
        return (Token){T_RPAREN, ")", lexer->line, lexer->column - 1};
    }

    if (cin == '{') {
        advance(lexer, 1, true);
        return (Token){T_LBRACE, "{", lexer->line, lexer->column - 1};
    }

    if (cin == '}') {
        advance(lexer, 1, true);
        return (Token){T_RBRACE, "}", lexer->line, lexer->column - 1};
    }

    if (cin == '[') {
        advance(lexer, 1, true);
        return (Token){T_LBRACKET, "[", lexer->line, lexer->column - 1};
    }

    if (cin == ']') {
        advance(lexer, 1, true);
        return (Token){T_RBRACKET, "]", lexer->line, lexer->column - 1};
    }

    if (cin == '%') {
        advance(lexer, 1, true);
        return (Token){T_MODULUS, "%", lexer->line, lexer->column - 1};
    }

    if (cin == '!') {
        advance(lexer, 1, true);
        return (Token){T_BANG, "!", lexer->line, lexer->column - 1};
    }

    if (cin == '@') {
        advance(lexer, 1, true);
        return (Token){T_AT, "@", lexer->line, lexer->column - 1};
    }

    if (cin == '~') {
        advance(lexer, 1, true);
        return (Token){T_TILDE, "~", lexer->line, lexer->column - 1};
    }

    if (cin == '.') {
        advance(lexer, 1, true);
        return (Token){T_DOT, ".", lexer->line, lexer->column - 1};
    }

    if (cin == ':') {
        advance(lexer, 1, true);
        return (Token){T_COLON, ":", lexer->line, lexer->column - 1};
    }

    if (cin == ',') {
        advance(lexer, 1, true);
        return (Token){T_COMMA, ",", lexer->line, lexer->column - 1};
    }

    if (cin == '\0') {
        advance(lexer, 1, true);
        return (Token){T_EOF, "EOF", lexer->line, lexer->column - 1};
    }

    advance(lexer, 1, true);

    // handle unknown token
    Token token  = {T_UNKNOWN, "", lexer->line, lexer->column - 1};
    token.str[0] = cin;
    token.str[1] = '\0';

    return token;
}

/**
 * @brief Scan the lexer and return the tokens array.
 * @param lexers
 * @return
 */
TokenList *scan(const char *src) {
    Lexer *lexer    = make_lexer(src);
    const int isize = 64;    // initial array size
    int csize       = isize; // current array size
    int idx         = 0;

    Token token;
    Token *arr = malloc(isize * sizeof(Token));
    if (!arr) {
        perror("Memory allocation error");
        exit(1);
    }

    do {
        token      = next(lexer);
        arr[idx++] = token;

        if (idx >= csize) {
            csize *= 2;
            arr = realloc(arr, csize * sizeof(Token));
            if (!arr) {
                perror("Memory allocation error");
                exit(1);
            }
        }
    } while (token.type != T_EOF);

    purge_lexer(lexer);

    // shrink to fit exact amount of tokens
    arr = realloc(arr, idx * sizeof(Token));
    if (!arr) {
        perror("Memory reallocation error");
        exit(1);
    }

    TokenList *list = malloc(sizeof(TokenList));
    if (!list) {
        perror("Memory reallocation error");
        exit(1);
    }

    list->tokens = arr;
    list->count  = idx;

    return list;
}

/**
 * @brief Cleanup resources allocated for lexer and it's `buffer`.
 * @param lexer
 */
void purge_lexer(Lexer *lexer) {
    if (!lexer) return;

    free(lexer->buffer);
    free(lexer);
}

/**
 * @brief Print formatted token to the terminal.
 * @param list
 */
void print_tokenlist(const TokenList *list) {
    printf("Scanned %d tokens:\n\n", list->count);

    Token token;
    for (int i = 0; i < list->count; i++) {
        token = list->tokens[i];
        printf(
            "%-16s %-10s typ:%-4d lin:%-4d col:%d\n", //
            ttypestr[token.type], token.str, token.type, token.line, token.column
        );
    }
}

/**
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_tokenlist(TokenList *list) {
    if (!list) return;

    free(list->tokens);
    free(list);
}
