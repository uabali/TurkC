#ifndef VM_H
#define VM_H

#include "codegen.h"
#include <stdbool.h>

/* ============================================================================
 *  TurkC Virtual Machine
 *  Stack-based bytecode interpreter.
 * ============================================================================ */

/* VM limits */
#define VM_STACK_SIZE 1024
#define VM_CALL_STACK_SIZE 64

/* Call frame for function calls */
typedef struct {
    int return_addr;    /* Address to return to */
    int base_ptr;       /* Base pointer for local variables */
    int func_idx;       /* Function index */
} CallFrame;

/* Virtual machine state */
typedef struct {
    /* Operand stack */
    int stack[VM_STACK_SIZE];
    int sp;             /* Stack pointer (points to next free slot) */
    
    /* Call stack */
    CallFrame frames[VM_CALL_STACK_SIZE];
    int fp;             /* Frame pointer (current frame index) */
    
    /* Execution state */
    int pc;             /* Program counter */
    bool running;       /* Is VM running? */
    int exit_code;      /* Exit code from program */
    
    /* Debug mode */
    bool debug;         /* Print each instruction as executed */
} VM;

/* ============================================================================
 *  VM API
 * ============================================================================ */

/* Create and destroy */
VM *vm_create(void);
void vm_destroy(VM *vm);

/* Execution */
int vm_execute(VM *vm, Bytecode *bc);

/* Configuration */
void vm_set_debug(VM *vm, bool debug);

/* Stack operations (for debugging) */
void vm_print_stack(VM *vm);

#endif /* VM_H */
