#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

// Token type to token string lookup table.
const char *ttypestr[] = {
    // Type specifiers
    [TOK_INT]   = "TOK_INT",
    [TOK_FLOAT] = "TOK_FLOAT",

    [TOK_BOOL]     = "TOK_BOOL",
    [TOK_ENUM]     = "TOK_ENUM",
    [TOK_STRUCT]   = "TOK_STRUCT",
    [TOK_CONTRACT] = "TOK_CONTRACT",
    [TOK_NUMBER]   = "TOK_NUMBER",
    [TOK_STRING]   = "TOK_STRING",
    [TOK_CHAR]     = "TOK_CHAR",
    [TOK_VOID]     = "TOK_VOID",

    // Type modifiers
    [TOK_TYPE] = "TOK_TYPE",

    // Async operations
    [TOK_ASYNC] = "TOK_ASYNC",
    [TOK_WAIT]  = "TOK_WAIT",

    // Type qualifiers
    [TOK_CONST]  = "TOK_CONST",
    [TOK_ATOMIC] = "TOK_ATOMIC",

    // Access specifiers
    [TOK_EXTERNAL] = "TOK_EXTERNAL",
    [TOK_INTERNAL] = "TOK_INTERNAL",
    [TOK_RESTRICT] = "TOK_RESTRICT",

    // Conditions
    [TOK_IF]      = "TOK_IF",
    [TOK_ELSE]    = "TOK_ELSE",
    [TOK_SWITCH]  = "TOK_SWITCH",
    [TOK_CASE]    = "TOK_CASE",
    [TOK_DEFAULT] = "TOK_DEFAULT",

    [TOK_CONTINUE] = "TOK_CONTINUE",

    // Loops
    [TOK_DO]      = "TOK_DO",
    [TOK_WHILE]   = "TOK_WHILE",
    [TOK_FOR]     = "TOK_FOR",
    [TOK_FOREACH] = "TOK_FOREACH",
    [TOK_IN]      = "TOK_IN",

    // Module
    [TOK_MODULE] = "TOK_MODULE",
    [TOK_IMPORT] = "TOK_IMPORT",
    [TOK_FROM]   = "TOK_FROM",

    // Functions
    [TOK_RETURN] = "TOK_RETURN",

    // Memory operations
    [TOK_NEW]    = "TOK_NEW",
    [TOK_NULL]   = "TOK_NULL",
    [TOK_SIZEOF] = "TOK_SIZEOF",
    [TOK_THIS]   = "TOK_THIS",
    [TOK_PURGE]  = "TOK_PURGE",

    // Identifiers
    [TOK_IDENT] = "TOK_IDENT",

    // Operators
    [TOK_ASSIGN]     = "TOK_ASSIGN",
    [TOK_PLUS]       = "TOK_PLUS",
    [TOK_MINUS]      = "TOK_MINUS",
    [TOK_BANG]       = "TOK_BANG",      // "!"
    [TOK_TILDE]      = "TOK_TILDE",     // "~"
    [TOK_ASTERISK]   = "TOK_ASTERISK",  //
    [TOK_AMPERSAND]  = "TOK_AMPERSAND", //
    [TOK_PIPE]       = "TOK_PIPE",      // "|"
    [TOK_CARET]      = "TOK_CARET",     // "^"
    [TOK_AT]         = "TOK_AT",
    [TOK_HASH]       = "TOK_HASH",
    [TOK_FSLASH]     = "TOK_FSLASH",
    [TOK_BSLASH]     = "TOK_BSLASH",
    [TOK_DOT]        = "TOK_DOT",
    [TOK_COLON]      = "TOK_COLON",
    [TOK_SEMI]       = "TOK_SEMI",
    [TOK_LT]         = "TOK_LT",
    [TOK_GT]         = "TOK_GT",
    [TOK_MOD]        = "TOK_MOD",
    [TOK_ARROW]      = "TOK_ARROW",
    [TOK_EQ]         = "TOK_EQ",
    [TOK_NEQ]        = "TOK_NEQ",
    [TOK_GEQ]        = "TOK_GEQ",
    [TOK_LEQ]        = "TOK_LEQ",
    [TOK_ADD_ASSIGN] = "TOK_ADD_ASSIGN",
    [TOK_SUB_ASSIGN] = "TOK_SUB_ASSIGN",
    [TOK_DIV_ASSIGN] = "TOK_DIV_ASSIGN",
    [TOK_MUL_ASSIGN] = "TOK_MUL_ASSIGN",
    [TOK_MOD_ASSIGN] = "TOK_MOD_ASSIGN",

    [TOK_AND] = "TOK_AND", // "&&"
    [TOK_OR]  = "TOK_OR",  // "||"

    [TOK_LSHIFT] = "TOK_LSHIFT", // "<<"
    [TOK_RSHIFT] = "TOK_RSHIFT", // ">>"

    // bitwise assign
    [TOK_LSHIFT_ASSIGN] = "TOK_LSHIFT_ASSIGN", // "<<="
    [TOK_RSHIFT_ASSIGN] = "TOK_RSHIFT_ASSIGN", // ">>="
    [TOK_AND_ASSIGN]    = "TOK_AND_ASSIGN",    // "&="
    [TOK_XOR_ASSIGN]    = "TOK_XOR_ASSIGN",    // "^="
    [TOK_OR_ASSIGN]     = "TOK_OR_ASSIGN",     // "|="

    // Grouping
    [TOK_LPAREN]   = "TOK_LPAREN",
    [TOK_RPAREN]   = "TOK_RPAREN",
    [TOK_LBRACE]   = "TOK_LBRACE",
    [TOK_RBRACE]   = "TOK_RBRACE",
    [TOK_LBRACKET] = "TOK_LBRACKET",
    [TOK_RBRACKET] = "TOK_RBRACKET",
    [TOK_LANGLE]   = "TOK_LANGLE",
    [TOK_RANGLE]   = "TOK_RANGLE",
    [TOK_COMMA]    = "TOK_COMMA",

    // Quotes
    [TOK_SQUOTE] = "TOK_SQUOTE",
    [TOK_DQUOTE] = "TOK_DQUOTE",
    [TOK_BQUOTE] = "TOK_BQUOTE",

    // Miscellaneous
    [TOK_ERROR]   = "TOK_ERROR",
    [TOK_UNKNOWN] = "TOK_UNKNOWN",
    [TOK_INVALID] = "TOK_INVALID",
    [TOK_EOF]     = "TOK_EOF",
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
    kwmap_add(TOK_TYPE, "type");

    // async
    kwmap_add(TOK_ASYNC, "async");
    kwmap_add(TOK_WAIT, "wait");

    // type qualifiers
    kwmap_add(TOK_CONST, "const");
    kwmap_add(TOK_ATOMIC, "atomic");

    // access specifier
    kwmap_add(TOK_EXTERNAL, "external");
    kwmap_add(TOK_INTERNAL, "internal");
    kwmap_add(TOK_RESTRICT, "restrict");

    // type specifiers
    kwmap_add(TOK_INT, "int");
    kwmap_add(TOK_FLOAT, "float");

    kwmap_add(TOK_BOOL, "bool");
    kwmap_add(TOK_ENUM, "enum");
    kwmap_add(TOK_STRUCT, "struct");
    kwmap_add(TOK_CONTRACT, "contract");
    kwmap_add(TOK_STRING, "string");
    kwmap_add(TOK_CHAR, "char");
    kwmap_add(TOK_VOID, "void");

    // conditions
    kwmap_add(TOK_IF, "if");
    kwmap_add(TOK_ELSE, "else");
    kwmap_add(TOK_SWITCH, "switch");
    kwmap_add(TOK_CASE, "case");
    kwmap_add(TOK_DEFAULT, "default");
    kwmap_add(TOK_CONTINUE, "continue");

    // loops
    kwmap_add(TOK_DO, "do");
    kwmap_add(TOK_WHILE, "while");
    kwmap_add(TOK_FOR, "for");
    kwmap_add(TOK_FOREACH, "foreach");
    kwmap_add(TOK_IN, "in");

    // module
    kwmap_add(TOK_MODULE, "module");
    kwmap_add(TOK_IMPORT, "import");
    kwmap_add(TOK_FROM, "from");

    // function
    kwmap_add(TOK_RETURN, "return");

    // memory operations
    kwmap_add(TOK_NEW, "new");
    kwmap_add(TOK_NULL, "null");
    kwmap_add(TOK_SIZEOF, "sizeof");
    kwmap_add(TOK_THIS, "this");
    kwmap_add(TOK_PURGE, "purge");

    // others
    kwmap_add(TOK_ERROR, "error");
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
    token.type = TOK_NUMBER;

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
    token.type = TOK_IDENT; // default is identifier

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
    token.type = TOK_STRING;

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
        token.type   = TOK_UNKNOWN;
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
    token.type = TOK_CHAR;

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
            token.type   = TOK_UNKNOWN;
            token.str[0] = '\0';
        }
    } else {
        // if it's just an empty quote
        token.type   = TOK_UNKNOWN;
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
        return (Token){TOK_LSHIFT_ASSIGN, "<<=", lexer->line, lexer->column - 3};
    }

    // ">>="
    if (cin == '>' && peek(lexer) == '>' && peeknext(lexer) == '=') {
        advance(lexer, 3, true);
        return (Token){TOK_RSHIFT_ASSIGN, ">>=", lexer->line, lexer->column - 3};
    }

    // "=="
    if (cin == '=' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_EQ, "==", lexer->line, lexer->column - 2};
    }

    // "!="
    if (cin == '!' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){TOK_NEQ, "!=", lexer->line, lexer->column - 2};
    }

    // "<="
    if (cin == '<' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){TOK_LEQ, "<=", lexer->line, lexer->column - 2};
    }

    // ">="
    if (cin == '>' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){TOK_GEQ, ">=", lexer->line, lexer->column - 2};
    }

    // "+="
    if (cin == '+' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_ADD_ASSIGN, "+=", lexer->line, lexer->column - 2};
    }

    // "-="
    if (cin == '-' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_SUB_ASSIGN, "-=", lexer->line, lexer->column - 2};
    }

    // "*="
    if (cin == '*' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MUL_ASSIGN, "*=", lexer->line, lexer->column - 2};
    }

    // "/="
    if (cin == '/' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_DIV_ASSIGN, "/=", lexer->line, lexer->column - 2};
    }

    // "%="
    if (cin == '%' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MOD_ASSIGN, "%=", lexer->line, lexer->column - 2};
    }

    // "&&"
    if (cin == '&' && peek(lexer) == '&') {
        advance(lexer, 2, true); // consume '&&'
        return (Token){TOK_AND, "<<", lexer->line, lexer->column - 2};
    }

    // "||"
    if (cin == '|' && peek(lexer) == '|') {
        advance(lexer, 2, true); // consume '||'
        return (Token){TOK_OR, "||", lexer->line, lexer->column - 2};
    }

    // "<<"
    if (cin == '<' && peek(lexer) == '<') {
        advance(lexer, 2, true); // consume '<<'
        return (Token){TOK_LSHIFT, "<<", lexer->line, lexer->column - 2};
    }

    // ">>"
    if (cin == '>' && peek(lexer) == '>') {
        advance(lexer, 2, true); // consume '>>'
        return (Token){TOK_RSHIFT, ">>", lexer->line, lexer->column - 2};
    }

    // "&="
    if (cin == '&' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_AND_ASSIGN, "&=", lexer->line, lexer->column - 2};
    }

    // "^="
    if (cin == '^' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_XOR_ASSIGN, "^=", lexer->line, lexer->column - 2};
    }

    // "|="
    if (cin == '|' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_OR_ASSIGN, "|=", lexer->line, lexer->column - 2};
    }

    /*********************************************
     * Single-character operators
     *********************************************/

    if (cin == '<') {
        advance(lexer, 1, true);
        return (Token){TOK_LT, "<", lexer->line, lexer->column - 1};
    }

    if (cin == '>') {
        advance(lexer, 1, true);
        return (Token){TOK_GT, ">", lexer->line, lexer->column - 1};
    }

    if (cin == '=') {
        advance(lexer, 1, true);
        return (Token){TOK_ASSIGN, "=", lexer->line, lexer->column - 1};
    }

    if (cin == '+') {
        advance(lexer, 1, true);
        return (Token){TOK_PLUS, "+", lexer->line, lexer->column - 1};
    }

    if (cin == '-') {
        advance(lexer, 1, true);
        return (Token){TOK_MINUS, "-", lexer->line, lexer->column - 1};
    }

    if (cin == '*') {
        advance(lexer, 1, true);
        return (Token){TOK_ASTERISK, "*", lexer->line, lexer->column - 1};
    }

    if (cin == '/') {
        advance(lexer, 1, true);
        return (Token){TOK_FSLASH, "/", lexer->line, lexer->column - 1};
    }

    if (cin == ';') {
        advance(lexer, 1, true);
        return (Token){TOK_SEMI, ";", lexer->line, lexer->column - 1};
    }

    if (cin == '\\') {
        advance(lexer, 1, true);
        return (Token){TOK_BSLASH, "\\", lexer->line, lexer->column - 1};
    }

    if (cin == '&') {
        advance(lexer, 1, true);
        return (Token){TOK_AMPERSAND, "&", lexer->line, lexer->column - 1};
    }

    if (cin == '|') {
        advance(lexer, 1, true);
        return (Token){TOK_PIPE, "|", lexer->line, lexer->column - 1};
    }

    if (cin == '^') {
        advance(lexer, 1, true);
        return (Token){TOK_CARET, "^", lexer->line, lexer->column - 1};
    }

    if (cin == '(') {
        advance(lexer, 1, true);
        return (Token){TOK_LPAREN, "(", lexer->line, lexer->column - 1};
    }

    if (cin == ')') {
        advance(lexer, 1, true);
        return (Token){TOK_RPAREN, ")", lexer->line, lexer->column - 1};
    }

    if (cin == '{') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACE, "{", lexer->line, lexer->column - 1};
    }

    if (cin == '}') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACE, "}", lexer->line, lexer->column - 1};
    }

    if (cin == '[') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACKET, "[", lexer->line, lexer->column - 1};
    }

    if (cin == ']') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACKET, "]", lexer->line, lexer->column - 1};
    }

    if (cin == '%') {
        advance(lexer, 1, true);
        return (Token){TOK_MOD, "%", lexer->line, lexer->column - 1};
    }

    if (cin == '!') {
        advance(lexer, 1, true);
        return (Token){TOK_BANG, "!", lexer->line, lexer->column - 1};
    }

    if (cin == '@') {
        advance(lexer, 1, true);
        return (Token){TOK_AT, "@", lexer->line, lexer->column - 1};
    }

    if (cin == '~') {
        advance(lexer, 1, true);
        return (Token){TOK_TILDE, "~", lexer->line, lexer->column - 1};
    }

    if (cin == '.') {
        advance(lexer, 1, true);
        return (Token){TOK_DOT, ".", lexer->line, lexer->column - 1};
    }

    if (cin == ',') {
        advance(lexer, 1, true);
        return (Token){TOK_COMMA, ",", lexer->line, lexer->column - 1};
    }

    if (cin == '\0') {
        advance(lexer, 1, true);
        return (Token){TOK_EOF, "EOF", lexer->line, lexer->column - 1};
    }

    advance(lexer, 1, true);

    // handle unknown token
    Token token  = {TOK_UNKNOWN, "", lexer->line, lexer->column - 1};
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
    } while (!token_eof(token));

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
 * @brief Checks if at the end of source file.
 * @param token
 * @return
 */
bool token_eof(Token token) {
    return token.type == TOK_EOF;
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
