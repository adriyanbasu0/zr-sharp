#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple value structure to store runtime values
typedef struct {
    enum {
        VAL_NUMBER,
        VAL_STRING,
        VAL_BOOL
    } type;
    union {
        double number;
        char* string;
        bool boolean;
    } as;
} Value;

#define MAX_SYMBOLS 100

typedef struct {
    char* name;
    Value value;
} Symbol;

static Symbol symbol_table[MAX_SYMBOLS];
static int symbol_count = 0;

// Helper functions for value operations
static Value create_number(double num) {
    Value val;
    val.type = VAL_NUMBER;
    val.as.number = num;
    return val;
}

static Value create_string(const char* str) {
    Value val;
    val.type = VAL_STRING;
    val.as.string = strdup(str);
    return val;
}

static Value create_bool(bool b) {
    Value val;
    val.type = VAL_BOOL;
    val.as.boolean = b;
    return val;
}

// Get a symbol from the symbol table
static Value* get_symbol(const char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i].value;
        }
    }
    return NULL;
}

// Set a symbol in the symbol table
static void set_symbol(const char* name, Value value) {
    // First check if symbol already exists
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            return;
        }
    }
    
    // Add new symbol if it doesn't exist
    if (symbol_count < MAX_SYMBOLS) {
        symbol_table[symbol_count].name = strdup(name);
        symbol_table[symbol_count].value = value;
        symbol_count++;
    }
}

// Print a value
static void print_value(Value value) {
    switch (value.type) {
        case VAL_NUMBER:
            printf("%.2f", value.as.number);
            break;
        case VAL_STRING:
            printf("%s", value.as.string);
            break;
        case VAL_BOOL:
            printf("%s", value.as.boolean ? "true" : "false");
            break;
    }
}

// Forward declaration
static Value evaluate_node(ASTNode* node);

// Evaluate a binary operation
static Value evaluate_binary_op(ASTNode* node) {
    Value left = evaluate_node(node->left);
    Value right = evaluate_node(node->right);
    
    if (left.type == VAL_NUMBER && right.type == VAL_NUMBER) {
        double result;
        if (strcmp(node->value, "+") == 0) {
            result = left.as.number + right.as.number;
            return create_number(result);
        } else if (strcmp(node->value, "-") == 0) {
            result = left.as.number - right.as.number;
            return create_number(result);
        } else if (strcmp(node->value, "*") == 0) {
            result = left.as.number * right.as.number;
            return create_number(result);
        } else if (strcmp(node->value, "/") == 0) {
            if (right.as.number == 0) {
                printf("Error: Division by zero\n");
                return create_number(0);
            }
            result = left.as.number / right.as.number;
            return create_number(result);
        } else if (strcmp(node->value, ">") == 0) {
            return create_bool(left.as.number > right.as.number);
        } else if (strcmp(node->value, "<") == 0) {
            return create_bool(left.as.number < right.as.number);
        } else if (strcmp(node->value, "==") == 0) {
            return create_bool(left.as.number == right.as.number);
        }
    }
    
    return create_number(0);
}

// Evaluate an if statement
static Value evaluate_if(ASTNode* node) {
    Value condition = evaluate_node(node->left);
    
    if (condition.type == VAL_BOOL && condition.as.boolean) {
        if (node->right != NULL && node->right->type == NODE_PROGRAM) {
            Value last_value = create_number(0);
            for (int i = 0; i < node->right->statements_count; i++) {
                last_value = evaluate_node(node->right->statements[i]);
            }
            return last_value;
        }
        return evaluate_node(node->right);
    } else if (node->else_block != NULL) {
        if (node->else_block->type == NODE_PROGRAM) {
            Value last_value = create_number(0);
            for (int i = 0; i < node->else_block->statements_count; i++) {
                last_value = evaluate_node(node->else_block->statements[i]);
            }
            return last_value;
        }
        return evaluate_node(node->else_block);
    }
    
    return create_number(0);
}

// Evaluate a let statement
static Value evaluate_let(ASTNode* node) {
    if (node == NULL || node->value == NULL || node->left == NULL) {
        return create_number(0);
    }
    
    Value val = evaluate_node(node->left);
    set_symbol(node->value, val);
    return val;
}

// Main evaluation function
static Value evaluate_node(ASTNode* node) {
    if (node == NULL) return create_number(0);
    
    switch (node->type) {
        case NODE_NUMBER:
            return create_number(atof(node->value));
            
        case NODE_IDENTIFIER: {
            Value* val = get_symbol(node->value);
            if (val == NULL) {
                printf("Error: Undefined variable '%s'\n", node->value);
                return create_number(0);
            }
            return *val;
        }
            
        case NODE_BINARY_OP:
            return evaluate_binary_op(node);
            
        case NODE_IF:
            return evaluate_if(node);
            
        case NODE_STATEMENT:
            return evaluate_let(node);
            
        case NODE_PRINT: {
            if (node->left == NULL) {
                printf("Error: Nothing to print\n");
                return create_number(0);
            }
            Value value = evaluate_node(node->left);
            print_value(value);
            printf("\n");  // Add newline after each print
            fflush(stdout);  // Ensure output is displayed immediately
            return value;
        }
            
        case NODE_PROGRAM: {
            Value last_value = create_number(0);
            for (int i = 0; i < node->statements_count; i++) {
                if (node->statements[i] != NULL) {
                    last_value = evaluate_node(node->statements[i]);
                }
            }
            return last_value;
        }
            
        default:
            return create_number(0);
    }
}

// Public interface
void interpret(ASTNode* program) {
    if (program != NULL) {
        evaluate_node(program);
    }
}
