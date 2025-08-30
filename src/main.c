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

// Initialize parser
void initParser(Parser* parser) {
    parser->tokens = malloc(64 * sizeof(Token));  // Start with 64 tokens
    if (!parser->tokens) {
        error("Memory allocation failed.", 0);
    }
    parser->current = 0;
    parser->count = 0;
    parser->capacity = 64;
}

// Free parser resources
void freeParser(Parser* parser) {
    if (parser->tokens) {
        free(parser->tokens);
        parser->tokens = NULL;
    }
}

// Add token to parser (grows array as needed)
void addToken(Parser* parser, Token token) {
    // Grow array if needed
    if (parser->count >= parser->capacity) {
        parser->capacity *= 2;
        parser->tokens = realloc(parser->tokens, parser->capacity * sizeof(Token));
        if (!parser->tokens) {
            error("Memory allocation failed.", token.line);
        }
    }
    
    parser->tokens[parser->count++] = token;
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
        case NODE_EXPR_GET:
            // object.property -> free object
            freeAST(node->get.object);
            break;
        case NODE_EXPR_INDEX:
            // target[index] -> free both
            freeAST(node->index.target);
            freeAST(node->index.index);
            break;
        case NODE_EXPR_CALL:
            freeAST(node->call.arguments);
            break;
        case NODE_STMT_VAR_DECL:
            freeAST(node->var_decl.initializer);
            break;
        case NODE_STMT_ASSIGN:
            freeAST(node->assign.value);
            break;
        case NODE_STMT_INDEX_ASSIGN:
            // target[index] = value
            freeAST(node->index_assign.target);
            freeAST(node->index_assign.index);
            freeAST(node->index_assign.value);
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
        case NODE_STMT_IMPORT:
            // tokens only; nothing to free
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

    // Initialize parser with dynamic array
    Parser parser;
    initParser(&parser);
    
    // Tokenize - no more token limit!
    while (true) {
        Token token = scanToken(&lexer);
        addToken(&parser, token);
        if (token.type == TOKEN_EOF) break;
    }

    printf("Tokenized %d tokens successfully.\n", parser.count);

    // Parse
    Node* ast = parse(&parser);

    // VM
    VM vm;
    initVM(&vm);
    interpret(&vm, ast);

    // Cleanup
    freeAST(ast);
    freeParser(&parser);
    freeVM(&vm);
    free(source);

    return 0;
}