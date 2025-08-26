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

// Find or insert function in specific environment
static FuncEntry* findFuncEntry(Environment* env, Token name, bool insert) {
    unsigned int h = hash(name.start, name.length);
    FuncEntry* entry = env->funcBuckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry;
        }
        entry = entry->next;
    }
    if (insert) {
        entry = malloc(sizeof(FuncEntry));
        if (!entry) {
            error("Memory allocation failed.", name.line);
        }
        entry->key = strndup(name.start, name.length);
        if (!entry->key) {
            free(entry);
            error("Memory allocation failed.", name.line);
        }
        entry->function = NULL;
        entry->next = env->funcBuckets[h];
        env->funcBuckets[h] = entry;
    }
    return entry;
}

// Find function in global environment (for function calls)
static Function* findFunction(VM* vm, Token name) {
    unsigned int h = hash(name.start, name.length);
    FuncEntry* entry = vm->globalEnv->funcBuckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry->function;
        }
        entry = entry->next;
    }
    return NULL;
}

// Forward declarations
static Value evaluate(VM* vm, Node* node);
static void execute(VM* vm, Node* node);

// Execute function call
static Value callFunction(VM* vm, Function* func, Value* args, int argCount) {
    if (vm->callStackTop >= CALL_STACK_MAX) {
        error("Call stack overflow.", 0);
    }
    
    // Create new environment for function execution
    Environment* funcEnv = malloc(sizeof(Environment));
    if (!funcEnv) error("Memory allocation failed.", 0);
    memset(funcEnv->buckets, 0, sizeof(funcEnv->buckets));
    memset(funcEnv->funcBuckets, 0, sizeof(funcEnv->funcBuckets));
    
    // Check parameter count
    if (argCount != func->paramCount) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), "Expected %d arguments but got %d.", func->paramCount, argCount);
        error(errorMsg, 0);
    }
    
    // Set up parameters in function environment
    for (int i = 0; i < argCount; i++) {
        VarEntry* paramEntry = malloc(sizeof(VarEntry));
        if (!paramEntry) error("Memory allocation failed.", 0);
        
        paramEntry->key = strndup(func->params[i].start, func->params[i].length);
        if (!paramEntry->key) {
            free(paramEntry);
            error("Memory allocation failed.", 0);
        }
        
        paramEntry->value = args[i];
        unsigned int hashVal = hash(paramEntry->key, strlen(paramEntry->key));
        paramEntry->next = funcEnv->buckets[hashVal];
        funcEnv->buckets[hashVal] = paramEntry;
    }
    
    // Push call frame
    CallFrame* frame = &vm->callStack[vm->callStackTop++];
    frame->env = vm->env;
    frame->hasReturned = false;
    frame->returnValue.type = VAL_INT;
    frame->returnValue.intVal = 0;
    
    // Switch to function environment
    Environment* oldEnv = vm->env;
    vm->env = funcEnv;
    
    // Execute function body
    execute(vm, func->body);
    
    // Get return value
    Value returnValue = frame->returnValue;
    
    // Pop call frame and restore environment
    vm->callStackTop--;
    vm->env = oldEnv;
    
    // Clean up function environment
    for (int i = 0; i < TABLE_SIZE; i++) {
        VarEntry* entry = funcEnv->buckets[i];
        while (entry) {
            VarEntry* next = entry->next;
            free(entry->key);
            if (entry->value.type == VAL_STRING && entry->value.stringVal) {
                free(entry->value.stringVal);
            }
            free(entry);
            entry = next;
        }
        
        // Function buckets should be empty in local function environment
        FuncEntry* funcEntry = funcEnv->funcBuckets[i];
        while (funcEntry) {
            FuncEntry* next = funcEntry->next;
            free(funcEntry->key);
            free(funcEntry);
            funcEntry = next;
        }
    }
    free(funcEnv);
    
    return returnValue;
}

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
                // Free old string value if exists
                if (entry->value.type == VAL_STRING && entry->value.stringVal) {
                    free(entry->value.stringVal);
                }
                entry->value = init;
            }
            break;
        }
        case NODE_STMT_ASSIGN: {
            Value value = evaluate(vm, node->assign.value);
            VarEntry* entry = findEntry(vm->env, node->assign.name, false);
            if (entry) {
                // Free old string value if exists
                if (entry->value.type == VAL_STRING && entry->value.stringVal) {
                    free(entry->value.stringVal);
                }
                entry->value = value;
            } else {
                error("Undefined variable.", node->assign.name.line);
            }
            break;
        }
        case NODE_STMT_PRINT: {
            Value value = evaluate(vm, node->print.expr);
            switch (value.type) {
                case VAL_INT: 
                    printf("%d\n", value.intVal); 
                    break;
                case VAL_FLOAT: 
                    printf("%.6g\n", value.floatVal); 
                    break;
                case VAL_STRING: 
                    printf("%s\n", value.stringVal ? value.stringVal : "(null)"); 
                    break;
                case VAL_BOOL: 
                    printf("%s\n", value.boolVal ? "true" : "false"); 
                    break;
            }
            break;
        }
        case NODE_STMT_IF: {
            Value cond = evaluate(vm, node->if_stmt.condition);
            bool isTrue = false;
            switch (cond.type) {
                case VAL_BOOL: 
                    isTrue = cond.boolVal; 
                    break;
                case VAL_INT: 
                    isTrue = cond.intVal != 0; 
                    break;
                case VAL_FLOAT: 
                    isTrue = cond.floatVal != 0.0; 
                    break;
                case VAL_STRING: 
                    isTrue = cond.stringVal != NULL && strlen(cond.stringVal) > 0; 
                    break;
            }
            
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
                bool isTrue = false;
                switch (cond.type) {
                    case VAL_BOOL: 
                        isTrue = cond.boolVal; 
                        break;
                    case VAL_INT: 
                        isTrue = cond.intVal != 0; 
                        break;
                    case VAL_FLOAT: 
                        isTrue = cond.floatVal != 0.0; 
                        break;
                    case VAL_STRING: 
                        isTrue = cond.stringVal != NULL && strlen(cond.stringVal) > 0; 
                        break;
                }
                if (!isTrue) break;
                execute(vm, node->while_stmt.body);
            }
            break;
        }
        case NODE_STMT_FOR: {
            if (node->for_stmt.initializer) {
                execute(vm, node->for_stmt.initializer);
            }
            
            while (true) {
                bool condTrue = true;
                if (node->for_stmt.condition) {
                    Value cond = evaluate(vm, node->for_stmt.condition);
                    switch (cond.type) {
                        case VAL_BOOL: 
                            condTrue = cond.boolVal; 
                            break;
                        case VAL_INT: 
                            condTrue = cond.intVal != 0; 
                            break;
                        case VAL_FLOAT: 
                            condTrue = cond.floatVal != 0.0; 
                            break;
                        case VAL_STRING: 
                            condTrue = cond.stringVal != NULL && strlen(cond.stringVal) > 0; 
                            break;
                    }
                }
                if (!condTrue) break;
                
                execute(vm, node->for_stmt.body);
                
                if (node->for_stmt.increment) {
                    execute(vm, node->for_stmt.increment);
                }
            }
            break;
        }
        case NODE_STMT_BLOCK: {
            for (int i = 0; i < node->block.count; i++) {
                execute(vm, node->block.statements[i]);
                
                // Check if we returned from within the block
                if (vm->callStackTop > 0 && vm->callStack[vm->callStackTop - 1].hasReturned) {
                    break;
                }
            }
            break;
        }
        case NODE_STMT_FUNCTION: {
            // Function definitions always go to global environment
            FuncEntry* entry = findFuncEntry(vm->globalEnv, node->function.name, true);
            if (entry && !entry->function) {
                Function* func = malloc(sizeof(Function));
                if (!func) error("Memory allocation failed.", node->function.name.line);
                
                func->name = node->function.name;
                func->params = node->function.params;
                func->paramCount = node->function.paramCount;
                func->body = node->function.body;
                func->closure = NULL; // We don't need closure for this implementation
                entry->function = func;
            } else {
                error("Function already defined.", node->function.name.line);
            }
            break;
        }
        case NODE_STMT_RETURN: {
            if (vm->callStackTop == 0) {
                error("Return statement outside function.", 0);
                break;
            }
            
            CallFrame* frame = &vm->callStack[vm->callStackTop - 1];
            frame->hasReturned = true;
            
            if (node->return_stmt.value) {
                frame->returnValue = evaluate(vm, node->return_stmt.value);
            } else {
                frame->returnValue.type = VAL_INT;
                frame->returnValue.intVal = 0;
            }
            break;
        }
        default:
            // Expression statement
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
                if (t.length >= 2) {
                    val.stringVal = strndup(t.start + 1, t.length - 2);
                } else {
                    val.stringVal = strdup("");
                }
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
            if (entry) {
                return entry->value;
            }
            error("Undefined variable.", node->var.name.line);
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
        }
        
        case NODE_EXPR_UNARY: {
            Value expr = evaluate(vm, node->unary.expr);
            if (node->unary.op.type == TOKEN_MINUS) {
                if (expr.type == VAL_INT) {
                    expr.intVal = -expr.intVal;
                } else if (expr.type == VAL_FLOAT) {
                    expr.floatVal = -expr.floatVal;
                } else {
                    error("Cannot negate non-numeric value.", node->unary.op.line);
                }
            }
            return expr;
        }
        
        case NODE_EXPR_BINARY: {
            Value left = evaluate(vm, node->binary.left);
            Value right = evaluate(vm, node->binary.right);
            Value result;
            
            // Handle string concatenation with +
            if (node->binary.op.type == TOKEN_PLUS && (left.type == VAL_STRING || right.type == VAL_STRING)) {
                char leftStr[256], rightStr[256];
                
                // Convert left operand to string
                switch (left.type) {
                    case VAL_INT: 
                        snprintf(leftStr, sizeof(leftStr), "%d", left.intVal); 
                        break;
                    case VAL_FLOAT: 
                        snprintf(leftStr, sizeof(leftStr), "%.6g", left.floatVal); 
                        break;
                    case VAL_BOOL: 
                        strcpy(leftStr, left.boolVal ? "true" : "false"); 
                        break;
                    case VAL_STRING: 
                        strncpy(leftStr, left.stringVal ? left.stringVal : "", sizeof(leftStr) - 1); 
                        leftStr[sizeof(leftStr) - 1] = '\0';
                        break;
                }
                
                // Convert right operand to string
                switch (right.type) {
                    case VAL_INT: 
                        snprintf(rightStr, sizeof(rightStr), "%d", right.intVal); 
                        break;
                    case VAL_FLOAT: 
                        snprintf(rightStr, sizeof(rightStr), "%.6g", right.floatVal); 
                        break;
                    case VAL_BOOL: 
                        strcpy(rightStr, right.boolVal ? "true" : "false"); 
                        break;
                    case VAL_STRING: 
                        strncpy(rightStr, right.stringVal ? right.stringVal : "", sizeof(rightStr) - 1); 
                        rightStr[sizeof(rightStr) - 1] = '\0';
                        break;
                }
                
                result.type = VAL_STRING;
                result.stringVal = malloc(strlen(leftStr) + strlen(rightStr) + 1);
                if (!result.stringVal) error("Memory allocation failed.", node->binary.op.line);
                strcpy(result.stringVal, leftStr);
                strcat(result.stringVal, rightStr);
                
                return result;
            }
            
            // Handle comparison operations for different types
            if (node->binary.op.type == TOKEN_EQUAL_EQUAL || node->binary.op.type == TOKEN_BANG_EQUAL) {
                result.type = VAL_BOOL;
                bool isEqual = false;
                
                // Same type comparisons
                if (left.type == right.type) {
                    switch (left.type) {
                        case VAL_INT:
                            isEqual = left.intVal == right.intVal;
                            break;
                        case VAL_FLOAT:
                            isEqual = left.floatVal == right.floatVal;
                            break;
                        case VAL_BOOL:
                            isEqual = left.boolVal == right.boolVal;
                            break;
                        case VAL_STRING:
                            if (left.stringVal && right.stringVal) {
                                isEqual = strcmp(left.stringVal, right.stringVal) == 0;
                            } else {
                                isEqual = (left.stringVal == NULL && right.stringVal == NULL);
                            }
                            break;
                    }
                }
                
                result.boolVal = (node->binary.op.type == TOKEN_EQUAL_EQUAL) ? isEqual : !isEqual;
                return result;
            }
            
            // Numeric operations
            if (left.type == VAL_INT && right.type == VAL_INT) {
                result.type = VAL_INT;
                switch (node->binary.op.type) {
                    case TOKEN_PLUS: 
                        result.intVal = left.intVal + right.intVal; 
                        break;
                    case TOKEN_MINUS: 
                        result.intVal = left.intVal - right.intVal; 
                        break;
                    case TOKEN_STAR: 
                        result.intVal = left.intVal * right.intVal; 
                        break;
                    case TOKEN_SLASH: 
                        if (right.intVal == 0) {
                            error("Division by zero.", node->binary.op.line);
                        }
                        result.intVal = left.intVal / right.intVal; 
                        break;
                    case TOKEN_PERCENT: 
                        if (right.intVal == 0) {
                            error("Modulo by zero.", node->binary.op.line);
                        }
                        result.intVal = left.intVal % right.intVal; 
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
                    default: 
                        error("Invalid binary operator for integers.", node->binary.op.line);
                }
            } else if (left.type == VAL_FLOAT && right.type == VAL_FLOAT) {
                // Handle float operations
                switch (node->binary.op.type) {
                    case TOKEN_PLUS: 
                        result.type = VAL_FLOAT;
                        result.floatVal = left.floatVal + right.floatVal; 
                        break;
                    case TOKEN_MINUS: 
                        result.type = VAL_FLOAT;
                        result.floatVal = left.floatVal - right.floatVal; 
                        break;
                    case TOKEN_STAR: 
                        result.type = VAL_FLOAT;
                        result.floatVal = left.floatVal * right.floatVal; 
                        break;
                    case TOKEN_SLASH: 
                        if (right.floatVal == 0.0) {
                            error("Division by zero.", node->binary.op.line);
                        }
                        result.type = VAL_FLOAT;
                        result.floatVal = left.floatVal / right.floatVal; 
                        break;
                    case TOKEN_GREATER: 
                        result.type = VAL_BOOL;
                        result.boolVal = left.floatVal > right.floatVal; 
                        break;
                    case TOKEN_GREATER_EQUAL: 
                        result.type = VAL_BOOL;
                        result.boolVal = left.floatVal >= right.floatVal; 
                        break;
                    case TOKEN_LESS: 
                        result.type = VAL_BOOL;
                        result.boolVal = left.floatVal < right.floatVal; 
                        break;
                    case TOKEN_LESS_EQUAL: 
                        result.type = VAL_BOOL;
                        result.boolVal = left.floatVal <= right.floatVal; 
                        break;
                    default: 
                        error("Invalid binary operator for floats.", node->binary.op.line);
                }
            } else if ((left.type == VAL_INT && right.type == VAL_FLOAT) || 
                       (left.type == VAL_FLOAT && right.type == VAL_INT)) {
                // Mixed int/float operations - convert to float
                double leftVal = (left.type == VAL_INT) ? (double)left.intVal : left.floatVal;
                double rightVal = (right.type == VAL_INT) ? (double)right.intVal : right.floatVal;
                
                switch (node->binary.op.type) {
                    case TOKEN_PLUS: 
                        result.type = VAL_FLOAT;
                        result.floatVal = leftVal + rightVal; 
                        break;
                    case TOKEN_MINUS: 
                        result.type = VAL_FLOAT;
                        result.floatVal = leftVal - rightVal; 
                        break;
                    case TOKEN_STAR: 
                        result.type = VAL_FLOAT;
                        result.floatVal = leftVal * rightVal; 
                        break;
                    case TOKEN_SLASH: 
                        if (rightVal == 0.0) {
                            error("Division by zero.", node->binary.op.line);
                        }
                        result.type = VAL_FLOAT;
                        result.floatVal = leftVal / rightVal; 
                        break;
                    case TOKEN_GREATER: 
                        result.type = VAL_BOOL;
                        result.boolVal = leftVal > rightVal; 
                        break;
                    case TOKEN_GREATER_EQUAL: 
                        result.type = VAL_BOOL;
                        result.boolVal = leftVal >= rightVal; 
                        break;
                    case TOKEN_LESS: 
                        result.type = VAL_BOOL;
                        result.boolVal = leftVal < rightVal; 
                        break;
                    case TOKEN_LESS_EQUAL: 
                        result.type = VAL_BOOL;
                        result.boolVal = leftVal <= rightVal; 
                        break;
                    default: 
                        error("Invalid binary operator for mixed numeric types.", node->binary.op.line);
                }
            } else {
                error("Type mismatch in binary operation.", node->binary.op.line);
            }
            
            return result;
        }
        
        case NODE_EXPR_CALL: {
            Function* func = findFunction(vm, node->call.name);
            if (!func) {
                error("Undefined function.", node->call.name.line);
                Value nullVal = {VAL_INT, .intVal = 0};
                return nullVal;
            }
            
            // Evaluate arguments
            Value args[16]; // Max 16 arguments
            int argCount = 0;
            Node* arg = node->call.arguments;
            while (arg && argCount < 16) {
                args[argCount++] = evaluate(vm, arg);
                arg = arg->next;
            }
            
            if (argCount >= 16 && arg) {
                error("Too many arguments (max 16).", node->call.name.line);
            }
            
            return callFunction(vm, func, args, argCount);
        }
        
        default:
            error("Invalid expression type.", 0);
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
    }
}

// Initialize VM
void initVM(VM* vm) {
    vm->stackTop = 0;
    vm->callStackTop = 0;
    
    // Create global environment
    vm->globalEnv = malloc(sizeof(Environment));
    if (!vm->globalEnv) error("Memory allocation failed.", 0);
    memset(vm->globalEnv->buckets, 0, sizeof(vm->globalEnv->buckets));
    memset(vm->globalEnv->funcBuckets, 0, sizeof(vm->globalEnv->funcBuckets));
    
    // Set current environment to global initially
    vm->env = vm->globalEnv;
}

// Free VM memory
void freeVM(VM* vm) {
    // Free variables from global environment
    for (int i = 0; i < TABLE_SIZE; i++) {
        VarEntry* entry = vm->globalEnv->buckets[i];
        while (entry) {
            VarEntry* next = entry->next;
            free(entry->key);
            if (entry->value.type == VAL_STRING && entry->value.stringVal) {
                free(entry->value.stringVal);
            }
            free(entry);
            entry = next;
        }
    }
    
    // Free functions from global environment
    for (int i = 0; i < TABLE_SIZE; i++) {
        FuncEntry* entry = vm->globalEnv->funcBuckets[i];
        while (entry) {
            FuncEntry* next = entry->next;
            free(entry->key);
            if (entry->function) {
                // Don't free params as they belong to AST
                free(entry->function);
            }
            free(entry);
            entry = next;
        }
    }
    
    free(vm->globalEnv);
}

// Interpret AST
void interpret(VM* vm, Node* ast) {
    execute(vm, ast);
}