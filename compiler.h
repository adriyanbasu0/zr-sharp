#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Token types
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_LET,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_PRINT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ASSIGN,
    TOKEN_GT,
    TOKEN_LT,
    TOKEN_EQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

// Lexer structure
typedef struct {
    const char* source;
    int position;
    int read_position;
    char current_char;
    int line;
    int column;
} Lexer;

// AST node types
typedef enum {
    NODE_PROGRAM,
    NODE_STATEMENT,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_BINARY_OP,
    NODE_IF,
    NODE_PRINT
} NodeType;

#define MAX_STATEMENTS 100

// AST node structure
typedef struct ASTNode {
    NodeType type;
    char* value;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* else_block;
    struct ASTNode** statements;
    int statements_count;
} ASTNode;

// Parser structure
typedef struct {
    Lexer* lexer;
    Token current_token;
} Parser;

// Function declarations
Lexer* init_lexer(const char* source);
Token get_next_token(Lexer* lexer);
Parser* init_parser(Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void interpret(ASTNode* program);

#endif
