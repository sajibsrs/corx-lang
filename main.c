#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    loadfile("../../source.cx"); // load source file

    Token token;

    // iterate and print tokens until EOF is reached
    do {
        token = get_next_token();
        printf("Token: Type = %d, Value = '%s'\n", token.type, token.value);
    } while (token.type != TOK_EOF);

    free((char *)input);

    return 0;
}
