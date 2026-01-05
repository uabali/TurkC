#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"
#include "symbol.h"
#include <stdbool.h>

/* ============================================================================
 *  TurkC Semantic Analyzer
 *  Performs type checking, variable resolution, and scope analysis.
 * ============================================================================ */

/* Maximum number of errors to collect before stopping */
#define MAX_SEMANTIC_ERRORS 100

/* Semantic error entry */
typedef struct SemanticError {
    int line;
    char *message;
} SemanticError;

/* Semantic analyzer context */
typedef struct SemanticAnalyzer {
    SymbolTable *symtab;
    SemanticError errors[MAX_SEMANTIC_ERRORS];
    int error_count;
    
    /* Current context for nested analysis */
    DataType current_return_type;  /* Return type of current function */
    bool in_loop;                  /* Inside a loop? */
} SemanticAnalyzer;

/* ============================================================================
 *  Semantic Analysis API
 * ============================================================================ */

/* Create and destroy analyzer */
SemanticAnalyzer *semantic_create(void);
void semantic_destroy(SemanticAnalyzer *analyzer);

/* Main analysis entry point - returns true if no errors */
bool semantic_analyze(SemanticAnalyzer *analyzer, ASTNode *program);

/* Get analysis results */
int semantic_error_count(SemanticAnalyzer *analyzer);
void semantic_print_errors(SemanticAnalyzer *analyzer);
SymbolTable *semantic_get_symbol_table(SemanticAnalyzer *analyzer);

/* Error reporting */
void semantic_error(SemanticAnalyzer *analyzer, int line, const char *format, ...);

#endif /* SEMANTIC_H */
