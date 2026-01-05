#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "symbol.h"
#include <stdbool.h>

/* ============================================================================
 *  TurkC Bytecode Definitions
 *  Stack-based virtual machine instruction set.
 * ============================================================================ */

/* Maximum bytecode size */
#define MAX_CODE_SIZE 4096
#define MAX_FUNCTIONS 64
#define MAX_LABELS 256

/* Bytecode opcodes */
typedef enum {
    /* Stack operations */
    OP_NOP,         /* No operation */
    OP_PUSH,        /* Push immediate value onto stack */
    OP_POP,         /* Pop top of stack */
    OP_DUP,         /* Duplicate top of stack */
    
    /* Variable access */
    OP_LOAD,        /* Load local variable (offset in operand) */
    OP_STORE,       /* Store to local variable (offset in operand) */
    OP_LOAD_GLOBAL, /* Load global variable */
    OP_STORE_GLOBAL,/* Store to global variable */
    
    /* Arithmetic operations (pop 2, push 1) */
    OP_ADD,         /* Addition */
    OP_SUB,         /* Subtraction */
    OP_MUL,         /* Multiplication */
    OP_DIV,         /* Division */
    OP_MOD,         /* Modulo */
    OP_NEG,         /* Negate (unary minus) */
    
    /* Comparison operations (pop 2, push 0 or 1) */
    OP_EQ,          /* Equal */
    OP_NEQ,         /* Not equal */
    OP_LT,          /* Less than */
    OP_GT,          /* Greater than */
    OP_LEQ,         /* Less than or equal */
    OP_GEQ,         /* Greater than or equal */
    
    /* Control flow */
    OP_JMP,         /* Unconditional jump */
    OP_JZ,          /* Jump if zero (false) */
    OP_JNZ,         /* Jump if non-zero (true) */
    
    /* Function operations */
    OP_CALL,        /* Call function (operand = function index) */
    OP_RET,         /* Return from function */
    OP_RETVAL,      /* Return with value */
    OP_ENTER,       /* Enter function (operand = local var count) */
    
    /* I/O (for testing) */
    OP_PRINT,       /* Print top of stack */
    OP_PRINT_STR,   /* Print string literal */
    
    /* Program control */
    OP_HALT         /* Stop execution */
} OpCode;

/* Single bytecode instruction */
typedef struct {
    OpCode opcode;
    int operand;
} Instruction;

/* Function entry in function table */
typedef struct {
    char *name;
    int entry_point;   /* Index into code array */
    int param_count;
    int local_count;
} FunctionEntry;

/* Complete bytecode program */
typedef struct {
    Instruction code[MAX_CODE_SIZE];
    int code_size;
    
    FunctionEntry functions[MAX_FUNCTIONS];
    int function_count;
    
    int main_entry;  /* Entry point for main/ana function */
} Bytecode;

/* Label for forward jumps */
typedef struct {
    int address;     /* Address to patch */
    int target;      /* Target label number (-1 if unresolved) */
} PendingJump;

/* Local symbol for codegen - simple name->offset mapping */
#define MAX_LOCAL_SYMBOLS 64

typedef struct {
    char *name;
    int offset;
} LocalSymbol;

/* Code generator context */
typedef struct {
    Bytecode *bc;
    SymbolTable *symtab;
    
    /* Label management for control flow */
    int label_count;
    int label_addresses[MAX_LABELS];
    
    /* Pending jumps to patch */
    PendingJump pending[MAX_LABELS];
    int pending_count;
    
    /* Current function context */
    int current_func_idx;
    int local_offset;
    
    /* Local symbol table for current function */
    LocalSymbol locals[MAX_LOCAL_SYMBOLS];
    int local_count;
} CodeGenerator;

/* ============================================================================
 *  Code Generation API
 * ============================================================================ */

/* Create and destroy */
CodeGenerator *codegen_create(SymbolTable *symtab);
void codegen_destroy(CodeGenerator *gen);

/* Main entry point */
Bytecode *codegen_generate(CodeGenerator *gen, ASTNode *program);

/* Bytecode operations */
void bytecode_print(Bytecode *bc);
void bytecode_save(Bytecode *bc, const char *filename);
void bytecode_free(Bytecode *bc);

/* Opcode name for debugging */
const char *opcode_name(OpCode op);

#endif /* CODEGEN_H */
