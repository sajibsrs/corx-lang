#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *input; // input string
int position = 0;  // current position in the input

const char *keywords[] = {
    "type",     // type declaration (aliasing)
    "enum",     // enum (enumeration)
    "struct",   // struct (structure)
    "contract", // contract (interface)
    "int",      // integer
    "float",    // floating point number
    "external", // visibility (public)
    "internal", // visibility (protected)
    "restrict", // visibility (private)
    "if",       // if condition
    "else",     // else condition
    "while",    // while loop
    "return",   // return
    "null",     // null
    NULL        // mark end of array
};

/**
 * @brief Load source from a file.
 * @param fname File name.
 */
void loadfile(const char *fname) {
    FILE *file = fopen(fname, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    // get length of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // allocate memory for the file
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        perror("Memory allocation error");
        exit(1);
    }

    // read the file into buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';

    // close file
    fclose(file);
    input = buffer;
}

/**
 * @brief Returns current input character.
 * @return
 */
char current_char() {
    return input[position];
}

/**
 * @brief Move to a certain position in input character.
 * @param n Characters to skip from current position.
 */
void advance(int n) {
    position += n;
}

/**
 * @brief Skip whitespace character and moves to next.
 * It keeps doing so until a non-whitespace character is found.
 */
void skip_whitespace() {
    char cc = current_char();
    while (cc == ' ' || cc == '\t' || cc == '\n' || cc == '\r') {
        advance(1);
        cc = current_char();
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
 * @return
 */
Token get_number() {
    Token token;

    token.type = TOK_NUMBER;
    char cc    = current_char();
    int i      = 0;

    while (is_digit(cc)) {
        token.value[i++] = cc;
        advance(1);
        cc = current_char();
    }
    token.value[i] = '\0'; // null-terminate string
    return token;
}

/**
 * @brief Recognise the identifier, tokenize and return it.
 * @return
 */
Token get_identifier() {
    Token token;

    token.type = TOK_IDENT; // default is identifier
    char cc    = current_char();
    int i      = 0;

    while (is_letter(cc)) {
        token.value[i++] = cc;
        advance(1);
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

/**
 * @brief Recognise next token, tokenize and return it.
 * @return
 */
Token get_next_token() {
    skip_whitespace();

    char cc = current_char();

    if (is_letter(cc)) {
        return get_identifier();
    }

    if (is_digit(cc)) {
        return get_number();
    }

    /*********************************************
     * compound operators
     *********************************************/

    // (==)
    if (cc == '=' && input[position + 1] == '=') {
        advance(2); // consume '=='
        return (Token){TOK_EQ, "=="};
    }

    // (!=)
    if (cc == '!' && input[position + 1] == '=') {
        advance(2); // consume '!='
        return (Token){TOK_NEQ, "!="};
    }

    // (<=)
    if (cc == '<' && input[position + 1] == '=') {
        advance(2); // consume '<='
        return (Token){TOK_LEQ, "<="};
    }

    // (>=)
    if (cc == '>' && input[position + 1] == '=') {
        advance(2); // consume '>='
        return (Token){TOK_GEQ, ">="};
    }

    // (++)
    if (cc == '+' && input[position + 1] == '+') {
        advance(2); // consume '++'
        return (Token){TOK_INCREMENT, "++"};
    }

    // (--)
    if (cc == '-' && input[position + 1] == '-') {
        advance(2); // consume '--'
        return (Token){TOK_DECREMENT, "--"};
    }

    /*********************************************
     * single-character operators
     *********************************************/

    if (cc == '<') {
        advance(1);
        return (Token){TOK_LT, "<"};
    }

    if (cc == '>') {
        advance(1);
        return (Token){TOK_GT, ">"};
    }

    if (cc == '>') {
        advance(1);
        return (Token){TOK_LT, "<"};
    }

    if (cc == '=') {
        advance(1);
        return (Token){TOK_ASSIGN, "="};
    }

    if (cc == '+') {
        advance(1);
        return (Token){TOK_PLUS, "+"};
    }

    if (cc == '-') {
        advance(1);
        return (Token){TOK_MINUS, "-"};
    }

    if (cc == '*') {
        advance(1);
        return (Token){TOK_STAR, "*"};
    }

    if (cc == '/') {
        advance(1);
        return (Token){TOK_FSLASH, "/"};
    }

    if (cc == ';') {
        advance(1);
        return (Token){TOK_SEMICOLON, ";"};
    }

    if (cc == '(') {
        advance(1);
        return (Token){TOK_LPAREN, "("};
    }

    if (cc == ')') {
        advance(1);
        return (Token){TOK_RPAREN, ")"};
    }

    if (cc == '{') {
        advance(1);
        return (Token){TOK_LBRACE, "{"};
    }

    if (cc == '}') {
        advance(1);
        return (Token){TOK_RBRACE, "}"};
    }

    if (cc == '[') {
        advance(1);
        return (Token){TOK_LBRACKET, "["};
    }

    if (cc == ']') {
        advance(1);
        return (Token){TOK_RBRACKET, "]"};
    }

    if (cc == '\0') {
        return (Token){TOK_EOF, ""};
    }

    // Handle invalid characters (error handling)
    fprintf(stderr, "Unexpected character: %c\n", cc);
    exit(1);
}
