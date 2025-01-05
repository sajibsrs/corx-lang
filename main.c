#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    input = "if x = 10; else return; x == y; x != z;";

    Token token;

    // Iterate and print tokens until EOF is reached
    do {
        token = get_next_token();
        printf("Token: Type = %d, Value = '%s'\n", token.type, token.value);
    } while (token.type != TOK_EOF);

    return 0;
}
