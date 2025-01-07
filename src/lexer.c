#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *keywords[] = {
    // primitive types
    "auto", // auto detect type
    "bool", // boolean

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

    "complex",   //
    "imaginary", //

    // abstract types
    "type",     // type declaration (aliasing)
    "enum",     // enum (enumeration)
    "struct",   // struct (structure)
    "contract", // contract (interface)

    // modifiers & specifiers
    "const",

    "external", // (public)
    "internal", // (protected)
    "restrict", // (private)

    // conditionals
    "if",      // if
    "else",    // else
    "switch",  //
    "case",    //
    "default", //

    // loops
    "continue", //
    "do",       //
    "for",      //
    "foreach",  //
    "in",       //
    "while",    // while

    // functions
    "return", // return
    "void",   // represents absence of a lexeme

    // memory
    "new",   // allocates memory
    "null",  // represents absence of a lexeme for memory location
    "purge", // releases memory
    "this",  //

    // errors
    "error", //

    // module
    "import", //
    "module", //

    // async
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
    case TOK_KEYWORD:    return "TOK_KEYWORD";
    case TOK_IDENT:      return "TOK_IDENT";
    case TOK_NUMBER:     return "TOK_NUMBER";
    case TOK_STRING:     return "TOK_STRING";
    case TOK_CHAR:       return "TOK_CHAR";
    case TOK_ASSIGN:     return "TOK_ASSIGN";
    case TOK_PLUS:       return "TOK_PLUS";
    case TOK_MINUS:      return "TOK_MINUS";
    case TOK_STAR:       return "TOK_STAR";
    case TOK_AMP:        return "TOK_AMP";
    case TOK_AT:         return "TOK_AT";
    case TOK_HASH:       return "TOK_HASH";
    case TOK_FSLASH:     return "TOK_FSLASH";
    case TOK_BSLASH:     return "TOK_BSLASH";
    case TOK_DOT:        return "TOK_DOT";
    case TOK_COLON:      return "TOK_COLON";
    case TOK_SEMI:       return "TOK_SEMI";
    case TOK_LT:         return "TOK_LT";
    case TOK_GT:         return "TOK_GT";
    case TOK_MOD:        return "TOK_MOD";
    case TOK_ARROW:      return "TOK_ARROW";
    case TOK_EQ:         return "TOK_EQ";
    case TOK_NEQ:        return "TOK_NEQ";
    case TOK_GEQ:        return "TOK_GEQ";
    case TOK_LEQ:        return "TOK_LEQ";
    case TOK_ADD_ASSIGN: return "TOK_ADD_ASSIGN";
    case TOK_SUB_ASSIGN: return "TOK_SUB_ASSIGN";
    case TOK_DIV_ASSIGN: return "TOK_DIV_ASSIGN";
    case TOK_MUL_ASSIGN: return "TOK_MUL_ASSIGN";
    case TOK_MOD_ASSIGN: return "TOK_MOD_ASSIGN";
    case TOK_POW:        return "TOK_POW";
    case TOK_INCR:       return "TOK_INCR";
    case TOK_DECR:       return "TOK_DECR";
    case TOK_LPAREN:     return "TOK_LPAREN";
    case TOK_RPAREN:     return "TOK_RPAREN";
    case TOK_LBRACE:     return "TOK_LBRACE";
    case TOK_RBRACE:     return "TOK_RBRACE";
    case TOK_LBRACKET:   return "TOK_LBRACKET";
    case TOK_RBRACKET:   return "TOK_RBRACKET";
    case TOK_LANGLE:     return "TOK_LANGLE";
    case TOK_RANGLE:     return "TOK_RANGLE";
    case TOK_COMMA:      return "TOK_COMMA";
    case TOK_UNKNOWN:    return "TOK_UNKNOWN";
    case TOK_EOF:        return "TOK_EOF";
    default:             return "Unknown token";
    }
}

/**
 * @brief Print formatted token to the terminal.
 * @param token
 */
void print_token(Token token) {
    printf(
        "token: id %-4d type %-16s lexeme %-8s line %-4d col %d\n", token.type,
        token_str(token.type), token.lexeme, token.line, token.col
    );
}

/**
 * @brief Creates lexer and store source code to it's `buffer`.
 * @param fname File name with path.
 * @return
 */
Lexer *create_lexer(const char *fname) {
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
 * @brief Cleanup resources allocated for lexer and it's `buffer`.
 * @param lexer
 */
void remove_lexer(Lexer *lexer) {
    if (!lexer) return;

    free(lexer->buffer);
    free(lexer);
}

/**
 * @brief Returns current input character.
 * @param lexer
 * @return
 */
char curr_input(Lexer *lexer) {
    return lexer->buffer[lexer->pos];
}

/**
 * @brief Move to a certain position in input character.
 * @param lexer
 * @param n Characters to skip from current position.
 * @param movecol Should `lexer` column update or not.
 */
void advance(Lexer *lexer, int n, bool movecol) {
    lexer->pos += n;
    if (movecol) lexer->col += n;
}

/**
 * @brief Skip whitespace character and moves to next.
 * It keeps doing so until a non-whitespace character is found.
 * @param lexer
 */
void skip_space(Lexer *lexer) {
    char input = curr_input(lexer);
    while (input == ' ' || input == '\t') {
        advance(lexer, 1, true);
        input = curr_input(lexer);
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
Token get_number(Lexer *lexer) {
    Token token;
    token.type = TOK_NUMBER;

    char input = curr_input(lexer);
    int i      = 0;

    while (is_digit(input)) {
        token.lexeme[i++] = input;
        advance(lexer, 1, true);
        input = curr_input(lexer);
    }
    token.lexeme[i] = '\0'; // null-terminate string

    token.line = lexer->line;
    token.col  = lexer->col;

    return token;
}

/**
 * @brief Recognise the identifier, tokenize and return it.
 * @param lexer
 * @return
 */
Token get_ident(Lexer *lexer) {
    Token token;
    token.type = TOK_IDENT; // default is identifier

    char input = curr_input(lexer);
    int i      = 0;

    // ensure the first character is a letter or underscore
    if (is_letter(input) || input == '_') {
        token.lexeme[i++] = input;
        advance(lexer, 1, true);
        input = curr_input(lexer);
    } else {
        // if it doesn't start with a valid character,
        // return an error token
        token.type      = TOK_UNKNOWN;
        token.lexeme[0] = '\0'; // empty lexeme
        return token;
    }

    token.line = lexer->line;
    token.col  = lexer->col;

    // continue allowing letters, digits,
    // or underscores for the rest of the identifier
    while (is_letter(input) || is_digit(input) || input == '_') {
        token.lexeme[i++] = input;
        advance(lexer, 1, true);
        input = curr_input(lexer);
    }

    token.lexeme[i] = '\0'; // null-terminate string

    // check if this identifier is a keyword
    for (int j = 0; keywords[j] != NULL; j++) {
        if (strcmp(token.lexeme, keywords[j]) == 0) {
            token.type = TOK_KEYWORD; // it's a keyword
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
Token get_string_literal(Lexer *lexer) {
    Token token;
    token.type = TOK_STRING;

    char input = curr_input(lexer);
    int i      = 0;

    advance(lexer, 1, true); // skip opening quote
    input = curr_input(lexer);

    while (input != '"' && input != '\0') { // end of string or EOF
        token.lexeme[i++] = input;
        advance(lexer, 1, true);
        input = curr_input(lexer);
    }

    if (input == '"') {
        token.lexeme[i] = '\0';  // null-terminate string
        advance(lexer, 1, true); // consume closing quote
    } else {
        token.type      = TOK_UNKNOWN;
        token.lexeme[0] = '\0'; // empty lexeme
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
Token get_char_literal(Lexer *lexer) {
    Token token;
    token.type = TOK_CHAR;

    char input = curr_input(lexer);

    advance(lexer, 1, true); // skip the opening quote
    input = curr_input(lexer);

    if (input != '\'') {
        token.lexeme[0] = input; // store the character
        advance(lexer, 1, true); // move past the character
        input = curr_input(lexer);

        if (input == '\'') {         // check for the closing quote
            advance(lexer, 1, true); // consume the closing quote
        } else {
            // if there's no closing quote, mark it as unknown
            token.type      = TOK_UNKNOWN;
            token.lexeme[0] = '\0'; // empty lexeme
        }
    } else {
        // if it's just an empty quote, mark as unknown
        token.type      = TOK_UNKNOWN;
        token.lexeme[0] = '\0'; // empty lexeme
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
char peek_next_input(Lexer *lexer) {
    return lexer->buffer[lexer->pos + 1];
}

/**
 * @brief Skip single and multi-line comments.
 * @param lexer
 */
void skip_comments(Lexer *lexer) {
    char input = curr_input(lexer);
    char peek  = peek_next_input(lexer);
    int buffsz = strlen(lexer->buffer);

    if (input == '/' && peek == '*') { // multi-line comment
        advance(lexer, 2, true);       // skip '/*'
        while (lexer->pos < buffsz) {
            if (curr_input(lexer) == '*' && peek_next_input(lexer) == '/') {
                advance(lexer, 2, true); // skip '*/'
                return;
            }
            if (curr_input(lexer) == '\n') {
                lexer->line++;
                lexer->col = 1;
                advance(lexer, 1, false);
            } else {
                advance(lexer, 1, true);
            }
        }
    } else if (input == '/' && peek == '/') { // single-line comment
        advance(lexer, 2, true);              // skip '//'
        while (lexer->pos < buffsz && curr_input(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    } else if (input == '#') {   // hash-style comment
        advance(lexer, 1, true); // skip '#'
        while (lexer->pos < buffsz && curr_input(lexer) != '\n') {
            advance(lexer, 1, true);
        }
    }
}

/**
 * @brief Fetch the next token.
 * @param lexer
 * @return
 */
Token get_next_token(Lexer *lexer) {
    skip_space(lexer);

    char input = curr_input(lexer);

    if (input == '\n' || input == '\r') {
        lexer->line++;
        lexer->col = 0; // reset for new line
        advance(lexer, 1, false);
        return get_next_token(lexer); // recursively get the next token
    }

    // handle comments
    if ((input == '/' && (peek_next_input(lexer) == '*' || peek_next_input(lexer) == '/')) ||
        input == '#') {
        skip_comments(lexer);         // skip the comment
        return get_next_token(lexer); // recursively get the next token after the comment
    }

    if (is_letter(input) || input == '_') {
        return get_ident(lexer);
    }

    if (is_digit(input)) {
        return get_number(lexer);
    }

    if (input == '"') {
        return get_string_literal(lexer);
    }
    if (input == '\'') {
        return get_char_literal(lexer);
    }

    /*********************************************
     * compound operators
     *********************************************/

    // (==)
    if (input == '=' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_EQ, "==", lexer->line, lexer->col};
    }

    // (!=)
    if (input == '!' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true); // consume '!='
        return (Token){TOK_NEQ, "!=", lexer->line, lexer->col};
    }

    // (<=)
    if (input == '<' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true); // consume '<='
        return (Token){TOK_LEQ, "<=", lexer->line, lexer->col};
    }

    // (>=)
    if (input == '>' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true); // consume '>='
        return (Token){TOK_GEQ, ">=", lexer->line, lexer->col};
    }

    // (++)
    if (input == '+' && peek_next_input(lexer) == '+') {
        advance(lexer, 2, true); // consume '++'
        return (Token){TOK_INCR, "++", lexer->line, lexer->col};
    }

    // (--)
    if (input == '-' && peek_next_input(lexer) == '-') {
        advance(lexer, 2, true); // consume '--'
        return (Token){TOK_DECR, "--", lexer->line, lexer->col};
    }

    // (+=)
    if (input == '+' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_ADD_ASSIGN, "+=", lexer->line, lexer->col};
    }

    // (-=)
    if (input == '-' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_SUB_ASSIGN, "-=", lexer->line, lexer->col};
    }

    // (*=)
    if (input == '*' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MUL_ASSIGN, "*=", lexer->line, lexer->col};
    }

    // (/=)
    if (input == '/' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_DIV_ASSIGN, "/=", lexer->line, lexer->col};
    }

    // (**)
    if (input == '*' && peek_next_input(lexer) == '*') {
        advance(lexer, 2, true);
        return (Token){TOK_POW, "**", lexer->line, lexer->col};
    }

    // (%=)
    if (input == '%' && peek_next_input(lexer) == '=') {
        advance(lexer, 2, true);
        return (Token){TOK_MOD_ASSIGN, "%=", lexer->line, lexer->col};
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
    Token token     = {TOK_UNKNOWN, "", lexer->line, lexer->col};
    token.lexeme[0] = input;
    token.lexeme[1] = '\0';

    return token;
}
