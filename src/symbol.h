#ifndef _SYMBOL_H
#define _SYMBOL_H

#include "lexer.h"

// Symbol group ( e.g. SG_TYPE, SG_VAR )
typedef enum {
    SG_TYPE,     // Data type
    SG_VAR,      // Variable
    SG_FUNC,     // Function
    SG_METHOD,   // Method
    SG_PARAM,    // Parameters
    SG_CONTRACT, //
    SG_STRUCT,   //
    SG_ENUM,     //
    SG_POINTER,  //
} SymGrp;

// Symbol action
typedef enum {
    SA_DEC = 1 << 0, // Declaration
    SA_DEF = 1 << 1, // Definition
    SA_INV = 1 << 2, // Invocation
    SA_REF = 1 << 3, // Reference / Usage
} SymAct;

// Combined modifiers and specifiers flags
typedef enum {
    SF_CONST    = 1 << 0, // Modifier const
    SF_STATIC   = 1 << 1, // Specifier static
    SF_EXTERNAL = 1 << 2, // Access external
    SF_INTERNAL = 1 << 3, // Access internal
    SF_RESTRICT = 1 << 4, // Access restrict
} SymMsp;

typedef struct Symbol {
    char *name;          // Symbol name (string)
    SymGrp group;        // Symbol group
    SymAct action;       // Symbol action flags
    unsigned modspec;    // Symbol modifier and specifier flags
    int scope;           // Scope level
    struct Symbol *type; // Type of the symbol (e.g. "int", "float")
} Symbol;

typedef struct SymNode {
    Symbol *symbol;       // Symbol data
    struct SymNode *next; // Next symbol in the bucket
} SymNode;

// Symbol table
typedef struct SymTab {
    SymNode **buckets; // Array of buckets
    unsigned size;     // Number of buckets
    unsigned count;    // Number of symbols in table
    unsigned scope;    // Current scope
} SymTab;

// Semantic error
typedef enum {
    SERR_OK,            // Ok
    SERR_UNDECLARED,    // Undeclared
    SERR_REDECLARATION, // Redeclaration
    SERR_TYPE_MISMATCH, // Type mismatch
} SemErr;

void setmodspec(Symbol *symbol, unsigned newflags);
void setaction(Symbol *symbol, unsigned action);

bool hasmodspec(Symbol *symbol, unsigned check);
bool hasaction(Symbol *symbol, unsigned action);

SymTab *make_symtab();

void init_symtab(SymTab *table);
void purge_symtab(SymTab *table);
void resize_symtab(SymTab *table);

Symbol *make_symbol(
    const char *name, SymGrp group, SymAct action, unsigned modspec, int scope, Symbol *type
);

void add_symbol(SymTab *table, Symbol *symbol);
Symbol *search_symbol(SymTab *table, const char *name, int scope);

#endif
