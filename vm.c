#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 *  Helper Functions
 * ============================================================================ */

static void *vm_xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memset(ptr, 0, size);
    return ptr;
}

static void vm_error(VM *vm, const char *msg) {
    fprintf(stderr, "VM Hatasi (PC=%d): %s\n", vm->pc, msg);
    vm->running = false;
    vm->exit_code = -1;
}

/* ============================================================================
 *  Stack Operations
 * ============================================================================ */

static void push(VM *vm, int value) {
    if (vm->sp >= VM_STACK_SIZE) {
        vm_error(vm, "Yigin tasmasi (stack overflow)");
        return;
    }
    vm->stack[vm->sp++] = value;
}

static int pop(VM *vm) {
    if (vm->sp <= 0) {
        vm_error(vm, "Yigin bos (stack underflow)");
        return 0;
    }
    return vm->stack[--vm->sp];
}

static int peek(VM *vm) {
    if (vm->sp <= 0) {
        vm_error(vm, "Yigin bos (stack underflow)");
        return 0;
    }
    return vm->stack[vm->sp - 1];
}

/* ============================================================================
 *  Local Variable Access
 * ============================================================================ */

static int get_local(VM *vm, int offset) {
    if (vm->fp < 0) {
        vm_error(vm, "Fonksiyon carevi yok");
        return 0;
    }
    int addr = vm->frames[vm->fp].base_ptr + offset;
    if (addr < 0 || addr >= VM_STACK_SIZE) {
        vm_error(vm, "Gecersiz lokal degisken adresi");
        return 0;
    }
    return vm->stack[addr];
}

static void set_local(VM *vm, int offset, int value) {
    if (vm->fp < 0) {
        vm_error(vm, "Fonksiyon carevi yok");
        return;
    }
    int addr = vm->frames[vm->fp].base_ptr + offset;
    if (addr < 0 || addr >= VM_STACK_SIZE) {
        vm_error(vm, "Gecersiz lokal degisken adresi");
        return;
    }
    vm->stack[addr] = value;
}

/* ============================================================================
 *  VM Creation/Destruction
 * ============================================================================ */

VM *vm_create(void) {
    VM *vm = (VM *)vm_xmalloc(sizeof(VM));
    vm->sp = 0;
    vm->fp = -1;
    vm->pc = 0;
    vm->running = false;
    vm->exit_code = 0;
    vm->debug = false;
    return vm;
}

void vm_destroy(VM *vm) {
    free(vm);
}

void vm_set_debug(VM *vm, bool debug) {
    vm->debug = debug;
}

void vm_print_stack(VM *vm) {
    printf("Stack (sp=%d): [", vm->sp);
    for (int i = 0; i < vm->sp && i < 10; i++) {
        printf("%d", vm->stack[i]);
        if (i < vm->sp - 1) printf(", ");
    }
    if (vm->sp > 10) printf(", ...");
    printf("]\n");
}

/* ============================================================================
 *  Main Execution Loop
 * ============================================================================ */

int vm_execute(VM *vm, Bytecode *bc) {
    if (!vm || !bc) return -1;
    
    /* Find main entry point */
    if (bc->main_entry < 0) {
        fprintf(stderr, "Hata: 'ana' fonksiyonu bulunamadi\n");
        return -1;
    }
    
    /* Set up initial call frame for main */
    vm->pc = bc->main_entry;
    vm->running = true;
    vm->fp = 0;
    vm->frames[0].return_addr = bc->code_size;  /* Return to end (HALT) */
    vm->frames[0].base_ptr = 0;
    vm->frames[0].func_idx = -1;
    
    /* Main execution loop */
    while (vm->running && vm->pc < bc->code_size) {
        Instruction inst = bc->code[vm->pc];
        
        if (vm->debug) {
            printf("PC=%04d: %-10s %d  ", vm->pc, opcode_name(inst.opcode), inst.operand);
            vm_print_stack(vm);
        }
        
        vm->pc++;
        
        switch (inst.opcode) {
            case OP_NOP:
                /* Do nothing */
                break;
            
            case OP_PUSH:
                push(vm, inst.operand);
                break;
            
            case OP_POP:
                pop(vm);
                break;
            
            case OP_DUP:
                push(vm, peek(vm));
                break;
            
            case OP_LOAD:
                push(vm, get_local(vm, inst.operand));
                break;
            
            case OP_STORE:
                set_local(vm, inst.operand, pop(vm));
                break;
            
            case OP_LOAD_GLOBAL:
                /* For simplicity, treat global as offset 0 in stack */
                if (inst.operand >= 0 && inst.operand < VM_STACK_SIZE) {
                    push(vm, vm->stack[inst.operand]);
                }
                break;
            
            case OP_STORE_GLOBAL:
                if (inst.operand >= 0 && inst.operand < VM_STACK_SIZE) {
                    vm->stack[inst.operand] = pop(vm);
                }
                break;
            
            case OP_ADD: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a + b);
                break;
            }
            
            case OP_SUB: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a - b);
                break;
            }
            
            case OP_MUL: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a * b);
                break;
            }
            
            case OP_DIV: {
                int b = pop(vm);
                int a = pop(vm);
                if (b == 0) {
                    vm_error(vm, "Sifira bolme hatasi");
                    break;
                }
                push(vm, a / b);
                break;
            }
            
            case OP_MOD: {
                int b = pop(vm);
                int a = pop(vm);
                if (b == 0) {
                    vm_error(vm, "Sifira bolme hatasi");
                    break;
                }
                push(vm, a % b);
                break;
            }
            
            case OP_NEG:
                push(vm, -pop(vm));
                break;
            
            case OP_EQ: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a == b ? 1 : 0);
                break;
            }
            
            case OP_NEQ: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a != b ? 1 : 0);
                break;
            }
            
            case OP_LT: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a < b ? 1 : 0);
                break;
            }
            
            case OP_GT: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a > b ? 1 : 0);
                break;
            }
            
            case OP_LEQ: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a <= b ? 1 : 0);
                break;
            }
            
            case OP_GEQ: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a >= b ? 1 : 0);
                break;
            }
            
            case OP_JMP:
                vm->pc = inst.operand;
                break;
            
            case OP_JZ:
                if (pop(vm) == 0) {
                    vm->pc = inst.operand;
                }
                break;
            
            case OP_JNZ:
                if (pop(vm) != 0) {
                    vm->pc = inst.operand;
                }
                break;
            
            case OP_CALL: {
                int func_idx = inst.operand;
                if (func_idx < 0 || func_idx >= bc->function_count) {
                    vm_error(vm, "Gecersiz fonksiyon indeksi");
                    break;
                }
                
                FunctionEntry *func = &bc->functions[func_idx];
                
                /* Save current state in new frame */
                if (vm->fp >= VM_CALL_STACK_SIZE - 1) {
                    vm_error(vm, "Cagri yigini tasmasi (call stack overflow)");
                    break;
                }
                
                vm->fp++;
                vm->frames[vm->fp].return_addr = vm->pc;
                vm->frames[vm->fp].func_idx = func_idx;
                
                /* Set up new base pointer - arguments are already on stack */
                /* Base pointer points to first local (after arguments) */
                vm->frames[vm->fp].base_ptr = vm->sp - func->param_count;
                
                /* Reserve space for locals */
                vm->sp = vm->frames[vm->fp].base_ptr + func->local_count;
                
                /* Jump to function entry point */
                vm->pc = func->entry_point;
                break;
            }
            
            case OP_RET: {
                if (vm->fp <= 0) {
                    /* Returning from main - exit with 0 */
                    vm->running = false;
                    vm->exit_code = 0;
                    break;
                }
                
                /* Restore caller state */
                int return_addr = vm->frames[vm->fp].return_addr;
                int old_base = vm->frames[vm->fp].base_ptr;
                
                vm->fp--;
                vm->sp = old_base;  /* Pop all locals */
                vm->pc = return_addr;
                break;
            }
            
            case OP_RETVAL: {
                int return_value = pop(vm);
                
                if (vm->fp <= 0) {
                    /* Returning from main - exit with return value */
                    vm->running = false;
                    vm->exit_code = return_value;
                    break;
                }
                
                /* Restore caller state */
                int return_addr = vm->frames[vm->fp].return_addr;
                int old_base = vm->frames[vm->fp].base_ptr;
                int param_count = 0;
                
                if (vm->frames[vm->fp].func_idx >= 0) {
                    param_count = bc->functions[vm->frames[vm->fp].func_idx].param_count;
                }
                
                vm->fp--;
                vm->sp = old_base - param_count;  /* Pop all locals and arguments */
                vm->pc = return_addr;
                
                /* Push return value */
                push(vm, return_value);
                break;
            }
            
            case OP_ENTER:
                /* OP_ENTER just reserves space - already handled in CALL */
                /* For main(), we need to allocate locals */
                if (vm->fp == 0) {
                    vm->frames[0].base_ptr = 0;
                    vm->sp = inst.operand;
                }
                break;
            
            case OP_PRINT:
                printf("%d\n", pop(vm));
                break;
            
            case OP_PRINT_STR:
                /* Not implemented - would need string table */
                printf("<string>\n");
                break;
            
            case OP_HALT:
                vm->running = false;
                if (vm->sp > 0) {
                    vm->exit_code = vm->stack[vm->sp - 1];
                }
                break;
            
            default:
                vm_error(vm, "Bilinmeyen opcode");
                break;
        }
    }
    
    return vm->exit_code;
}
