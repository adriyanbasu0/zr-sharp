#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h> // For PRId64, PRId32
#include <errno.h>    // For ERANGE with strtoll

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
static RuntimeValue create_int64_runtime_value(int64_t num) {
    RuntimeValue rt_val;
    rt_val.type = TYPE_INT64;
    rt_val.val.int64_val = num;
    return rt_val;
}

static RuntimeValue create_int32_runtime_value(int32_t num) {
    RuntimeValue rt_val;
    rt_val.type = TYPE_INT32;
    rt_val.val.int32_val = num;
    return rt_val;
}

static RuntimeValue create_number_runtime_value(double num) { // This creates TYPE_FLOAT
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

// Helper function to get string representation of a data type
static const char* get_type_name(DataType type) {
    switch (type) {
        case TYPE_INT: return "int_legacy"; // Should ideally not be produced by parser anymore
        case TYPE_INT32: return "int32";
        case TYPE_INT64: return "int64";
        case TYPE_FLOAT: return "float";
        case TYPE_BOOL: return "bool";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_ERROR: return "error";
        default: return "unknown_type";
    }
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
            printf("%d", rt_value.val.int_val); // Kept for existing TYPE_INT, though might become dead code
            break;
        case TYPE_INT32:
            printf("%" PRId32, rt_value.val.int32_val);
            break;
        case TYPE_INT64:
            printf("%" PRId64, rt_value.val.int64_val);
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

    // Type promotion and operation logic
    DataType left_type = left_rt.type;
    DataType right_type = right_rt.type;

    // Promote INT32 to INT64 for simplicity in binary operations for now
    if (left_type == TYPE_INT32) {
        left_rt.val.int64_val = (int64_t)left_rt.val.int32_val;
        left_type = TYPE_INT64;
    }
    if (right_type == TYPE_INT32) {
        right_rt.val.int64_val = (int64_t)right_rt.val.int32_val;
        right_type = TYPE_INT64;
    }

    // Handle TYPE_INT as TYPE_INT64
    if (left_type == TYPE_INT) {
        left_rt.val.int64_val = (int64_t)left_rt.val.int_val;
        left_type = TYPE_INT64;
    }
    if (right_type == TYPE_INT) {
        right_rt.val.int64_val = (int64_t)right_rt.val.int_val;
        right_type = TYPE_INT64;
    }


    // Arithmetic Operators (+, -, *, /)
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        if (left_type == TYPE_INT64 && right_type == TYPE_INT64) {
            int64_t result_val;
            if (strcmp(op, "+") == 0) result_val = left_rt.val.int64_val + right_rt.val.int64_val;
            else if (strcmp(op, "-") == 0) result_val = left_rt.val.int64_val - right_rt.val.int64_val;
            else if (strcmp(op, "*") == 0) result_val = left_rt.val.int64_val * right_rt.val.int64_val;
            else if (strcmp(op, "/") == 0) {
                if (right_rt.val.int64_val == 0) {
                    fprintf(stderr, "Error: Division by zero (integer)\n");
                    return create_error_runtime_value();
                }
                result_val = left_rt.val.int64_val / right_rt.val.int64_val; // Integer division
            } else { /* Should not happen */ return create_error_runtime_value(); }
            return create_int64_runtime_value(result_val);
        } else if ((left_type == TYPE_FLOAT && (right_type == TYPE_INT64 || right_type == TYPE_FLOAT)) ||
                   (right_type == TYPE_FLOAT && (left_type == TYPE_INT64 || left_type == TYPE_FLOAT))) {
            double l_val = (left_type == TYPE_FLOAT) ? left_rt.val.float_val : (double)left_rt.val.int64_val;
            double r_val = (right_type == TYPE_FLOAT) ? right_rt.val.float_val : (double)right_rt.val.int64_val;
            double result_val;
            if (strcmp(op, "+") == 0) result_val = l_val + r_val;
            else if (strcmp(op, "-") == 0) result_val = l_val - r_val;
            else if (strcmp(op, "*") == 0) result_val = l_val * r_val;
            else if (strcmp(op, "/") == 0) {
                if (r_val == 0.0) {
                    fprintf(stderr, "Error: Division by zero (float)\n");
                    return create_error_runtime_value();
                }
                result_val = l_val / r_val;
            } else { /* Should not happen */ return create_error_runtime_value(); }
            return create_number_runtime_value(result_val); // create_number_runtime_value creates TYPE_FLOAT
        } else {
            fprintf(stderr, "Error: Type error: Operands for arithmetic operator '%s' must be numbers.\n", op);
            return create_error_runtime_value();
        }
    }
    // Comparison Operators (>, <, ==, <=, >=, !=)
    else if (strcmp(op, ">") == 0 || strcmp(op, "<") == 0 || strcmp(op, "==") == 0 ||
             strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0 || strcmp(op, "!=") == 0) {
        bool cmp_res;
        if (left_type == TYPE_INT64 && right_type == TYPE_INT64) {
            if (strcmp(op, ">") == 0) cmp_res = left_rt.val.int64_val > right_rt.val.int64_val;
            else if (strcmp(op, "<") == 0) cmp_res = left_rt.val.int64_val < right_rt.val.int64_val;
            else if (strcmp(op, "==") == 0) cmp_res = left_rt.val.int64_val == right_rt.val.int64_val;
            else if (strcmp(op, "<=") == 0) cmp_res = left_rt.val.int64_val <= right_rt.val.int64_val;
            else if (strcmp(op, ">=") == 0) cmp_res = left_rt.val.int64_val >= right_rt.val.int64_val;
            else if (strcmp(op, "!=") == 0) cmp_res = left_rt.val.int64_val != right_rt.val.int64_val;
            else { /* Should not happen */ return create_error_runtime_value(); }
        } else if ((left_type == TYPE_FLOAT && (right_type == TYPE_INT64 || right_type == TYPE_FLOAT)) ||
                   (right_type == TYPE_FLOAT && (left_type == TYPE_INT64 || left_type == TYPE_FLOAT))) {
            double l_val = (left_type == TYPE_FLOAT) ? left_rt.val.float_val : (double)left_rt.val.int64_val;
            double r_val = (right_type == TYPE_FLOAT) ? right_rt.val.float_val : (double)right_rt.val.int64_val;
            if (strcmp(op, ">") == 0) cmp_res = l_val > r_val;
            else if (strcmp(op, "<") == 0) cmp_res = l_val < r_val;
            else if (strcmp(op, "==") == 0) cmp_res = l_val == r_val; // Note: float equality can be tricky
            else if (strcmp(op, "<=") == 0) cmp_res = l_val <= r_val;
            else if (strcmp(op, ">=") == 0) cmp_res = l_val >= r_val;
            else if (strcmp(op, "!=") == 0) cmp_res = l_val != r_val;
            else { /* Should not happen */ return create_error_runtime_value(); }
        }
        // Add string comparison for "==" and "!="
        else if (left_type == TYPE_STRING && right_type == TYPE_STRING && (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0)) {
            if (strcmp(op, "==") == 0) cmp_res = strcmp(left_rt.val.string_val, right_rt.val.string_val) == 0;
            else cmp_res = strcmp(left_rt.val.string_val, right_rt.val.string_val) != 0;
        }
        else {
            fprintf(stderr, "Error: Type error: Operands for comparison operator '%s' are incompatible (%d, %d).\n", op, left_type, right_type);
            return create_error_runtime_value();
        }
        return create_bool_runtime_value(cmp_res);
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
    
    RuntimeValue expr_val = evaluate_node(node->left);
    if (expr_val.type == TYPE_ERROR) {
        return expr_val; // Propagate error
    }

    RuntimeValue final_val = expr_val; // Start with expr_val, potentially convert

    if (node->explicit_type != TYPE_VOID) {
        DataType declared_type = node->explicit_type;
        DataType actual_type = expr_val.type;

        if (declared_type == actual_type) {
            // Types match, no conversion needed
        } else {
            // Attempt conversions
            if (declared_type == TYPE_FLOAT) {
                if (actual_type == TYPE_INT64) {
                    final_val.type = TYPE_FLOAT;
                    final_val.val.float_val = (double)expr_val.val.int64_val;
                } else if (actual_type == TYPE_INT32) {
                    final_val.type = TYPE_FLOAT;
                    final_val.val.float_val = (double)expr_val.val.int32_val;
                } else if (actual_type == TYPE_INT) { // Legacy TYPE_INT
                    final_val.type = TYPE_FLOAT;
                    final_val.val.float_val = (double)expr_val.val.int_val;
                }
                // Add other conversions to FLOAT if necessary (e.g. BOOL to 0.0/1.0)
                else if (actual_type != TYPE_FLOAT) { // If not already float and not convertible above
                    goto type_error;
                }
            } else if (declared_type == TYPE_INT64) {
                if (actual_type == TYPE_INT32) {
                    final_val.type = TYPE_INT64;
                    final_val.val.int64_val = (int64_t)expr_val.val.int32_val;
                } else if (actual_type == TYPE_INT) { // Legacy TYPE_INT
                    final_val.type = TYPE_INT64;
                    final_val.val.int64_val = (int64_t)expr_val.val.int_val;
                }
                // Add other conversions to INT64 if necessary
                else if (actual_type != TYPE_INT64) {
                    goto type_error;
                }
            } else if (declared_type == TYPE_INT32) {
                if (actual_type == TYPE_INT64) {
                    if (expr_val.val.int64_val >= INT32_MIN && expr_val.val.int64_val <= INT32_MAX) {
                        final_val.type = TYPE_INT32;
                        final_val.val.int32_val = (int32_t)expr_val.val.int64_val;
                    } else {
                        fprintf(stderr, "Runtime Error: Value %" PRId64 " for variable '%s' overflows declared type int32.\n",
                                expr_val.val.int64_val, node->value.string_val);
                        // expr_val is not a string here, so no need to free its string_val
                        return create_error_runtime_value();
                    }
                } else if (actual_type == TYPE_INT) { // Legacy TYPE_INT
                     if (expr_val.val.int_val >= INT32_MIN && expr_val.val.int_val <= INT32_MAX) {
                        final_val.type = TYPE_INT32;
                        final_val.val.int32_val = (int32_t)expr_val.val.int_val;
                    } else {
                        fprintf(stderr, "Runtime Error: Value %d for variable '%s' overflows declared type int32.\n",
                                expr_val.val.int_val, node->value.string_val);
                        return create_error_runtime_value();
                    }
                }
                // Add other conversions to INT32 if necessary
                else if (actual_type != TYPE_INT32) {
                    goto type_error;
                }
            }
            // Add cases for TYPE_BOOL, TYPE_STRING if direct assignment or conversions are allowed
            // For now, any other declared_type means we fall through to type_error if not matched above
            else if (declared_type == TYPE_BOOL && actual_type != TYPE_BOOL) {
                 goto type_error; // No implicit conversions to bool yet
            } else if (declared_type == TYPE_STRING && actual_type != TYPE_STRING) {
                 goto type_error; // No implicit conversions to string yet
            }
            // Default to error if no specific conversion rule matched
            else if (declared_type != actual_type) { // If we fell through all conversions
                 goto type_error;
            }
        }
    }
    // If no explicit type, or if types matched, or if conversion was successful,
    // final_val holds the value to be set.

    set_symbol(node->value.string_val, final_val);
    // Note: set_symbol takes ownership of final_val.string_val if it's a string.
    // If expr_val was a string and final_val is different (e.g. after a hypothetical conversion from string),
    // and final_val is NOT a string, then expr_val.val.string_val might need freeing here.
    // However, current conversions are numeric. If expr_val was a string and declared_type is numeric,
    // it would hit type_error, and expr_val.val.string_val should be freed there.

    return final_val; // Return the value that was actually stored (could be converted)

type_error:
    fprintf(stderr, "Runtime Error: Cannot assign expression of type %s to variable '%s' of declared type %s.\n",
            get_type_name(expr_val.type), node->value.string_val, get_type_name(node->explicit_type));
    if (expr_val.type == TYPE_STRING && expr_val.val.string_val != NULL) {
        safe_free(expr_val.val.string_val); // Free the string from the expression if it's not being used
    }
    return create_error_runtime_value();
}

// Main evaluation function
static RuntimeValue evaluate_node(ASTNode* node) {
    if (node == NULL) return create_error_runtime_value();
    
    switch (node->type) {
        case NODE_NUMBER:
            if (node->data_type == TYPE_INT64 || node->data_type == TYPE_INT) { // Treat old TYPE_INT as INT64
                char *endptr;
                long long int_val = strtoll(node->value.string_val, &endptr, 10);
                if (node->value.string_val == endptr || *endptr != '\0') {
                    fprintf(stderr, "Error: Invalid integer literal '%s'\n", node->value.string_val);
                    return create_error_runtime_value();
                }
                if (errno == ERANGE) {
                    fprintf(stderr, "Error: Integer literal '%s' out of range for int64.\n", node->value.string_val);
                    return create_error_runtime_value();
                }
                return create_int64_runtime_value(int_val);
            } else if (node->data_type == TYPE_FLOAT) {
                // Assuming create_number_runtime_value handles TYPE_FLOAT correctly by parsing string to double
                return create_number_runtime_value(atof(node->value.string_val));
            } else if (node->data_type == TYPE_INT32) { // Explicitly handle INT32 if parser produces it
                 char *endptr;
                // For now, parse as long long and cast, or use strtol if strict 32-bit range is needed
                long int_val = strtol(node->value.string_val, &endptr, 10);
                if (node->value.string_val == endptr || *endptr != '\0') {
                    fprintf(stderr, "Error: Invalid int32 literal '%s'\n", node->value.string_val);
                    return create_error_runtime_value();
                }
                if (errno == ERANGE || int_val > INT32_MAX || int_val < INT32_MIN) {
                    fprintf(stderr, "Error: Integer literal '%s' out of range for int32.\n", node->value.string_val);
                    return create_error_runtime_value();
                }
                return create_int32_runtime_value((int32_t)int_val);
            } else {
                fprintf(stderr, "Error: Unknown data type for NODE_NUMBER: %d\n", node->data_type);
                return create_error_runtime_value();
            }
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
void interpret(ASTNode* program_node) {
    if (program_node == NULL) return;

    RuntimeValue final_result = evaluate_node(program_node);

    // Clean up any dynamically allocated memory in the result, if needed
    switch (final_result.type) {
        case TYPE_STRING:
            if (final_result.val.string_val != NULL) {
                safe_free(final_result.val.string_val);
            }
            break;
        case TYPE_ERROR:
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_INT64:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        default:
            // No dynamic memory to free for these types
            break;
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
