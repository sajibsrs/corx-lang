#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    loadfile("../../source.ox"); // load source file

    Token token;

    do {
        token = get_next_token(); // Fetch the next token
        token_print(token);       // print formatted token
    } while (token.type != TOK_EOF);
    return 0;

    free((char *)input);

    return 0;
}
