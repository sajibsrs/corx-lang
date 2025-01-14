#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/lexer.h"

int main() {
    const char *src = "../../source.crx";

    Lexer *lexer     = make_lexer(src);
    TokenArr *tokarr = lexer_scan(lexer);

    printf("Scanned %d tokens:\n", tokarr->size);

    for (int i = 0; i < tokarr->size; i++) {
        render_token(tokarr->tokens[i]);
    }

    // cleanup
    purge_lexer(lexer);
    purge_tokarr(tokarr);

    return 0;
}
