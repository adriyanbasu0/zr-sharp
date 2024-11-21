#include "compiler.h"
#include <stdlib.h>
#include <string.h>

// Advance to the next token
static void advance_token(Parser* parser) {
    parser->current_token = get_next_token(parser->lexer);
}

// Initialize parser with a lexer
Parser* init_parser(Lexer* lexer) {
    Parser* parser = (Parser*)malloc(sizeof(Parser));
    parser->lexer = lexer;
    advance_token(parser);  // Load first token
    return parser;
}

// Create a new AST node
static ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    if (node == NULL) return NULL;
    
    node->type = type;
    node->value = NULL;
    node->left = NULL;
    node->right = NULL;
    node->statements = NULL;
    node->statements_count = 0;
    
    return node;
}

// Parse an identifier
static ASTNode* parse_identifier(Parser* parser) {
    ASTNode* node = create_node(NODE_IDENTIFIER);
    node->value = strdup(parser->current_token.value);
    advance_token(parser);
    return node;
}

// Parse a number
static ASTNode* parse_number(Parser* parser) {
    ASTNode* node = create_node(NODE_NUMBER);
    node->value = strdup(parser->current_token.value);
    advance_token(parser);
    return node;
}

// Forward declarations
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_statement(Parser* parser);
static ASTNode* parse_block(Parser* parser);

// Parse a binary operation
static ASTNode* parse_binary_operation(Parser* parser, ASTNode* left) {
    ASTNode* node = create_node(NODE_BINARY_OP);
    node->value = strdup(parser->current_token.value);
    node->left = left;
    
    advance_token(parser);
    node->right = parse_expression(parser);
    
    return node;
}

// Parse an expression
static ASTNode* parse_expression(Parser* parser) {
    ASTNode* left;
    
    switch (parser->current_token.type) {
        case TOKEN_IDENTIFIER:
            left = parse_identifier(parser);
            break;
        case TOKEN_NUMBER:
            left = parse_number(parser);
            break;
        default:
            return NULL;
    }
    
    while (parser->current_token.type == TOKEN_PLUS ||
           parser->current_token.type == TOKEN_MINUS ||
           parser->current_token.type == TOKEN_MULTIPLY ||
           parser->current_token.type == TOKEN_DIVIDE ||
           parser->current_token.type == TOKEN_GT ||
           parser->current_token.type == TOKEN_LT ||
           parser->current_token.type == TOKEN_EQ) {
        ASTNode* node = create_node(NODE_BINARY_OP);
        node->value = strdup(parser->current_token.value);
        node->left = left;
        
        advance_token(parser);
        node->right = parse_expression(parser);
        
        left = node;
    }
    
    return left;
}

// Parse a let statement
static ASTNode* parse_let_statement(Parser* parser) {
    advance_token(parser);  // consume 'let'
    
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        return NULL;
    }
    
    char* name = strdup(parser->current_token.value);
    advance_token(parser);  // consume identifier
    
    if (parser->current_token.type != TOKEN_ASSIGN) {
        free(name);
        return NULL;
    }
    advance_token(parser);  // consume '='
    
    ASTNode* value = parse_expression(parser);
    if (value == NULL) {
        free(name);
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_STATEMENT);
    node->value = name;
    node->left = value;
    
    return node;
}

// Parse a print statement
static ASTNode* parse_print_statement(Parser* parser) {
    advance_token(parser);  // consume 'print'
    
    ASTNode* node = create_node(NODE_PRINT);
    node->left = parse_expression(parser);
    
    return node;
}

// Parse a block of statements
static ASTNode* parse_block(Parser* parser) {
    advance_token(parser);  // consume '{'
    
    ASTNode* block = create_node(NODE_PROGRAM);
    block->statements = malloc(sizeof(ASTNode*) * MAX_STATEMENTS);
    
    while (parser->current_token.type != TOKEN_RBRACE && 
           parser->current_token.type != TOKEN_EOF) {
        ASTNode* statement = parse_statement(parser);
        if (statement != NULL) {
            block->statements[block->statements_count++] = statement;
        }
    }
    
    advance_token(parser);  // consume '}'
    return block;
}

// Parse an if statement
static ASTNode* parse_if_statement(Parser* parser) {
    advance_token(parser);  // consume 'if'
    
    if (parser->current_token.type != TOKEN_LPAREN) {
        return NULL;
    }
    advance_token(parser);  // consume '('
    
    ASTNode* condition = parse_expression(parser);
    
    if (parser->current_token.type != TOKEN_RPAREN) {
        return NULL;
    }
    advance_token(parser);  // consume ')'
    
    ASTNode* if_block = parse_block(parser);
    ASTNode* else_block = NULL;
    
    if (parser->current_token.type == TOKEN_ELSE) {
        advance_token(parser);  // consume 'else'
        else_block = parse_block(parser);
    }
    
    ASTNode* node = create_node(NODE_IF);
    node->left = condition;
    node->right = if_block;
    node->else_block = else_block;
    
    return node;
}

// Parse a statement
static ASTNode* parse_statement(Parser* parser) {
    ASTNode* statement = NULL;
    
    switch (parser->current_token.type) {
        case TOKEN_LET:
            statement = parse_let_statement(parser);
            break;
        case TOKEN_IF:
            statement = parse_if_statement(parser);
            break;
        case TOKEN_PRINT:
            statement = parse_print_statement(parser);
            break;
        default:
            statement = parse_expression(parser);
            break;
    }
    
    // Consume semicolon if present
    if (parser->current_token.type == TOKEN_SEMICOLON) {
        advance_token(parser);
    }
    
    return statement;
}

// Parse a program (entry point)
ASTNode* parse_program(Parser* parser) {
    ASTNode* program = create_node(NODE_PROGRAM);
    program->statements = malloc(sizeof(ASTNode*) * MAX_STATEMENTS);
    
    while (parser->current_token.type != TOKEN_EOF) {
        ASTNode* statement = parse_statement(parser);
        if (statement != NULL) {
            program->statements[program->statements_count++] = statement;
        }
    }
    
    return program;
}
