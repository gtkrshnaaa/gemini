#include "parser.h"

// Helper to advance parser
static Token advance(Parser* parser) {
    if (parser->current < parser->count) {
        return parser->tokens[parser->current++];
    }
    return (Token){TOKEN_EOF, NULL, 0, 0};
}

// Check current token type
static bool check(Parser* parser, TokenType type) {
    if (parser->current >= parser->count) return false;
    return parser->tokens[parser->current].type == type;
}

// Match and advance if type
static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

// Consume token or error
static Token consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) return advance(parser);
    error(message, parser->tokens[parser->current].line);
    return (Token){TOKEN_EOF, NULL, 0, 0};
}

// Forward declarations for recursive parsing
static Node* expression(Parser* parser);
static Node* statement(Parser* parser);
static Node* declaration(Parser* parser);

// Parse primary (literals, vars, groups, function calls)
static Node* primary(Parser* parser) {
    if (match(parser, TOKEN_NUMBER) || match(parser, TOKEN_STRING)) {
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_LITERAL;
        node->literal.token = parser->tokens[parser->current - 1];
        return node;
    }
    if (match(parser, TOKEN_IDENTIFIER)) {
        // Check if this is a function call
        if (check(parser, TOKEN_LEFT_PAREN)) {
            // Function call
            Token name = parser->tokens[parser->current - 1];
            advance(parser); // Consume '('
            
            Node* node = malloc(sizeof(Node));
            node->type = NODE_EXPR_CALL;
            node->call.name = name;
            node->call.arguments = NULL;
            node->call.argumentCount = 0;
            
            // Parse arguments
            if (!check(parser, TOKEN_RIGHT_PAREN)) {
                Node** currentArg = &node->call.arguments;
                do {
                    *currentArg = expression(parser);
                    node->call.argumentCount++;
                    currentArg = &(*currentArg)->next;
                } while (match(parser, TOKEN_COMMA));
            }
            
            consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
            return node;
        } else {
            // Variable reference
            Node* node = malloc(sizeof(Node));
            node->type = NODE_EXPR_VAR;
            node->var.name = parser->tokens[parser->current - 1];
            return node;
        }
    }
    if (match(parser, TOKEN_LEFT_PAREN)) {
        Node* expr = expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return expr;
    }
    error("Expect expression.", parser->tokens[parser->current].line);
    return NULL;
}

// Unary
static Node* unary(Parser* parser) {
    if (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS)) {
        Token op = parser->tokens[parser->current - 1];
        Node* expr = unary(parser);
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_UNARY;
        node->unary.op = op;
        node->unary.expr = expr;
        return node;
    }
    return primary(parser);
}

// Factor (* / %)
static Node* factor(Parser* parser) {
    Node* expr = unary(parser);
    while (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH) || match(parser, TOKEN_PERCENT)) {
        Token op = parser->tokens[parser->current - 1];
        Node* right = unary(parser);
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_BINARY;
        node->binary.left = expr;
        node->binary.op = op;
        node->binary.right = right;
        expr = node;
    }
    return expr;
}

// Term (+ -)
static Node* term(Parser* parser) {
    Node* expr = factor(parser);
    while (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS)) {
        Token op = parser->tokens[parser->current - 1];
        Node* right = factor(parser);
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_BINARY;
        node->binary.left = expr;
        node->binary.op = op;
        node->binary.right = right;
        expr = node;
    }
    return expr;
}

// Comparison
static Node* comparison(Parser* parser) {
    Node* expr = term(parser);
    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        Token op = parser->tokens[parser->current - 1];
        Node* right = term(parser);
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_BINARY;
        node->binary.left = expr;
        node->binary.op = op;
        node->binary.right = right;
        expr = node;
    }
    return expr;
}

// Equality
static Node* equality(Parser* parser) {
    Node* expr = comparison(parser);
    while (match(parser, TOKEN_EQUAL_EQUAL) || match(parser, TOKEN_BANG_EQUAL)) {
        Token op = parser->tokens[parser->current - 1];
        Node* right = comparison(parser);
        Node* node = malloc(sizeof(Node));
        node->type = NODE_EXPR_BINARY;
        node->binary.left = expr;
        node->binary.op = op;
        node->binary.right = right;
        expr = node;
    }
    return expr;
}

// Assignment (for var = expr)
static Node* assignment(Parser* parser) {
    Node* expr = equality(parser);
    if (match(parser, TOKEN_EQUAL)) {
        Token equals = parser->tokens[parser->current - 1];
        Node* value = assignment(parser); // Right-assoc
        if (expr->type == NODE_EXPR_VAR) {
            Node* node = malloc(sizeof(Node));
            node->type = NODE_STMT_ASSIGN;
            node->assign.name = expr->var.name;
            node->assign.value = value;
            return node;
        }
        error("Invalid assignment target.", equals.line);
    }
    return expr;
}

// Expression (top level)
static Node* expression(Parser* parser) {
    return assignment(parser);
}

// Block { ... }
static Node* block(Parser* parser) {
    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_BLOCK;
    node->block.statements = malloc(8 * sizeof(Node*));
    node->block.count = 0;
    node->block.capacity = 8;

    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        Node* decl = declaration(parser);
        if (node->block.count == node->block.capacity) {
            node->block.capacity *= 2;
            node->block.statements = realloc(node->block.statements, node->block.capacity * sizeof(Node*));
        }
        node->block.statements[node->block.count++] = decl;
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    return node;
}

// Function declaration
static Node* function(Parser* parser) {
    Token name = consume(parser, TOKEN_IDENTIFIER, "Expect function name.");
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_FUNCTION;
    node->function.name = name;
    node->function.params = malloc(8 * sizeof(Token));
    node->function.paramCount = 0;
    int capacity = 8;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (node->function.paramCount == capacity) {
                capacity *= 2;
                node->function.params = realloc(node->function.params, capacity * sizeof(Token));
            }
            node->function.params[node->function.paramCount++] = consume(parser, TOKEN_IDENTIFIER, "Expect parameter name.");
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    node->function.body = block(parser);
    return node;
}

// Var declaration
static Node* varDeclaration(Parser* parser) {
    Token name = consume(parser, TOKEN_IDENTIFIER, "Expect variable name.");

    Node* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_VAR_DECL;
    node->var_decl.name = name;
    node->var_decl.initializer = initializer;
    return node;
}

// Print statement
static Node* printStatement(Parser* parser) {
    Node* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after print value.");

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_PRINT;
    node->print.expr = expr;
    return node;
}

// Return statement
static Node* returnStatement(Parser* parser) {
    Node* value = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        value = expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after return value.");

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_RETURN;
    node->return_stmt.value = value;
    return node;
}

// If statement
static Node* ifStatement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    Node* condition = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

    Node* thenBranch = statement(parser);
    Node* elseBranch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        elseBranch = statement(parser);
    }

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_IF;
    node->if_stmt.condition = condition;
    node->if_stmt.thenBranch = thenBranch;
    node->if_stmt.elseBranch = elseBranch;
    return node;
}

// While statement
static Node* whileStatement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    Node* condition = expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    Node* body = statement(parser);

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_WHILE;
    node->while_stmt.condition = condition;
    node->while_stmt.body = body;
    return node;
}

// For statement
static Node* forStatement(Parser* parser) {
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    
    Node* initializer = NULL;
    if (match(parser, TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(parser, TOKEN_VAR)) {
        initializer = varDeclaration(parser);
    } else {
        initializer = expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop start.");
    }

    Node* condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    Node* increment = NULL;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        increment = expression(parser);
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    Node* body = statement(parser);

    Node* node = malloc(sizeof(Node));
    node->type = NODE_STMT_FOR;
    node->for_stmt.initializer = initializer;
    node->for_stmt.condition = condition;
    node->for_stmt.increment = increment;
    node->for_stmt.body = body;
    return node;
}

// Statement
static Node* statement(Parser* parser) {
    if (match(parser, TOKEN_PRINT)) return printStatement(parser);
    if (match(parser, TOKEN_IF)) return ifStatement(parser);
    if (match(parser, TOKEN_WHILE)) return whileStatement(parser);
    if (match(parser, TOKEN_FOR)) return forStatement(parser);
    if (match(parser, TOKEN_RETURN)) return returnStatement(parser);
    if (match(parser, TOKEN_LEFT_BRACE)) return block(parser);

    Node* exprStmt = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    return exprStmt;
}

// Declaration (var or stmt or function)
static Node* declaration(Parser* parser) {
    if (match(parser, TOKEN_VAR)) return varDeclaration(parser);
    if (match(parser, TOKEN_FUNCTION)) return function(parser);
    return statement(parser);
}

// Main parse function
Node* parse(Parser* parser) {
    // Parse top-level declarations into a block
    Node* root = malloc(sizeof(Node));
    root->type = NODE_STMT_BLOCK;
    root->block.statements = malloc(8 * sizeof(Node*));
    root->block.count = 0;
    root->block.capacity = 8;

    while (!match(parser, TOKEN_EOF)) {
        Node* decl = declaration(parser);
        if (root->block.count == root->block.capacity) {
            root->block.capacity *= 2;
            root->block.statements = realloc(root->block.statements, root->block.capacity * sizeof(Node*));
        }
        root->block.statements[root->block.count++] = decl;
    }

    return root;
}