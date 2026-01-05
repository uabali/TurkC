#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void *ast_xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char *ast_strdup(const char *src) {
    if (!src) {
        return NULL;
    }
    size_t len = strlen(src);
    char *copy = (char *)ast_xmalloc(len + 1);
    memcpy(copy, src, len + 1);
    return copy;
}

ASTNode *ast_create_node(ASTNodeType type, const char *text) {
    ASTNode *node = (ASTNode *)ast_xmalloc(sizeof(ASTNode));
    node->type = type;
    node->text = ast_strdup(text);
    node->data_type = NULL;
    node->line_number = 0;
    node->first_child = NULL;
    node->next_sibling = NULL;
    return node;
}

ASTNode *ast_create_node_with_line(ASTNodeType type, const char *text, int line) {
    ASTNode *node = ast_create_node(type, text);
    node->line_number = line;
    return node;
}

void ast_set_line(ASTNode *node, int line) {
    if (node) {
        node->line_number = line;
    }
}

ASTNode *ast_create_typed_node(ASTNodeType type, const char *text, const char *data_type) {
    ASTNode *node = ast_create_node(type, text);
    ast_set_type(node, data_type);
    return node;
}

void ast_set_type(ASTNode *node, const char *data_type) {
    if (!node) {
        return;
    }
    free(node->data_type);
    node->data_type = ast_strdup(data_type);
}

ASTNode *ast_append_sibling(ASTNode *list, ASTNode *node) {
    if (!node) {
        return list;
    }
    if (!list) {
        return node;
    }
    ASTNode *tail = list;
    while (tail->next_sibling) {
        tail = tail->next_sibling;
    }
    tail->next_sibling = node;
    return list;
}

void ast_append_child(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) {
        return;
    }
    if (!parent->first_child) {
        parent->first_child = child;
        return;
    }
    ASTNode *tail = parent->first_child;
    while (tail->next_sibling) {
        tail = tail->next_sibling;
    }
    tail->next_sibling = child;
}

static void ast_print_internal(const ASTNode *node, int indent) {
    if (!node) {
        return;
    }
    for (int i = 0; i < indent; ++i) {
        printf("  ");
    }
    const char *type_name = ast_node_type_name(node->type);
    printf("%s", type_name ? type_name : "AST_NODE");
    if (node->text) {
        printf(" [%s]", node->text);
    }
    if (node->data_type) {
        printf(" :%s", node->data_type);
    }
    printf("\n");
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        ast_print_internal(child, indent + 1);
    }
}

void ast_print(const ASTNode *node, int indent) {
    ast_print_internal(node, indent);
}

void ast_free(ASTNode *node) {
    if (!node) {
        return;
    }
    ASTNode *child = node->first_child;
    while (child) {
        ASTNode *next = child->next_sibling;
        ast_free(child);
        child = next;
    }
    free(node->text);
    free(node->data_type);
    free(node);
}

const char *ast_node_type_name(ASTNodeType type) {
    static const char *names[] = {
        "PROGRAM",
        "FUNCTION",
        "PARAM_LIST",
        "PARAM",
        "BLOCK",
        "VAR_DECL",
        "IDENTIFIER",
        "NUMBER_LITERAL",
        "STRING_LITERAL",
        "ASSIGNMENT",
        "BINARY_EXPR",
        "UNARY_EXPR",
        "IF",
        "IF_ELSE",
        "WHILE",
        "FOR",
        "RETURN",
        "EXPR_STATEMENT",
        "FUNCTION_CALL",
        "ARGUMENT_LIST",
        "EMPTY"
    };
    if (type >= 0 && type < AST_NODE_TYPE_COUNT) {
        return names[type];
    }
    return "UNKNOWN";
}

