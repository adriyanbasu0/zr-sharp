#include "compiler.h"
#include <stdlib.h>
#include <string.h>

// Advance to the next token
static void advance_token(Parser* parser) {
    parser->current_token = get_next_token(parser->lexer);
}

// Initialize parser with a lexer
Parser* init_parser(Lexer* lexer) {
    Parser* parser = (Parser*)safe_malloc(sizeof(Parser)); // Use safe_malloc
    parser->lexer = lexer;
    advance_token(parser);  // Load first token
    return parser;
}

// Create a new AST node (no longer static)
ASTNode* create_node(NodeType type) {
    ASTNode* node = (ASTNode*)safe_malloc(sizeof(ASTNode)); // Use safe_malloc
    // safe_malloc handles exit on failure, so no NULL check needed here for node itself
    
    node->type = type;
    // Initialize Value union members
    node->value.string_val = NULL; 
    node->value.int_val = 0;
    node->value.float_val = 0.0;
    node->value.bool_val = false;
    // Initialize other ASTNode members
    node->left = NULL;
    node->right = NULL;
    node->condition = NULL; // Added for completeness, as per compiler.h
    node->body = NULL;      // Added for completeness
    node->else_body = NULL; // Added for completeness
    node->params = NULL;    // Added for completeness
    node->param_count = 0;  // Added for completeness
    node->statements = NULL;
    node->statement_count = 0; // Renamed from statements_count for consistency with compiler.h
    node->data_type = TYPE_VOID; // Default data type for the node's own evaluated type
    node->explicit_type = TYPE_VOID; // Default: no explicit type declaration
    
    return node;
}

// Parse an identifier
static ASTNode* parse_identifier(Parser* parser) {
    ASTNode* node = create_node(NODE_IDENT); 
    if (!node) return NULL;
    if (parser->current_token.text == NULL) { // Should not happen for IDENT if lexer is correct
        fprintf(stderr, "Parser Error: Identifier token has NULL text.\n");
        safe_free(node);
        return NULL;
    }
    node->value.string_val = parser->current_token.text; // Transfer ownership
    parser->current_token.text = NULL;                  // Nullify original pointer

    // Inspect the string to determine if it's a float or int64
    if (strchr(node->value.string_val, '.') != NULL) {
        node->data_type = TYPE_FLOAT;
    } else {
        node->data_type = TYPE_INT64; // Default to INT64 for whole numbers
    }

    advance_token(parser);
    return node;
}

// Parse a number
static ASTNode* parse_number(Parser* parser) {
    ASTNode* node = create_node(NODE_NUMBER);
    if (!node) return NULL;
    if (parser->current_token.text == NULL) { // Should not happen for NUMBER if lexer is correct
        fprintf(stderr, "Parser Error: Number token has NULL text.\n");
        safe_free(node);
        return NULL;
    }
    node->value.string_val = parser->current_token.text; // Transfer ownership
    parser->current_token.text = NULL;                  // Nullify original pointer

    // Inspect the string to determine if it's a float or int64
    if (strchr(node->value.string_val, '.') != NULL) {
        node->data_type = TYPE_FLOAT;
    } else {
        node->data_type = TYPE_INT64; // Default to INT64 for whole numbers
    }

    advance_token(parser);
    return node;
}

// Forward declarations
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_statement(Parser* parser); // Will call parse_loadin_statement
static ASTNode* parse_block(Parser* parser);
static ASTNode* parse_loadin_statement(Parser* parser); // New forward declaration

// parse_binary_operation is removed as it's unused. 
// Logic is handled in parse_expression.

// Parse an expression
static ASTNode* parse_expression(Parser* parser) {
    ASTNode* left = NULL; // Initialize left to NULL
    
    // Parse primary expressions like identifiers, numbers, literals, parenthesized expressions
    switch (parser->current_token.type) {
        case TOKEN_IDENT:
            left = parse_identifier(parser);
            break;
        case TOKEN_NUMBER:
            left = parse_number(parser);
            break;
        case TOKEN_STRING: {
            ASTNode* node = create_node(NODE_STRING);
            if (parser->current_token.text == NULL) { // Should not happen for STRING if lexer is correct
                fprintf(stderr, "Parser Error: String token has NULL text.\n");
                safe_free(node);
                left = NULL;
            } else {
                node->value.string_val = parser->current_token.text; // Transfer ownership
                parser->current_token.text = NULL;                  // Nullify original pointer
                node->data_type = TYPE_STRING; 
                left = node; 
            }
            advance_token(parser); // Consume TOKEN_STRING
            break;
        }
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            ASTNode* node = create_node(NODE_BOOL);
            node->value.bool_val = (parser->current_token.type == TOKEN_TRUE);
            node->data_type = TYPE_BOOL;
            advance_token(parser); // Consume the token
            left = node;
            break;
        }
        case TOKEN_LPAREN: {
            advance_token(parser); // Consume TOKEN_LPAREN
            ASTNode* inner_expression = parse_expression(parser);
            if (inner_expression == NULL || parser->current_token.type != TOKEN_RPAREN) {
                fprintf(stderr, "Parser Error: Mismatched parentheses or invalid expression within parentheses.\n");
                if (inner_expression) free_ast(inner_expression);
                left = NULL; 
            } else {
                advance_token(parser); // Consume TOKEN_RPAREN
                left = inner_expression;
            }
            break;
        }
        default:
            // If the token cannot start an expression, print an error and return NULL.
            // This prevents falling through to the binary operator loop with a NULL left operand
            // if the token is not a binary operator either.
            fprintf(stderr, "Parser Error: Token '%s' cannot start an expression.\n", parser->current_token.text);
            return NULL; // Return NULL directly
    }
    
    // If left is NULL after trying to parse a primary expression (e.g. strdup failed, or error in LPAREN),
    // and it wasn't an explicit "cannot start expression" error from default, then return NULL.
    if (left == NULL) {
        // This path might be taken if, for example, strdup failed for TOKEN_STRING
        // or if LPAREN parsing had an error that set left to NULL.
        // The specific error message would have been printed in the case.
        return NULL;
    }

    // Parse binary operations
    while (parser->current_token.type == TOKEN_PLUS ||
           parser->current_token.type == TOKEN_MINUS ||
           parser->current_token.type == TOKEN_STAR ||   // Requirement 3
           parser->current_token.type == TOKEN_SLASH ||  // Requirement 3
           parser->current_token.type == TOKEN_GT ||
           parser->current_token.type == TOKEN_LT ||
           parser->current_token.type == TOKEN_EQ ||     // This TOKEN_EQ is for assignment in some contexts, but here it's for comparison if it's part of general expression parsing. Let's assume it's general for now.
           parser->current_token.type == TOKEN_EQEQ ||   // Requirement 4
           parser->current_token.type == TOKEN_LTEQ ||   // Requirement 4
           parser->current_token.type == TOKEN_GTEQ ||   // Requirement 4
           parser->current_token.type == TOKEN_NOTEQ ||  // Requirement 4
           parser->current_token.type == TOKEN_AND ||    // Requirement 4
           parser->current_token.type == TOKEN_OR) {     // Requirement 4
        ASTNode* node = create_node(NODE_BINARY); // Requirement 1
        if (!node) { if (left) free_ast(left); return NULL; } // Clean up left if node creation fails
        
        // Store operator token text
        if (parser->current_token.text == NULL) { // Should not happen for operator tokens
             fprintf(stderr, "Parser Error: Operator token has NULL text.\n");
             free_ast(left); // Free the left operand
             safe_free(node); // Free the partially created binary node
             return NULL;    // Critical error
        }
        node->value.string_val = parser->current_token.text; // Transfer ownership
        parser->current_token.text = NULL;                  // Nullify original pointer
        node->left = left;
        
        // Store operator type as node's data_type or a specific field if added
        // For now, the type of binary operation is implicitly known by parser logic
        // or could be stored in node->data_type if appropriate
        // node->data_type = operator_token_to_type(parser->current_token.type);

        advance_token(parser); // Consume operator
        
        // This recursive call to parse_expression might need precedence handling for correctness
        // For now, it implements left-associativity for operators of same precedence.
        ASTNode* right_operand = parse_expression(parser); 
        if (!right_operand) {
            // If right operand parsing fails, free the operator node and its left child
            free_ast(node); // This should free node->value.string_val and node->left (which is 'left')
            return NULL;
        }
        node->right = right_operand;
        
        left = node; // Current binary operation becomes the left operand for the next
    }
    
    return left;
}

// Parse a let statement
static ASTNode* parse_let_statement(Parser* parser) {
    advance_token(parser);  // consume 'let'
    
    if (parser->current_token.type != TOKEN_IDENT) { // Requirement 3
        // error("Expected identifier after 'let'");
        return NULL;
    }
    
    // Directly create the node and store the identifier in it
    ASTNode* node = create_node(NODE_LET); // Requirement 1
    if (!node) return NULL;

    // Assuming current_token.text is the correct field from lexer
    node->value.string_val = strdup(parser->current_token.text);
    if (!node->value.string_val) {
        fprintf(stderr, "Parser Error: Failed to duplicate identifier name for 'let' statement.\n");
        free_ast(node);
        return NULL;
    }

    advance_token(parser);  // consume identifier

    // Check for explicit type declaration (e.g., let x : int64 = 10)
    if (parser->current_token.type == TOKEN_COLON) {
        advance_token(parser); // consume ':'

        switch (parser->current_token.type) {
            case TOKEN_TYPE_INT:
                node->explicit_type = TYPE_INT64; // Default 'int' to TYPE_INT64
                break;
            case TOKEN_TYPE_INT32:
                node->explicit_type = TYPE_INT32;
                break;
            case TOKEN_TYPE_INT64:
                node->explicit_type = TYPE_INT64;
                break;
            case TOKEN_TYPE_FLOAT:
                node->explicit_type = TYPE_FLOAT;
                break;
            case TOKEN_TYPE_BOOL:
                node->explicit_type = TYPE_BOOL;
                break;
            case TOKEN_TYPE_STRING:
                node->explicit_type = TYPE_STRING;
                break;
            default:
                fprintf(stderr, "Parser Error: Expected type keyword (int, int32, int64, float, bool, string) after ':' in let statement, got %s.\n", parser->current_token.text);
                free_ast(node); // Free the partially created node
                return NULL;
        }
        advance_token(parser); // consume type keyword
    }
    // If no TOKEN_COLON, node->explicit_type remains TYPE_VOID (from create_node)

    if (parser->current_token.type != TOKEN_EQ) {
        fprintf(stderr, "Parser Error: Expected '=' after identifier or type in let statement, got %s.\n", parser->current_token.text);
        free_ast(node); // Free the partially created node
        return NULL;
    }
    advance_token(parser);  // consume '='
    
    ASTNode* value_expr = parse_expression(parser);
    if (value_expr == NULL) {
        fprintf(stderr, "Parser Error: Expected expression after '=' in let statement.\n");
        free_ast(node); // Free the partially created node (name is in node->value.string_val)
        return NULL;
    }
    
    // The 'name' is node->value.string_val. The 'value_expr' is the expression.
    // For NODE_LET, 'left' can be used for the expression.
    node->left = value_expr; 
    // No 'right' child for NODE_LET in this structure, 'value' holds the var name.
    
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
    
    ASTNode* block_node = create_node(NODE_BLOCK); // Requirement 1
    // if (!block_node) return NULL; // create_node uses safe_malloc

    // Allocate statements array for the block node
    block_node->statements = safe_malloc(sizeof(ASTNode*) * MAX_STATEMENTS);

    // Use statement_count (updated in create_node)
    while (parser->current_token.type != TOKEN_RBRACE && 
           parser->current_token.type != TOKEN_EOF) {
        ASTNode* statement = parse_statement(parser);
        if (statement != NULL) {
            // TODO: Implement dynamic array for statements if MAX_STATEMENTS is not desired
            if (block_node->statement_count < MAX_STATEMENTS) {
                 block_node->statements[block_node->statement_count++] = statement;
            } else {
                // error("Exceeded maximum number of statements in a block.");
                free_ast(statement); // Free the statement that couldn't be added
                // Potentially free all previously added statements and the block node itself
                // For now, just stop adding and report error or handle gracefully
                break; 
            }
        } else {
            // If parse_statement returns NULL, it might mean an error or just end of a construct
            // Depending on error recovery strategy, might need to break or skip token
            // For now, assume error and break.
            // error("Invalid statement in block.");
            break;
        }
    }
    
    if (parser->current_token.type != TOKEN_RBRACE) {
        // error("Expected '}' to close block");
        free_ast(block_node); // Free the partially formed block
        return NULL;
    }
    advance_token(parser);  // consume '}'
    return block_node;
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
    node->condition = condition; // Use condition field for if condition
    node->body = if_block;       // Use body field for if block
    node->else_body = else_block; // Corrected from else_block
    
    return node;
}

// Parse a loadin statement (e.g., loadin "module_name")
static ASTNode* parse_loadin_statement(Parser* parser) {
    advance_token(parser); // Consume 'loadin' keyword

    if (parser->current_token.type != TOKEN_STRING) {
        error("Parser Error: Expected string literal (file path) after 'loadin', got %s at line %d, column %d.",
              parser->current_token.text ? parser->current_token.text : "unknown token",
              parser->current_token.line, parser->current_token.column);
        return NULL;
    }

    ASTNode* node = create_node(NODE_LOADIN);
    if (!node) return NULL; // Should not happen with safe_malloc

    if (parser->current_token.text == NULL) {
        error("Parser Error: String token for 'loadin' path has NULL text at line %d, column %d.",
              parser->current_token.line, parser->current_token.column);
        free_ast(node); // free_ast can handle node if value.string_val is NULL
        return NULL;
    }

    node->value.string_val = strdup(parser->current_token.text); // Duplicate the path string
    if (!node->value.string_val) {
        error("Parser Error: Failed to duplicate path string for 'loadin' at line %d, column %d.",
              parser->current_token.line, parser->current_token.column);
        // No need to free parser->current_token.text, lexer owns it until transferred or node is fully formed
        free_ast(node); // Free the node itself
        return NULL;
    }
    // Original token text from lexer is not freed here; it will be freed when the token itself is
    // effectively discarded or its text pointer nullified after transfer.
    // Since we strdup'd, the original token's text is untouched by this AST node.

    advance_token(parser); // Consume the string literal token

    // loadin statements are typically followed by a semicolon or newline in many languages.
    // For now, we don't enforce a semicolon strictly after loadin,
    // it will be handled by the general semicolon consumption in parse_statement if present.
    return node;
}


// Parse a statement
static ASTNode* parse_statement(Parser* parser) {
    ASTNode* statement = NULL;
    
    switch (parser->current_token.type) {
        case TOKEN_LET: // Stays TOKEN_LET, was already correct
            statement = parse_let_statement(parser);
            break;
        case TOKEN_IF: // Stays TOKEN_IF
            statement = parse_if_statement(parser);
            break;
        case TOKEN_PRINT: // Stays TOKEN_PRINT
            statement = parse_print_statement(parser);
            break;
        case TOKEN_LOADIN: // Added case for TOKEN_LOADIN
            statement = parse_loadin_statement(parser);
            break;
        // Add other statement types: TOKEN_WHILE, TOKEN_FUNC, TOKEN_RETURN etc.
        default:
            // If it's not a recognized statement keyword, try parsing it as an expression statement
            statement = parse_expression(parser);
            // Expression statements must be followed by a semicolon (usually)
            if (statement && parser->current_token.type != TOKEN_SEMICOLON) {
                // error("Expression statement must be followed by a semicolon.");
                // free_ast(statement); // Free the parsed expression
                // return NULL; // Or attempt recovery
            }
            break;
    }
    
    // Consume semicolon if present, and it's expected for the statement type
    if (parser->current_token.type == TOKEN_SEMICOLON) {
        advance_token(parser);
    }
    
    return statement;
}

// Parse a program (entry point)
ASTNode* parse_program(Parser* parser) {
    // A program is parsed as a top-level block
    ASTNode* program_node = create_node(NODE_BLOCK); // Changed NODE_PROGRAM to NODE_BLOCK
    // if (!program_node) return NULL; // create_node uses safe_malloc

    // Allocate statements array for the program node
    program_node->statements = safe_malloc(sizeof(ASTNode*) * MAX_STATEMENTS);
    // if (!program_node->statements) { free_ast(program_node); return NULL; } // safe_malloc handles exit

    while (parser->current_token.type != TOKEN_EOF) {
        ASTNode* statement = parse_statement(parser);
        if (statement != NULL) {
             if (program_node->statement_count < MAX_STATEMENTS) {
                program_node->statements[program_node->statement_count++] = statement;
            } else {
                // error("Exceeded maximum number of statements in program.");
                free_ast(statement);
                break;
            }
        } else {
            // error("Invalid statement in program.");
            // Attempt to recover by skipping token or stop parsing
            // For now, stop if a statement parsing fails.
            break; 
        }
    }
    
    return program_node;
}

// In parser.c
void free_ast(ASTNode* node) {
    if (node == NULL) {
        return;
    }

    // Corrected conditional free:
    if ((node->type == NODE_IDENT ||
         node->type == NODE_NUMBER ||
         node->type == NODE_STRING ||
         node->type == NODE_BINARY ||
         node->type == NODE_LET) && /* NODE_LET's value.string_val is var name */
        node->value.string_val != NULL) {
        safe_free(node->value.string_val);
        node->value.string_val = NULL;
    }
    // For other node types like NODE_BOOL, NODE_PRINT, NODE_IF, NODE_BLOCK, etc.,
    // value.string_val is not used for dynamically allocated strings that free_ast should free.

    // Recursively free children nodes
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->condition);
    free_ast(node->body);
    free_ast(node->else_body);

    // Free parameters if any (for function nodes, not fully implemented yet)
    if (node->params != NULL) {
        for (int i = 0; i < node->param_count; i++) {
            free_ast(node->params[i]);
        }
        safe_free(node->params);
        node->params = NULL;
    }

    // Free statements if it's a block-like node
    if (node->statements != NULL) {
        for (int i = 0; i < node->statement_count; i++) {
            free_ast(node->statements[i]);
        }
        safe_free(node->statements);
        node->statements = NULL;
    }

    // Finally, free the node itself
    safe_free(node);
}

// Free parser resources
void free_parser(Parser* parser) {
    // Note: parser->lexer is managed (created and freed) externally.
    // We only free the Parser struct itself.
    if (parser != NULL) {
        // If parser owns lexer, free lexer here.
        // For now, assuming lexer is freed separately or by main.
        // if (parser->lexer) {
        //     free_lexer(parser->lexer); // Example if parser owns lexer
        //     parser->lexer = NULL;
        // }
        safe_free(parser);
    }
}
