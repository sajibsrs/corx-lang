#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *type_specifiers[] = {
    "bool", //

    "char",   //
    "string", //
    "utf8",   //
    "utf16",  //
    "utf32",  //

    "float",   //
    "float16", //
    "float32", //
    "float64", //

    "int",    //
    "uint",   //
    "int8",   //
    "uint8",  //
    "int16",  //
    "uint16", //
    "int32",  //
    "uint32", //
    "int64",  //
    "uint64", //

    "enum",     // enum (enumeration)
    "struct",   // struct (structure)
    "contract", // contract (interface)

    "void", //

    NULL // marks end of array
};

const char *type_modifiers[] = {
    "type", // type declaration (aliasing)

    NULL // marks end of array
};

const char *type_qualifiers[] = {
    "const",  //
    "atomic", //

    NULL // marks end of array
};

const char *access_specifiers[] = {
    "external", // (public)
    "internal", // (protected)
    "restrict", // (private)

    NULL // marks end of array
};

const char *conditionals[] = {
    "if",      // if
    "else",    // else
    "switch",  //
    "case",    //
    "default", //

    NULL // marks end of array
};

const char *loops[] = {
    "do",      //
    "for",     //
    "foreach", //
    "in",      //
    "while",   //
    "break",
    "continue", //

    NULL // marks end of array
};

const char *functions[] = {
    "return", //

    NULL // marks end of array
};

const char *memory[] = {
    "new",    // allocates memory
    "null",   // represents absence of a lexeme for memory location
    "purge",  // releases memory
    "sizeof", //
    "this",   //

    NULL // marks end of array
};

const char *errors[] = {
    "error", //

    NULL // marks end of array
};

const char *module[] = {
    "import", //
    "module", //

    NULL // marks end of array
};

const char *async[] = {
    "async", //
    "wait",  //

    NULL // mark end of array
};

/**
 * @brief Returns token name string.
 * @param type Token type.
 * @return
 */
char *token_str(const TokenType type) {
    switch (type) {
    case TOK_TYPE_SPEC: return "TOK_TYPE_SPEC";
    case TOK_TYPE_MOD:  return "TOK_TYPE_MOD";
    case TOK_TYPE_QF:   return "TOK_TYPE_QF";
    case TOK_ACC_SPEC:  return "TOK_ACC_SPEC";
    case TOK_FUNC:      return "TOK_FUNC";
    case TOK_COND:      return "TOK_COND";
    case TOK_LOOPS:     return "TOK_LOOPS";
    case TOK_MEM:       return "TOK_MEM";
    case TOK_ERROR:     return "TOK_ERROR";
    case TOK_MODULE:    return "TOK_MODULE";
    case TOK_ASYNC:     return "TOK_ASYNC";
    case TOK_IDENT:     return "TOK_IDENT";
    case TOK_NUMBER:    return "TOK_NUMBER";
    case TOK_STRING:    return "TOK_STRING";
    case TOK_CHAR:      return "TOK_CHAR";
    case TOK_ASSIGN:    return "TOK_ASSIGN";
    case TOK_PLUS:      return "TOK_PLUS";
    case TOK_MINUS:     return "TOK_MINUS";
    case TOK_STAR:      return "TOK_STAR";
    case TOK_AMP:       return "TOK_AMP";
    case TOK_AT:        return "TOK_AT";
    case TOK_HASH:      return "TOK_HASH";
    case TOK_FSLASH:    return "TOK_FSLASH";
    case TOK_BSLASH:    return "TOK_BSLASH";
    case TOK_DOT:       return "TOK_DOT";
    case TOK_COLON:     return "TOK_COLON";
    case TOK_SEMI:      return "TOK_SEMI";
    case TOK_LT:        return "TOK_LT";
    case TOK_GT:        return "TOK_GT";
    case TOK_MOD:       return "TOK_MOD";
    case TOK_ARROW:     return "TOK_ARROW";
    case TOK_EQ:        return "TOK_EQ";
    case TOK_NEQ:       return "TOK_NEQ";
    case TOK_GEQ:       return "TOK_GEQ";
    case TOK_LEQ:       return "TOK_LEQ";
    case TOK_ADD_ASN:   return "TOK_ADD_ASN";
    case TOK_SUB_ASN:   return "TOK_SUB_ASN";
    case TOK_DIV_ASN:   return "TOK_DIV_ASN";
    case TOK_MUL_ASN:   return "TOK_MUL_ASN";
    case TOK_MOD_ASN:   return "TOK_MOD_ASN";
    case TOK_POW:       return "TOK_POW";
    case TOK_INCR:      return "TOK_INCR";
    case TOK_DECR:      return "TOK_DECR";
    case TOK_LPAREN:    return "TOK_LPAREN";
    case TOK_RPAREN:    return "TOK_RPAREN";
    case TOK_LBRACE:    return "TOK_LBRACE";
    case TOK_RBRACE:    return "TOK_RBRACE";
    case TOK_LBRACKET:  return "TOK_LBRACKET";
    case TOK_RBRACKET:  return "TOK_RBRACKET";
    case TOK_LANGLE:    return "TOK_LANGLE";
    case TOK_RANGLE:    return "TOK_RANGLE";
    case TOK_COMMA:     return "TOK_COMMA";
    case TOK_UNKNOWN:   return "TOK_UNKNOWN";
    case TOK_EOF:       return "TOK_EOF";
    default:            return "Unknown token";
    }
}

/**
 * @brief Print formatted token to the terminal.
 * @param token
 */
void render_token(Token token) {
    printf(
        "token: id %-4d type %-16s value %-8s line %-4d col %d\n", token.type,
        token_str(token.type), token.value, token.line, token.col
    );
}

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param fname File name with path.
 * @return
 */
Lexer *make_lexer(const char *fname) {
    Lexer *lexer = malloc(sizeof(Lexer));
    if (!lexer) {
        perror("Memory allocation error");
        exit(1);
    }

    lexer->pos  = 0;
    lexer->line = 1;
    lexer->col  = 0;

    FILE *file = fopen(fname, "r");
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
static char curinput(Lexer *lexer) {
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
static void skip_space(Lexer *lexer) {
    char input = curinput(lexer);
    while (input == ' ' || input == '\t') {
        advance(lexer, 1, true);
        input = curinput(lexer);
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

    char input = curinput(lexer);
    int i      = 0;

    while (is_digit(input)) {
        token.value[i++] = input;
        advance(lexer, 1, true);
        input = curinput(lexer);
    }
    token.value[i] = '\0'; // null-terminate string

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

    char input = curinput(lexer);
    int i      = 0;

    // ensure the first character is a letter or underscore
    if (is_letter(input) || input == '_') {
        token.value[i++] = input;
        advance(lexer, 1, true);
        input = curinput(lexer);
    } else {
        // return error token if doesn't start with a valid token
        token.type     = TOK_UNKNOWN;
        token.value[0] = '\0'; // empty value
        return token;
    }

    token.line = lexer->line;
    token.col  = lexer->col;

    // continue allowing letters, digits and underscores
    while (is_letter(input) || is_digit(input) || input == '_') {
        token.value[i++] = input;
        advance(lexer, 1, true);
        input = curinput(lexer);
    }

    token.value[i] = '\0'; // null-terminate string

    // access specifiers
    for (int j = 0; access_specifiers[j] != NULL; j++) {
        if (strcmp(token.value, access_specifiers[j]) == 0) {
            token.type = TOK_ACC_SPEC; // keyword
            break;
        }
    }

    // type specifiers
    for (int j = 0; type_specifiers[j] != NULL; j++) {
        if (strcmp(token.value, type_specifiers[j]) == 0) {
            token.type = TOK_TYPE_SPEC; // keyword
            break;
        }
    }

    // type qualifiers
    for (int j = 0; type_qualifiers[j] != NULL; j++) {
        if (strcmp(token.value, type_qualifiers[j]) == 0) {
            token.type = TOK_TYPE_QF; // keyword
            break;
        }
    }

    // type modifiers
    for (int j = 0; type_modifiers[j] != NULL; j++) {
        if (strcmp(token.value, type_modifiers[j]) == 0) {
            token.type = TOK_TYPE_MOD; // keyword
            break;
        }
    }

    // module
    for (int j = 0; module[j] != NULL; j++) {
        if (strcmp(token.value, module[j]) == 0) {
            token.type = TOK_MODULE; // keyword
            break;
        }
    }

    // memory
    for (int j = 0; memory[j] != NULL; j++) {
        if (strcmp(token.value, memory[j]) == 0) {
            token.type = TOK_MEM; // keyword
            break;
        }
    }

    // errors
    for (int j = 0; errors[j] != NULL; j++) {
        if (strcmp(token.value, errors[j]) == 0) {
            token.type = TOK_ERROR; // keyword
            break;
        }
    }

    // functions
    for (int j = 0; functions[j] != NULL; j++) {
        if (strcmp(token.value, functions[j]) == 0) {
            token.type = TOK_FUNC; // keyword
            break;
        }
    }

    // conditionals
    for (int j = 0; conditionals[j] != NULL; j++) {
        if (strcmp(token.value, conditionals[j]) == 0) {
            token.type = TOK_COND; // keyword
            break;
        }
    }

    // loops
    for (int j = 0; loops[j] != NULL; j++) {
        if (strcmp(token.value, loops[j]) == 0) {
            token.type = TOK_LOOPS; // keyword
            break;
        }
    }

    // async
    for (int j = 0; async[j] != NULL; j++) {
        if (strcmp(token.value, async[j]) == 0) {
            token.type = TOK_ASYNC; // keyword
            break;
        }
    }

    return token;
}

/**
 * @brief Handle string literal.
 * @param lexer
 * @return
 */
static Token string_literal(Lexer *lexer) {
    Token token;
    token.type = TOK_STRING;

    char input = curinput(lexer);
    int i      = 0;

    advance(lexer, 1, true); // skip opening quote
    input = curinput(lexer);

    while (input != '"' && input != '\0') { // end of string or EOF
        token.value[i++] = input;
        advance(lexer, 1, true);
        input = curinput(lexer);
    }

    if (input == '"') {
        token.value[i] = '\0';   // null-terminate string
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
static Token char_literal(Lexer *lexer) {
    Token token;
    token.type = TOK_CHAR;

    char input = curinput(lexer);

    advance(lexer, 1, true); // skip the opening quote
    input = curinput(lexer);

    if (input != '\'') {
        token.value[0] = input;  // store the character
        advance(lexer, 1, true); // move past the character
        input = curinput(lexer);

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
static char peek_next(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 1];
}

/**
 * @brief Skip single and multi-line comments.
 * @param lexer
 */
static void skip_comments(Lexer *lexer) {
    char input = curinput(lexer);
    char peek  = peek_next(lexer);
    int buffsz = strlen(lexer->buffer);

    if (input == '/' && peek == '*') { // multi-line comment
        advance(lexer, 2, true);       // skip '/*'
        while (lexer->pos < buffsz) {
            if (curinput(lexer) == '*' && peek_next(lexer) == '/') {
                advance(lexer, 2, true); // skip '*/'
                return;
            }
            if (curinput(lexer) == '\n') {
                lexer->line++;
                lexer->col = 1;
                advance(lexer, 1, false);
            } else {
                advance(lexer, 1, true);
            }
        }
    } else if (input == '/' && peek == '/') { // single-line comment
        advance(lexer, 2, true);              // skip '//'
        while (lexer->pos < buffsz && curinput(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    } else if (input == '#') {   // hash-style comment
        advance(lexer, 1, true); // skip '#'
        while (lexer->pos < buffsz && curinput(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    }
}

/**
 * @brief Fetch the next token.
 * @param lexer
 * @return
 */
static Token next_token(Lexer *lexer) {
    skip_space(lexer);

    char input = curinput(lexer);

    if (input == '\n' || input == '\r') {
        lexer->line++;
        lexer->col = 0; // reset for new line
        advance(lexer, 1, false);
        return next_token(lexer); // recursively get the next token
    }

    // handle comments
    if ((input == '/' && (peek_next(lexer) == '*' || peek_next(lexer) == '/')) || input == '#') {
        skip_comments(lexer);     // skip the comment
        return next_token(lexer); // recursively get the next token after the comment
    }

    if (is_letter(input) || input == '_') {
        return identifier(lexer);
    }

    if (is_digit(input)) {
        return number(lexer);
    }

    if (input == '"') {
        return string_literal(lexer);
    }
    if (input == '\'') {
        return char_literal(lexer);
    }

    /*********************************************
     * compound operators
     *********************************************/

    // (==)
    if (input == '=' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_EQ, "==", lexer->line, lexer->col};
    }

    // (!=)
    if (input == '!' && peek_next(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){TOK_NEQ, "!=", lexer->line, lexer->col};
    }

    // (<=)
    if (input == '<' && peek_next(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){TOK_LEQ, "<=", lexer->line, lexer->col};
    }

    // (>=)
    if (input == '>' && peek_next(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){TOK_GEQ, ">=", lexer->line, lexer->col};
    }

    // (++)
    if (input == '+' && peek_next(lexer) == '+') {
        advance(lexer, 2, true); // consume '++'
        return (Token){TOK_INCR, "++", lexer->line, lexer->col};
    }

    // (--)
    if (input == '-' && peek_next(lexer) == '-') {
        advance(lexer, 2, true); // consume '--'
        return (Token){TOK_DECR, "--", lexer->line, lexer->col};
    }

    // (+=)
    if (input == '+' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_ADD_ASN, "+=", lexer->line, lexer->col};
    }

    // (-=)
    if (input == '-' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_SUB_ASN, "-=", lexer->line, lexer->col};
    }

    // (*=)
    if (input == '*' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MUL_ASN, "*=", lexer->line, lexer->col};
    }

    // (/=)
    if (input == '/' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_DIV_ASN, "/=", lexer->line, lexer->col};
    }

    // (**)
    if (input == '*' && peek_next(lexer) == '*') {
        advance(lexer, 2, true);
        return (Token){TOK_POW, "**", lexer->line, lexer->col};
    }

    // (%=)
    if (input == '%' && peek_next(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MOD_ASN, "%=", lexer->line, lexer->col};
    }

    /*********************************************
     * single-character operators
     *********************************************/

    if (input == '<') {
        advance(lexer, 1, true);
        return (Token){TOK_LT, "<", lexer->line, lexer->col};
    }

    if (input == '>') {
        advance(lexer, 1, true);
        return (Token){TOK_GT, ">", lexer->line, lexer->col};
    }

    if (input == '=') {
        advance(lexer, 1, true);
        return (Token){TOK_ASSIGN, "=", lexer->line, lexer->col};
    }

    if (input == '+') {
        advance(lexer, 1, true);
        return (Token){TOK_PLUS, "+", lexer->line, lexer->col};
    }

    if (input == '-') {
        advance(lexer, 1, true);
        return (Token){TOK_MINUS, "-", lexer->line, lexer->col};
    }

    if (input == '*') {
        advance(lexer, 1, true);
        return (Token){TOK_STAR, "*", lexer->line, lexer->col};
    }

    if (input == '/') {
        advance(lexer, 1, true);
        return (Token){TOK_FSLASH, "/", lexer->line, lexer->col};
    }

    if (input == ';') {
        advance(lexer, 1, true);
        return (Token){TOK_SEMI, ";", lexer->line, lexer->col};
    }

    if (input == '(') {
        advance(lexer, 1, true);
        return (Token){TOK_LPAREN, "(", lexer->line, lexer->col};
    }

    if (input == ')') {
        advance(lexer, 1, true);
        return (Token){TOK_RPAREN, ")", lexer->line, lexer->col};
    }

    if (input == '{') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACE, "{", lexer->line, lexer->col};
    }

    if (input == '}') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACE, "}", lexer->line, lexer->col};
    }

    if (input == '[') {
        advance(lexer, 1, true);
        return (Token){TOK_LBRACKET, "[", lexer->line, lexer->col};
    }

    if (input == ']') {
        advance(lexer, 1, true);
        return (Token){TOK_RBRACKET, "]", lexer->line, lexer->col};
    }

    if (input == '%') {
        advance(lexer, 1, true);
        return (Token){TOK_MOD, "%", lexer->line, lexer->col};
    }

    if (input == '\0') {
        advance(lexer, 1, true);
        return (Token){TOK_EOF, "EOF", lexer->line, lexer->col};
    }

    advance(lexer, 1, true);

    // handle unknown token
    Token token    = {TOK_UNKNOWN, "", lexer->line, lexer->col};
    token.value[0] = input;
    token.value[1] = '\0';

    return token;
}

/**
 * @brief Scan the lexer and return the tokens array.
 * @param lexer
 * @return
 */
TokArr *lexer_scan(Lexer *lexer) {
    const int initsize = 64;       // initial array size
    int cursize        = initsize; // current array size
    int idx            = 0;

    Token token;
    Token *arr = malloc(initsize * sizeof(Token));
    if (!arr) {
        perror("Memory allocation erro");
        exit(1);
    }

    do {
        token      = next_token(lexer);
        arr[idx++] = token;

        if (idx >= cursize) {
            cursize *= 2;
            arr = realloc(arr, cursize * sizeof(Token));
            if (!arr) {
                perror("Memory allocation error");
                exit(1);
            }
        }
    } while (!token_eof(token));

    // shrink to fit exact amount of tokens
    arr = realloc(arr, idx * sizeof(Token));
    if (!arr) {
        perror("Memory reallocation error");
        exit(1);
    }

    TokArr *tokarr = malloc(sizeof(TokArr));
    if (!tokarr) {
        perror("Memory reallocation error");
        exit(1);
    }

    tokarr->tokens = arr;
    tokarr->size   = idx;

    return tokarr;
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
 * @param tokarr
 */
void purge_tokarr(TokArr *tokarr) {
    if (!tokarr) return;

    free(tokarr->tokens);
    free(tokarr);
}
