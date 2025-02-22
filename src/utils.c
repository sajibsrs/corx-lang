#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "parser.h"

/**
 * @brief Binary operator type to string.
 * @param op
 * @return
 */
const char *binop_str(BinOpr op) {
    switch (op) {
    case BIN_ADD:  return "+";
    case BIN_SUB:  return "-";
    case BIN_MUL:  return "*";
    case BIN_DIV:  return "/";
    case BIN_MOD:  return "%%";
    case BIN_AND:  return "&&";
    case BIN_OR:   return "||";
    case BIN_EQ:   return "==";
    case BIN_NEQ:  return "!=";
    case BIN_LT:   return "<";
    case BIN_LTEQ: return "<=";
    case BIN_GT:   return ">";
    case BIN_GTEQ: return ">=";
    default:       return "<unknown binop>";
    }
}

/**
 * @brief Unary operator type to string.
 * @param op
 * @return
 */
const char *unop_str(UnOpr op) {
    switch (op) {
    case UN_COMPL: return "~";
    case UN_PLUS:  return "+";
    case UN_MINUS: return "-";
    case UN_NOT:   return "!";
    default:       return "<unknown unop>";
    }
}

/**
 * @brief Print indentation.
 * @param indent Number of indentation.
 */
void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf(". ");
}

/**
 * @brief Print parameters.
 * @param node
 */
void print_param(Node *node) {
    if (!node) return;

    if (node->type == NODE_VAR_DECL) {
        VarNode *var = (VarNode *)node;
        printf("%s %s", var->dtype ? var->dtype : "<unknown>", var->name ? var->name : "<unnamed>");

        if (var->init) {
            printf(" = ");
            print_ast(var->init, 0);
        }
    } else {
        print_ast(node, 0);
    }
}

/**
 * @brief Print AST nodes.
 * @param node Node to start with.
 * @param indent Number of indentation.
 */
void print_ast(Node *node, int indent) {
    if (!node) return;

    switch (node->type) {
    case NODE_PROGRAM: {
        ProgNode *prog = (ProgNode *)node;
        print_indent(indent);
        printf("<NODE_PROGRAM>: \n");
        for (int i = 0; i < prog->icount; i++) {
            print_ast(prog->items[i], indent + 1);
        }
        break;
    }

    case NODE_VAR_DECL: {
        VarNode *var = (VarNode *)node;

        print_indent(indent);
        printf(
            "<NODE_VAR_DECL>: %s %s", var->dtype ? var->dtype : "<unknown>",
            var->name ? var->name : "<unnamed>"
        );

        if (var->init) {
            printf(" = ");
            print_ast(var->init, 0);
        }
        printf(";\n");
        break;
    }

    case NODE_FUNC_DECL: {
        FuncNode *fn = (FuncNode *)node;

        print_indent(indent);
        printf(
            "<NODE_FUNC_DECL>: %s %s(", fn->dtype ? fn->dtype : "<unknown>",
            fn->name ? fn->name : "<unnamed>"
        );

        for (int i = 0; i < fn->pcount; i++) {
            print_param(fn->params[i]);
            if (i < fn->pcount - 1) printf(", ");
        }
        printf(")");

        if (fn->body) {
            printf("\n");
            print_ast(fn->body, indent + 1);
            print_indent(indent);
            printf("\n");
        } else {
            printf(";\n");
        }
        break;
    }

    case NODE_BLOCK: {
        BlockNode *blk = (BlockNode *)node;
        print_indent(indent);
        printf("<NODE_BLOCK>: {\n");

        for (int i = 0; i < blk->icount; i++) {
            print_ast(blk->items[i], indent + 1);
        }
        print_indent(indent);
        printf("}\n");
        break;
    }

    case NODE_STATEMENT: {
        StmtNode *stmt = (StmtNode *)node;

        switch (stmt->type) {
        case STMT_RETURN:
            print_indent(indent);
            printf("<STMT_RETURN>: return ");
            print_ast(stmt->u.simple.expr, 0);
            printf(";\n");
            break;

        case STMT_EXPRESSION:
        case STMT_CALL:
            print_indent(indent);
            printf("<STMT_CALL>: ");
            print_ast(stmt->u.simple.expr, 0);
            printf(";\n");
            break;

        case STMT_ASSIGNMENT:
            print_indent(indent);
            printf(
                "<STMT_ASSIGNMENT>: %s = ",
                stmt->u.assignment.lhs ? stmt->u.assignment.lhs : "<unnamed>"
            );
            print_ast(stmt->u.assignment.rhs, 0);
            printf(";\n");
            break;

        case STMT_IF: {
            print_indent(indent);
            printf("<STMT_IF>: if (");

            Node *cond_node = stmt->u.if_stmt.condition;

            if (cond_node->type == NODE_EXPRESSION) {
                ExprNode *cond = (ExprNode *)cond_node;
                if (cond->type == EXPR_BINARY) {
                    print_ast(cond->u.binary.left, 0);
                    printf(" %s ", binop_str(cond->u.binary.op));
                    print_ast(cond->u.binary.right, 0);
                } else {
                    print_ast(cond_node, 0);
                }
            } else {
                print_ast(cond_node, 0);
            }
            printf(")\n");
            print_ast(stmt->u.if_stmt.then_stmt, indent + 1);

            if (stmt->u.if_stmt.else_stmt) {
                print_indent(indent);

                if (stmt->u.if_stmt.else_stmt->type == NODE_STATEMENT) {
                    StmtNode *else_stmt = (StmtNode *)stmt->u.if_stmt.else_stmt;

                    if (else_stmt->type == STMT_IF) {
                        printf("<STMT_IF>: else if (");

                        Node *else_cond_node = else_stmt->u.if_stmt.condition;

                        if (else_cond_node->type == NODE_EXPRESSION) {
                            ExprNode *else_cond = (ExprNode *)else_cond_node;

                            if (else_cond->type == EXPR_BINARY) {
                                print_ast(else_cond->u.binary.left, 0);
                                printf(" %s ", binop_str(else_cond->u.binary.op));
                                print_ast(else_cond->u.binary.right, 0);
                            } else {
                                print_ast(else_cond_node, 0);
                            }
                        } else {
                            print_ast(else_cond_node, 0);
                        }
                        printf(")\n");
                        print_ast(else_stmt->u.if_stmt.then_stmt, indent + 1);

                        if (else_stmt->u.if_stmt.else_stmt) {
                            print_indent(indent);
                            printf("else\n");
                            print_ast(else_stmt->u.if_stmt.else_stmt, indent + 1);
                        }
                    } else {
                        printf("else\n");
                        print_ast(stmt->u.if_stmt.else_stmt, indent + 1);
                    }
                } else {
                    printf("else\n");
                    print_ast(stmt->u.if_stmt.else_stmt, indent + 1);
                }
            }
            break;
        }

        case STMT_FOR: {
            print_indent(indent);
            printf("<STMT_FOR>: for (");
            print_param(stmt->u.for_stmt.init);
            printf("; ");

            Node *cond_node = stmt->u.for_stmt.condition;
            if (cond_node) {

                if (cond_node->type == NODE_EXPRESSION) {
                    ExprNode *cond = (ExprNode *)cond_node;

                    if (cond->type == EXPR_BINARY) {
                        print_ast(cond->u.binary.left, 0);
                        printf(" %s ", binop_str(cond->u.binary.op));
                        print_ast(cond->u.binary.right, 0);
                    } else {
                        print_ast(cond_node, 0);
                    }
                } else {
                    print_ast(cond_node, 0);
                }
            }
            printf("; ");

            Node *post_node = stmt->u.for_stmt.post;
            if (post_node) {
                print_ast(post_node, 0);
            }
            printf(")\n");
            print_ast(stmt->u.for_stmt.body, indent + 1);
            break;
        }

        case STMT_WHILE: {
            print_indent(indent);
            printf("<STMT_WHILE>: while (");
            print_ast(stmt->u.while_stmt.condition, 0);
            printf(")\n");
            print_ast(stmt->u.while_stmt.body, indent + 1);
            break;
        }

        case STMT_DO_WHILE: {
            print_indent(indent);
            printf("<STMT_DO_WHILE>: do\n");
            print_ast(stmt->u.while_stmt.body, indent + 1);
            print_indent(indent);
            printf("while (");
            print_ast(stmt->u.while_stmt.condition, 0);
            printf(");\n");
            break;
        }

        case STMT_COMPOUND: print_ast(stmt->u.simple.expr, indent); break;

        case STMT_BREAK:
            print_indent(indent);
            printf("<STMT_BREAK>: break;\n");
            break;

        case STMT_CONTINUE:
            print_indent(indent);
            printf("<STMT_CONTINUE>: continue;\n");
            break;

        case STMT_NULL:
            print_indent(indent);
            printf("<STMT_NULL>: ;\n");
            break;

        default: print_indent(indent); printf("<unknown stmt>;\n");
        }
        break;
    }

    case NODE_EXPRESSION: {
        ExprNode *expr = (ExprNode *)node;

        switch (expr->type) {
        case EXPR_CONSTANT: {
            printf("%d", expr->u.value);
            break;
        }

        case EXPR_VAR: {
            printf("%s", expr->u.name ? expr->u.name : "<unnamed>");
            break;
        }

        case EXPR_UNARY: {
            printf("(");
            printf("%s", unop_str(expr->u.unary.op));
            print_ast(expr->u.unary.operand, 0);
            printf(")");
            break;
        }

        case EXPR_BINARY: {
            printf("(");
            print_ast(expr->u.binary.left, 0);
            printf(" %s ", binop_str(expr->u.binary.op));
            print_ast(expr->u.binary.right, 0);
            printf(")");
            break;
        }

        case EXPR_CONDITIONAL: {
            printf("(");
            print_ast(expr->u.conditional.condition, 0);
            printf(" ? ");
            print_ast(expr->u.conditional.true_expr, 0);
            printf(" : ");
            print_ast(expr->u.conditional.false_expr, 0);
            printf(")");
            break;
        }

        case EXPR_ASSIGNMENT: {
            print_ast(expr->u.assignment.lhs, 0);
            printf(" = ");
            print_ast(expr->u.assignment.rhs, 0);
            break;
        }

        case EXPR_CALL: {
            print_ast(expr->u.call.callee, 0);
            printf("(");
            for (int i = 0; i < expr->u.call.acount; i++) {
                print_ast(expr->u.call.args[i], 0);
                if (i < expr->u.call.acount - 1) printf(", ");
            }
            printf(")");
            break;
        }

        default: printf("<unknown expr>");
        }
        break;
    }

    default:
        print_indent(indent);
        printf("<unknown node>\n");
        break;
    }
}

/**
 * @brief Prints error message and exits.
 * @param msg
 */
void errexit(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
    exit(1);
}

/**
 * @brief Prints error message and continues program.
 * @param msg
 */
void errwarn(const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

/**
 * @brief Returns hash for string (Fowler–Noll–Vo hash).
 * @param str String input.
 * @param size Hash map size.
 * @return
 */
unsigned hashfnv(const char *str, const int size) {
    unsigned hash = 2166136261u; // FNV offset basis
    while (*str) {
        hash ^= (unsigned char)(*str); // XOR with byte
        hash *= 16777619u;             // FNV prime
        str++;
    }

    return hash % size;
}

/**
 * @brief Print formatted token to the terminal.
 * @param list
 */
void print_tlist(const TokList *list) {
    printf("Scanned %d tokens:\n\n", list->count);

    Token token;

    for (int i = 0; i < list->count; i++) {
        token = list->tokens[i];
        printf(
            "%-16s %-10s typ:%-4d lin:%-4d col:%d\n", //
            ttypestr[token.type], token.str, token.type, token.line, token.col
        );
    }
}
