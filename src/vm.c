#include "vm.h"
#include "lexer.h"
#include "parser.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

// Hash function for variables
static unsigned int hash(const char* key, int length) {
    unsigned int hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (unsigned char)key[i];
        hash *= 16777619;
    }
    return hash % TABLE_SIZE;
}

// Module cache lookup/insert by logical module name
static ModuleEntry* findModuleEntry(VM* vm, const char* name, int length, bool insert) {
    unsigned int h = hash(name, length);
    ModuleEntry* e = vm->moduleBuckets[h];
    while (e) {
        if (strncmp(e->key, name, length) == 0 && strlen(e->key) == (size_t)length) {
            return e;
        }
        e = e->next;
    }
    if (!insert) return NULL;
    e = (ModuleEntry*)malloc(sizeof(ModuleEntry));
    if (!e) error("Memory allocation failed.", 0);
    e->key = strndup(name, length);
    if (!e->key) error("Memory allocation failed.", 0);
    e->module = NULL;
    e->next = vm->moduleBuckets[h];
    vm->moduleBuckets[h] = e;
    return e;
}

// Check if regular file exists
static bool fileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

// Find or insert variable with proper scope resolution
static VarEntry* findEntry(VM* vm, Token name, bool insert) {
    unsigned int h = hash(name.start, name.length);
    
    // First, search in current environment
    VarEntry* entry = vm->env->buckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry;
        }
        entry = entry->next;
    }
    
    // Next, if reading (not inserting), try definition environment (e.g., module env of the function)
    if (!insert && vm->defEnv && vm->defEnv != vm->env) {
        entry = vm->defEnv->buckets[h];
        while (entry) {
            if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
                return entry;
            }
            entry = entry->next;
        }
    }
    
    // If not found and not inserting, search in global environment (if different from current)
    if (!insert && vm->env != vm->globalEnv) {
        entry = vm->globalEnv->buckets[h];
        while (entry) {
            if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
                return entry;
            }
            entry = entry->next;
        }
    }
    
    // If inserting, create new entry in current environment
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
        entry->next = vm->env->buckets[h];
        vm->env->buckets[h] = entry;
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

static Function* findFunctionInEnv(Environment* env, Token name) {
    unsigned int h = hash(name.start, name.length);
    FuncEntry* entry = env->funcBuckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry->function;
        }
        entry = entry->next;
    }
    return NULL;
}

// Find variable in a specific environment
static VarEntry* findVarInEnv(Environment* env, Token name) {
    unsigned int h = hash(name.start, name.length);
    VarEntry* entry = env->buckets[h];
    while (entry) {
        if (strncmp(entry->key, name.start, name.length) == 0 && strlen(entry->key) == (size_t)name.length) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

// Read whole file
static char* readFileAll(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) return NULL;
    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buf = malloc(size + 1);
    if (!buf) { fclose(file); return NULL; }
    size_t n = fread(buf, 1, size, file);
    buf[n] = '\0';
    fclose(file);
    return buf;
}

// Recursively search for a filename under root. Returns malloc'd full path or NULL
static char* searchFileRecursive(const char* root, const char* filename) {
    DIR* dir = opendir(root);
    if (!dir) return NULL;
    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        // Skip common build dirs
        if (strcmp(ent->d_name, ".git") == 0 || strcmp(ent->d_name, "bin") == 0 || strcmp(ent->d_name, "obj") == 0) continue;
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", root, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            char* found = searchFileRecursive(path, filename);
            if (found) { closedir(dir); return found; }
        } else if (S_ISREG(st.st_mode)) {
            if (strcmp(ent->d_name, filename) == 0) {
                char* full = strdup(path);
                closedir(dir);
                return full;
            }
        }
    }
    closedir(dir);
    return NULL;
}

// Convert 1-char string to int code if applicable
static bool tryCharCode(Value v, int* out) {
    if (v.type == VAL_STRING && v.stringVal && strlen(v.stringVal) == 1) {
        *out = (unsigned char)v.stringVal[0];
        return true;
    }
    return false;
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
    
    // Switch to function environment and set definition env to function's closure
    Environment* oldEnv = vm->env;
    Environment* oldDef = vm->defEnv;
    vm->env = funcEnv;
    if (func->closure) vm->defEnv = func->closure;
    
    // Execute function body
    execute(vm, func->body);
    
    // Get return value
    Value returnValue = frame->returnValue;
    
    // Pop call frame and restore environments
    vm->callStackTop--;
    vm->env = oldEnv;
    vm->defEnv = oldDef;
    
    // Clean up function environment
    for (int i = 0; i < TABLE_SIZE; i++) {
        VarEntry* entry = funcEnv->buckets[i];
        while (entry) {
            VarEntry* next = entry->next;
            free(entry->key);
            // Do NOT free entry->value.stringVal here.
            // String values passed as arguments or assigned to locals may alias
            // memory owned by outer scopes; freeing here can cause double-free
            // or use-after-free when returning strings.
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
            VarEntry* entry = findEntry(vm, node->var_decl.name, true);
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
            VarEntry* entry = findEntry(vm, node->assign.name, false);
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
                case VAL_MODULE:
                    printf("[module %s]\n", (value.moduleVal && value.moduleVal->name) ? value.moduleVal->name : "<anon>");
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
                case VAL_MODULE:
                    isTrue = true; // treat as truthy
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
                    case VAL_MODULE:
                        isTrue = true;
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
                        case VAL_MODULE:
                            condTrue = true;
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
            // Register function in current definition environment (global or module)
            Environment* target = vm->defEnv ? vm->defEnv : vm->globalEnv;
            FuncEntry* entry = findFuncEntry(target, node->function.name, true);
            if (entry && !entry->function) {
                Function* func = malloc(sizeof(Function));
                if (!func) error("Memory allocation failed.", node->function.name.line);
                
                func->name = node->function.name;
                func->params = node->function.params;
                func->paramCount = node->function.paramCount;
                func->body = node->function.body;
                // Capture the environment where the function is defined (for module/global lookup)
                func->closure = target;
                entry->function = func;
            } else {
                error("Function already defined.", node->function.name.line);
            }
            break;
        }
        case NODE_STMT_IMPORT: {
            // Build filename <module>.gemini and resolve via cache, GEMINI_PATH, then projectRoot
            char modName[256];
            snprintf(modName, sizeof(modName), "%.*s.gemini", node->import_stmt.module.length, node->import_stmt.module.start);
            // Cache check by logical module name (not alias)
            ModuleEntry* mentry = findModuleEntry(vm, node->import_stmt.module.start, node->import_stmt.module.length, false);
            if (mentry && mentry->module) {
                VarEntry* aliasEntry = findEntry(vm, node->import_stmt.alias, true);
                aliasEntry->value.type = VAL_MODULE;
                aliasEntry->value.moduleVal = mentry->module;
                break;
            }

            char* fullPath = NULL;
            char candidate[2048];
            // Try GEMINI_PATH first for speed and explicitness
            const char* gp = getenv("GEMINI_PATH");
            if (gp && *gp) {
                const char* p = gp;
                while (*p) {
                    char dirbuf[1024];
                    int di = 0;
                    while (*p && *p != ':' && di < (int)sizeof(dirbuf) - 1) {
                        dirbuf[di++] = *p++;
                    }
                    dirbuf[di] = '\0';
                    if (*p == ':') p++;
                    if (di == 0) continue;
                    snprintf(candidate, sizeof(candidate), "%s/%s", dirbuf, modName);
                    if (fileExists(candidate)) { fullPath = strdup(candidate); break; }
                }
            }
            // Fallback: search under projectRoot
            if (!fullPath) {
                fullPath = searchFileRecursive(vm->projectRoot, modName);
            }
            if (!fullPath) {
                error("Module file not found in project.", node->import_stmt.module.line);
            }
            char* source = readFileAll(fullPath);
            if (!source) {
                free(fullPath);
                error("Failed to read module file.", node->import_stmt.module.line);
            }

            // Prepare module environment
            Environment* moduleEnv = malloc(sizeof(Environment));
            if (!moduleEnv) error("Memory allocation failed.", node->import_stmt.module.line);
            memset(moduleEnv->buckets, 0, sizeof(moduleEnv->buckets));
            memset(moduleEnv->funcBuckets, 0, sizeof(moduleEnv->funcBuckets));

            // Parse and execute module in its env
            Lexer lx; initLexer(&lx, source);
            Parser ps; initParser(&ps);
            while (1) {
                Token tk = scanToken(&lx);
                addToken(&ps, tk);
                if (tk.type == TOKEN_EOF) break;
            }
            Node* ast = parse(&ps);

            // Save current env/defEnv
            Environment* oldEnv = vm->env;
            Environment* oldDef = vm->defEnv;
            vm->env = moduleEnv;
            vm->defEnv = moduleEnv;
            execute(vm, ast);
            // Restore
            vm->env = oldEnv;
            vm->defEnv = oldDef;

            // Wrap module value
            Module* module = malloc(sizeof(Module));
            if (!module) error("Memory allocation failed.", node->import_stmt.module.line);
            module->name = strndup(node->import_stmt.alias.start, node->import_stmt.alias.length);
            module->env = moduleEnv;
            module->source = source; // keep source alive for token/text lifetime

            VarEntry* aliasEntry = findEntry(vm, node->import_stmt.alias, true);
            aliasEntry->value.type = VAL_MODULE;
            aliasEntry->value.moduleVal = module;

            // Store in cache by logical module name (not alias)
            ModuleEntry* store = findModuleEntry(vm, node->import_stmt.module.start, node->import_stmt.module.length, true);
            store->module = module;

            // Cleanup parser/ast (keep moduleEnv and source for module lifetime)
            // Free AST and parser tokens
            // We don't have freeAST here; import inside VM uses execute recursively, assume main frees AST of top-level only.
            // Minimal cleanup for module parsing structures:
            freeParser(&ps);
            free(fullPath);
            // Note: ast nodes are part of module functions' bodies, not executed again; freeing here would invalidate. So we intentionally leak small AST of module or could store for module lifetime.
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
            VarEntry* entry = findEntry(vm, node->var.name, false);
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
                    case VAL_MODULE:
                        strncpy(leftStr, "[module]", sizeof(leftStr) - 1);
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
                    case VAL_MODULE:
                        strncpy(rightStr, "[module]", sizeof(rightStr) - 1);
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
                        case VAL_MODULE:
                            // Compare by module identity (pointer equality)
                            isEqual = left.moduleVal == right.moduleVal;
                            break;
                    }
                }
                
                result.boolVal = (node->binary.op.type == TOKEN_EQUAL_EQUAL) ? isEqual : !isEqual;
                return result;
            }
            
            // Numeric operations
            // Coerce 1-char strings to ints for arithmetic if needed
            if ((left.type == VAL_STRING && right.type != VAL_STRING) || (right.type == VAL_STRING && left.type != VAL_STRING)) {
                int code;
                if (left.type == VAL_STRING && tryCharCode(left, &code)) { left.type = VAL_INT; left.intVal = code; }
                if (right.type == VAL_STRING && tryCharCode(right, &code)) { right.type = VAL_INT; right.intVal = code; }
            } else if (left.type == VAL_STRING && right.type == VAL_STRING) {
                // If both are strings, try to coerce both when operator is not string concatenation
                int lc, rc;
                if (tryCharCode(left, &lc) && tryCharCode(right, &rc)) {
                    left.type = VAL_INT; left.intVal = lc;
                    right.type = VAL_INT; right.intVal = rc;
                }
            }

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
            // Resolve function from callee expression
            Function* func = NULL;
            int errLine = 0;
            if (node->call.callee->type == NODE_EXPR_VAR) {
                // Try global functions first
                func = findFunction(vm, node->call.callee->var.name);
                // Then try functions in current definition environment (e.g., current module)
                if (!func && vm->defEnv) {
                    func = findFunctionInEnv(vm->defEnv, node->call.callee->var.name);
                }
                errLine = node->call.callee->var.name.line;
            } else if (node->call.callee->type == NODE_EXPR_GET) {
                Value obj = evaluate(vm, node->call.callee->get.object);
                if (obj.type == VAL_MODULE) {
                    func = findFunctionInEnv(obj.moduleVal->env, node->call.callee->get.name);
                    errLine = node->call.callee->get.name.line;
                } else {
                    error("Only modules support method calls.", node->call.callee->get.name.line);
                }
            } else {
                error("Invalid call target.", 0);
            }
            if (!func) {
                error("Undefined function.", errLine);
                Value nullVal = {VAL_INT, .intVal = 0};
                return nullVal;
            }

            // Evaluate arguments
            Value args[16];
            int argCount = 0;
            Node* arg = node->call.arguments;
            while (arg && argCount < 16) {
                args[argCount++] = evaluate(vm, arg);
                arg = arg->next;
            }
            if (argCount >= 16 && arg) {
                error("Too many arguments (max 16).", errLine);
            }
            return callFunction(vm, func, args, argCount);
        }
        case NODE_EXPR_GET: {
            Value obj = evaluate(vm, node->get.object);
            // String property: length
            if (obj.type == VAL_STRING) {
                if (strncmp(node->get.name.start, "length", node->get.name.length) == 0 && strlen("length") == (size_t)node->get.name.length) {
                    Value v; v.type = VAL_INT; v.intVal = obj.stringVal ? (int)strlen(obj.stringVal) : 0; return v;
                }
                error("Unknown string property.", node->get.name.line);
            } else if (obj.type == VAL_MODULE) {
                // module constant/variable or function name as value isn't supported; only variables returned.
                VarEntry* ve = findVarInEnv(obj.moduleVal->env, node->get.name);
                if (ve) return ve->value;
                // If not a variable, allow chained call to resolve function. Here return int 0 to keep flow if used wrongly.
                error("Unknown module member.", node->get.name.line);
            } else {
                error("Property access not supported on this type.", node->get.name.line);
            }
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
        }
        case NODE_EXPR_INDEX: {
            Value target = evaluate(vm, node->index.target);
            Value idx = evaluate(vm, node->index.index);
            if (target.type == VAL_STRING && idx.type == VAL_INT) {
                int len = target.stringVal ? (int)strlen(target.stringVal) : 0;
                if (idx.intVal < 0 || idx.intVal >= len) {
                    error("String index out of range.", 0);
                }
                char* s = malloc(2);
                if (!s) error("Memory allocation failed.", 0);
                s[0] = target.stringVal[idx.intVal];
                s[1] = '\0';
                Value v; v.type = VAL_STRING; v.stringVal = s; return v;
            }
            error("Indexing not supported for this type.", 0);
            Value nullVal = {VAL_INT, .intVal = 0};
            return nullVal;
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
    memset(vm->moduleBuckets, 0, sizeof(vm->moduleBuckets));
    
    // Set current environment to global initially
    vm->env = vm->globalEnv;
    vm->defEnv = vm->globalEnv;
    // Set project root from current working directory
    if (!getcwd(vm->projectRoot, sizeof(vm->projectRoot))) {
        strcpy(vm->projectRoot, ".");
    }
}

// Free VM memory
void freeVM(VM* vm) {
    // TODO: Re-enable structured cleanup after memory ownership is clarified.
    // Temporarily disabled to avoid post-run crash while validating modularity feature.
    (void)vm;
}

// Interpret AST
void interpret(VM* vm, Node* ast) {
    execute(vm, ast);
}