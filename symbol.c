#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 *  Internal Helper Functions
 * ============================================================================ */

static void *sym_xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memset(ptr, 0, size);
    return ptr;
}

static char *sym_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *copy = (char *)sym_xmalloc(len + 1);
    memcpy(copy, src, len + 1);
    return copy;
}

/* DJB2 hash function */
static unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % SYMBOL_TABLE_SIZE;
}

/* ============================================================================
 *  Symbol Table Creation and Destruction
 * ============================================================================ */

SymbolTable *symbol_table_create(void) {
    SymbolTable *table = (SymbolTable *)sym_xmalloc(sizeof(SymbolTable));
    
    /* Initialize buckets to NULL */
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    
    /* Create global scope (level 0) */
    table->current_scope = (Scope *)sym_xmalloc(sizeof(Scope));
    table->current_scope->level = 0;
    table->current_scope->local_offset = 0;
    table->current_scope->parent = NULL;
    table->scope_level = 0;
    table->current_function = NULL;
    table->total_locals = 0;
    
    return table;
}

static void free_symbol(Symbol *sym) {
    if (!sym) return;
    free(sym->name);
    if (sym->func_info) {
        for (int i = 0; i < sym->func_info->param_count; i++) {
            free(sym->func_info->param_names[i]);
        }
        free(sym->func_info);
    }
    free(sym);
}

void symbol_table_destroy(SymbolTable *table) {
    if (!table) return;
    
    /* Free all symbols */
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol *sym = table->buckets[i];
        while (sym) {
            Symbol *next = sym->next;
            free_symbol(sym);
            sym = next;
        }
    }
    
    /* Free all scopes */
    Scope *scope = table->current_scope;
    while (scope) {
        Scope *parent = scope->parent;
        free(scope);
        scope = parent;
    }
    
    free(table);
}

/* ============================================================================
 *  Scope Management
 * ============================================================================ */

void symbol_enter_scope(SymbolTable *table) {
    Scope *new_scope = (Scope *)sym_xmalloc(sizeof(Scope));
    new_scope->level = table->scope_level + 1;
    new_scope->local_offset = table->current_scope->local_offset;
    new_scope->parent = table->current_scope;
    
    table->current_scope = new_scope;
    table->scope_level++;
}

void symbol_exit_scope(SymbolTable *table) {
    if (table->scope_level == 0) {
        fprintf(stderr, "Uyari: Global kapsamdan cikilamaz\n");
        return;
    }
    
    /* Remove all symbols from the current scope */
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        Symbol **ptr = &table->buckets[i];
        while (*ptr) {
            if ((*ptr)->scope_level == table->scope_level) {
                Symbol *to_remove = *ptr;
                *ptr = to_remove->next;
                free_symbol(to_remove);
            } else {
                ptr = &(*ptr)->next;
            }
        }
    }
    
    /* Pop scope */
    Scope *old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    table->scope_level--;
    free(old_scope);
}

int symbol_current_scope_level(SymbolTable *table) {
    return table->scope_level;
}

/* ============================================================================
 *  Symbol Operations
 * ============================================================================ */

Symbol *symbol_declare(SymbolTable *table, const char *name, SymbolKind kind,
                       DataType type, int line) {
    unsigned int idx = hash_string(name);
    
    /* Check for redeclaration in current scope */
    Symbol *existing = symbol_lookup_current_scope(table, name);
    if (existing) {
        return NULL;  /* Redeclaration error - caller should handle */
    }
    
    /* Create new symbol */
    Symbol *sym = (Symbol *)sym_xmalloc(sizeof(Symbol));
    sym->name = sym_strdup(name);
    sym->kind = kind;
    sym->type = type;
    sym->scope_level = table->scope_level;
    sym->line_declared = line;
    sym->func_info = NULL;
    
    /* Assign stack offset for variables and parameters */
    if (kind == SYMBOL_VARIABLE || kind == SYMBOL_PARAMETER) {
        sym->offset = table->current_scope->local_offset++;
        table->total_locals++;
    }
    
    /* Insert at head of bucket chain */
    sym->next = table->buckets[idx];
    table->buckets[idx] = sym;
    
    return sym;
}

Symbol *symbol_lookup(SymbolTable *table, const char *name) {
    unsigned int idx = hash_string(name);
    
    /* Search through hash chain, find highest scope level match */
    Symbol *found = NULL;
    for (Symbol *sym = table->buckets[idx]; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0) {
            if (!found || sym->scope_level > found->scope_level) {
                found = sym;
            }
        }
    }
    
    return found;
}

Symbol *symbol_lookup_current_scope(SymbolTable *table, const char *name) {
    unsigned int idx = hash_string(name);
    
    for (Symbol *sym = table->buckets[idx]; sym; sym = sym->next) {
        if (strcmp(sym->name, name) == 0 && 
            sym->scope_level == table->scope_level) {
            return sym;
        }
    }
    
    return NULL;
}

/* ============================================================================
 *  Function-Specific Operations
 * ============================================================================ */

Symbol *symbol_declare_function(SymbolTable *table, const char *name,
                                 DataType return_type, int line) {
    Symbol *func = symbol_declare(table, name, SYMBOL_FUNCTION, return_type, line);
    if (!func) return NULL;
    
    /* Initialize function info */
    func->func_info = (FunctionInfo *)sym_xmalloc(sizeof(FunctionInfo));
    func->func_info->return_type = return_type;
    func->func_info->param_count = 0;
    
    return func;
}

void symbol_add_parameter(Symbol *func, const char *name, DataType type) {
    if (!func || !func->func_info) return;
    
    int idx = func->func_info->param_count;
    if (idx >= MAX_PARAMS) {
        fprintf(stderr, "Uyari: Maksimum parametre sayisi asildi\n");
        return;
    }
    
    func->func_info->param_names[idx] = sym_strdup(name);
    func->func_info->param_types[idx] = type;
    func->func_info->param_count++;
}

void symbol_set_current_function(SymbolTable *table, Symbol *func) {
    table->current_function = func;
    table->total_locals = 0;
}

Symbol *symbol_get_current_function(SymbolTable *table) {
    return table->current_function;
}

/* ============================================================================
 *  Type Utilities
 * ============================================================================ */

DataType datatype_from_string(const char *type_str) {
    if (!type_str) return TYPE_UNKNOWN;
    if (strcmp(type_str, "int") == 0) return TYPE_INT;
    if (strcmp(type_str, "void") == 0) return TYPE_VOID;
    return TYPE_UNKNOWN;
}

const char *datatype_to_string(DataType type) {
    switch (type) {
        case TYPE_INT:   return "int";
        case TYPE_VOID:  return "void";
        case TYPE_ERROR: return "<hata>";
        default:         return "<bilinmeyen>";
    }
}

bool types_compatible(DataType left, DataType right) {
    /* Void is not compatible with anything in expressions */
    if (left == TYPE_VOID || right == TYPE_VOID) return false;
    if (left == TYPE_ERROR || right == TYPE_ERROR) return false;
    
    /* For now, int == int */
    return left == right;
}

/* ============================================================================
 *  Debug Output
 * ============================================================================ */

void symbol_table_print(SymbolTable *table) {
    printf("\n=== Sembol Tablosu ===\n");
    printf("%-20s %-12s %-10s %-8s %-8s\n", 
           "Isim", "Tur", "Veri Tipi", "Kapsam", "Offset");
    printf("--------------------------------------------------------\n");
    
    for (int i = 0; i < SYMBOL_TABLE_SIZE; i++) {
        for (Symbol *sym = table->buckets[i]; sym; sym = sym->next) {
            const char *kind_str = 
                sym->kind == SYMBOL_FUNCTION ? "fonksiyon" :
                sym->kind == SYMBOL_PARAMETER ? "parametre" : "degisken";
            
            printf("%-20s %-12s %-10s %-8d %-8d\n",
                   sym->name,
                   kind_str,
                   datatype_to_string(sym->type),
                   sym->scope_level,
                   sym->offset);
            
            /* Print function parameters */
            if (sym->func_info && sym->func_info->param_count > 0) {
                printf("  Parametreler: ");
                for (int j = 0; j < sym->func_info->param_count; j++) {
                    printf("%s %s", 
                           datatype_to_string(sym->func_info->param_types[j]),
                           sym->func_info->param_names[j]);
                    if (j < sym->func_info->param_count - 1) printf(", ");
                }
                printf("\n");
            }
        }
    }
    printf("======================\n\n");
}
