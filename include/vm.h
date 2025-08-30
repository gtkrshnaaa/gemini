#ifndef VM_H
#define VM_H

#include "common.h"
#include "parser.h"

// Value types for VM
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL,
    VAL_MODULE
} ValueType;

// Value structure for runtime values
typedef struct Module Module;

// Value structure for runtime values
typedef struct {
    ValueType type;
    union {
        int intVal;
        double floatVal;
        char* stringVal;
        bool boolVal;
        Module* moduleVal;
    };
} Value;

// Forward declarations
typedef struct VarEntry VarEntry;
typedef struct FuncEntry FuncEntry;
typedef struct Function Function;
typedef struct Environment Environment;
typedef struct CallFrame CallFrame;
typedef struct VM VM;

// Variable entry structure for hash table
struct VarEntry {
    char* key;              // Variable name
    Value value;            // Variable value
    struct VarEntry* next;  // Next entry in hash bucket
};

// Function entry structure for hash table
struct FuncEntry {
    char* key;              // Function name
    Function* function;     // Function definition
    struct FuncEntry* next; // Next entry in hash bucket
};

// Hash table size for environments
#define TABLE_SIZE 256

// Environment structure (scope)
struct Environment {
    VarEntry* buckets[TABLE_SIZE];     // Variable hash table
    FuncEntry* funcBuckets[TABLE_SIZE]; // Function hash table
};

// Module wrapper
struct Module {
    char* name;
    Environment* env;
    // Keep the module source buffer alive for the lifetime of the module,
    // because AST Tokens point into this buffer.
    char* source;
};

// Function structure
struct Function {
    Token name;             // Function name
    Token* params;          // Parameter names
    int paramCount;         // Number of parameters
    Node* body;             // Function body (AST node)
    Environment* closure;   // Closure environment (for lexical scoping)
};

// VM constants
#define STACK_MAX 256           // Maximum stack size
#define CALL_STACK_MAX 64       // Maximum call stack depth

// Call frame structure for function calls
struct CallFrame {
    Environment* env;       // Previous environment
    Value returnValue;      // Function return value
    bool hasReturned;       // Whether function has returned
};

// Virtual Machine structure
struct VM {
    Value stack[STACK_MAX];         // Value stack
    int stackTop;                   // Stack pointer
    Environment* env;               // Current environment
    Environment* globalEnv;         // Global environment
    Environment* defEnv;            // Target environment for function definitions
    CallFrame callStack[CALL_STACK_MAX]; // Call stack
    int callStackTop;               // Call stack pointer
    char projectRoot[1024];         // Project root directory for module search
};

// VM function prototypes

/**
 * Initialize the virtual machine
 * @param vm Pointer to VM structure
 */
void initVM(VM* vm);

/**
 * Free all VM resources
 * @param vm Pointer to VM structure
 */
void freeVM(VM* vm);

/**
 * Interpret and execute AST
 * @param vm Pointer to VM structure
 * @param ast Root AST node to execute
 */
void interpret(VM* vm, Node* ast);

#endif // VM_H