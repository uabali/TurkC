#include "codegen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 *  Forward Declarations
 * ============================================================================ */

static void gen_node(CodeGenerator *gen, ASTNode *node);
static void gen_function(CodeGenerator *gen, ASTNode *node);
static void gen_block(CodeGenerator *gen, ASTNode *node);
static void gen_statement(CodeGenerator *gen, ASTNode *node);
static void gen_var_decl(CodeGenerator *gen, ASTNode *node);
static void gen_if(CodeGenerator *gen, ASTNode *node);
static void gen_while(CodeGenerator *gen, ASTNode *node);
static void gen_for(CodeGenerator *gen, ASTNode *node);
static void gen_return(CodeGenerator *gen, ASTNode *node);
static void gen_expression(CodeGenerator *gen, ASTNode *node);

/* ============================================================================
 *  Helper Functions
 * ============================================================================ */

static void *cg_xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Bellek tahsisi basarisiz oldu\n");
        exit(EXIT_FAILURE);
    }
    memset(ptr, 0, size);
    return ptr;
}

static char *cg_strdup(const char *src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char *copy = (char *)cg_xmalloc(len + 1);
    memcpy(copy, src, len + 1);
    return copy;
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
 *  Local Symbol Management (for codegen only)
 * ============================================================================ */

/* Find local variable by name, returns offset or -1 if not found */
static int find_local(CodeGenerator *gen, const char *name) {
    for (int i = 0; i < gen->local_count; i++) {
        if (gen->locals[i].name && strcmp(gen->locals[i].name, name) == 0) {
            return gen->locals[i].offset;
        }
    }
    return -1;
}

/* Add a new local variable, returns its offset */
static int add_local(CodeGenerator *gen, const char *name) {
    /* Check if already exists */
    int existing = find_local(gen, name);
    if (existing >= 0) {
        return existing;
    }
    
    /* Add new local */
    if (gen->local_count >= MAX_LOCAL_SYMBOLS) {
        fprintf(stderr, "Hata: Maksimum lokal degisken sayisi asildi\n");
        return 0;
    }
    
    int offset = gen->local_offset++;
    gen->locals[gen->local_count].name = cg_strdup(name);
    gen->locals[gen->local_count].offset = offset;
    gen->local_count++;
    
    return offset;
}

/* Clear all local symbols (when entering new function) */
static void clear_locals(CodeGenerator *gen) {
    for (int i = 0; i < gen->local_count; i++) {
        free(gen->locals[i].name);
        gen->locals[i].name = NULL;
    }
    gen->local_count = 0;
    gen->local_offset = 0;
}


static void emit(CodeGenerator *gen, OpCode op, int operand) {
    if (gen->bc->code_size >= MAX_CODE_SIZE) {
        fprintf(stderr, "Hata: Bytecode boyutu asild\n");
        exit(EXIT_FAILURE);
    }
    gen->bc->code[gen->bc->code_size].opcode = op;
    gen->bc->code[gen->bc->code_size].operand = operand;
    gen->bc->code_size++;
}

static void emit_simple(CodeGenerator *gen, OpCode op) {
    emit(gen, op, 0);
}

static int current_address(CodeGenerator *gen) {
    return gen->bc->code_size;
}

/* Label management */
static int new_label(CodeGenerator *gen) {
    int label = gen->label_count++;
    gen->label_addresses[label] = -1;  /* Unresolved */
    return label;
}

static void set_label(CodeGenerator *gen, int label) {
    gen->label_addresses[label] = current_address(gen);
}

static void emit_jump(CodeGenerator *gen, OpCode op, int label) {
    /* If label is already resolved, use its address directly */
    if (gen->label_addresses[label] >= 0) {
        emit(gen, op, gen->label_addresses[label]);
    } else {
        /* Emit with placeholder, record for later patching */
        int addr = current_address(gen);
        emit(gen, op, -1);
        gen->pending[gen->pending_count].address = addr;
        gen->pending[gen->pending_count].target = label;
        gen->pending_count++;
    }
}

static void patch_jumps(CodeGenerator *gen) {
    for (int i = 0; i < gen->pending_count; i++) {
        int addr = gen->pending[i].address;
        int label = gen->pending[i].target;
        int target_addr = gen->label_addresses[label];
        
        if (target_addr < 0) {
            fprintf(stderr, "Hata: Cozulmemis etiket %d\n", label);
            continue;
        }
        
        gen->bc->code[addr].operand = target_addr;
    }
}

/* ============================================================================
 *  Code Generator Creation/Destruction
 * ============================================================================ */

CodeGenerator *codegen_create(SymbolTable *symtab) {
    CodeGenerator *gen = (CodeGenerator *)cg_xmalloc(sizeof(CodeGenerator));
    gen->bc = (Bytecode *)cg_xmalloc(sizeof(Bytecode));
    gen->symtab = symtab;
    gen->label_count = 0;
    gen->pending_count = 0;
    gen->current_func_idx = -1;
    gen->local_offset = 0;
    gen->local_count = 0;
    
    gen->bc->code_size = 0;
    gen->bc->function_count = 0;
    gen->bc->main_entry = -1;
    
    return gen;
}

void codegen_destroy(CodeGenerator *gen) {
    if (!gen) return;
    /* Don't free bytecode - it's returned to caller */
    free(gen);
}

/* ============================================================================
 *  Function Code Generation
 * ============================================================================ */

static int register_function(CodeGenerator *gen, const char *name, 
                             int param_count, int local_count) {
    if (gen->bc->function_count >= MAX_FUNCTIONS) {
        fprintf(stderr, "Hata: Maksimum fonksiyon sayisi asildi\n");
        exit(EXIT_FAILURE);
    }
    
    int idx = gen->bc->function_count++;
    gen->bc->functions[idx].name = cg_strdup(name);
    gen->bc->functions[idx].entry_point = current_address(gen);
    gen->bc->functions[idx].param_count = param_count;
    gen->bc->functions[idx].local_count = local_count;
    
    return idx;
}

static int find_function(CodeGenerator *gen, const char *name) {
    for (int i = 0; i < gen->bc->function_count; i++) {
        if (strcmp(gen->bc->functions[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

static void gen_function(CodeGenerator *gen, ASTNode *node) {
    const char *func_name = node->text;
    
    /* Clear local symbols from previous function */
    clear_locals(gen);
    
    /* Get function info from symbol table */
    Symbol *func_sym = symbol_lookup(gen->symtab, func_name);
    int param_count = func_sym && func_sym->func_info ? 
                      func_sym->func_info->param_count : 0;
    
    /* Add parameters to local symbol table */
    if (func_sym && func_sym->func_info) {
        for (int i = 0; i < func_sym->func_info->param_count; i++) {
            add_local(gen, func_sym->func_info->param_names[i]);
        }
    }
    
    /* Also collect parameters from AST if symbol table is empty */
    ASTNode *param_list = get_child(node, 0);
    if (param_list && param_list->type == AST_PARAM_LIST) {
        for (ASTNode *param = param_list->first_child; param; param = param->next_sibling) {
            if (param->type == AST_PARAM && param->text) {
                add_local(gen, param->text);
            }
        }
    }
    
    /* For simplicity, allocate extra space for locals */
    int local_count = 32;  /* Reserve space for locals */
    
    /* Register function */
    int func_idx = register_function(gen, func_name, param_count, local_count);
    gen->current_func_idx = func_idx;
    
    /* Check if this is main/ana */
    if (strcmp(func_name, "ana") == 0) {
        gen->bc->main_entry = gen->bc->functions[func_idx].entry_point;
    }
    
    /* Emit function prologue */
    emit(gen, OP_ENTER, local_count);
    
    /* Generate body */
    ASTNode *body = get_child(node, 1);
    if (body && body->type == AST_BLOCK) {
        gen_block(gen, body);
    }
    
    /* Default return (in case function doesn't have explicit return) */
    emit(gen, OP_PUSH, 0);
    emit_simple(gen, OP_RETVAL);
    
    gen->current_func_idx = -1;
}

/* ============================================================================
 *  Statement Code Generation
 * ============================================================================ */

static void gen_block(CodeGenerator *gen, ASTNode *node) {
    for (ASTNode *stmt = node->first_child; stmt; stmt = stmt->next_sibling) {
        gen_statement(gen, stmt);
    }
}

static void gen_statement(CodeGenerator *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL:
            gen_var_decl(gen, node);
            break;
            
        case AST_BLOCK:
            gen_block(gen, node);
            break;
            
        case AST_IF:
        case AST_IF_ELSE:
            gen_if(gen, node);
            break;
            
        case AST_WHILE:
            gen_while(gen, node);
            break;
            
        case AST_FOR:
            gen_for(gen, node);
            break;
            
        case AST_RETURN:
            gen_return(gen, node);
            break;
            
        case AST_EXPR_STATEMENT:
            if (node->first_child) {
                gen_expression(gen, node->first_child);
                emit_simple(gen, OP_POP);  /* Discard result */
            }
            break;
            
        case AST_ASSIGNMENT:
            gen_expression(gen, node);
            emit_simple(gen, OP_POP);  /* Discard result */
            break;
            
        default:
            break;
    }
}

static void gen_var_decl(CodeGenerator *gen, ASTNode *node) {
    const char *var_name = node->text;
    
    /* Add variable to local symbol table and get its offset */
    int offset = add_local(gen, var_name);
    
    /* If there's an initializer, generate it and store */
    if (node->first_child) {
        gen_expression(gen, node->first_child);
        emit(gen, OP_STORE, offset);
    }
}

static void gen_if(CodeGenerator *gen, ASTNode *node) {
    ASTNode *condition = get_child(node, 0);
    ASTNode *then_block = get_child(node, 1);
    ASTNode *else_block = (node->type == AST_IF_ELSE) ? get_child(node, 2) : NULL;
    
    int else_label = new_label(gen);
    int end_label = new_label(gen);
    
    /* Generate condition */
    gen_expression(gen, condition);
    
    if (else_block) {
        /* if-else */
        emit_jump(gen, OP_JZ, else_label);
        gen_statement(gen, then_block);
        emit_jump(gen, OP_JMP, end_label);
        set_label(gen, else_label);
        gen_statement(gen, else_block);
        set_label(gen, end_label);
    } else {
        /* if without else */
        emit_jump(gen, OP_JZ, end_label);
        gen_statement(gen, then_block);
        set_label(gen, end_label);
    }
}

static void gen_while(CodeGenerator *gen, ASTNode *node) {
    ASTNode *condition = get_child(node, 0);
    ASTNode *body = get_child(node, 1);
    
    int start_label = new_label(gen);
    int end_label = new_label(gen);
    
    set_label(gen, start_label);
    
    /* Generate condition */
    gen_expression(gen, condition);
    emit_jump(gen, OP_JZ, end_label);
    
    /* Generate body */
    gen_statement(gen, body);
    
    /* Jump back to start */
    emit_jump(gen, OP_JMP, start_label);
    
    set_label(gen, end_label);
}

static void gen_for(CodeGenerator *gen, ASTNode *node) {
    ASTNode *init = get_child(node, 0);
    ASTNode *condition = get_child(node, 1);
    ASTNode *update = get_child(node, 2);
    ASTNode *body = get_child(node, 3);
    
    int start_label = new_label(gen);
    int end_label = new_label(gen);
    
    /* Generate init */
    if (init && init->type != AST_EMPTY) {
        gen_statement(gen, init);
    }
    
    set_label(gen, start_label);
    
    /* Generate condition */
    if (condition && condition->type != AST_EMPTY) {
        gen_expression(gen, condition);
        emit_jump(gen, OP_JZ, end_label);
    }
    
    /* Generate body */
    gen_statement(gen, body);
    
    /* Generate update */
    if (update && update->type != AST_EMPTY) {
        gen_expression(gen, update);
        emit_simple(gen, OP_POP);  /* Discard update result */
    }
    
    /* Jump back to start */
    emit_jump(gen, OP_JMP, start_label);
    
    set_label(gen, end_label);
}

static void gen_return(CodeGenerator *gen, ASTNode *node) {
    if (node->first_child) {
        gen_expression(gen, node->first_child);
        emit_simple(gen, OP_RETVAL);
    } else {
        emit_simple(gen, OP_RET);
    }
}

/* ============================================================================
 *  Expression Code Generation
 * ============================================================================ */

static void gen_expression(CodeGenerator *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_NUMBER_LITERAL: {
            int value = atoi(node->text);
            emit(gen, OP_PUSH, value);
            break;
        }
        
        case AST_STRING_LITERAL:
            /* For now, just push 0 (strings not fully supported) */
            emit(gen, OP_PUSH, 0);
            break;
        
        case AST_IDENTIFIER: {
            int offset = find_local(gen, node->text);
            if (offset >= 0) {
                emit(gen, OP_LOAD, offset);
            } else {
                fprintf(stderr, "Hata: '%s' degiskeni bulunamadi\n", node->text);
                emit(gen, OP_PUSH, 0);
            }
            break;
        }
        
        case AST_ASSIGNMENT: {
            ASTNode *left = get_child(node, 0);
            ASTNode *right = get_child(node, 1);
            
            /* Generate RHS */
            gen_expression(gen, right);
            emit_simple(gen, OP_DUP);  /* Keep value on stack for expression result */
            
            /* Store to LHS */
            if (left && left->type == AST_IDENTIFIER) {
                int offset = find_local(gen, left->text);
                if (offset >= 0) {
                    emit(gen, OP_STORE, offset);
                } else {
                    /* Variable might not exist yet, add it */
                    offset = add_local(gen, left->text);
                    emit(gen, OP_STORE, offset);
                }
            }
            break;
        }
        
        case AST_BINARY_EXPR: {
            ASTNode *left = get_child(node, 0);
            ASTNode *right = get_child(node, 1);
            const char *op = node->text;
            
            /* Generate left operand */
            gen_expression(gen, left);
            
            /* Generate right operand */
            gen_expression(gen, right);
            
            /* Emit operator */
            if (strcmp(op, "+") == 0) emit_simple(gen, OP_ADD);
            else if (strcmp(op, "-") == 0) emit_simple(gen, OP_SUB);
            else if (strcmp(op, "*") == 0) emit_simple(gen, OP_MUL);
            else if (strcmp(op, "/") == 0) emit_simple(gen, OP_DIV);
            else if (strcmp(op, "%") == 0) emit_simple(gen, OP_MOD);
            else if (strcmp(op, "==") == 0) emit_simple(gen, OP_EQ);
            else if (strcmp(op, "!=") == 0) emit_simple(gen, OP_NEQ);
            else if (strcmp(op, "<") == 0) emit_simple(gen, OP_LT);
            else if (strcmp(op, ">") == 0) emit_simple(gen, OP_GT);
            else if (strcmp(op, "<=") == 0) emit_simple(gen, OP_LEQ);
            else if (strcmp(op, ">=") == 0) emit_simple(gen, OP_GEQ);
            else {
                fprintf(stderr, "Uyari: Bilinmeyen operator '%s'\n", op);
            }
            break;
        }
        
        case AST_UNARY_EXPR: {
            ASTNode *operand = get_child(node, 0);
            const char *op = node->text;
            
            gen_expression(gen, operand);
            
            if (op && strcmp(op, "-") == 0) {
                emit_simple(gen, OP_NEG);
            }
            break;
        }
        
        case AST_FUNCTION_CALL: {
            const char *func_name = node->text;
            ASTNode *arg_list = get_child(node, 0);
            
            /* Push arguments in order */
            if (arg_list && arg_list->type == AST_ARGUMENT_LIST) {
                for (ASTNode *arg = arg_list->first_child; arg; 
                     arg = arg->next_sibling) {
                    gen_expression(gen, arg);
                }
            }
            
            /* Find function and call it */
            int func_idx = find_function(gen, func_name);
            if (func_idx >= 0) {
                emit(gen, OP_CALL, func_idx);
            } else {
                fprintf(stderr, "Hata: '%s' fonksiyonu bulunamadi\n", func_name);
                emit(gen, OP_PUSH, 0);
            }
            break;
        }
        
        case AST_EXPR_STATEMENT:
            gen_expression(gen, node->first_child);
            break;
        
        default:
            break;
    }
}

/* ============================================================================
 *  Main Entry Point
 * ============================================================================ */

static void gen_node(CodeGenerator *gen, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            /* First pass: generate all functions */
            for (ASTNode *child = node->first_child; child; 
                 child = child->next_sibling) {
                if (child->type == AST_FUNCTION) {
                    gen_function(gen, child);
                }
            }
            break;
            
        case AST_FUNCTION:
            gen_function(gen, node);
            break;
            
        default:
            break;
    }
}

Bytecode *codegen_generate(CodeGenerator *gen, ASTNode *program) {
    if (!gen || !program) return NULL;
    
    /* Generate code for entire program */
    gen_node(gen, program);
    
    /* Emit halt at end (in case main doesn't return) */
    emit_simple(gen, OP_HALT);
    
    /* Patch all forward jumps */
    patch_jumps(gen);
    
    return gen->bc;
}

/* ============================================================================
 *  Bytecode Output
 * ============================================================================ */

const char *opcode_name(OpCode op) {
    static const char *names[] = {
        "NOP", "PUSH", "POP", "DUP",
        "LOAD", "STORE", "LOAD_GLOBAL", "STORE_GLOBAL",
        "ADD", "SUB", "MUL", "DIV", "MOD", "NEG",
        "EQ", "NEQ", "LT", "GT", "LEQ", "GEQ",
        "JMP", "JZ", "JNZ",
        "CALL", "RET", "RETVAL", "ENTER",
        "PRINT", "PRINT_STR",
        "HALT"
    };
    
    if (op >= 0 && op <= OP_HALT) {
        return names[op];
    }
    return "???";
}

void bytecode_print(Bytecode *bc) {
    printf("\n=== Fonksiyon Tablosu ===\n");
    for (int i = 0; i < bc->function_count; i++) {
        printf("[%d] %s: entry=%d, params=%d, locals=%d\n",
               i, bc->functions[i].name,
               bc->functions[i].entry_point,
               bc->functions[i].param_count,
               bc->functions[i].local_count);
    }
    
    printf("\n=== Uretilen Bytecode ===\n");
    for (int i = 0; i < bc->code_size; i++) {
        Instruction inst = bc->code[i];
        
        /* Check if this is a function entry point */
        for (int f = 0; f < bc->function_count; f++) {
            if (bc->functions[f].entry_point == i) {
                printf("\n; %s:\n", bc->functions[f].name);
            }
        }
        
        /* Print instruction */
        printf("%04d: %-10s", i, opcode_name(inst.opcode));
        
        /* Print operand for instructions that have one */
        switch (inst.opcode) {
            case OP_PUSH:
            case OP_LOAD:
            case OP_STORE:
            case OP_LOAD_GLOBAL:
            case OP_STORE_GLOBAL:
            case OP_JMP:
            case OP_JZ:
            case OP_JNZ:
            case OP_CALL:
            case OP_ENTER:
                printf(" %d", inst.operand);
                break;
            default:
                break;
        }
        printf("\n");
    }
    printf("\n=== Bytecode Sonu (toplam %d instruction) ===\n", bc->code_size);
}

void bytecode_save(Bytecode *bc, const char *filename) {
    if (!bc || !filename) return;
    
    FILE *f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Hata: '%s' dosyasi acilamadi\n", filename);
        return;
    }
    
    /* Write magic number */
    const char magic[4] = {'T', 'K', 'B', 'C'};  /* TurkC ByteCode */
    fwrite(magic, 1, 4, f);
    
    /* Write function count */
    fwrite(&bc->function_count, sizeof(int), 1, f);
    
    /* Write functions */
    for (int i = 0; i < bc->function_count; i++) {
        int name_len = strlen(bc->functions[i].name);
        fwrite(&name_len, sizeof(int), 1, f);
        fwrite(bc->functions[i].name, 1, name_len, f);
        fwrite(&bc->functions[i].entry_point, sizeof(int), 1, f);
        fwrite(&bc->functions[i].param_count, sizeof(int), 1, f);
        fwrite(&bc->functions[i].local_count, sizeof(int), 1, f);
    }
    
    /* Write main entry */
    fwrite(&bc->main_entry, sizeof(int), 1, f);
    
    /* Write code size */
    fwrite(&bc->code_size, sizeof(int), 1, f);
    
    /* Write instructions */
    for (int i = 0; i < bc->code_size; i++) {
        fwrite(&bc->code[i].opcode, sizeof(OpCode), 1, f);
        fwrite(&bc->code[i].operand, sizeof(int), 1, f);
    }
    
    fclose(f);
    printf("Bytecode '%s' dosyasina kaydedildi.\n", filename);
}

void bytecode_free(Bytecode *bc) {
    if (!bc) return;
    
    for (int i = 0; i < bc->function_count; i++) {
        free(bc->functions[i].name);
    }
    free(bc);
}
