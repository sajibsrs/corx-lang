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
    [T_NUMBER]   = "T_NUMBER",
    [T_STRING]   = "T_STRING",
    [T_CHAR]     = "T_CHAR",

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
    add_keyword("atomic", T_ATOMIC);

    // access specifier
    add_keyword("external", T_EXTERNAL);
    add_keyword("internal", T_INTERNAL);
    add_keyword("restrict", T_RESTRICT);

    // types
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
    lexer->pos  = 0;
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
static char peek(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 1];
}

/**
 * @brief Peek ahead of next input from lexer buffer.
 * @param lexer
 * @return
 */
static char peekfw(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 2];
}

/**
 * @brief Skip space and tab and moves to next.
 * It keeps doing so until other character is found.
 * @param lexer
 */
static void skip_blank(Lexer *lexer) {
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
    int tscol = lexer->col; // token start col

    // Read digits
    while (isdigit(cin) || cin == '.' || cin == '_') {
        token.str[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    token.str[idx] = '\0';

    token.line = lexer->line;
    token.col  = tscol;

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
    int tscol = lexer->col; // token start col

    // continue allowing letters, digits and underscores
    while (isalnum(cin) || cin == '_') {
        token.str[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    token.str[idx] = '\0'; // null-terminate string

    // check if it's a keyword
    KWElement *elem = search_keyword(token.str);
    if (elem) token.type = elem->type;

    token.line = lexer->line;
    token.col  = tscol; // lexer->col;

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
    int tscol = lexer->col; // token start col

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

    token.line = lexer->line;
    token.col  = tscol;

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

    token.line = lexer->line;
    token.col  = lexer->col - 1;

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
                lexer->col = 1;
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
    skip_blank(lexer);

    char cin = current(lexer);

    if (cin == '\n' || cin == '\r') {
        lexer->line++;
        lexer->col = 1; // reset for new line
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
    if (cin == '<' && peek(lexer) == '<' && peekfw(lexer) == '=') {
        advance(lexer, 3, true);
        return (Token){T_LSHIFTEQ, "<<=", lexer->line, lexer->col - 3};
    }

    // ">>="
    if (cin == '>' && peek(lexer) == '>' && peekfw(lexer) == '=') {
        advance(lexer, 3, true);
        return (Token){T_RSHIFTEQ, ">>=", lexer->line, lexer->col - 3};
    }

    // "=="
    if (cin == '=' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_EQEQ, "==", lexer->line, lexer->col - 2};
    }

    // "!="
    if (cin == '!' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){T_NTEQ, "!=", lexer->line, lexer->col - 2};
    }

    // "<="
    if (cin == '<' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){T_LTEQ, "<=", lexer->line, lexer->col - 2};
    }

    // ">="
    if (cin == '>' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){T_GTEQ, ">=", lexer->line, lexer->col - 2};
    }

    // "++"
    if (cin == '+' && peek(lexer) == '+') {
        advance(lexer, 2, true);
        return (Token){T_INCR, "++", lexer->line, lexer->col - 2};
    }

    // "--"
    if (cin == '-' && peek(lexer) == '-') {
        advance(lexer, 2, true);
        return (Token){T_DECR, "--", lexer->line, lexer->col - 2};
    }

    // "+="
    if (cin == '+' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_PLUSEQ, "+=", lexer->line, lexer->col - 2};
    }

    // "-="
    if (cin == '-' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MINUSEQ, "-=", lexer->line, lexer->col - 2};
    }

    // "*="
    if (cin == '*' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MULEQ, "*=", lexer->line, lexer->col - 2};
    }

    // "/="
    if (cin == '/' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_DIVEQ, "/=", lexer->line, lexer->col - 2};
    }

    // "%="
    if (cin == '%' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_MODEQ, "%=", lexer->line, lexer->col - 2};
    }

    // "&&"
    if (cin == '&' && peek(lexer) == '&') {
        advance(lexer, 2, true); // consume '&&'
        return (Token){T_AND, "<<", lexer->line, lexer->col - 2};
    }

    // "||"
    if (cin == '|' && peek(lexer) == '|') {
        advance(lexer, 2, true); // consume '||'
        return (Token){T_OR, "||", lexer->line, lexer->col - 2};
    }

    // "<<"
    if (cin == '<' && peek(lexer) == '<') {
        advance(lexer, 2, true); // consume '<<'
        return (Token){T_LSHIFT, "<<", lexer->line, lexer->col - 2};
    }

    // ">>"
    if (cin == '>' && peek(lexer) == '>') {
        advance(lexer, 2, true); // consume '>>'
        return (Token){T_RSHIFT, ">>", lexer->line, lexer->col - 2};
    }

    // "&="
    if (cin == '&' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_ANDEQ, "&=", lexer->line, lexer->col - 2};
    }

    // "^="
    if (cin == '^' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_XOREQ, "^=", lexer->line, lexer->col - 2};
    }

    // "|="
    if (cin == '|' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){T_OREQ, "|=", lexer->line, lexer->col - 2};
    }

    /*********************************************
     * Single-character Operators
     *********************************************/

    if (cin == '<') {
        advance(lexer, 1, true);
        return (Token){T_LT, "<", lexer->line, lexer->col - 1};
    }

    if (cin == '>') {
        advance(lexer, 1, true);
        return (Token){T_GT, ">", lexer->line, lexer->col - 1};
    }

    if (cin == '=') {
        advance(lexer, 1, true);
        return (Token){T_EQ, "=", lexer->line, lexer->col - 1};
    }

    if (cin == '+') {
        advance(lexer, 1, true);
        return (Token){T_PLUS, "+", lexer->line, lexer->col - 1};
    }

    if (cin == '-') {
        advance(lexer, 1, true);
        return (Token){T_MINUS, "-", lexer->line, lexer->col - 1};
    }

    if (cin == '*') {
        advance(lexer, 1, true);
        return (Token){T_ASTERISK, "*", lexer->line, lexer->col - 1};
    }

    if (cin == '/') {
        advance(lexer, 1, true);
        return (Token){T_FSLASH, "/", lexer->line, lexer->col - 1};
    }

    if (cin == ';') {
        advance(lexer, 1, true);
        return (Token){T_SCOLON, ";", lexer->line, lexer->col - 1};
    }

    if (cin == '\\') {
        advance(lexer, 1, true);
        return (Token){T_BSLASH, "\\", lexer->line, lexer->col - 1};
    }

    if (cin == '&') {
        advance(lexer, 1, true);
        return (Token){T_AMPERSAND, "&", lexer->line, lexer->col - 1};
    }

    if (cin == '?') {
        advance(lexer, 1, true);
        return (Token){T_QMARK, "?", lexer->line, lexer->col - 1};
    }

    if (cin == '|') {
        advance(lexer, 1, true);
        return (Token){T_PIPE, "|", lexer->line, lexer->col - 1};
    }

    if (cin == '^') {
        advance(lexer, 1, true);
        return (Token){T_CARET, "^", lexer->line, lexer->col - 1};
    }

    if (cin == '(') {
        advance(lexer, 1, true);
        return (Token){T_LPAREN, "(", lexer->line, lexer->col - 1};
    }

    if (cin == ')') {
        advance(lexer, 1, true);
        return (Token){T_RPAREN, ")", lexer->line, lexer->col - 1};
    }

    if (cin == '{') {
        advance(lexer, 1, true);
        return (Token){T_LBRACE, "{", lexer->line, lexer->col - 1};
    }

    if (cin == '}') {
        advance(lexer, 1, true);
        return (Token){T_RBRACE, "}", lexer->line, lexer->col - 1};
    }

    if (cin == '[') {
        advance(lexer, 1, true);
        return (Token){T_LBRACKET, "[", lexer->line, lexer->col - 1};
    }

    if (cin == ']') {
        advance(lexer, 1, true);
        return (Token){T_RBRACKET, "]", lexer->line, lexer->col - 1};
    }

    if (cin == '%') {
        advance(lexer, 1, true);
        return (Token){T_MODULUS, "%", lexer->line, lexer->col - 1};
    }

    if (cin == '!') {
        advance(lexer, 1, true);
        return (Token){T_BANG, "!", lexer->line, lexer->col - 1};
    }

    if (cin == '@') {
        advance(lexer, 1, true);
        return (Token){T_AT, "@", lexer->line, lexer->col - 1};
    }

    if (cin == '~') {
        advance(lexer, 1, true);
        return (Token){T_TILDE, "~", lexer->line, lexer->col - 1};
    }

    if (cin == '.') {
        advance(lexer, 1, true);
        return (Token){T_DOT, ".", lexer->line, lexer->col - 1};
    }

    if (cin == ':') {
        advance(lexer, 1, true);
        return (Token){T_COLON, ":", lexer->line, lexer->col - 1};
    }

    if (cin == ',') {
        advance(lexer, 1, true);
        return (Token){T_COMMA, ",", lexer->line, lexer->col - 1};
    }

    if (cin == '\0') {
        advance(lexer, 1, true);
        return (Token){T_EOF, "EOF", lexer->line, lexer->col - 1};
    }

    advance(lexer, 1, true);

    // handle unknown token
    Token token  = {T_UNKNOWN, "", lexer->line, lexer->col - 1};
    token.str[0] = cin;
    token.str[1] = '\0';

    return token;
}

/**
 * @brief Scan the lexer and return the tokens array.
 * @param lexers
 * @return
 */
TokList *scan(const char *src) {
    // Initialize keyword hash-table
    make_kwtable();

    Lexer *lexer = make_lexer(src);
    int isize    = 64;    // initial array size
    int csize    = isize; // current array size
    int idx      = 0;

    Token token;

    // Allocate memory for token list
    Token *arr = malloc(isize * sizeof(Token));
    if (!arr) errexit("memory allocation error");

    do {
        token      = next(lexer);
        arr[idx++] = token;

        if (idx >= csize) {
            csize *= 2;
            arr = realloc(arr, csize * sizeof(Token));
            if (!arr) errexit("memory allocation error");
        }
    } while (token.type != T_EOF);

    purge_lexer(lexer);

    // shrink to fit exact amount of tokens
    arr = realloc(arr, idx * sizeof(Token));
    if (!arr) errexit("memory reallocation error");

    TokList *list = malloc(sizeof(TokList));
    if (!list) errexit("memory reallocation error");

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
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_tlist(TokList *list) {
    if (!list) return;

    free(list->tokens);
    free(list);
}
