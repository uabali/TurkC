#include <stdio.h>
#include <stdlib.h>

#include "parser.tab.h"
#include "ast.h"

extern FILE *yyin;
extern int yyparse(void);
extern ASTNode *ast_root;

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            perror(argv[1]);
            return EXIT_FAILURE;
        }
    }

    int parse_result = yyparse();

    if (yyin && yyin != stdin) {
        fclose(yyin);
    }

    if (parse_result == 0) {
        if (ast_root) {
            ast_print(ast_root, 0);
            ast_free(ast_root);
        }
        return EXIT_SUCCESS;
    }

    if (ast_root) {
        ast_free(ast_root);
    }
    return EXIT_FAILURE;
}

