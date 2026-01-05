#include "semantic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ============================================================================
 *  Forward Declarations - AST Traversal Functions
 * ============================================================================ */

static void analyze_node(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_function(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_block(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_statement(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_var_decl(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_if(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_while(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_for(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_return(SemanticAnalyzer *analyzer, ASTNode *node);
static DataType analyze_expression(SemanticAnalyzer *analyzer, ASTNode *node);

/* ============================================================================
 *  Helper Functions
 * ============================================================================ */

static void *sem_xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memset(ptr, 0, size);
    return ptr;
}

static char *sem_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *copy = (char *)sem_xmalloc(len + 1);
    memcpy(copy, src, len + 1);
    return copy;
}

/* Get line number from AST node (using line_number field if available) */
static int get_line(ASTNode *node) {
    if (!node) return 0;
    return node->line_number;
}

/* Count children of a node */
static int count_children(ASTNode *node) {
    int count = 0;
    for (ASTNode *child = node->first_child; child; child = child->next_sibling) {
        count++;
    }
    return count;
}

/* Get nth child (0-indexed) */
static ASTNode *get_child(ASTNode *node, int n) {
    ASTNode *child = node->first_child;
    for (int i = 0; i < n && child; i++) {
        child = child->next_sibling;
    }
    return child;
}

/* ============================================================================
 *  Semantic Analyzer Creation/Destruction
 * ============================================================================ */

SemanticAnalyzer *semantic_create(void) {
    SemanticAnalyzer *analyzer = (SemanticAnalyzer *)sem_xmalloc(sizeof(SemanticAnalyzer));
    analyzer->symtab = symbol_table_create();
    analyzer->error_count = 0;
    analyzer->current_return_type = TYPE_VOID;
    analyzer->in_loop = false;
    return analyzer;
}

void semantic_destroy(SemanticAnalyzer *analyzer) {
    if (!analyzer) return;
    
    /* Free error messages */
    for (int i = 0; i < analyzer->error_count; i++) {
        free(analyzer->errors[i].message);
    }
    
    /* Note: symbol table is kept for code generation, freed separately */
    free(analyzer);
}

/* ============================================================================
 *  Error Reporting
 * ============================================================================ */

void semantic_error(SemanticAnalyzer *analyzer, int line, const char *format, ...) {
    if (analyzer->error_count >= MAX_SEMANTIC_ERRORS) {
        return;
    }
    
    /* Format error message */
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* Store error */
    analyzer->errors[analyzer->error_count].line = line;
    analyzer->errors[analyzer->error_count].message = sem_strdup(buffer);
    analyzer->error_count++;
}

int semantic_error_count(SemanticAnalyzer *analyzer) {
    return analyzer->error_count;
}

void semantic_print_errors(SemanticAnalyzer *analyzer) {
    for (int i = 0; i < analyzer->error_count; i++) {
        fprintf(stderr, "Semantik hata (satir %d): %s\n",
                analyzer->errors[i].line,
                analyzer->errors[i].message);
    }
    
    if (analyzer->error_count > 0) {
        fprintf(stderr, "\nDerleme %d hata ile basarisiz oldu.\n", 
                analyzer->error_count);
    }
}

SymbolTable *semantic_get_symbol_table(SemanticAnalyzer *analyzer) {
    return analyzer->symtab;
}

/* ============================================================================
 *  Pass 1: Collect all function declarations
 * ============================================================================ */

static void collect_functions(SemanticAnalyzer *analyzer, ASTNode *program) {
    for (ASTNode *child = program->first_child; child; child = child->next_sibling) {
        if (child->type == AST_FUNCTION) {
            const char *func_name = child->text;
            DataType return_type = datatype_from_string(child->data_type);
            int line = get_line(child);
            
            /* Check for redeclaration */
            Symbol *existing = symbol_lookup(analyzer->symtab, func_name);
            if (existing && existing->kind == SYMBOL_FUNCTION) {
                semantic_error(analyzer, line,
                    "'%s' fonksiyonu zaten tanimlanmis (satir %d)",
                    func_name, existing->line_declared);
                continue;
            }
            
            /* Declare function */
            Symbol *func = symbol_declare_function(analyzer->symtab, func_name, 
                                                    return_type, line);
            if (!func) {
                semantic_error(analyzer, line,
                    "'%s' fonksiyonu tanimlanamadi", func_name);
                continue;
            }
            
            /* Collect parameter information */
            ASTNode *param_list = get_child(child, 0);
            if (param_list && param_list->type == AST_PARAM_LIST) {
                for (ASTNode *param = param_list->first_child; param; 
                     param = param->next_sibling) {
                    if (param->type == AST_PARAM) {
                        DataType ptype = datatype_from_string(param->data_type);
                        symbol_add_parameter(func, param->text, ptype);
                    }
                }
            }
        }
    }
}

/* ============================================================================
 *  Pass 2: Full semantic analysis
 * ============================================================================ */

static void analyze_function(SemanticAnalyzer *analyzer, ASTNode *node) {
    const char *func_name = node->text;
    Symbol *func = symbol_lookup(analyzer->symtab, func_name);
    
    if (!func) {
        /* Already reported in Pass 1 */
        return;
    }
    
    /* Set current function context */
    symbol_set_current_function(analyzer->symtab, func);
    analyzer->current_return_type = func->func_info->return_type;
    
    /* Enter function scope */
    symbol_enter_scope(analyzer->symtab);
    
    /* Declare parameters as local variables */
    if (func->func_info) {
        for (int i = 0; i < func->func_info->param_count; i++) {
            const char *pname = func->func_info->param_names[i];
            DataType ptype = func->func_info->param_types[i];
            Symbol *param = symbol_declare(analyzer->symtab, pname, 
                                           SYMBOL_PARAMETER, ptype,
                                           get_line(node));
            if (!param) {
                semantic_error(analyzer, get_line(node),
                    "Parametre '%s' birden fazla kez tanimlanmis", pname);
            }
        }
    }
    
    /* Analyze function body (block) */
    ASTNode *body = get_child(node, 1);  /* Second child is the block */
    if (body && body->type == AST_BLOCK) {
        /* Don't enter new scope - we already did for function */
        for (ASTNode *stmt = body->first_child; stmt; stmt = stmt->next_sibling) {
            analyze_statement(analyzer, stmt);
        }
    }
    
    /* Exit function scope */
    symbol_exit_scope(analyzer->symtab);
    symbol_set_current_function(analyzer->symtab, NULL);
}

static void analyze_block(SemanticAnalyzer *analyzer, ASTNode *node) {
    symbol_enter_scope(analyzer->symtab);
    
    for (ASTNode *stmt = node->first_child; stmt; stmt = stmt->next_sibling) {
        analyze_statement(analyzer, stmt);
    }
    
    symbol_exit_scope(analyzer->symtab);
}

static void analyze_statement(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL:
            analyze_var_decl(analyzer, node);
            break;
            
        case AST_BLOCK:
            analyze_block(analyzer, node);
            break;
            
        case AST_IF:
        case AST_IF_ELSE:
            analyze_if(analyzer, node);
            break;
            
        case AST_WHILE:
            analyze_while(analyzer, node);
            break;
            
        case AST_FOR:
            analyze_for(analyzer, node);
            break;
            
        case AST_RETURN:
            analyze_return(analyzer, node);
            break;
            
        case AST_EXPR_STATEMENT:
            if (node->first_child) {
                analyze_expression(analyzer, node->first_child);
            }
            break;
            
        case AST_ASSIGNMENT:
            analyze_expression(analyzer, node);
            break;
            
        default:
            /* Other node types - traverse children */
            for (ASTNode *child = node->first_child; child; 
                 child = child->next_sibling) {
                analyze_statement(analyzer, child);
            }
            break;
    }
}

static void analyze_var_decl(SemanticAnalyzer *analyzer, ASTNode *node) {
    const char *var_name = node->text;
    DataType var_type = datatype_from_string(node->data_type);
    int line = get_line(node);
    
    /* Check for void variable */
    if (var_type == TYPE_VOID) {
        semantic_error(analyzer, line,
            "'%s' degiskeni void tipinde olamaz", var_name);
        return;
    }
    
    /* Check for redeclaration in current scope */
    Symbol *existing = symbol_lookup_current_scope(analyzer->symtab, var_name);
    if (existing) {
        semantic_error(analyzer, line,
            "'%s' degiskeni ayni kapsamda zaten tanimlanmis (satir %d)",
            var_name, existing->line_declared);
        return;
    }
    
    /* Declare variable */
    Symbol *var = symbol_declare(analyzer->symtab, var_name, 
                                  SYMBOL_VARIABLE, var_type, line);
    if (!var) {
        semantic_error(analyzer, line,
            "'%s' degiskeni tanimlanamadi", var_name);
        return;
    }
    
    /* Check initializer if present */
    if (node->first_child) {
        DataType init_type = analyze_expression(analyzer, node->first_child);
        if (init_type != TYPE_ERROR && !types_compatible(var_type, init_type)) {
            semantic_error(analyzer, line,
                "Tip uyumsuzlugu: '%s' tipi %s, ancak %s atanmaya calisiliyor",
                var_name, datatype_to_string(var_type), 
                datatype_to_string(init_type));
        }
    }
}

static void analyze_if(SemanticAnalyzer *analyzer, ASTNode *node) {
    /* First child is condition */
    ASTNode *condition = get_child(node, 0);
    if (condition) {
        DataType cond_type = analyze_expression(analyzer, condition);
        /* In C-like languages, any non-zero is true, so int is OK */
        if (cond_type == TYPE_VOID) {
            semantic_error(analyzer, get_line(node),
                "Kosul ifadesi void olamaz");
        }
    }
    
    /* Second child is then-block */
    ASTNode *then_block = get_child(node, 1);
    if (then_block) {
        analyze_statement(analyzer, then_block);
    }
    
    /* Third child is else-block (for IF_ELSE) */
    if (node->type == AST_IF_ELSE) {
        ASTNode *else_block = get_child(node, 2);
        if (else_block) {
            analyze_statement(analyzer, else_block);
        }
    }
}

static void analyze_while(SemanticAnalyzer *analyzer, ASTNode *node) {
    /* First child is condition */
    ASTNode *condition = get_child(node, 0);
    if (condition) {
        DataType cond_type = analyze_expression(analyzer, condition);
        if (cond_type == TYPE_VOID) {
            semantic_error(analyzer, get_line(node),
                "Dongu kosulu void olamaz");
        }
    }
    
    /* Second child is body */
    bool was_in_loop = analyzer->in_loop;
    analyzer->in_loop = true;
    
    ASTNode *body = get_child(node, 1);
    if (body) {
        analyze_statement(analyzer, body);
    }
    
    analyzer->in_loop = was_in_loop;
}

static void analyze_for(SemanticAnalyzer *analyzer, ASTNode *node) {
    /* For loop has: init, condition, update, body */
    /* Enter a scope for the for-loop (for init declarations) */
    symbol_enter_scope(analyzer->symtab);
    
    /* Init (child 0) */
    ASTNode *init = get_child(node, 0);
    if (init && init->type != AST_EMPTY) {
        analyze_statement(analyzer, init);
    }
    
    /* Condition (child 1) */
    ASTNode *condition = get_child(node, 1);
    if (condition && condition->type != AST_EMPTY) {
        DataType cond_type = analyze_expression(analyzer, condition);
        if (cond_type == TYPE_VOID) {
            semantic_error(analyzer, get_line(node),
                "For dongusu kosulu void olamaz");
        }
    }
    
    /* Update (child 2) */
    ASTNode *update = get_child(node, 2);
    if (update && update->type != AST_EMPTY) {
        analyze_expression(analyzer, update);
    }
    
    /* Body (child 3) */
    bool was_in_loop = analyzer->in_loop;
    analyzer->in_loop = true;
    
    ASTNode *body = get_child(node, 3);
    if (body) {
        analyze_statement(analyzer, body);
    }
    
    analyzer->in_loop = was_in_loop;
    symbol_exit_scope(analyzer->symtab);
}

static void analyze_return(SemanticAnalyzer *analyzer, ASTNode *node) {
    DataType expected = analyzer->current_return_type;
    DataType actual = TYPE_VOID;
    
    if (node->first_child) {
        actual = analyze_expression(analyzer, node->first_child);
    }
    
    /* Check return type */
    if (expected == TYPE_VOID && actual != TYPE_VOID) {
        semantic_error(analyzer, get_line(node),
            "void fonksiyondan deger dondurulemez");
    } else if (expected != TYPE_VOID && actual == TYPE_VOID) {
        semantic_error(analyzer, get_line(node),
            "Fonksiyon %s dondermeli", datatype_to_string(expected));
    } else if (expected != TYPE_VOID && actual != TYPE_VOID && 
               !types_compatible(expected, actual)) {
        semantic_error(analyzer, get_line(node),
            "Donus tipi uyumsuz: beklenen %s, gelen %s",
            datatype_to_string(expected), datatype_to_string(actual));
    }
}

/* ============================================================================
 *  Expression Analysis - Returns the type of the expression
 * ============================================================================ */

static DataType analyze_expression(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return TYPE_ERROR;
    
    switch (node->type) {
        case AST_NUMBER_LITERAL:
            return TYPE_INT;
            
        case AST_STRING_LITERAL:
            /* Strings are not really supported, treat as error for now */
            return TYPE_INT;  /* Could add TYPE_STRING later */
            
        case AST_IDENTIFIER: {
            Symbol *sym = symbol_lookup(analyzer->symtab, node->text);
            if (!sym) {
                semantic_error(analyzer, get_line(node),
                    "'%s' degiskeni tanimlanmamis", node->text);
                return TYPE_ERROR;
            }
            if (sym->kind == SYMBOL_FUNCTION) {
                semantic_error(analyzer, get_line(node),
                    "'%s' bir fonksiyondur, degisken olarak kullanilamaz",
                    node->text);
                return TYPE_ERROR;
            }
            return sym->type;
        }
        
        case AST_ASSIGNMENT: {
            /* Left side must be an lvalue (identifier) */
            ASTNode *left = get_child(node, 0);
            ASTNode *right = get_child(node, 1);
            
            if (!left || left->type != AST_IDENTIFIER) {
                semantic_error(analyzer, get_line(node),
                    "Atama hedefi bir degisken olmali");
                return TYPE_ERROR;
            }
            
            Symbol *sym = symbol_lookup(analyzer->symtab, left->text);
            if (!sym) {
                semantic_error(analyzer, get_line(node),
                    "'%s' degiskeni tanimlanmamis", left->text);
                return TYPE_ERROR;
            }
            
            DataType right_type = analyze_expression(analyzer, right);
            if (!types_compatible(sym->type, right_type)) {
                semantic_error(analyzer, get_line(node),
                    "Tip uyumsuzlugu: '%s' tipi %s, ancak %s atanmaya calisiliyor",
                    left->text, datatype_to_string(sym->type),
                    datatype_to_string(right_type));
            }
            
            return sym->type;
        }
        
        case AST_BINARY_EXPR: {
            ASTNode *left = get_child(node, 0);
            ASTNode *right = get_child(node, 1);
            
            DataType left_type = analyze_expression(analyzer, left);
            DataType right_type = analyze_expression(analyzer, right);
            
            if (left_type == TYPE_VOID || right_type == TYPE_VOID) {
                semantic_error(analyzer, get_line(node),
                    "Void ifadeler aritmetik islemlerde kullanilamaz");
                return TYPE_ERROR;
            }
            
            if (!types_compatible(left_type, right_type)) {
                semantic_error(analyzer, get_line(node),
                    "Tip uyumsuzlugu: %s ve %s arasinda islem yapilamaz",
                    datatype_to_string(left_type),
                    datatype_to_string(right_type));
                return TYPE_ERROR;
            }
            
            /* Comparison operators return int (0 or 1) */
            const char *op = node->text;
            if (op && (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
                       strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
                       strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0)) {
                return TYPE_INT;
            }
            
            return left_type;
        }
        
        case AST_UNARY_EXPR: {
            ASTNode *operand = get_child(node, 0);
            DataType op_type = analyze_expression(analyzer, operand);
            
            if (op_type == TYPE_VOID) {
                semantic_error(analyzer, get_line(node),
                    "Void ifadeler uzerinde unary islem yapilamaz");
                return TYPE_ERROR;
            }
            
            return op_type;
        }
        
        case AST_FUNCTION_CALL: {
            const char *func_name = node->text;
            Symbol *func = symbol_lookup(analyzer->symtab, func_name);
            
            if (!func) {
                semantic_error(analyzer, get_line(node),
                    "'%s' fonksiyonu tanimlanmamis", func_name);
                return TYPE_ERROR;
            }
            
            if (func->kind != SYMBOL_FUNCTION) {
                semantic_error(analyzer, get_line(node),
                    "'%s' bir fonksiyon degil", func_name);
                return TYPE_ERROR;
            }
            
            /* Check argument count and types */
            ASTNode *arg_list = get_child(node, 0);
            int arg_count = 0;
            
            if (arg_list && arg_list->type == AST_ARGUMENT_LIST) {
                for (ASTNode *arg = arg_list->first_child; arg; 
                     arg = arg->next_sibling) {
                    
                    DataType arg_type = analyze_expression(analyzer, arg);
                    
                    if (arg_count < func->func_info->param_count) {
                        DataType expected = func->func_info->param_types[arg_count];
                        if (!types_compatible(expected, arg_type)) {
                            semantic_error(analyzer, get_line(node),
                                "'%s' fonksiyonu %d. parametre: beklenen %s, gelen %s",
                                func_name, arg_count + 1,
                                datatype_to_string(expected),
                                datatype_to_string(arg_type));
                        }
                    }
                    arg_count++;
                }
            }
            
            if (arg_count != func->func_info->param_count) {
                semantic_error(analyzer, get_line(node),
                    "'%s' fonksiyonu %d parametre bekliyor, %d verildi",
                    func_name, func->func_info->param_count, arg_count);
            }
            
            return func->func_info->return_type;
        }
        
        case AST_EXPR_STATEMENT:
            if (node->first_child) {
                return analyze_expression(analyzer, node->first_child);
            }
            return TYPE_VOID;
        
        default:
            /* Unknown expression type */
            return TYPE_ERROR;
    }
}

/* ============================================================================
 *  Main Entry Point
 * ============================================================================ */

static void analyze_node(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            /* Analyze all children (functions and global declarations) */
            for (ASTNode *child = node->first_child; child; 
                 child = child->next_sibling) {
                analyze_node(analyzer, child);
            }
            break;
            
        case AST_FUNCTION:
            analyze_function(analyzer, node);
            break;
            
        case AST_VAR_DECL:
            analyze_var_decl(analyzer, node);
            break;
            
        default:
            /* Traverse children */
            for (ASTNode *child = node->first_child; child; 
                 child = child->next_sibling) {
                analyze_node(analyzer, child);
            }
            break;
    }
}

bool semantic_analyze(SemanticAnalyzer *analyzer, ASTNode *program) {
    if (!analyzer || !program) return false;
    
    /* Pass 1: Collect all function declarations */
    collect_functions(analyzer, program);
    
    /* Pass 2: Full semantic analysis */
    analyze_node(analyzer, program);
    
    return analyzer->error_count == 0;
}
