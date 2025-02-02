#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "src/lexer.h"
#include "src/parser.h"

void print_tree(const Node *node, int depth) {
    if (!node) {
        printf("NULL node at depth %d\n", depth);
        return;
    }

    // indentation based on depth
    for (int i = 0; i < depth; i++) {
        printf("- "); // Two spaces for each level
    }

    // print node information
    printf(
        " %-18s %-26s (%d child%s)\n", node->str ? node->str : "NULL", ntypestr[node->type],
        node->count,
        node->count == 1 ? "" : "ren" // proper grammar for singular/plural
    );

    // recursively print children, if any
    for (int i = 0; i < node->count; i++) {
        print_tree(node->nodes[i], depth + 1);
    }
}

int main() {
    clock_t stime = clock();

    const char *src = "../../source.cx";
    TokenList *list = scan(src);

    // print_tokenlist(list);

    Parser *prs = make_parser(list);
    Node *node  = parse_prog(prs);

    // print_tree(node, 0);

    // cleanup
    purge_parser(prs);
    purge_tokenlist(list);

    clock_t etime = clock();
    double ttime  = ((double)(etime - stime)) / CLOCKS_PER_SEC * 1000;
    printf("Total time: %f ms\n", ttime);

    return 0;
}
