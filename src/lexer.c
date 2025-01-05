#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

const char *input; // input string
int position = 0;  // current position in the input

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
    "void",   // represents absence of a value

    // memory
    "new",   // allocates memory
    "null",  // represents absence of a value for memory location
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
char *tokenstr(const TokenType type) {
    switch (type) {
    case TOK_KEYWORD:    return "TOK_KEYWORD";
    case TOK_IDENT:      return "TOK_IDENT";
    case TOK_NUMBER:     return "TOK_NUMBER";
    case TOK_ASSIGN:     return "TOK_ASSIGN";
    case TOK_PLUS:       return "TOK_PLUS";
    case TOK_MINUS:      return "TOK_MINUS";
    case TOK_STAR:       return "TOK_STAR";
    case TOK_AMPERSAND:  return "TOK_AMPERSAND";
    case TOK_AT:         return "TOK_AT";
    case TOK_HASH:       return "TOK_HASH";
    case TOK_FSLASH:     return "TOK_FSLASH";
    case TOK_BSLASH:     return "TOK_BSLASH";
    case TOK_DOT:        return "TOK_DOT";
    case TOK_COLON:      return "TOK_COLON";
    case TOK_SEMICOLON:  return "TOK_SEMICOLON";
    case TOK_LT:         return "TOK_LT";
    case TOK_GT:         return "TOK_GT";
    case TOK_ARROW:      return "TOK_ARROW";
    case TOK_EQ:         return "TOK_EQ";
    case TOK_NEQ:        return "TOK_NEQ";
    case TOK_GEQ:        return "TOK_GEQ";
    case TOK_LEQ:        return "TOK_LEQ";
    case TOK_ADD_ASSIGN: return "TOK_ADD_ASSIGN";
    case TOK_SUB_ASSIGN: return "TOK_SUB_ASSIGN";
    case TOK_DIV_ASSIGN: return "TOK_DIV_ASSIGN";
    case TOK_MUL_ASSIGN: return "TOK_MUL_ASSIGN";
    case TOK_POW:        return "TOK_POW";
    case TOK_INCREMENT:  return "TOK_INCREMENT";
    case TOK_DECREMENT:  return "TOK_DECREMENT";
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
void token_print(Token token) {
    printf(
        "token: id = %-4d type = %-15s value = '%s'\n", token.type, tokenstr(token.type),
        token.value
    );
}

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
