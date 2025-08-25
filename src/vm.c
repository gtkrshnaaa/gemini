#include "vm.h"

// Hash function for variables
static unsigned int hash(const char* key, int length) {
    unsigned int hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (unsigned char)key[i];
        hash *= 16777619;
    }
    return hash % TABLE_SIZE;
}

// Find or insert variable
static VarEntry* findEntry(Environment* env, Token name, bool insert) {
    unsigned int h = hash(name.start, name.length);
    VarEntry* entry = env->buckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry;
        }
        entry = entry->next;
    }
    if (insert) {
        entry = malloc(sizeof(VarEntry));
        if (!entry) {
            error("Memory allocation failed.", name.line);
        }
        entry->key = strndup(name.start, name.length);
        if (!entry->key) {
            free(entry);
            error("Memory allocation failed.", name.line);
        }
        entry->value.type = VAL_INT; // Default init
        entry->value.intVal = 0;
        entry->next = env->buckets[h];
        env->buckets[h] = entry;
    }
    return entry;
}

// Evaluate expression
static Value evaluate(VM* vm, Node* node);

// Execute statement
static void execute(VM* vm, Node* node) {
    if (!node) {
        error("Null statement.", 0);
        return;
    }
    switch (node->type) {
        case NODE_STMT_VAR_DECL: {
            Value init = {VAL_INT, .intVal = 0}; // Default 0
            if (node->var_decl.initializer) {
                init = evaluate(vm, node->var_decl.initializer);
            }
            VarEntry* entry = findEntry(vm->env, node->var_decl.name, true);
            if (entry) {
                entry->value = init;
            }
            break;
        }
        case NODE_STMT_ASSIGN: {
            Value value = evaluate(vm, node->assign.value);
            VarEntry* entry = findEntry(vm->env, node->assign.name, false);
            if (entry) {
                entry->value = value;
            } else {
                error("Undefined variable.", node->assign.name.line);
            }
            break;
        }
        case NODE_STMT_PRINT: {
            Value value = evaluate(vm, node->print.expr);
            switch (value.type) {
                case VAL_INT: printf("%d\n", value.intVal); break;
                case VAL_FLOAT: printf("%f\n", value.floatVal); break;
                case VAL_STRING: printf("%s\n", value.stringVal); break;
                case VAL_BOOL: printf("%s\n", value.boolVal ? "true" : "false"); break;
            }
            break;
        }
        case NODE_STMT_IF: {
            Value cond = evaluate(vm, node->if_stmt.condition);
            bool isTrue = (cond.type == VAL_BOOL ? cond.boolVal : (cond.type == VAL_INT ? cond.intVal != 0 : true));
            if (isTrue) {
                execute(vm, node->if_stmt.thenBranch);
            } else if (node->if_stmt.elseBranch) {
                execute(vm, node->if_stmt.elseBranch);
            }
            break;
        }
        case NODE_STMT_WHILE: {
            while (true) {
                Value cond = evaluate(vm, node->while_stmt.condition);
                bool isTrue = (cond.type == VAL_BOOL ? cond.boolVal : (cond.type == VAL_INT ? cond.intVal != 0 : true));
                if (!isTrue) break;
                execute(vm, node->while_stmt.body);
            }
            break;
        }
        case NODE_STMT_FOR: {
            if (node->for_stmt.initializer) execute(vm, node->for_stmt.initializer);
            while (true) {
                bool condTrue = true;
                if (node->for_stmt.condition) {
                    Value cond = evaluate(vm, node->for_stmt.condition);
                    condTrue = (cond.type == VAL_BOOL ? cond.boolVal : (cond.type == VAL_INT ? cond.intVal != 0 : true));
                }
                if (!condTrue) break;
                execute(vm, node->for_stmt.body);
                if (node->for_stmt.increment) execute(vm, node->for_stmt.increment); // Changed to execute
            }
            break;
        }
        case NODE_STMT_BLOCK: {
            for (int i = 0; i < node->block.count; i++) {
                execute(vm, node->block.statements[i]);
            }
            break;
        }
        case NODE_STMT_FUNCTION: {
            printf("Function declaration not fully implemented.\n");
            break;
        }
        case NODE_STMT_RETURN: {
            break;
        }
        default:
            evaluate(vm, node);
            break;
    }
}

// Evaluate expression
static Value evaluate(VM* vm, Node* node) {
    if (!node) {
        error("Null expression.", 0);
        Value nullVal = {VAL_INT, .intVal = 0};
        return nullVal;
    }
    switch (node->type) {
        case NODE_EXPR_LITERAL: {
            Token t = node->literal.token;
            Value val;
            if (t.type == TOKEN_NUMBER) {
                char* str = strndup(t.start, t.length);
                if (!str) error("Memory allocation failed.", t.line);
                if (strchr(str, '.')) {
                    val.type = VAL_FLOAT;
                    val.floatVal = atof(str);
                } else {
                    val.type = VAL_INT;
                    val.intVal = atoi(str);
                }
                free(str);
            } else if (t.type == TOKEN_STRING) {
                val.type = VAL_STRING;
                // Skip quotes in string (start + 1, length - 2)
                val.stringVal = strndup(t.start + 1, t.length - 2);
                if (!val.stringVal) error("Memory allocation failed.", t.line);
            } else {
                error("Invalid literal type.", t.line);
                val.type = VAL_INT;
                val.intVal = 0;
            }
            return val;
        }
        case NODE_EXPR_VAR: {
            VarEntry* entry = findEntry(vm->env, node->var.name, false);
            if (entry) return entry->value;
            error("Undefined variable.", node->var.name.line);
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
        }
        case NODE_EXPR_UNARY: {
            Value expr = evaluate(vm, node->unary.expr);
            if (node->unary.op.type == TOKEN_MINUS) {
                if (expr.type == VAL_INT) expr.intVal = -expr.intVal;
                else if (expr.type == VAL_FLOAT) expr.floatVal = -expr.floatVal;
            }
            return expr;
        }
        case NODE_EXPR_BINARY: {
            Value left = evaluate(vm, node->binary.left);
            Value right = evaluate(vm, node->binary.right);
            Value result;
            if (left.type != VAL_INT || right.type != VAL_INT) {
                error("Type mismatch in binary op.", node->binary.op.line);
            }
            result.type = VAL_INT;
            switch (node->binary.op.type) {
                case TOKEN_PLUS: result.intVal = left.intVal + right.intVal; break;
                case TOKEN_MINUS: result.intVal = left.intVal - right.intVal; break;
                case TOKEN_STAR: result.intVal = left.intVal * right.intVal; break;
                case TOKEN_SLASH: result.intVal = left.intVal / right.intVal; break;
                case TOKEN_PERCENT: result.intVal = left.intVal % right.intVal; break;
                case TOKEN_EQUAL_EQUAL: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal == right.intVal; 
                    break;
                case TOKEN_BANG_EQUAL: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal != right.intVal; 
                    break;
                case TOKEN_GREATER: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal > right.intVal; 
                    break;
                case TOKEN_GREATER_EQUAL: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal >= right.intVal; 
                    break;
                case TOKEN_LESS: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal < right.intVal; 
                    break;
                case TOKEN_LESS_EQUAL: 
                    result.type = VAL_BOOL;
                    result.boolVal = left.intVal <= right.intVal; 
                    break;
                default: error("Invalid binary operator.", node->binary.op.line);
            }
            return result;
        }
        default:
            error("Invalid expression.", node->type == NODE_STMT_ASSIGN ? node->assign.name.line : 0);
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
    }
}

void initVM(VM* vm) {
    vm->stackTop = 0;
    vm->env = malloc(sizeof(Environment));
    if (!vm->env) error("Memory allocation failed.", 0);
    memset(vm->env->buckets, 0, sizeof(vm->env->buckets));
}

void freeVM(VM* vm) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        VarEntry* entry = vm->env->buckets[i];
        while (entry) {
            VarEntry* next = entry->next;
            free(entry->key);
            if (entry->value.type == VAL_STRING) free(entry->value.stringVal);
            free(entry);
            entry = next;
        }
    }
    free(vm->env);
}

void interpret(VM* vm, Node* ast) {
    execute(vm, ast);
}