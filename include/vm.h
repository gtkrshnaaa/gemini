#ifndef VM_H
#define VM_H

#include "common.h"
#include "parser.h"

// Value types for VM
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL
} ValueType;

typedef struct {
    ValueType type;
    union {
        int intVal;
        double floatVal;
        char* stringVal;
        bool boolVal;
    };
} Value;

// Variable structure (simple hash table for variables)
typedef struct VarEntry {
    char* key;
    Value value;
    struct VarEntry* next;
} VarEntry;

#define TABLE_SIZE 256

typedef struct {
    VarEntry* buckets[TABLE_SIZE];
} Environment;

// VM structure (stack-based)
#define STACK_MAX 256

typedef struct {
    Value stack[STACK_MAX];
    int stackTop;
    Environment* env;
    // For functions, we can add call stack later
} VM;

// Initialize VM
void initVM(VM* vm);

// Free VM
void freeVM(VM* vm);

// Interpret AST
void interpret(VM* vm, Node* ast);

#endif // VM_H