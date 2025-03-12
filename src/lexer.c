#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"
#include "lexer.h"

#define TABLE_SIZE 256 // Hash-table size
#define MAX_CHAIN 4    // Maximum

void purge_lexer(Lexer *lexer);

// Token type to token string lookup table.
const char *ttypestr[] = {
    [T_ENUM]     = "T_ENUM",
    [T_STRUCT]   = "T_STRUCT",
    [T_CONTRACT] = "T_CONTRACT",
    [T_IDENT]    = "T_IDENT",

    // Type modifiers
    [T_TYPE]   = "T_TYPE",
    [T_INT]    = "T_INT",
    [T_FLOAT]  = "T_FLOAT",
    [T_CHAR]   = "T_CHAR",
    [T_STRING] = "T_STRING",

    // Values
    [T_INT_LIT]    = "T_INT_LIT",    //
    [T_FLOAT_LIT]  = "T_FLOAT_LIT",  //
    [T_CHAR_LIT]   = "T_CHAR_LIT",   //
    [T_STRING_LIT] = "T_STRING_LIT", //

    // Async operations
    [T_ASYNC] = "T_ASYNC",
    [T_WAIT]  = "T_WAIT",

    // Type qualifiers
    [T_CONST]    = "T_CONST",
    [T_VOLATILE] = "T_VOLATILE",
    [T_ATOMIC]   = "T_ATOMIC",

    // Access specifiers
    [T_PUBLIC]    = "T_PUBLIC",
    [T_PROTECTED] = "T_PROTECTED",
    [T_PRIVATE]   = "T_PRIVATE",

    // Storage class
    [T_STATIC] = "T_STATIC",
    [T_EXTERN] = "T_EXTERN",
    [T_THREAD] = "T_THREAD",

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
    [T_EOF]     = "T_EOF",
};

typedef struct {
    const char *kw;
    TokType type;
} KWElement;

typedef struct {
    KWElement elems[MAX_CHAIN];
    int count;
} KWBucket;

// keyword hash-table
static KWBucket kwtable[TABLE_SIZE] = {0};

/**
 * @brief Adds new keyword entry to hash-table.
 * @param kw Keyword string.
 * @param type Token type.
 */
void add_keyword(const char *kw, TokType type) {
    unsigned idx  = hashfnv(kw, TABLE_SIZE);
    KWBucket *bkt = &kwtable[idx];

    if (bkt->count < MAX_CHAIN) {
        bkt->elems[bkt->count].kw   = kw;
        bkt->elems[bkt->count].type = type;
        bkt->count++;
    } else {
        printf("Warning: Hash table overflow at index %u\n", idx);
    }
}

/**
 * @brief Initialize keyword hash-table.
 */
void make_kwtable() {
    // type modifiers
    add_keyword("type", T_TYPE);

    // async
    add_keyword("async", T_ASYNC);
    add_keyword("wait", T_WAIT);

    // type qualifiers
    add_keyword("const", T_CONST);
    add_keyword("volatile", T_VOLATILE);
    add_keyword("atomic", T_ATOMIC);

    // access specifier
    add_keyword("public", T_PUBLIC);
    add_keyword("protected", T_PROTECTED);
    add_keyword("private", T_PRIVATE);

    // storage class
    add_keyword("static", T_STATIC);
    add_keyword("extern", T_EXTERN);
    add_keyword("thread", T_THREAD);

    // types
    add_keyword("int", T_INT);
    add_keyword("float", T_FLOAT);
    add_keyword("char", T_CHAR);
    add_keyword("string", T_STRING);
    add_keyword("enum", T_ENUM);
    add_keyword("struct", T_STRUCT);
    add_keyword("contract", T_CONTRACT);

    // conditions
    add_keyword("if", T_IF);
    add_keyword("else", T_ELSE);
    add_keyword("switch", T_SWITCH);
    add_keyword("case", T_CASE);
    add_keyword("default", T_DEFAULT);
    add_keyword("break", T_BREAK);
    add_keyword("continue", T_CONTINUE);

    // loops
    add_keyword("do", T_DO);
    add_keyword("while", T_WHILE);
    add_keyword("for", T_FOR);
    add_keyword("foreach", T_FOREACH);
    add_keyword("in", T_IN);

    // module
    add_keyword("module", T_MODULE);
    add_keyword("import", T_IMPORT);
    add_keyword("from", T_FROM);

    // function
    add_keyword("return", T_RETURN);

    // memory operations
    add_keyword("new", T_NEW);
    add_keyword("null", T_NULL);
    add_keyword("sizeof", T_SIZEOF);
    add_keyword("this", T_THIS);
    add_keyword("purge", T_PURGE);

    // others
    add_keyword("error", T_ERROR);
}

/**
 * @brief Finds a keyword from the hash-table.
 * @param kw Key to search.
 * @return
 */
KWElement *search_keyword(const char *kw) {
    unsigned idx  = hashfnv(kw, TABLE_SIZE);
    KWBucket *bkt = &kwtable[idx];

    for (int i = 0; i < bkt->count; i++) {
        if (strcmp(bkt->elems[i].kw, kw) == 0) {
            return &bkt->elems[i];
        }
    }
    return NULL; // Not found
}

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param path File name with path.
 * @return
 */
Lexer *make_lexer(const char *path) {
    Lexer *lexer = malloc(sizeof(Lexer));
    if (!lexer) errexit("lexer allocation failed");

    struct stat st;
    if (stat(path, &st) == -1) errexit("failed to get stats");

    long fsize  = st.st_size;
    lexer->col  = 1;
    lexer->pos  = -1;
    lexer->line = 1;

    FILE *file = fopen(path, "r");
    if (!file) errexit("failed to open file");

    // allocate memory for the file
    lexer->buffer = malloc(fsize + 1);
    if (!lexer->buffer) errexit("buffer allocation failed");

    // read the file into buffer
    fread(lexer->buffer, 1, fsize, file);
    lexer->buffer[fsize] = '\0';

    // close file
    fclose(file);

    return lexer;
}

/**
 * @brief Move to a certain position in input character.
 * @param lexer
 * @param n Characters to skip from current position.
 * @param movecol Should `lexer` col update or not.
 */
static void advance(Lexer *lexer, int n, bool movecol) {
    lexer->pos += n;
    if (movecol) lexer->col += n;
}

/**
 * @brief Peek next input from lexer buffer.
 * @param lexer
 * @return
 */
static char peekfw1(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 1];
}

/**
 * @brief Peek ahead of next input from lexer buffer.
 * @param lexer
 * @return
 */
static char peekfw2(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 2];
}

static char peekfw3(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 3];
}

/**
 * @brief Skip space and tab and moves to next.
 * It keeps doing so until other character is found.
 * @param lexer
 */
static void skip_blank(Lexer *lexer) {
    char next = peekfw1(lexer);
    while (isblank(next)) {
        advance(lexer, 1, true);
        next = peekfw1(lexer);
    }
}

static Token *new_token(Lexer *lexer, TokType type, int len, bool movecol) {
    advance(lexer, len, movecol);

    Token *token = malloc(sizeof(Token));
    token->type  = type;
    token->value = NULL;
    token->line  = lexer->line;
    token->col   = lexer->col - len;

    return token;
}

/**
 * @brief Recognise the number, tokenize return it.
 * @param lexer
 * @return
 */
static Token *scan_number(Lexer *lexer) {
    Token *token = malloc(sizeof(Token));
    token->type  = T_INT_LIT;
    token->value = NULL;
    token->line  = lexer->line;
    token->col   = lexer->col;

    size_t cap = 64;
    char *buf  = malloc(cap);
    size_t len = 0;
    char next  = peekfw1(lexer);

    while (isdigit(next) || next == '.' || next == '_') {
        if (next == '.') token->type = T_FLOAT_LIT;

        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = next;
        advance(lexer, 1, true);
        next = peekfw1(lexer);
    }

    buf[len]     = '\0';
    token->value = buf;
    return token;
}

/**
 * @brief Recognise the identifier, tokenize and return it.
 * @param lexer
 * @return
 */
static Token *scan_identifier(Lexer *lexer) {
    Token *token = malloc(sizeof(Token));
    token->type  = T_IDENT;
    token->value = NULL;
    token->line  = lexer->line;
    token->col   = lexer->col;

    size_t cap = 64;
    char *buf  = malloc(cap);
    size_t len = 0;
    char c     = peekfw1(lexer);

    while (isalnum(c) || c == '_') {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = c;
        advance(lexer, 1, true);
        c = peekfw1(lexer);
    }
    buf[len] = '\0';

    // Check for keywords
    KWElement *kw = search_keyword(buf);
    if (kw) {
        token->type = kw->type;
        free(buf); // Don't need value for keywords
    } else {
        token->value = buf;
    }

    return token;
}

/**
 * @brief Handle string literal.
 * @param lexer
 * @return
 */
static Token *scan_string(Lexer *lexer) {
    Token *token = malloc(sizeof(Token));
    token->type  = T_STRING_LIT;
    token->value = NULL;
    token->line  = lexer->line;
    token->col   = lexer->col;

    advance(lexer, 1, true); // Skip opening quote
    size_t cap = 64;
    char *buf  = malloc(cap);
    size_t len = 0;
    char next  = peekfw1(lexer);

    while (next != '"' && next != '\0') {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = next;
        advance(lexer, 1, true);
        next = peekfw1(lexer);
    }

    if (next == '"') {
        advance(lexer, 1, true);
        buf[len]     = '\0';
        token->value = buf;
    } else {
        free(buf);
        token->type = T_UNKNOWN;
    }

    return token;
}

/**
 * @brief Handle character literal.
 * @param lexer
 * @return
 */
static Token *scan_character(Lexer *lexer) {
    Token *token = malloc(sizeof(Token));
    token->type  = T_CHAR_LIT;
    token->value = NULL;
    token->line  = lexer->line;
    token->col   = lexer->col;

    advance(lexer, 1, true); // Skip opening quote
    char next = peekfw1(lexer);

    if (next != '\'') {
        token->value    = malloc(2);
        token->value[0] = next;
        token->value[1] = '\0';
        advance(lexer, 1, true);

        if (peekfw1(lexer) != '\'') {
            free(token->value);
            token->value = NULL;
            token->type  = T_UNKNOWN;
        } else {
            advance(lexer, 1, true);
        }
    } else {
        token->type = T_UNKNOWN;
    }

    return token;
}

/**
 * @brief Skip single and multi-line comments.
 * @param lexer
 */
static void scan_comment(Lexer *lexer) {
    char next  = peekfw1(lexer);
    char ptk   = peekfw1(lexer);
    int buffsz = strlen(lexer->buffer);

    if (next == '/' && ptk == '*') { // multi-line comment
        advance(lexer, 2, true);     // skip '/*'
        while (lexer->pos < buffsz) {
            if (peekfw1(lexer) == '*' && peekfw1(lexer) == '/') {
                advance(lexer, 2, true); // skip '*/'
                return;
            }
            if (peekfw1(lexer) == '\n') {
                lexer->line++;
                lexer->col = 1;
                advance(lexer, 1, false);
            } else {
                advance(lexer, 1, true);
            }
        }
    } else if (next == '/' && ptk == '/') { // single-line comment
        advance(lexer, 2, true);            // skip '//'
        while (lexer->pos < buffsz && peekfw1(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    } else if (next == '#') {    // hash-style comment
        advance(lexer, 1, true); // skip '#'
        while (lexer->pos < buffsz && peekfw1(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    }
}

/**
 * @brief Fetch the next token.
 * @param lexer
 * @return
 */
static Token *scan_next(Lexer *lexer) {
    skip_blank(lexer);
    char next = peekfw1(lexer);

    if (next == '\n' || next == '\r') {
        lexer->line++;
        lexer->col = 1; // reset for new line
        advance(lexer, 1, false);
        return scan_next(lexer); // recursively get the next token
    }

    // handle comments
    if ((next == '/' && (peekfw2(lexer) == '*' || peekfw2(lexer) == '/')) || next == '#') {
        scan_comment(lexer);     // skip the comment
        return scan_next(lexer); // recursively get the next token after the comment
    }

    if (isalpha(next) || next == '_') return scan_identifier(lexer);
    if (isdigit(next)) return scan_number(lexer);
    if (next == '"') return scan_string(lexer);
    if (next == '\'') return scan_character(lexer);

    // Triple-character Operators
    if (next == '<' && peekfw2(lexer) == '<' && peekfw3(lexer) == '=')
        return new_token(lexer, T_LSHIFTEQ, 3, true);
    if (next == '>' && peekfw2(lexer) == '>' && peekfw3(lexer) == '=')
        return new_token(lexer, T_RSHIFTEQ, 3, true);

    // Double-character Operators
    if (next == '=' && peekfw2(lexer) == '=') return new_token(lexer, T_EQEQ, 2, true);
    if (next == '!' && peekfw2(lexer) == '=') return new_token(lexer, T_NTEQ, 2, true);
    if (next == '<' && peekfw2(lexer) == '=') return new_token(lexer, T_LTEQ, 2, true);
    if (next == '>' && peekfw2(lexer) == '=') return new_token(lexer, T_GTEQ, 2, true);
    if (next == '+' && peekfw2(lexer) == '=') return new_token(lexer, T_PLUSEQ, 2, true);
    if (next == '-' && peekfw2(lexer) == '=') return new_token(lexer, T_MINUSEQ, 2, true);
    if (next == '*' && peekfw2(lexer) == '=') return new_token(lexer, T_MULEQ, 2, true);
    if (next == '/' && peekfw2(lexer) == '=') return new_token(lexer, T_DIVEQ, 2, true);
    if (next == '%' && peekfw2(lexer) == '=') return new_token(lexer, T_MODEQ, 2, true);
    if (next == '&' && peekfw2(lexer) == '&') return new_token(lexer, T_AND, 2, true);
    if (next == '|' && peekfw2(lexer) == '|') return new_token(lexer, T_OR, 2, true);
    if (next == '<' && peekfw2(lexer) == '<') return new_token(lexer, T_LSHIFT, 2, true);
    if (next == '>' && peekfw2(lexer) == '>') return new_token(lexer, T_RSHIFT, 2, true);
    if (next == '&' && peekfw2(lexer) == '=') return new_token(lexer, T_ANDEQ, 2, true);
    if (next == '^' && peekfw2(lexer) == '=') return new_token(lexer, T_XOREQ, 2, true);
    if (next == '|' && peekfw2(lexer) == '=') return new_token(lexer, T_OREQ, 2, true);

    // Single-character Operators
    if (next == '>') return new_token(lexer, T_GT, 1, true);
    if (next == '=') return new_token(lexer, T_EQ, 1, true);
    if (next == '+') return new_token(lexer, T_PLUS, 1, true);
    if (next == '-') return new_token(lexer, T_MINUS, 1, true);
    if (next == '*') return new_token(lexer, T_ASTERISK, 1, true);
    if (next == '/') return new_token(lexer, T_FSLASH, 1, true);
    if (next == ';') return new_token(lexer, T_SCOLON, 1, true);
    if (next == '\\') return new_token(lexer, T_BSLASH, 1, true);
    if (next == '&') return new_token(lexer, T_AMPERSAND, 1, true);
    if (next == '?') return new_token(lexer, T_QMARK, 1, true);
    if (next == '|') return new_token(lexer, T_PIPE, 1, true);
    if (next == '^') return new_token(lexer, T_CARET, 1, true);
    if (next == '(') return new_token(lexer, T_LPAREN, 1, true);
    if (next == ')') return new_token(lexer, T_RPAREN, 1, true);
    if (next == '{') return new_token(lexer, T_LBRACE, 1, true);
    if (next == '}') return new_token(lexer, T_RBRACE, 1, true);
    if (next == '[') return new_token(lexer, T_LBRACKET, 1, true);
    if (next == ']') return new_token(lexer, T_RBRACKET, 1, true);
    if (next == '%') return new_token(lexer, T_MODULUS, 1, true);
    if (next == '!') return new_token(lexer, T_BANG, 1, true);
    if (next == '@') return new_token(lexer, T_AT, 1, true);
    if (next == '~') return new_token(lexer, T_TILDE, 1, true);
    if (next == '.') return new_token(lexer, T_DOT, 1, true);
    if (next == ':') return new_token(lexer, T_COLON, 1, true);
    if (next == ',') return new_token(lexer, T_COMMA, 1, true);
    if (next == '\0') return new_token(lexer, T_EOF, 1, true);

    return new_token(lexer, T_UNKNOWN, 1, true);
}

/**
 * @brief Scan the source code and return the tokens array.
 * @param lexers
 * @return
 */
TokList *scan(const char *src) {
    make_kwtable();

    int capacity = 64;
    int count    = 0;

    Lexer *lexer   = make_lexer(src);
    Token **tokens = malloc(capacity * sizeof(Token *));
    Token *tok;

    do {
        tok             = scan_next(lexer); // Assume next() returns Token*
        tokens[count++] = tok;

        if (count >= capacity) {
            capacity *= 2;
            tokens = realloc(tokens, capacity * sizeof(Token *));
        }
    } while (tok->type != T_EOF);

    // Shrink to fit
    tokens = realloc(tokens, count * sizeof(Token *));

    TokList *list = malloc(sizeof(TokList));
    list->tokens  = tokens;
    list->count   = count;

    purge_lexer(lexer);
    return list;
}

/**
 * @brief Cleanup resources allocated for lexer and it's `buffer`.
 * @param lexer
 */
void purge_lexer(Lexer *lexer) {
    if (lexer) {
        free(lexer->buffer);
        free(lexer);
    }
}

/**
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_toklist(TokList *list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        Token *t = list->tokens[i];
        if (t->value) free(t->value);
        free(t);
    }

    free(list->tokens);
    free(list);
}

/**
 * @brief Print formatted token to the terminal.
 * @param list
 */
void print_toklist(const TokList *list) {
    printf("Scanned %d tokens:\n\n", list->count);

    Token *token;

    for (int i = 0; i < list->count; i++) {
        token = list->tokens[i];
        printf(
            "%-16s %-10s typ:%-4d lin:%-4d col:%d\n", //
            ttypestr[token->type], token->value, token->type, token->line, token->col
        );
    }
}
