#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *input; // input string
int position = 0;  // current position in the input

const char *keywords[] = {"type",  "enum",     "struct",   "contract", "int",
                          "float", "external", "internal", "restrict", "if",
                          "else",  "while",    "return",   "null",     NULL};

char current_char() {
    return input[position];
}

void advance() {
    position++;
}

void skip_whitespace() {
    char cc = current_char();
    while (cc == ' ' || cc == '\t') {
        advance();
        cc = current_char();
    }
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

Token get_number() {
    Token token;

    token.type = TOK_NUMBER;
    char cc    = current_char();
    int i      = 0;

    while (is_digit(cc)) {
        token.value[i++] = cc;
        advance();
        cc = current_char();
    }
    token.value[i] = '\0'; // null-terminate string
    return token;
}

Token get_identifier() {
    Token token;

    token.type = TOK_IDENTIFIER; // default is identifier
    char cc    = current_char();
    int i      = 0;

    while (is_letter(cc)) {
        token.value[i++] = cc;
        advance();
        cc = current_char();
    }
    token.value[i] = '\0'; // null-terminate string

    // check if this identifier is a keyword
    for (int j = 0; keywords[j] != NULL; j++) {
        if (strcmp(token.value, keywords[j]) == 0) {
            token.type = TOK_KEYWORD; // it's a keyword
            break;
        }
    }

    return token;
}

Token get_next_token() {
    skip_whitespace();

    char cc = current_char();

    if (is_letter(cc)) {
        return get_identifier();
    }

    if (is_digit(cc)) {
        return get_number();
    }

    if (cc == '=') {
        advance();
        return (Token){TOK_ASSIGN, "="};
    }

    if (cc == '+') {
        advance();
        return (Token){TOK_PLUS, "+"};
    }

    if (cc == '-') {
        advance();
        return (Token){TOK_MINUS, "-"};
    }

    if (cc == '*') {
        advance();
        return (Token){TOK_STAR, "*"};
    }

    if (cc == '/') {
        advance();
        return (Token){TOK_FSLASH, "/"};
    }

    if (cc == ';') {
        advance();
        return (Token){TOK_SEMICOLON, ";"};
    }

    if (cc == '\0') {
        return (Token){TOK_EOF, ""};
    }

    // Handle invalid characters (error handling)
    fprintf(stderr, "Unexpected character: %cc\n", cc);
    exit(1);
}
