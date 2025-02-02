#include <stdio.h>
#include <time.h>

#include "src/utils.h"
#include "src/lexer.h"
#include "src/parser.h"

int main() {
    clock_t stime = clock();

    const char *src = "../../source.cx";
    TokenList *list = scan(src);

    // print_tokenlist(list); // Print parsed tokens

    Parser *prs = make_parser(list);
    Node *node  = parse_prog(prs);

    // print_ast(node, 0); // Prints AST

    clock_t etime = clock();
    double ttime  = ((double)(etime - stime)) / CLOCKS_PER_SEC * 1000;
    printf("Total time: %f ms\n", ttime);

    // cleanup
    purge_parser(prs);
    purge_tokenlist(list);

    return 0;
}
