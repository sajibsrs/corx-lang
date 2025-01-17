#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

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

// Keyword mapper
KWMap *kwmap[HASH_SIZE] = {NULL};

void kwmap_add(TokenType type, const char *token);

void init_keymap() {
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
    kwmap_add(TOK_BOOL, "bool");
    kwmap_add(TOK_ENUM, "enum");
    kwmap_add(TOK_STRUCT, "struct");
    kwmap_add(TOK_CONTRACT, "contract");
    kwmap_add(TOK_NUMBER, "number");
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

KWMap *kwmap_find(const char *key) {
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

void print_kwmap() {
    for (int i = 0; i < HASH_SIZE; i++) {
        KWMap *current = kwmap[i];
        if (current) {
            printf("Bucket %d:\n", i);
            while (current) {
                printf("  Token: %s, Type: %d\n", current->token, current->type);
                current = current->next;
            }
        }
    }
}

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

    lexer->pos  = 0;
    lexer->line = 1;
    lexer->col  = 0;

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
    if (movecol) lexer->col += n;
}

/**
 * @brief Skip whitespace character and moves to next.
 * It keeps doing so until a non-whitespace character is found.
 * @param lexer
 */
static void space(Lexer *lexer) {
    char input = current(lexer);
    while (input == ' ' || input == '\t') {
        advance(lexer, 1, true);
        input = current(lexer);
    }
}

/**
 * @brief Returns true if it's a digit false otherwise.
 * @param c Input character.
 * @return
 */
bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

/**
 * @brief Returns true if it's a letter false otherwise.
 * @param c Input character.
 * @return
 */
bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/**
 * @brief Recognise the number, tokenize return it.
 * @param lexer
 * @return
 */
static Token number(Lexer *lexer) {
    Token token;
    token.type = TOK_NUMBER;

    char cin = current(lexer);
    int idx  = 0;

    while (is_digit(cin)) {
        token.value[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }
    token.value[idx] = '\0'; // null-terminate string

    token.line = lexer->line;
    token.col  = lexer->col;

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

    char cin = current(lexer);
    int idx  = 0;

    // ensure the first character is a letter or underscore
    if (is_letter(cin) || cin == '_') {
        token.value[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    } else {
        // return error token if doesn't start with a valid token
        token.type     = TOK_INVALID;
        token.value[0] = '\0'; // empty value
        return token;
    }

    token.line = lexer->line;
    token.col  = lexer->col;

    // continue allowing letters, digits and underscores
    while (is_letter(cin) || is_digit(cin) || cin == '_') {
        token.value[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    token.value[idx] = '\0'; // null-terminate string

    KWMap *kw = kwmap_find(token.value);
    if (kw) token.type = kw->type;

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

    char cin = current(lexer);
    int idx  = 0;

    advance(lexer, 1, true); // skip opening quote
    cin = current(lexer);

    while (cin != '"' && cin != '\0') { // end of string or EOF
        token.value[idx++] = cin;
        advance(lexer, 1, true);
        cin = current(lexer);
    }

    if (cin == '"') {
        token.value[idx] = '\0'; // null-terminate string
        advance(lexer, 1, true); // consume closing quote
    } else {
        token.type     = TOK_UNKNOWN;
        token.value[0] = '\0'; // empty value
    }

    token.line = lexer->line;
    token.col  = lexer->col;

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

    char input = current(lexer);

    advance(lexer, 1, true); // skip the opening quote
    input = current(lexer);

    if (input != '\'') {
        token.value[0] = input;  // store the character
        advance(lexer, 1, true); // move past the character
        input = current(lexer);

        if (input == '\'') {         // check for the closing quote
            advance(lexer, 1, true); // consume the closing quote
        } else {
            // if there's no closing quote, mark it as unknown
            token.type     = TOK_UNKNOWN;
            token.value[0] = '\0'; // empty value
        }
    } else {
        // if it's just an empty quote, mark as unknown
        token.type     = TOK_UNKNOWN;
        token.value[0] = '\0'; // empty value
    }

    token.line = lexer->line;
    token.col  = lexer->col;

    return token;
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
    space(lexer);

    char cin = current(lexer);

    if (cin == '\n' || cin == '\r') {
        lexer->line++;
        lexer->col = 0; // reset for new line
        advance(lexer, 1, false);
        return next(lexer); // recursively get the next token
    }

    // handle comments
    if ((cin == '/' && (peek(lexer) == '*' || peek(lexer) == '/')) || cin == '#') {
        comment(lexer);     // skip the comment
        return next(lexer); // recursively get the next token after the comment
    }

    if (is_letter(cin) || cin == '_') {
        return identifier(lexer);
    }

    if (is_digit(cin)) {
        return number(lexer);
    }

    if (cin == '"') {
        return string(lexer);
    }
    if (cin == '\'') {
        return character(lexer);
    }

    /*********************************************
     * compound operators
     *********************************************/

    // (==)
    if (cin == '=' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_EQ, "==", lexer->line, lexer->col};
    }

    // (!=)
    if (cin == '!' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){TOK_NEQ, "!=", lexer->line, lexer->col};
    }

    // (<=)
    if (cin == '<' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){TOK_LEQ, "<=", lexer->line, lexer->col};
    }

    // (>=)
    if (cin == '>' && peek(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){TOK_GEQ, ">=", lexer->line, lexer->col};
    }

    // (++)
    if (cin == '+' && peek(lexer) == '+') {
        advance(lexer, 2, true); // consume '++'
        return (Token){TOK_INCR, "++", lexer->line, lexer->col};
    }

    // (--)
    if (cin == '-' && peek(lexer) == '-') {
        advance(lexer, 2, true); // consume '--'
        return (Token){TOK_DECR, "--", lexer->line, lexer->col};
    }

    // (+=)
    if (cin == '+' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_ADD_ASSIGN, "+=", lexer->line, lexer->col};
    }

    // (-=)
    if (cin == '-' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_SUB_ASSIGN, "-=", lexer->line, lexer->col};
    }

    // (*=)
    if (cin == '*' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MUL_ASSIGN, "*=", lexer->line, lexer->col};
    }

    // (/=)
    if (cin == '/' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_DIV_ASSIGN, "/=", lexer->line, lexer->col};
    }

    // (**)
    if (cin == '*' && peek(lexer) == '*') {
        advance(lexer, 2, true);
        return (Token){TOK_POW, "**", lexer->line, lexer->col};
    }

    // (%=)
    if (cin == '%' && peek(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MOD_ASSIGN, "%=", lexer->line, lexer->col};
    }

    /*********************************************
     * single-character operators
     *********************************************/

    if (cin == '<') {
        advance(lexer, 1, true);
        return (Token){TOK_LT, "<", lexer->line, lexer->col};
    }

    if (cin == '>') {
        advance(lexer, 1, true);
        return (Token){TOK_GT, ">", lexer->line, lexer->col};
    }

    if (cin == '=') {
        advance(lexer, 1, true);
        return (Token){TOK_ASSIGN, "=", lexer->line, lexer->col};
    }

    if (cin == '+') {
        advance(lexer, 1, true);
        return (Token){TOK_PLUS, "+", lexer->line, lexer->col};
    }

    if (cin == '-') {
        advance(lexer, 1, true);
        return (Token){TOK_MINUS, "-", lexer->line, lexer->col};
    }

    if (cin == '*') {
        advance(lexer, 1, true);
        return (Token){TOK_ASTERISK, "*", lexer->line, lexer->col};
    }

    if (cin == '/') {
        advance(lexer, 1, true);
        return (Token){TOK_FSLASH, "/", lexer->line, lexer->col};
    }

    if (cin == ';') {
        advance(lexer, 1, true);
        return (Token){TOK_SEMI, ";", lexer->line, lexer->col};
    }

    if (cin == '(') {
        advance(lexer, 1, true);
        return (Token){TOK_LPAREN, "(", lexer->line, lexer->col};
    }

    if (cin == ')') {
        advance(lexer, 1, true);
        return (Token){TOK_RPAREN, ")", lexer->line, lexer->col};
    }

    if (cin == '{') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACE, "{", lexer->line, lexer->col};
    }

    if (cin == '}') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACE, "}", lexer->line, lexer->col};
    }

    if (cin == '[') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACKET, "[", lexer->line, lexer->col};
    }

    if (cin == ']') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACKET, "]", lexer->line, lexer->col};
    }

    if (cin == '%') {
        advance(lexer, 1, true);
        return (Token){TOK_MOD, "%", lexer->line, lexer->col};
    }

    if (cin == '\0') {
        advance(lexer, 1, true);
        return (Token){TOK_EOF, "EOF", lexer->line, lexer->col};
    }

    advance(lexer, 1, true);

    // handle unknown token
    Token token    = {TOK_UNKNOWN, "", lexer->line, lexer->col};
    token.value[0] = cin;
    token.value[1] = '\0';

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
 * @brief Cleanup allocated memory from `tokens`.
 * @param list
 */
void purge_tokenlist(TokenList *list) {
    if (!list) return;

    free(list->tokens);
    free(list);
}
