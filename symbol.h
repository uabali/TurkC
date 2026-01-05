#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdbool.h>

/* ============================================================================
 *  TurkC Symbol Table - Semantic Analysis Phase
 *  Manages variables, functions, and scopes for the TurkC compiler.
 * ============================================================================ */

/* Maximum sizes for symbol table */
#define SYMBOL_TABLE_SIZE 256
#define MAX_SCOPE_DEPTH 32
#define MAX_PARAMS 16

/* Type enumeration for TurkC */
typedef enum {
    TYPE_UNKNOWN,
    TYPE_VOID,
    TYPE_INT,
    TYPE_ERROR   /* Used for type checking errors */
} DataType;

/* Symbol kinds */
typedef enum {
    SYMBOL_VARIABLE,
    SYMBOL_FUNCTION,
    SYMBOL_PARAMETER
} SymbolKind;

/* Function signature information */
typedef struct {
    DataType return_type;
    int param_count;
    DataType param_types[MAX_PARAMS];
    char *param_names[MAX_PARAMS];
} FunctionInfo;

/* Symbol entry */
typedef struct Symbol {
    char *name;              /* Identifier name */
    SymbolKind kind;         /* Variable, function, or parameter */
    DataType type;           /* Data type */
    int scope_level;         /* Nesting level (0 = global) */
    int offset;              /* Stack offset for variables */
    int line_declared;       /* Line number where declared */
    FunctionInfo *func_info; /* Function-specific info (NULL for variables) */
    struct Symbol *next;     /* Hash chain */
} Symbol;

/* Scope representation */
typedef struct Scope {
    int level;               /* Scope nesting level */
    int local_offset;        /* Next available stack offset */
    struct Scope *parent;    /* Enclosing scope */
} Scope;

/* Symbol table structure */
typedef struct SymbolTable {
    Symbol *buckets[SYMBOL_TABLE_SIZE];  /* Hash buckets */
    Scope *current_scope;                 /* Current scope */
    int scope_level;                      /* Current nesting level */
    
    /* For code generation - track current function context */
    Symbol *current_function;
    int total_locals;                     /* Count of local variables */
} SymbolTable;

/* ============================================================================
 *  Symbol Table API
 * ============================================================================ */

/* Create and destroy symbol table */
SymbolTable *symbol_table_create(void);
void symbol_table_destroy(SymbolTable *table);

/* Scope management */
void symbol_enter_scope(SymbolTable *table);
void symbol_exit_scope(SymbolTable *table);
int symbol_current_scope_level(SymbolTable *table);

/* Symbol operations */
Symbol *symbol_declare(SymbolTable *table, const char *name, SymbolKind kind, 
                       DataType type, int line);
Symbol *symbol_lookup(SymbolTable *table, const char *name);
Symbol *symbol_lookup_current_scope(SymbolTable *table, const char *name);

/* Function-specific operations */
Symbol *symbol_declare_function(SymbolTable *table, const char *name,
                                 DataType return_type, int line);
void symbol_add_parameter(Symbol *func, const char *name, DataType type);
void symbol_set_current_function(SymbolTable *table, Symbol *func);
Symbol *symbol_get_current_function(SymbolTable *table);

/* Type utilities */
DataType datatype_from_string(const char *type_str);
const char *datatype_to_string(DataType type);
bool types_compatible(DataType left, DataType right);

/* Debug */
void symbol_table_print(SymbolTable *table);

#endif /* SYMBOL_H */
