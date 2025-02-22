#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "symbol.h"

#define INITIAL_SIZE 64 // Initial size

/*********************************************
 * Utility Functions
 *********************************************/

/**
 * @brief Sets the given flags.
 * @param symbol
 * @param newflag
 */
void setmodspec(Symbol *symbol, unsigned newmodspec) {
    symbol->modspec |= newmodspec; // Bitwise OR to set bits
}

/**
 * @brief Checks if specific flags are set.
 * @param symbol
 * @param check
 * @return
 */
bool hasmodspec(Symbol *symbol, unsigned check) {
    return (symbol->modspec & check) == check; // Checks if all bits in 'check' are set
}

/**
 * @brief Sets the given actions.
 * @param symbol
 * @param action
 */
void setaction(Symbol *symbol, unsigned action) {
    symbol->action |= action;
}

/**
 * @brief Checks if specific actions are set.
 * @param symbol
 * @param action
 * @return
 */
bool hasaction(Symbol *symbol, unsigned action) {
    return (symbol->action & action) == action;
}

/*********************************************
 * Symbol Table Functions
 *********************************************/

/**
 * @brief Creates a symbol table.
 * @return Pointer to the new symbol table.
 */
SymTab *make_symtab() {
    SymTab *table = malloc(sizeof(SymTab));
    if (!table) errexit("memory allocation error");

    table->size  = INITIAL_SIZE;
    table->count = 0;
    table->scope = 0;

    table->buckets = calloc(table->size, sizeof(SymNode *));
    if (!table->buckets) errexit("memory allocation error");

    return table;
}

/**
 * @brief Resizes the symbol table when load factor is exceeded.
 */
void resize_symtab(SymTab *table) {
    unsigned new_size = table->size * 2; // Double size

    SymNode **new_buckets = calloc(new_size, sizeof(SymNode *));
    if (!new_buckets) errexit("memory allocation error");

    for (unsigned i = 0; i < table->size; i++) {
        SymNode *node = table->buckets[i];

        while (node != NULL) {
            SymNode *next = node->next;

            // Rehash and insert into new buckets
            unsigned new_index     = hashfnv(node->symbol->name, new_size);
            node->next             = new_buckets[new_index];
            new_buckets[new_index] = node;

            node = next;
        }
    }

    free(table->buckets);
    table->buckets = new_buckets;
    table->size    = new_size;
}

/**
 * @brief Creates a new symbol.
 * @param name Symbol name.
 * @param group Symbol group (variable, function, etc.).
 * @param action Symbol action (declaration, definition, invocation).
 * @param modspec Symbol flags (modifiers, access, etc.).
 * @param scope Scope level.
 * @param type Pointer to the type symbol
 * @return Pointer to the created symbol.
 */
Symbol *make_symbol(
    const char *name, SymGrp group, SymAct action, unsigned modspec, int scope, Symbol *type
) {
    Symbol *symbol = malloc(sizeof(Symbol));
    if (!symbol) errexit("memory allocation error");

    symbol->name = strdup(name); // Copy name
    if (!symbol->name) errexit("strdup failed");

    symbol->group   = group;
    symbol->action  = action;
    symbol->modspec = modspec;
    symbol->scope   = scope;
    symbol->type    = type; // Set the data type

    return symbol;
}

/**
 * @brief Adds a symbol to the symbol table.
 * @param table Symbol table.
 * @param symbol Symbol to add.
 */
void add_symbol(SymTab *table, Symbol *symbol) {
    if (table->count >= INITIAL_SIZE) {
        resize_symtab(table);
    }

    unsigned index = hashfnv(symbol->name, table->size);
    SymNode *snode = malloc(sizeof(SymNode));
    if (!snode) errexit("memory allocation error");

    snode->symbol         = symbol;
    snode->next           = table->buckets[index];
    table->buckets[index] = snode;
    table->count++;
}

/**
 * @brief Searches for a symbol by name.
 * @param table Symbol table.
 * @param name Name of the symbol.
 * @param scope Current scope.
 * @return Pointer to the found symbol, or NULL if not found.
 */
Symbol *search_symbol(SymTab *table, const char *name, int current_scope) {
    for (int scope = current_scope; scope >= 0; scope--) {
        unsigned index = hashfnv(name, table->size);
        SymNode *node  = table->buckets[index];
        while (node) {
            if (strcmp(node->symbol->name, name) == 0 && node->symbol->scope == scope) {
                return node->symbol;
            }
            node = node->next;
        }
    }
    return NULL;
}

/**
 * @brief Initialize symbol table.
 * @param table
 */
void init_symtab(SymTab *table) {
    Symbol *intsym = make_symbol("int", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, intsym);
    intsym->type = intsym;

    Symbol *fltsym = make_symbol("float", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, fltsym);
    fltsym->type = fltsym;

    Symbol *charsym = make_symbol("char", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, charsym);
    charsym->type = charsym;

    Symbol *strsym = make_symbol("string", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, strsym);
    strsym->type = strsym;

    Symbol *voidsym = make_symbol("void", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, voidsym);
    voidsym->type = voidsym;

    Symbol *boolsym = make_symbol("bool", SG_TYPE, SA_DEC, 0, 0, NULL);
    add_symbol(table, boolsym);
    boolsym->type = boolsym;
}

/*********************************************
 * Cleanup Functions
 *********************************************/

/**
 * @brief Frees the entire symbol table.
 * @param table Symbol table to free.
 */
void purge_symtab(SymTab *table) {
    if (!table) return;

    for (unsigned i = 0; i < table->size; i++) {
        SymNode *node = table->buckets[i];

        while (node != NULL) {
            SymNode *temp = node;
            node          = node->next;

            free(temp->symbol->name); // Free symbol name
            free(temp->symbol);       // Free symbol itself
            free(temp);               // Free node
        }
    }
    free(table->buckets);
    free(table);
}
