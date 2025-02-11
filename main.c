#include <stdio.h>
#include <time.h>

#include "src/utils.h"
#include "src/lexer.h"
#include "src/parser.h"
#include "src/symbol.h"

int main() {
    clock_t stime = clock();

    const char *src = "../../source.cx";
    TokenList *list = scan(src);

    // print_toklist(list); // Print parsed tokens

    Parser *parser = make_parser(list);
    Node *root     = parse_program(parser);

    // print_ast(root, 0); // Prints AST

    clock_t etime = clock();
    double ttime  = ((double)(etime - stime)) / CLOCKS_PER_SEC * 1000;
    printf("Total time: %f ms\n", ttime);

    // cleanup
    purge_parser(parser);
    // purge_tlist(list);

    return 0;
}
