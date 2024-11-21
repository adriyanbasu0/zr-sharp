#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

// Read the entire contents of a file
char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        printf("Error: Could not allocate memory for file contents\n");
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, length, file);
    buffer[read] = '\0';
    
    fclose(file);
    return buffer;
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
    char* source = malloc(size + 1);
    if (source == NULL) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    
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
    } else {
        fprintf(stderr, "Error: Failed to parse program\n");
        return 1;
    }
    
    // Clean up
    free(source);
    
    return 0;
}
