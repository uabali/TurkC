#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_PARAM_LIST,
    AST_PARAM,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_IDENTIFIER,
    AST_NUMBER_LITERAL,
    AST_STRING_LITERAL,
    AST_ASSIGNMENT,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_IF,
    AST_IF_ELSE,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_EXPR_STATEMENT,
    AST_FUNCTION_CALL,
    AST_ARGUMENT_LIST,
    AST_EMPTY,
    AST_NODE_TYPE_COUNT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *text;
    char *data_type;
    struct ASTNode *first_child;
    struct ASTNode *next_sibling;
} ASTNode;

ASTNode *ast_create_node(ASTNodeType type, const char *text);
ASTNode *ast_create_typed_node(ASTNodeType type, const char *text, const char *data_type);
void ast_set_type(ASTNode *node, const char *data_type);
void ast_append_child(ASTNode *parent, ASTNode *child);
ASTNode *ast_append_sibling(ASTNode *list, ASTNode *node);
void ast_print(const ASTNode *node, int indent);
void ast_free(ASTNode *node);
char *ast_strdup(const char *src);
const char *ast_node_type_name(ASTNodeType type);

#endif /* AST_H */

