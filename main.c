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

    // print_tlist(list); // Print parsed tokens

    Parser *parser = make_parser(list);
    Node *root     = parse_program(parser);

    // print_ast(root, 0); // Prints AST

    // SymbolTable *table = make_symtbl();
    // init_symtbl(table);

    // Symbol *sint = make_symbol("int", SG_TYPE, SA_REF, SF_EXTERNAL, 1);
    // add_symbol(table, sint);

    // Symbol *sfloat = make_symbol("float", SG_TYPE, SA_REF, SF_EXTERNAL, 1);
    // add_symbol(table, sfloat);

    // resprog(node, table);

    // if (table) {
    //     for (unsigned i = 0; i < table->size; i++) {
    //         SymbolNode *node = table->buckets[i];
    //         while (node) {
    //             Symbol *symbol = node->symbol;
    //             printf(
    //                 "Symbol: %s, Group: %d, Action: %d, Modifiers: %u, Scope: %d\n",
    //                 symbol->name, symbol->group, symbol->action, symbol->modspec, symbol->scope
    //             );
    //             node = node->next; // Move to the next symbol in the bucket
    //         }
    //     }
    // }

    // purge_symtbl(table);

    clock_t etime = clock();
    double ttime  = ((double)(etime - stime)) / CLOCKS_PER_SEC * 1000;
    printf("Total time: %f ms\n", ttime);

    // cleanup
    purge_parser(parser);
    // purge_tlist(list);

    return 0;
}
