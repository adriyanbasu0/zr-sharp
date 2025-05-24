#include "compiler.h"
#include <stdarg.h> // For va_list, va_start, va_end
#include <stdio.h>
#include <stdlib.h>

// Function to report errors and exit
void error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: "); // Prefix with "Error: "
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n"); // Add a newline for better formatting
    va_end(args);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source_file>\n", argv[0]);
        return 1;
    }
    
    // Read source file
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", argv[1]);
        return 1;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    char* source = safe_malloc(size + 1); // Use safe_malloc
    // safe_malloc handles exit on failure, so no NULL check needed here
    
    size_t read_size = fread(source, 1, size, file);
    source[read_size] = '\0';
    fclose(file);
    
    // Initialize lexer and parser
    Lexer* lexer = init_lexer(source);
    Parser* parser = init_parser(lexer);
    
    // Parse and interpret
    ASTNode* program = parse_program(parser);
    if (program != NULL) {
        interpret(program);
        // Main success path cleanup:
        free_ast(program);           // 1. Free AST // Restored
        free_interpreter_memory();   // 2. Free interpreter memory
        if (parser) free_parser(parser); // 3. Free parser
        if (lexer) free_lexer(lexer);   // 4. Free lexer
        safe_free(source);                // 5. Free source code string with safe_free
    } else {
        fprintf(stderr, "Error: Failed to parse program\n");
        // Error path cleanup (program is NULL):
        if (parser) free_parser(parser); // 1. Free parser
        if (lexer) free_lexer(lexer);   // 2. Free lexer
        safe_free(source);                // 3. Free source code string with safe_free
        return 1;
    }
    
    // Normal exit, all resources should have been freed in the if (program != NULL) block.
    // No duplicate free calls here.
    return 0;
}
