#ifndef _ANALYZER_H
#define _ANALYZER_H

#include "parser.h"
#include "symbol.h"

typedef struct Analyzer {
    SymTab *symtab; // Symbol table
    int line;       // Current line in the source
    bool err;       // Error flag
    Symbol *sym;    // Current symbol
} Analyzer;

Analyzer *make_analyzer();

void resolve_program(Analyzer *analyzer, Node *node);
void purge_analyzer(Analyzer *analyzer);

#endif
