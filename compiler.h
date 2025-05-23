#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Maximum number of statements in a program
#define MAX_STATEMENTS 1000
// Maximum length of identifiers
#define MAX_IDENT_LEN 64
// Maximum number of variables in scope
#define MAX_VARIABLES 256
// Maximum string length
#define MAX_STRING_LEN 1024
// Maximum number of parameters
#define MAX_PARAMS 16

// Token types
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENT,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQ,
    TOKEN_EQEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_NOTEQ,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_LET,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_PRINT,
    TOKEN_FUNC,
    TOKEN_RETURN,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT
} TokenType;

// Data types
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_ERROR
} DataType;

// Value union
typedef union {
    int int_val;
    double float_val;
    bool bool_val;
    char* string_val;
} Value;

// Variable structure
typedef struct {
    char name[MAX_IDENT_LEN];
    DataType type;
    Value value;
    int scope_level;
} Variable;

// Token structure
typedef struct {
    TokenType type;
    char* text;
    int line;
    int column;
} Token;

// AST node types
typedef enum {
    NODE_NUMBER,
    NODE_STRING,
    NODE_BOOL,
    NODE_IDENT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_LET,
    NODE_IF,
    NODE_WHILE,
    NODE_BLOCK,
    NODE_PRINT,
    NODE_FUNC,
    NODE_CALL,
    NODE_RETURN
} NodeType;

// AST node structure
typedef struct ASTNode {
    NodeType type;
    DataType data_type;
    Value value;
    struct ASTNode* left;
    struct ASTNode* right;
    struct ASTNode* condition;
    struct ASTNode* body;
    struct ASTNode* else_body;
    struct ASTNode** params;
    int param_count;
    struct ASTNode** statements;
    int statement_count;
} ASTNode;

// Function structure
typedef struct {
    char name[MAX_IDENT_LEN];
    DataType return_type;
    DataType param_types[MAX_PARAMS];
    char param_names[MAX_PARAMS][MAX_IDENT_LEN];
    int param_count;
    ASTNode* body;
} Function;

// Lexer structure
typedef struct {
    char* input;
    int position;
    int line;
    int column;
    Token current_token;
} Lexer;

// Parser structure
typedef struct {
    Lexer* lexer;
    Token current_token;
    Variable variables[MAX_VARIABLES];
    int variable_count;
    int scope_level;
    Function functions[MAX_VARIABLES];
    int function_count;
} Parser;

// Function declarations
Lexer* init_lexer(char* input);
Token get_next_token(Lexer* lexer);
void free_lexer(Lexer* lexer); // Added declaration
Parser* init_parser(Lexer* lexer);
ASTNode* parse_program(Parser* parser);
void free_parser(Parser* parser); // Added declaration
void interpret(ASTNode* node);
void free_ast(ASTNode* node);

// Error handling
void error(const char* format, ...);
void warning(const char* format, ...);

// Type checking
DataType check_types(ASTNode* node);
bool is_compatible_type(DataType left, DataType right);

// Memory management
void* safe_malloc(size_t size);
void safe_free(void* ptr);

// Interpreter memory cleanup
void free_interpreter_memory(void);

#endif // COMPILER_H
