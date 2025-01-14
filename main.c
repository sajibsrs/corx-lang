#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    const char *src = "../../source.crx";

    Lexer *lex  = make_lexer(src);
    TokArr *arr = lexer_scan(lex);

    printf("Scanned %d tokens:\n", arr->size);

    for (int i = 0; i < arr->size; i++) {
        render_token(arr->tokens[i]);
    }

    // cleanup
    purge_lexer(lex);
    purge_tokarr(arr);

    return 0;
}
