#include "stdio.h"
#include "utils.h"

/**
 * @brief Returns hash for string (Fowler–Noll–Vo hash).
 * @param str String input.
 * @param size Hash map size.
 * @return
 */
unsigned int hashfnv(const char *str, const int size) {
    unsigned int hash = 2166136261u; // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)(*str); // XOR with byte
        hash *= 16777619u;             // FNV prime
        str++;
    }

    return hash % size;
}

/**
 * @brief Print abstract syntax tree (AST).
 * @param node Node to start with.
 * @param depth Initial depth.
 */
void print_ast(const Node *node, int depth) {
    if (!node) {
        printf("NULL node at depth %d\n", depth);

        return;
    }

    // Indentation based on depth
    for (int i = 0; i < depth; i++) {
        printf("- "); // Two spaces for each level
    }

    // Print node information
    printf(
        " %-18s %-26s (%d child%s)\n", node->str ? node->str : "NULL", ntypestr[node->type],
        node->count,
        node->count == 1 ? "" : "ren" // Singular/plural
    );

    // Recursively print children
    for (int i = 0; i < node->count; i++) {
        print_ast(node->nodes[i], depth + 1);
    }
}

/**
 * @brief Print formatted token to the terminal.
 * @param list
 */
void print_tokenlist(const TokenList *list) {
    printf("Scanned %d tokens:\n\n", list->count);

    Token token;

    for (int i = 0; i < list->count; i++) {
        token = list->tokens[i];
        printf(
            "%-16s %-10s typ:%-4d lin:%-4d col:%d\n", //
            ttypestr[token.type], token.str, token.type, token.line, token.column
        );
    }
}
