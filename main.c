#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    Lexer *lexer = create_lexer("../../source.crx");
    Token token;

    do {
        token = get_next_token(lexer); // fetch the next token
        print_token(token);            // print formatted token
    } while (token.type != TOK_EOF);

    remove_lexer(lexer);
    printf("info # Lexer and lexer buffer cleaned up successfully.\n");

    return 0;
}
