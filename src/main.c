#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"

// Error function
void error(const char* message, int line) {
    fprintf(stderr, "[line %d] Error: %s\n", line, message);
    exit(1);
}

// Read file
static char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(1);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = malloc(fileSize + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        free(buffer);
        fclose(file);
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(1);
    }

    buffer[bytesRead] = '\0';
    fclose(file);
    return buffer;
}

// Free AST
static void freeAST(Node* node) {
    if (!node) return;
    switch (node->type) {
        case NODE_EXPR_LITERAL:
            if (node->literal.token.type == TOKEN_STRING) {
                // No need to free token.start, it's from source
            }
            break;
        case NODE_EXPR_BINARY:
            freeAST(node->binary.left);
            freeAST(node->binary.right);
            break;
        case NODE_EXPR_UNARY:
            freeAST(node->unary.expr);
            break;
        case NODE_EXPR_VAR:
            break;
        case NODE_STMT_VAR_DECL:
            freeAST(node->var_decl.initializer);
            break;
        case NODE_STMT_ASSIGN:
            freeAST(node->assign.value);
            break;
        case NODE_STMT_PRINT:
            freeAST(node->print.expr);
            break;
        case NODE_STMT_IF:
            freeAST(node->if_stmt.condition);
            freeAST(node->if_stmt.thenBranch);
            freeAST(node->if_stmt.elseBranch);
            break;
        case NODE_STMT_WHILE:
            freeAST(node->while_stmt.condition);
            freeAST(node->while_stmt.body);
            break;
        case NODE_STMT_FOR:
            freeAST(node->for_stmt.initializer);
            freeAST(node->for_stmt.condition);
            freeAST(node->for_stmt.increment);
            freeAST(node->for_stmt.body);
            break;
        case NODE_STMT_BLOCK:
            for (int i = 0; i < node->block.count; i++) {
                freeAST(node->block.statements[i]);
            }
            free(node->block.statements);
            break;
        case NODE_STMT_FUNCTION:
            freeAST(node->function.body);
            free(node->function.params);
            break;
        case NODE_STMT_RETURN:
            freeAST(node->return_stmt.value);
            break;
    }
    free(node);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.gemini>\n", argv[0]);
        return 1;
    }

    char* source = readFile(argv[1]);

    // Lexer
    Lexer lexer;
    initLexer(&lexer, source);

    // Tokenize
    Parser parser;
    parser.current = 0;
    parser.count = 0;
    while (true) {
        Token token = scanToken(&lexer);
        parser.tokens[parser.count++] = token;
        if (token.type == TOKEN_EOF) break;
        if (parser.count >= MAX_TOKENS) error("Too many tokens.", 0);
    }

    // Parse
    Node* ast = parse(&parser);

    // VM
    VM vm;
    initVM(&vm);
    interpret(&vm, ast);

    // Cleanup
    freeAST(ast);
    freeVM(&vm);
    free(source);

    return 0;
}