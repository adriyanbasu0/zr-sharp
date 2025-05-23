#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Value (union) and DataType (enum) are from compiler.h

// Local struct to hold runtime values (DataType + Value union)
typedef struct {
    DataType type;
    Value val; // Value here is the union from compiler.h
} RuntimeValue;

#define MAX_SYMBOLS 100

typedef struct {
    char* name;
    DataType type; // Store DataType separately
    Value val;     // Store Value union separately
} Symbol;

static Symbol symbol_table[MAX_SYMBOLS];
static int symbol_count = 0;

// Helper function to create an error value
static RuntimeValue create_error_runtime_value() {
    RuntimeValue rt_val;
    rt_val.type = TYPE_ERROR;
    rt_val.val.int_val = 0; // Default error payload
    return rt_val;
}

// Helper functions for value operations
static RuntimeValue create_number_runtime_value(double num) {
    RuntimeValue rt_val;
    rt_val.type = TYPE_FLOAT;
    rt_val.val.float_val = num;
    return rt_val;
}

static RuntimeValue create_string_runtime_value(const char* str) {
    RuntimeValue rt_val;
    rt_val.type = TYPE_STRING;
    rt_val.val.string_val = strdup(str);
    if (str != NULL && rt_val.val.string_val == NULL) {
        fprintf(stderr, "Error: Memory allocation failed in create_string_runtime_value.\n");
        return create_error_runtime_value();
    }
    return rt_val;
}

static RuntimeValue create_bool_runtime_value(bool b) {
    RuntimeValue rt_val;
    rt_val.type = TYPE_BOOL;
    rt_val.val.bool_val = b;
    return rt_val;
}

// Get a symbol from the symbol table (returns a pointer to the Symbol struct)
static Symbol* get_symbol(const char* name) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return &symbol_table[i];
        }
    }
    return NULL;
}

// Set a symbol in the symbol table
static void set_symbol(const char* name, RuntimeValue rt_new_value) {
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            // If old value was a string, free it
            if (symbol_table[i].type == TYPE_STRING && symbol_table[i].val.string_val != NULL) {
                safe_free(symbol_table[i].val.string_val); // Use safe_free
            }
            symbol_table[i].type = rt_new_value.type;
            symbol_table[i].val = rt_new_value.val;
            return;
        }
    }
    
    if (symbol_count < MAX_SYMBOLS) {
        symbol_table[symbol_count].name = strdup(name);
        if (symbol_table[symbol_count].name == NULL && name != NULL) {
             fprintf(stderr, "Error: Memory allocation failed for symbol name.\n");
             if (rt_new_value.type == TYPE_STRING && rt_new_value.val.string_val != NULL) {
                 safe_free(rt_new_value.val.string_val); // Use safe_free
             }
             return;
        }
        symbol_table[symbol_count].type = rt_new_value.type;
        symbol_table[symbol_count].val = rt_new_value.val;
        symbol_count++;
    } else {
        fprintf(stderr, "Error: Symbol table overflow.\n");
        if (rt_new_value.type == TYPE_STRING && rt_new_value.val.string_val != NULL) {
            safe_free(rt_new_value.val.string_val); // Use safe_free
        }
    }
}

// Print a runtime value
static void print_runtime_value(RuntimeValue rt_value) {
    switch (rt_value.type) {
        case TYPE_FLOAT:
            printf("%.2f", rt_value.val.float_val);
            break;
        case TYPE_STRING:
            printf("%s", rt_value.val.string_val);
            break;
        case TYPE_BOOL:
            printf("%s", rt_value.val.bool_val ? "true" : "false");
            break;
        case TYPE_INT:
            printf("%d", rt_value.val.int_val);
            break;
        case TYPE_VOID:
            printf("(void)");
            break;
        case TYPE_ERROR:
            fprintf(stderr, "ErrorValue");
            break;
        default:
            fprintf(stderr, "Unknown data type: %d\n", rt_value.type);
    }
}

// Forward declaration
static RuntimeValue evaluate_node(ASTNode* node);

// Evaluate a binary operation
static RuntimeValue evaluate_binary_op(ASTNode* node) {
    RuntimeValue left_rt = evaluate_node(node->left);
    RuntimeValue right_rt = evaluate_node(node->right);

    // Check for errors from operands
    if (left_rt.type == TYPE_ERROR) return left_rt;
    if (right_rt.type == TYPE_ERROR) return right_rt;

    // Using node->value.string_val for operator (Req 3)
    const char* op = node->value.string_val;
    if (op == NULL) {
        fprintf(stderr, "Error: Binary operator token has NULL text.\n");
        return create_error_runtime_value();
    }

    // Arithmetic Operators (+, -, *, /) - Require TYPE_FLOAT for both operands
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        if (left_rt.type != TYPE_FLOAT || right_rt.type != TYPE_FLOAT) {
            fprintf(stderr, "Error: Type error: Operands for arithmetic operator '%s' must be numbers.\n", op);
            return create_error_runtime_value();
        }
        double result_val;
        if (strcmp(op, "+") == 0) result_val = left_rt.val.float_val + right_rt.val.float_val;
        else if (strcmp(op, "-") == 0) result_val = left_rt.val.float_val - right_rt.val.float_val;
        else if (strcmp(op, "*") == 0) result_val = left_rt.val.float_val * right_rt.val.float_val;
        else if (strcmp(op, "/") == 0) {
            if (right_rt.val.float_val == 0) {
                fprintf(stderr, "Error: Division by zero\n");
                return create_error_runtime_value();
            }
            result_val = left_rt.val.float_val / right_rt.val.float_val;
        } else {
            fprintf(stderr, "Error: Unknown arithmetic operator '%s'\n", op);
            return create_error_runtime_value();
        }
        return create_number_runtime_value(result_val);
    }
    // Comparison Operators (>, <, ==, <=, >=, !=)
    else if (strcmp(op, ">") == 0 || strcmp(op, "<") == 0 || strcmp(op, "==") == 0 ||
             strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 || strcmp(op, "!=") == 0) {
        if (left_rt.type == TYPE_FLOAT && right_rt.type == TYPE_FLOAT) {
            bool cmp_res;
            if (strcmp(op, ">") == 0) cmp_res = left_rt.val.float_val > right_rt.val.float_val;
            else if (strcmp(op, "<") == 0) cmp_res = left_rt.val.float_val < right_rt.val.float_val;
            else if (strcmp(op, "==") == 0) cmp_res = left_rt.val.float_val == right_rt.val.float_val;
            else if (strcmp(op, "<=") == 0) cmp_res = left_rt.val.float_val <= right_rt.val.float_val;
            else if (strcmp(op, ">=") == 0) cmp_res = left_rt.val.float_val >= right_rt.val.float_val;
            else if (strcmp(op, "!=") == 0) cmp_res = left_rt.val.float_val != right_rt.val.float_val;
            else { /* Should not happen */ return create_error_runtime_value(); }
            return create_bool_runtime_value(cmp_res);
        }
        // Add string comparison for "==" and "!=" if desired later
        else {
            fprintf(stderr, "Error: Type error: Operands for comparison operator '%s' must be numbers (for now).\n", op);
            return create_error_runtime_value();
        }
    }
    // Logical Operators (&&, ||) - Require TYPE_BOOL for both operands
    else if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
        if (left_rt.type != TYPE_BOOL || right_rt.type != TYPE_BOOL) {
            fprintf(stderr, "Error: Type error: Operands for logical operator '%s' must be booleans.\n", op);
            return create_error_runtime_value();
        }
        bool logical_res;
        if (strcmp(op, "&&") == 0) logical_res = left_rt.val.bool_val && right_rt.val.bool_val;
        else if (strcmp(op, "||") == 0) logical_res = left_rt.val.bool_val || right_rt.val.bool_val;
        else { /* Should not happen */ return create_error_runtime_value(); }
        return create_bool_runtime_value(logical_res);
    }
    
    fprintf(stderr, "Error: Operator '%s' not defined for operand types %d and %d\n", op, left_rt.type, right_rt.type);
    return create_error_runtime_value();
}

// Evaluate an if statement
static RuntimeValue evaluate_if(ASTNode* node) {
    RuntimeValue condition_rt_val = evaluate_node(node->condition);
    
    if (condition_rt_val.type == TYPE_ERROR) return condition_rt_val;

    if (condition_rt_val.type != TYPE_BOOL) {
        fprintf(stderr, "Error: If statement condition must be a boolean.\n");
        return create_error_runtime_value();
    }

    if (condition_rt_val.val.bool_val) {
        if (node->body != NULL) { // body is node->body
             if (node->body->type == NODE_BLOCK) {
                RuntimeValue last_rt_val = create_error_runtime_value();
                for (int i = 0; i < node->body->statement_count; i++) {
                    last_rt_val = evaluate_node(node->body->statements[i]);
                    if (last_rt_val.type == TYPE_ERROR) return last_rt_val;
                }
                return last_rt_val;
            } else {
                 return evaluate_node(node->body);
            }
        }
    } else if (node->else_body != NULL) {
        if (node->else_body->type == NODE_BLOCK) {
            RuntimeValue last_rt_val = create_error_runtime_value();
            for (int i = 0; i < node->else_body->statement_count; i++) {
                last_rt_val = evaluate_node(node->else_body->statements[i]);
                if (last_rt_val.type == TYPE_ERROR) return last_rt_val;
            }
            return last_rt_val;
        } else {
            return evaluate_node(node->else_body);
        }
    }
    // If condition is false and no else block, or empty block
    return create_error_runtime_value(); // Or a specific void/undefined RuntimeValue
}

// Evaluate a let statement
static RuntimeValue evaluate_let(ASTNode* node) {
    if (node == NULL || node->value.string_val == NULL || node->left == NULL) {
        fprintf(stderr, "Error: Invalid let statement structure.\n");
        return create_error_runtime_value();
    }
    
    RuntimeValue rt_val_expr = evaluate_node(node->left);
    if (rt_val_expr.type == TYPE_ERROR) return rt_val_expr;

    set_symbol(node->value.string_val, rt_val_expr);
    return rt_val_expr;
}

// Main evaluation function
static RuntimeValue evaluate_node(ASTNode* node) {
    if (node == NULL) return create_error_runtime_value();
    
    switch (node->type) {
        case NODE_NUMBER:
            return create_number_runtime_value(atof(node->value.string_val));
        case NODE_STRING: // Added case
            // The string value is already strdup'd by the parser
            // create_string_runtime_value will strdup it again. This is important as
            // the RuntimeValue might be freed (e.g. after print)
            return create_string_runtime_value(node->value.string_val);
        case NODE_BOOL:   // Added case
            return create_bool_runtime_value(node->value.bool_val);
            
        case NODE_IDENT: {
            Symbol* sym = get_symbol(node->value.string_val);
            if (sym == NULL) {
                fprintf(stderr, "Error: Undefined variable '%s'\n", node->value.string_val);
                return create_error_runtime_value();
            }
            // Return a copy of the stored RuntimeValue essentially
            RuntimeValue id_val;
            id_val.type = sym->type;
            id_val.val = sym->val; 
            // If it's a string, strdup it for the new RuntimeValue to own,
            // as symbol table's string might be freed/changed.
            if (id_val.type == TYPE_STRING && id_val.val.string_val != NULL) {
                 id_val.val.string_val = strdup(id_val.val.string_val);
                 if (id_val.val.string_val == NULL) return create_error_runtime_value(); // strdup failed
            }
            return id_val;
        }
            
        case NODE_BINARY:
            return evaluate_binary_op(node);
            
        case NODE_IF:
            return evaluate_if(node);
            
        case NODE_LET:
            return evaluate_let(node);
            
        case NODE_PRINT: {
            if (node->left == NULL) {
                fprintf(stderr, "Error: Nothing to print\n");
                return create_error_runtime_value();
            }
            RuntimeValue rt_val_to_print = evaluate_node(node->left);
            if (rt_val_to_print.type == TYPE_ERROR) return rt_val_to_print;

            print_runtime_value(rt_val_to_print);
            printf("\n");
            fflush(stdout);
            // If the printed value was a string that was strdup'd (e.g. from IDENT), free it.
            if (rt_val_to_print.type == TYPE_STRING && rt_val_to_print.val.string_val != NULL) {
                 safe_free(rt_val_to_print.val.string_val); // Use safe_free
            }
            // Print probably shouldn't return the value, but a status or void type
            RuntimeValue print_status; print_status.type = TYPE_VOID; return print_status; 
        }
        case NODE_BLOCK: { // Changed from NODE_PROGRAM
            RuntimeValue last_rt_val_in_block = create_error_runtime_value();
            bool first_stmt_in_block = true;
            for (int i = 0; i < node->statement_count; i++) {
                if (node->statements[i] != NULL) {
                    // Free previous statement's string result if it was one and not the one we are about to return
                    if (!first_stmt_in_block && last_rt_val_in_block.type == TYPE_STRING && last_rt_val_in_block.val.string_val != NULL) {
                        // This logic is tricky: only free if it's an intermediate string result
                        // For now, assume strings are handled/freed by who creates them (e.g. print)
                    }
                    last_rt_val_in_block = evaluate_node(node->statements[i]);
                    if (last_rt_val_in_block.type == TYPE_ERROR) return last_rt_val_in_block;
                }
                first_stmt_in_block = false;
            }
            if (first_stmt_in_block) { // Empty block
                 RuntimeValue empty_block_val; empty_block_val.type = TYPE_VOID; return empty_block_val;
            }
            return last_rt_val_in_block;
        }
            
        default:
            fprintf(stderr, "Error: Unknown AST node type %d\n", node->type);
            return create_error_runtime_value();
    }
}

// Public interface
void interpret(ASTNode* program_node) { // Renamed parameter for clarity
    if (program_node != NULL) {
        RuntimeValue final_result = evaluate_node(program_node);
        // If the final result of the program is a string, it needs to be freed.
        if (final_result.type == TYPE_STRING && final_result.val.string_val != NULL) {
            safe_free(final_result.val.string_val); // Use safe_free
        }
    }
}

// Function to free all memory allocated by the interpreter (symbol table)
void free_interpreter_memory() {
    for (int i = 0; i < symbol_count; i++) {
        if (symbol_table[i].name != NULL) {
            safe_free(symbol_table[i].name); // Use safe_free
            symbol_table[i].name = NULL;
        }
        if (symbol_table[i].type == TYPE_STRING && symbol_table[i].val.string_val != NULL) {
            safe_free(symbol_table[i].val.string_val); // Use safe_free
            symbol_table[i].val.string_val = NULL;
        }
    }
    symbol_count = 0; // Reset symbol count
}
