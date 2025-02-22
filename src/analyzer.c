#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "analyzer.h"

/* Function prototypes */
void resolve_program(Analyzer *anz, Node *node);

static void resolve_node(Analyzer *anz, Node *node);
static void resolve_block(Analyzer *anz, BlockNode *block);
static void resolve_func(Analyzer *anz, FuncNode *fn);
static void resolve_param(Analyzer *anz, VarNode *param);
static void resolve_var_decl(Analyzer *anz, VarNode *var);
static void resolve_for(Analyzer *anz, StmtNode *stmt);
static void resolve_statement(Analyzer *anz, StmtNode *stmt);
static void resolve_assignment(Analyzer *anz, StmtNode *stmt);
static void resolve_if(Analyzer *anz, StmtNode *stmt);
static void resolve_while(Analyzer *anz, StmtNode *stmt);
static void resolve_do_while(Analyzer *anz, StmtNode *stmt);
static void resolve_return(Analyzer *anz, StmtNode *stmt);

static Symbol *resolve_expression(Analyzer *anz, Node *node);
static Symbol *resolve_binary_expr(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_expr_node(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_var_expr(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_unary_expr(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_call_expr(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_conditional_expr(Analyzer *anz, ExprNode *expr);
static Symbol *resolve_variable(SymTab *table, const char *name, int scope);

static bool is_same_type(Symbol *t1, Symbol *t2);
static bool is_arithmetic(Symbol *type);
static bool is_pointer(Symbol *type);
static bool is_boolean(Symbol *type);
static bool is_comparable(Symbol *t1, Symbol *t2);
static bool is_scalar(Symbol *type);
static bool is_compatible(Symbol *t1, Symbol *t2);

static Symbol *get_bool_type(Analyzer *type);
static Symbol *numeric_promotion(Analyzer *anz, Symbol *s1, Symbol *s2);

static inline char *make_unique(const char *name, int scope);

/*********************************************
 * Helper Functions
 *********************************************/

/**
 * @brief Checks if two symbols have the same type.
 *
 * @param a First symbol.
 * @param b Second symbol.
 * @return true if both are of type SG_TYPE and their names match.
 */
static bool is_same_type(Symbol *t1, Symbol *t2) {
    return (t1->group == SG_TYPE && t2->group == SG_TYPE) && (strcmp(t1->name, t2->name) == 0);
}

/**
 * @brief Determines if the given symbol represents an arithmetic type.
 *
 * @param type Pointer to the symbol.
 * @return true if the symbol's name is "int", "float", or "char".
 */
static bool is_arithmetic(Symbol *type) {
    return type->group == SG_TYPE &&
           ((strcmp(type->name, "int") == 0 || strcmp(type->name, "float") == 0 ||
             strcmp(type->name, "char") == 0));
}

/**
 * @brief Determines if the given symbol represents a pointer type.
 *
 * @param type Pointer to the symbol.
 * @return true if the symbol's group is SG_POINTER.
 */
static bool is_pointer(Symbol *type) {
    return type->group == SG_POINTER;
}

/**
 * @brief Determines if the given symbol represents a boolean type.
 *
 * @param type Pointer to the symbol.
 * @return true if the symbol's name is "bool".
 */
static bool is_boolean(Symbol *type) {
    return type->group == SG_TYPE && strcmp(type->name, "bool") == 0;
}

/**
 * @brief Checks if two symbols are comparable.
 *
 * Two symbols are comparable if they are both arithmetic or both pointer types.
 *
 * @param t1 First symbol.
 * @param t2 Second symbol.
 * @return true if they are comparable, false otherwise.
 */
static bool is_comparable(Symbol *t1, Symbol *t2) {
    return ((is_arithmetic(t1) && is_arithmetic(t2)) || (is_pointer(t1) && is_pointer(t2)));
}

/**
 * @brief Retrieves the boolean type symbol.
 *
 * @param anz Pointer to the analyzer.
 * @return Pointer to the symbol for "bool".
 */
static Symbol *get_bool_type(Analyzer *anz) {
    return search_symbol(anz->symtbl, "bool", 0);
}

/**
 * @brief Performs numeric promotion on two symbols.
 *
 * If either symbol is "float", returns the float type; otherwise returns int.
 *
 * @param a Pointer to the analyzer.
 * @param s1 First symbol.
 * @param s2 Second symbol.
 * @return Pointer to the promoted type symbol.
 */
static Symbol *numeric_promotion(Analyzer *a, Symbol *s1, Symbol *s2) {
    if (strcmp(s1->name, "float") == 0 || strcmp(s2->name, "float") == 0) {
        return search_symbol(a->symtbl, "float", 0);
    }

    return search_symbol(a->symtbl, "int", 0);
}

/**
 * @brief Checks if a symbol is scalar.
 *
 * A scalar is an arithmetic, pointer, or boolean type.
 *
 * @param type Pointer to the symbol.
 * @return true if scalar, false otherwise.
 */
static bool is_scalar(Symbol *type) {
    return (is_arithmetic(type) || is_pointer(type) || is_boolean(type));
}

/**
 * @brief Determines if two symbols are compatible.
 *
 * Compatibility means they are the same type or both arithmetic.
 *
 * @param s1 First symbol.
 * @param s2 Second symbol.
 * @return true if compatible, false otherwise.
 */
static bool is_compatible(Symbol *s1, Symbol *s2) {
    if (is_same_type(s1, s2)) return true;
    return (is_arithmetic(s1) && is_arithmetic(s2));
}

/**
 * @brief Checks for a variable redeclaration in the current scope.
 *
 * Exits the program if a duplicate is found.
 *
 * @param table Pointer to the symbol table.
 * @param name The variable name.
 * @param line Source code line number.
 */
void check_vardecl(SymTab *table, const char *name, int line) {
    Symbol *sym = search_symbol(table, name, table->scope);
    if (sym) {
        fprintf(stderr, "Error (line %d): Redeclaration of '%s'\n", line, name);
        exit(1);
    }
}

/**
 * @brief Checks that the assignment between two symbols is valid.
 *
 * Exits the program if the types are not the same.
 *
 * @param table Pointer to the symbol table.
 * @param lhs Left-hand side symbol.
 * @param rhs Right-hand side symbol.
 * @param line Source code line number.
 */
void is_assignable(SymTab *table, Symbol *lhs, Symbol *rhs, int line) {
    if (!is_same_type(lhs->type, rhs->type)) {
        fprintf(
            stderr, "Error (line %d): Cannot assign %s to %s\n", line, rhs->type->name,
            lhs->type->name
        );
        exit(1);
    }
}

/**
 * @brief Searches for a variable in the symbol table using unique naming.
 *
 * For each scope from the given scope down to 0, constructs a unique name (e.g., a.0, a.1)
 * and searches for the corresponding symbol.
 *
 * @param table Pointer to the symbol table.
 * @param name The base variable name.
 * @param scope The starting scope level.
 * @return Pointer to the found symbol or NULL if not found.
 */
static Symbol *resolve_variable(SymTab *table, const char *name, int scope) {
    for (int s = scope; s >= 0; s--) {
        char *uname = make_unique(name, s);
        Symbol *sym = search_symbol(table, uname, s);
        free(uname);
        if (sym) return sym;
    }
    return NULL;
}

/**
 * @brief Enters a new scope.
 *
 * Increments the scope level in the symbol table.
 *
 * @param table Pointer to the symbol table.
 */
void scope_enter(SymTab *table) {
    table->scope++;
}

/**
 * @brief Exits the current scope.
 *
 * Removes all symbols declared in the current scope and decrements the scope level.
 *
 * @param table Pointer to the symbol table.
 */
void scope_exit(SymTab *table) {
    for (unsigned i = 0; i < table->size; i++) {
        SymNode **ptr = &table->buckets[i];

        while (*ptr) {
            if ((*ptr)->symbol->scope == table->scope) {
                SymNode *temp = *ptr;
                *ptr          = temp->next;
                free(temp->symbol->name);
                free(temp->symbol);
                free(temp);
                table->count--;
            } else {
                ptr = &(*ptr)->next;
            }
        }
    }
    table->scope--;
}

/**
 * @brief Generates a unique name based on the given name and scope.
 *
 * Allocates a new string in the format "name.scope".
 *
 * @param name The base name.
 * @param scope The scope level.
 * @return Pointer to the unique name. Must be freed by the caller.
 */
static inline char *make_unique(const char *name, int scope) {
    char *uname = malloc(32);
    if (!uname) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    snprintf(uname, 32, "%s.%d", name, scope);
    return uname;
}

/*********************************************
 * Core Analyzer Functions
 *********************************************/

/**
 * @brief Creates and initializes a new analyzer.
 *
 * Allocates memory for an Analyzer and initializes its symbol table.
 *
 * @return Pointer to the newly created Analyzer.
 */
Analyzer *make_analyzer() {
    Analyzer *anz = malloc(sizeof(Analyzer));
    if (!anz) errexit("make_analyzer allocation failed");

    anz->symtbl = make_symtbl();
    anz->line   = 0;
    anz->err    = false;

    init_symtbl(anz->symtbl);
    return anz;
}

/**
 * @brief Analyzes the entire program.
 *
 * Expects the root node to be of type NODE_PROGRAM and processes all child nodes.
 *
 * @param anz Pointer to the Analyzer.
 * @param node Pointer to the root node.
 */
void resolve_program(Analyzer *anz, Node *node) {
    if (node->type != NODE_PROGRAM) errexit("Expected program node");

    ProgNode *prog = (ProgNode *)node;
    for (int i = 0; i < prog->icount; i++) {
        anz->line = prog->items[i]->line;
        resolve_node(anz, prog->items[i]);
    }

    if (anz->err) {
        fprintf(stderr, "Compilation failed with semantic errors\n");
        exit(1);
    }

    printf("Compilation successful.\n");
}

/*********************************************
 * Node Analysis
 *********************************************/

/**
 * @brief Analyzes a generic AST node.
 *
 * Dispatches analysis based on the node type.
 *
 * @param anz Pointer to the Analyzer.
 * @param node Pointer to the node.
 */
static void resolve_node(Analyzer *anz, Node *node) {
    switch (node->type) {
    case NODE_VAR_DECL:  resolve_var_decl(anz, (VarNode *)node); break;
    case NODE_FUNC_DECL: resolve_func(anz, (FuncNode *)node); break;
    case NODE_BLOCK:     resolve_block(anz, (BlockNode *)node); break;
    case NODE_STATEMENT: resolve_statement(anz, (StmtNode *)node); break;
    default:             fprintf(stderr, "Warning: Unhandled node type %d\n", node->type);
    }
}

/*********************************************
 * Variable Declaration
 *********************************************/

/**
 * @brief Analyzes a variable declaration.
 *
 * Resolves the variable type, generates a unique name, checks for duplicates,
 * and processes the initializer if present.
 *
 * @param anz Pointer to the Analyzer.
 * @param var Pointer to the variable declaration node.
 */
static void resolve_var_decl(Analyzer *anz, VarNode *var) {
    Symbol *vtype = search_symbol(anz->symtbl, var->dtype, 0);
    if (!anz) {
        fprintf(stderr, "Error (line %d): Unknown type '%s'\n", var->base.line, var->dtype);
        anz->err = true;
        return;
    }

    printf(
        "Variable (%s) resolves to type (%s) (scope %d)\n", var->name, vtype->name,
        anz->symtbl->scope
    );

    char *name = make_unique(var->name, anz->symtbl->scope);

    Symbol *dup = search_symbol(anz->symtbl, name, anz->symtbl->scope);
    if (dup) {
        fprintf(
            stderr, "Error (line %d): Redeclaration of variable '%s'\n", var->base.line, var->name
        );
        anz->err = true;
        return;
    }

    Symbol *sym = make_symbol(name, SG_VAR, SA_DEC, 0, anz->symtbl->scope, vtype);
    add_symbol(anz->symtbl, sym);

    if (var->init) {
        Symbol *init_type = resolve_expression(anz, var->init);
        if (!init_type) {
            fprintf(
                stderr, "Error (line %d): Invalid initializer for '%s'\n", var->base.line, var->name
            );
            anz->err = true;
            return;
        }
        if (!is_compatible(vtype, init_type)) {
            fprintf(
                stderr, "Error (line %d): Invalid initializer type for '%s'\n", var->base.line,
                var->name
            );
            anz->err = true;
        }
    }
}

/*********************************************
 * Function Analysis
 *********************************************/

/**
 * @brief Analyzes a function declaration.
 *
 * Resolves the return type, checks for duplicates, processes parameters,
 * and analyzes the function body.
 *
 * @param anz Pointer to the Analyzer.
 * @param fn Pointer to the function declaration node.
 */
static void resolve_func(Analyzer *anz, FuncNode *fn) {
    Symbol *rtype = search_symbol(anz->symtbl, fn->dtype, 0);
    if (!rtype) {
        fprintf(stderr, "Error (line %d): Unknown return type '%s'\n", fn->base.line, fn->dtype);
        anz->err = true;
        return;
    }

    Symbol *ex_decl = search_symbol(anz->symtbl, fn->name, anz->symtbl->scope);
    if (ex_decl) {
        fprintf(
            stderr, "Error (line %d): Redeclaration of function '%s'\n", fn->base.line, fn->name
        );
        anz->err = true;
        return;
    }

    Symbol *fsym = make_symbol(fn->name, SG_FUNC, SA_DEC, 0, 0, rtype);
    add_symbol(anz->symtbl, fsym);

    anz->sym = fsym;
    scope_enter(anz->symtbl);

    for (int i = 0; i < fn->pcount; i++) {
        if (fn->params[i]->type != NODE_VAR_DECL) {
            fprintf(stderr, "Error (line %d): Invalid parameter syntax\n", fn->base.line);
            anz->err = true;
            continue;
        }
        VarNode *param = (VarNode *)fn->params[i];
        resolve_param(anz, param);
    }

    if (fn->body) {
        resolve_block(anz, (BlockNode *)fn->body);
    }

    scope_exit(anz->symtbl);
    anz->sym = NULL;
}

/**
 * @brief Analyzes a function parameter.
 *
 * Resolves the parameter's type, generates a unique name, checks for duplicates,
 * and adds the parameter to the current scope.
 *
 * @param anz Pointer to the Analyzer.
 * @param param Pointer to the parameter node.
 */
static void resolve_param(Analyzer *anz, VarNode *param) {
    Symbol *ptype = search_symbol(anz->symtbl, param->dtype, 0);
    if (!ptype) {
        fprintf(
            stderr, "Error (line %d): Unknown parameter type '%s'\n", param->base.line, param->dtype
        );
        anz->err = true;
        return;
    }

    char *uname = make_unique(param->name, anz->symtbl->scope);

    Symbol *duplicate = search_symbol(anz->symtbl, uname, anz->symtbl->scope);
    if (duplicate) {
        fprintf(
            stderr, "Error (line %d): Duplicate parameter '%s'\n", param->base.line, param->name
        );
        anz->err = true;
        free(uname);
        return;
    }

    Symbol *psym = make_symbol(uname, SG_PARAM, SA_DEC, 0, anz->symtbl->scope, ptype);
    add_symbol(anz->symtbl, psym);

    printf(
        "Resolved param type %s for '%s' (scope %d)\n", ptype->name, param->name, anz->symtbl->scope
    );
}

/*********************************************
 * Statement Analysis
 *********************************************/

/**
 * @brief Analyzes a statement.
 *
 * Dispatches to the appropriate statement analysis function based on the statement type.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the statement node.
 */
static void resolve_statement(Analyzer *anz, StmtNode *stmt) {
    anz->line = stmt->base.line;

    switch (stmt->type) {
    case STMT_RETURN:     resolve_return(anz, stmt); break;
    case STMT_EXPRESSION: resolve_expression(anz, stmt->u.simple.expr); break;
    case STMT_ASSIGNMENT: resolve_assignment(anz, stmt); break;
    case STMT_IF:         resolve_if(anz, stmt); break;
    case STMT_FOR:        resolve_for(anz, stmt); break;
    case STMT_WHILE:      resolve_while(anz, stmt); break;
    case STMT_DO_WHILE:   resolve_do_while(anz, stmt); break;
    case STMT_COMPOUND:   resolve_block(anz, (BlockNode *)stmt->u.simple.expr); break;
    default:
        fprintf(stderr, "Unhandled statement type: %d\n", stmt->type);
        errexit("Unsupported statement");
    }
}

/**
 * @brief Analyzes an assignment statement.
 *
 * Verifies that the left-hand side is declared and that the right-hand side
 * is compatible with it.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the assignment statement node.
 */
static void resolve_assignment(Analyzer *anz, StmtNode *stmt) {
    Symbol *lhs = resolve_variable(anz->symtbl, stmt->u.assignment.lhs, anz->symtbl->scope);
    if (!lhs) {
        fprintf(
            stderr, "Error (line %d): Undeclared variable '%s'\n", anz->line, stmt->u.assignment.lhs
        );
        anz->err = true;
        return;
    }

    Symbol *rhs = resolve_expression(anz, stmt->u.assignment.rhs);
    if (!is_compatible(lhs->type, rhs->type)) {
        fprintf(
            stderr, "Error (line %d): Cannot assign %s to %s\n", anz->line, rhs->type->name,
            lhs->type->name
        );
        anz->err = true;
    }
}

/**
 * @brief Analyzes a block of code.
 *
 * Enters a new scope, analyzes each statement in the block, and then exits the scope.
 *
 * @param anz Pointer to the Analyzer.
 * @param block Pointer to the BlockNode.
 */
static void resolve_block(Analyzer *anz, BlockNode *block) {
    scope_enter(anz->symtbl);

    for (int i = 0; i < block->icount; i++) {
        resolve_node(anz, block->items[i]);
    }
    scope_exit(anz->symtbl);
}

/*********************************************
 * Control Flow Analysis
 *********************************************/

/**
 * @brief Analyzes an if statement.
 *
 * Verifies that the condition is scalar and analyzes both the then and else branches.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the if statement node.
 */
static void resolve_if(Analyzer *anz, StmtNode *stmt) {
    Symbol *cond = resolve_expression(anz, stmt->u.if_stmt.condition);

    if (!is_scalar(cond)) {
        fprintf(stderr, "Error (line %d): If condition must be scalar type\n", stmt->base.line);
        anz->err = true;
    }
    resolve_node(anz, stmt->u.if_stmt.then_stmt);

    if (stmt->u.if_stmt.else_stmt) resolve_node(anz, stmt->u.if_stmt.else_stmt);
}

/**
 * @brief Analyzes a for loop.
 *
 * Enters a new scope for loop variables, analyzes initialization, condition, and body.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the for loop statement node.
 */
static void resolve_for(Analyzer *anz, StmtNode *stmt) {
    scope_enter(anz->symtbl);

    if (stmt->u.for_stmt.init) resolve_node(anz, stmt->u.for_stmt.init);

    if (stmt->u.for_stmt.condition) {
        Symbol *cond = resolve_expression(anz, stmt->u.for_stmt.condition);
        if (!is_scalar(cond)) {
            fprintf(stderr, "Error (line %d): For condition must be scalar\n", stmt->base.line);
        }
    }
    resolve_node(anz, stmt->u.for_stmt.body);
    scope_exit(anz->symtbl);
}

/**
 * @brief Analyzes a do-while loop.
 *
 * Analyzes the loop body and condition, ensuring the condition is scalar.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the do-while loop statement node.
 */
static void resolve_do_while(Analyzer *anz, StmtNode *stmt) {
    resolve_node(anz, stmt->u.while_stmt.body);

    Symbol *cond = resolve_expression(anz, stmt->u.while_stmt.condition);

    if (!is_scalar(cond)) {
        fprintf(
            stderr, "Error (line %d): Do-while condition must be scalar type\n", stmt->base.line
        );
        anz->err = true;
    }
}

/*********************************************
 * Expression Analysis
 *********************************************/

/**
 * @brief Analyzes an expression.
 *
 * Dispatches to the appropriate expression analysis function based on the node type.
 *
 * @param anz Pointer to the Analyzer.
 * @param node Pointer to the expression node.
 * @return Pointer to the symbol representing the expression type.
 */
static Symbol *resolve_expression(Analyzer *anz, Node *node) {
    switch (node->type) {
    case NODE_EXPRESSION: return resolve_expr_node(anz, (ExprNode *)node);
    default:              fprintf(stderr, "Unexpected expression node type: %d\n", node->type); return NULL;
    }
}

/**
 * @brief Analyzes a generic expression node.
 *
 * Dispatches to specialized functions based on the expression type.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the expression node.
 * @return Pointer to the symbol representing the expression type.
 */
static Symbol *resolve_expr_node(Analyzer *anz, ExprNode *expr) {
    switch (expr->type) {
    case EXPR_CONSTANT: {
        Symbol *int_type = search_symbol(anz->symtbl, "int", 0);

        printf(
            "Constant (%d) resolves to type (%s) (scope %d)\n", expr->u.value, int_type->name,
            anz->symtbl->scope
        );

        return int_type;
    }
    case EXPR_VAR:         return resolve_var_expr(anz, expr);
    case EXPR_UNARY:       return resolve_unary_expr(anz, expr);
    case EXPR_BINARY:      return resolve_binary_expr(anz, expr);
    case EXPR_CONDITIONAL: return resolve_conditional_expr(anz, expr);
    case EXPR_CALL:        return resolve_call_expr(anz, expr);
    default:               fprintf(stderr, "Unhandled expression type: %d\n", expr->type); return NULL;
    }
}

/**
 * @brief Analyzes a variable expression.
 *
 * Looks up the variable using the unique naming scheme across scopes.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the variable expression node.
 * @return Pointer to the symbol for the variable.
 */
static Symbol *resolve_var_expr(Analyzer *anz, ExprNode *expr) {
    Symbol *sym = resolve_variable(anz->symtbl, expr->u.name, anz->symtbl->scope);
    if (!sym) {
        fprintf(stderr, "Error (line %d): Undeclared variable '%s'\n", anz->line, expr->u.name);
        anz->err = true;
    }
    return sym;
}

/**
 * @brief Analyzes a unary expression.
 *
 * Analyzes the operand and performs type checking based on the unary operator.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the unary expression node.
 * @return Pointer to the resulting symbol type.
 */
static Symbol *resolve_unary_expr(Analyzer *anz, ExprNode *expr) {
    Symbol *operand = resolve_expression(anz, expr->u.unary.operand);

    switch (expr->u.unary.op) {
    case UN_NOT:
        if (!is_boolean(operand)) {
            fprintf(stderr, "Error (line %d): Logical NOT requires boolean\n", anz->line);
            anz->err = true;
        }
        return operand;
    default: return operand;
    }
}

/**
 * @brief Analyzes a binary expression.
 *
 * Checks operands for compatibility based on the operator and performs
 * numeric promotion if necessary.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the binary expression node.
 * @return Pointer to the symbol representing the result type.
 */
static Symbol *resolve_binary_expr(Analyzer *anz, ExprNode *expr) {
    Symbol *lsym = resolve_expression(anz, expr->u.binary.left);
    Symbol *rsym = resolve_expression(anz, expr->u.binary.right);

    if (!lsym || !rsym || !lsym->type || !rsym->type) {
        anz->err = true;
        return NULL;
    }

    switch (expr->u.binary.op) {
    case BIN_GT:
    case BIN_LT:
        if (!is_comparable(lsym->type, rsym->type)) {
            fprintf(
                stderr, "Error (line %d): Cannot compare %s and %s\n", expr->base.line,
                lsym->type->name, rsym->type->name
            );
            anz->err = true;
        }
        return get_bool_type(anz);
    case BIN_LTEQ:
    case BIN_GTEQ:
    case BIN_EQ:
    case BIN_NEQ:
        if (!is_comparable(lsym->type, rsym->type)) {
            fprintf(
                stderr, "Error (line %d): Cannot compare %s and %s\n", expr->base.line,
                lsym ? lsym->name : "<?>", rsym ? rsym->name : "<?>"
            );
            anz->err = true;
        }
        return get_bool_type(anz);
    case BIN_ADD:
    case BIN_SUB:
    case BIN_MUL:
    case BIN_DIV:
    case BIN_MOD: {
        if (!is_arithmetic(lsym->type) || !is_arithmetic(rsym->type)) {
            fprintf(stderr, "Error (line %d): Invalid arithmetic operands\n", expr->base.line);
            anz->err = true;
            return NULL;
        }

        if (expr->u.binary.op == BIN_MOD) {
            bool left_is_int =
                (strcmp(lsym->type->name, "int") == 0 || strcmp(lsym->type->name, "char") == 0);
            bool right_is_int =
                (strcmp(rsym->type->name, "int") == 0 || strcmp(rsym->type->name, "char") == 0);

            if (!left_is_int || !right_is_int) {
                fprintf(
                    stderr, "Error (line %d): '%%' requires integer operands\n", expr->base.line
                );
                anz->err = true;
            }
        }
        return numeric_promotion(anz, lsym->type, rsym->type);
    }
    case BIN_AND:
    case BIN_OR:
        if (!is_boolean(lsym->type) || !is_boolean(rsym->type)) {
            fprintf(stderr, "Error (line %d): Logical operators need booleans\n", expr->base.line);
            anz->err = true;
        }
        return get_bool_type(anz);
    default: return lsym;
    }
}

/**
 * @brief Analyzes a conditional (ternary) expression.
 *
 * Verifies that the condition is scalar and that both result expressions
 * are compatible.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the conditional expression node.
 * @return Pointer to the symbol representing the resulting type.
 */
static Symbol *resolve_conditional_expr(Analyzer *anz, ExprNode *expr) {
    Symbol *cond_sym  = resolve_expression(anz, expr->u.conditional.condition);
    Symbol *cond_type = cond_sym->type;

    if (!is_scalar(cond_type)) {
        fprintf(stderr, "Error (line %d): Ternary condition must be scalar\n", expr->base.line);
        anz->err = true;
    }
    Symbol *true_sym   = resolve_expression(anz, expr->u.conditional.true_expr);
    Symbol *false_sym  = resolve_expression(anz, expr->u.conditional.false_expr);
    Symbol *true_type  = true_sym->type;
    Symbol *false_type = false_sym->type;

    if (!is_compatible(true_type, false_type)) {
        fprintf(
            stderr, "Error (line %d): Ternary types mismatch (%s vs %s)\n", expr->base.line,
            true_type->name, false_type->name
        );
        anz->err = true;
    }
    return numeric_promotion(anz, true_type, false_type);
}

/**
 * @brief Analyzes a function call expression.
 *
 * Checks that the callee is a valid function and returns its type.
 *
 * @param anz Pointer to the Analyzer.
 * @param expr Pointer to the call expression node.
 * @return Pointer to the symbol representing the function's return type.
 */
static Symbol *resolve_call_expr(Analyzer *anz, ExprNode *expr) {
    ExprNode *exp = (ExprNode *)expr->u.call.callee;
    if (!exp || exp->type != EXPR_VAR) {
        fprintf(stderr, "Error (line %d): Invalid function call\n", anz->line);
        anz->err = true;

        return NULL;
    }

    // TODO: Add support for function call

    Symbol *callee = search_symbol(anz->symtbl, exp->u.name, anz->symtbl->scope);
    if (!callee || callee->group != SG_FUNC) {
        fprintf(stderr, "Error (line %d): Undeclared function '%s'\n", anz->line, exp->u.name);
        anz->err = true;
    }
    return callee ? callee : search_symbol(anz->symtbl, "int", 0);
}

/*********************************************
 * Return Statement Analysis
 *********************************************/

/**
 * @brief Analyzes a return statement.
 *
 * Ensures the return statement appears within a function and that the returned
 * expression's type matches the function's declared return type.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the return statement node.
 */
static void resolve_return(Analyzer *anz, StmtNode *stmt) {
    if (!anz->sym) {
        fprintf(stderr, "Error (line %d): return statement outside function\n", stmt->base.line);
        anz->err = true;
        return;
    }

    Symbol *rtype = anz->sym->type;
    Symbol *expr  = resolve_expression(anz, stmt->u.simple.expr);

    if (!rtype || !expr->type) {
        anz->err = true;
        return;
    }

    if (strcmp(rtype->name, "void") == 0) {
        if (expr->type) {
            fprintf(
                stderr, "Error (line %d): Void function cannot return value\n", stmt->base.line
            );
            anz->err = true;
        }
    } else if (!is_compatible(rtype, expr->type)) {
        fprintf(
            stderr, "Error (line %d): Return type mismatch (expected %s, got %s)\n",
            stmt->base.line, rtype->name, expr->type->name
        );
        anz->err = true;
    }
}

/*********************************************
 * While Loop Analysis
 *********************************************/

/**
 * @brief Analyzes a while loop.
 *
 * Verifies that the loop condition is scalar and analyzes the loop body.
 *
 * @param anz Pointer to the Analyzer.
 * @param stmt Pointer to the while loop statement node.
 */
static void resolve_while(Analyzer *anz, StmtNode *stmt) {
    Symbol *cond = resolve_expression(anz, stmt->u.while_stmt.condition);

    if (!is_scalar(cond)) {
        fprintf(stderr, "Error (line %d): While condition must be scalar type\n", stmt->base.line);
        anz->err = true;
    }
    resolve_node(anz, stmt->u.while_stmt.body);
}

/*********************************************
 * Cleanup Functions
 *********************************************/

/**
 * @brief Frees the resources associated with the analyzer.
 *
 * Purges the symbol table and frees the analyzer.
 *
 * @param anz Pointer to the Analyzer.
 */
void purge_analyzer(Analyzer *anz) {
    if (anz) {
        purge_symtbl(anz->symtbl);
        free(anz);
    }
}
