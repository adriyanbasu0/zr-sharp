#include "compiler.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Keywords table
static struct {
    const char* keyword;
    TokenType type;
} keywords[] = {
    {"let", TOKEN_LET},
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"while", TOKEN_WHILE},
    {"print", TOKEN_PRINT},
    {"func", TOKEN_FUNC},
    {"return", TOKEN_RETURN},
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    // Type keywords
    {"int", TOKEN_TYPE_INT},
    {"int32", TOKEN_TYPE_INT32},
    {"int64", TOKEN_TYPE_INT64},
    {"float", TOKEN_TYPE_FLOAT},
    {"bool", TOKEN_TYPE_BOOL},
    {"string", TOKEN_TYPE_STRING},
    {NULL, TOKEN_EOF} // Keep NULL terminator at the end
};

// Initialize lexer
Lexer* init_lexer(char* input) {
    Lexer* lexer = safe_malloc(sizeof(Lexer));
    lexer->input = input;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    return lexer;
}

// Skip whitespace and comments
static void skip_whitespace_and_comments(Lexer* lexer) {
    while (lexer->input[lexer->position] != '\0') {
        char c = lexer->input[lexer->position];
        
        if (isspace(c)) {
            if (c == '\n') {
                lexer->line++;
                lexer->column = 1;
            } else {
                lexer->column++;
            }
            lexer->position++;
        }
        // Skip single-line comments
        else if (c == '/' && lexer->input[lexer->position + 1] == '/') {
            while (lexer->input[lexer->position] != '\0' && 
                   lexer->input[lexer->position] != '\n') {
                lexer->position++;
            }
        }
        else {
            break;
        }
    }
}

// Read identifier or keyword
static Token read_identifier(Lexer* lexer) {
    int start = lexer->position;
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    while (isalnum(lexer->input[lexer->position]) || 
           lexer->input[lexer->position] == '_') {
        lexer->position++;
        lexer->column++;
    }
    
    int length = lexer->position - start;
    char* value = safe_malloc(length + 1);
    strncpy(value, &lexer->input[start], length);
    value[length] = '\0';
    
    // Check if it's a keyword
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(value, keywords[i].keyword) == 0) {
            token.type = keywords[i].type;
            token.text = value;
            return token;
        }
    }
    
    token.type = TOKEN_IDENT;
    token.text = value;
    return token;
}

// Read number (integer or float)
static Token read_number(Lexer* lexer) {
    Token token;
    token.type = TOKEN_NUMBER;
    token.line = lexer->line;
    token.column = lexer->column;
    
    int start = lexer->position;
    bool has_dot = false;
    
    while (isdigit(lexer->input[lexer->position]) || 
           (!has_dot && lexer->input[lexer->position] == '.')) {
        if (lexer->input[lexer->position] == '.') {
            has_dot = true;
        }
        lexer->position++;
        lexer->column++;
    }
    
    int length = lexer->position - start;
    char* value = safe_malloc(length + 1);
    strncpy(value, &lexer->input[start], length);
    value[length] = '\0';
    
    token.text = value;
    return token;
}

// Read string literal
static Token read_string(Lexer* lexer) {
    Token token;
    token.type = TOKEN_STRING;
    token.line = lexer->line;
    token.column = lexer->column;
    
    lexer->position++; // Skip opening quote
    int start = lexer->position;
    
    while (lexer->input[lexer->position] != '"' && 
           lexer->input[lexer->position] != '\0') {
        if (lexer->input[lexer->position] == '\n') {
            // Unterminated string literal due to newline
            token.type = TOKEN_EOF;
            token.text = "Unterminated string literal"; // Static string
            // lexer->position and lexer->column remain at the newline character
            return token;
        }
        lexer->position++;
        lexer->column++;
    }
    
    if (lexer->input[lexer->position] == '\0') {
        // Unterminated string literal due to EOF
        token.type = TOKEN_EOF;
        token.text = "Unterminated string literal"; // Static string
        // lexer->position and lexer->column remain at the EOF
        return token;
    }
    
    int length = lexer->position - start;
    char* value = safe_malloc(length + 1);
    strncpy(value, &lexer->input[start], length);
    value[length] = '\0';
    
    lexer->position++; // Skip closing quote
    lexer->column++;
    
    token.text = value;
    return token;
}

// Get next token
Token get_next_token(Lexer* lexer) {
    skip_whitespace_and_comments(lexer);
    
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    if (lexer->input[lexer->position] == '\0') {
        token.type = TOKEN_EOF;
        token.text = NULL;
        return token;
    }
    
    char c = lexer->input[lexer->position];
    
    if (isalpha(c) || c == '_') {
        return read_identifier(lexer);
    }
    
    if (isdigit(c)) {
        return read_number(lexer);
    }
    
    if (c == '"') {
        return read_string(lexer);
    }
    
    // For single and double character tokens, we'll set their text representation
    // The general path is to advance position and column, then set type and text.
    
    char next_char = lexer->input[lexer->position + 1]; // Peek ahead

    switch (c) {
        case '+': 
            token.type = TOKEN_PLUS; 
            token.text = strdup("+"); 
            lexer->position++; lexer->column++;
            break;
        case '-': 
            token.type = TOKEN_MINUS; 
            token.text = strdup("-"); 
            lexer->position++; lexer->column++;
            break;
        case '*': 
            token.type = TOKEN_STAR; 
            token.text = strdup("*"); 
            lexer->position++; lexer->column++;
            break;
        case '/': 
            token.type = TOKEN_SLASH; 
            token.text = strdup("/"); 
            lexer->position++; lexer->column++;
            break;
        case '(': 
            token.type = TOKEN_LPAREN; 
            token.text = strdup("("); 
            lexer->position++; lexer->column++;
            break;
        case ')': 
            token.type = TOKEN_RPAREN; 
            token.text = strdup(")"); 
            lexer->position++; lexer->column++;
            break;
        case '{': 
            token.type = TOKEN_LBRACE; 
            token.text = strdup("{"); 
            lexer->position++; lexer->column++;
            break;
        case '}': 
            token.type = TOKEN_RBRACE; 
            token.text = strdup("}"); 
            lexer->position++; lexer->column++;
            break;
        case ';': 
            token.type = TOKEN_SEMICOLON; 
            token.text = strdup(";"); 
            lexer->position++; lexer->column++;
            break;
        case ',': 
            token.type = TOKEN_COMMA; 
            token.text = strdup(","); 
            lexer->position++; lexer->column++;
            break;
        case ':':
            token.type = TOKEN_COLON;
            token.text = strdup(":");
            lexer->position++; lexer->column++;
            break;
        case '=':
            if (next_char == '=') {
                token.type = TOKEN_EQEQ;
                token.text = strdup("==");
                lexer->position += 2; lexer->column += 2;
            } else {
                token.type = TOKEN_EQ;
                token.text = strdup("=");
                lexer->position++; lexer->column++;
            }
            break;
        case '<':
            if (next_char == '=') {
                token.type = TOKEN_LTEQ;
                token.text = strdup("<=");
                lexer->position += 2; lexer->column += 2;
            } else {
                token.type = TOKEN_LT;
                token.text = strdup("<");
                lexer->position++; lexer->column++;
            }
            break;
        case '>':
            if (next_char == '=') {
                token.type = TOKEN_GTEQ;
                token.text = strdup(">=");
                lexer->position += 2; lexer->column += 2;
            } else {
                token.type = TOKEN_GT;
                token.text = strdup(">");
                lexer->position++; lexer->column++;
            }
            break;
        case '!':
            if (next_char == '=') {
                token.type = TOKEN_NOTEQ;
                token.text = strdup("!=");
                lexer->position += 2; lexer->column += 2;
            } else {
                // Assuming '!' is TOKEN_NOT as per original logic for single '!'
                token.type = TOKEN_NOT;
                token.text = strdup("!");
                lexer->position++; lexer->column++;
            }
            break;
        case '&':
            if (next_char == '&') {
                token.type = TOKEN_AND; // Assuming && maps to TOKEN_AND
                token.text = strdup("&&");
                lexer->position += 2; lexer->column += 2;
            } else {
                error("Invalid character '&' at line %d, column %d. Did you mean '&&'?", 
                      token.line, token.column);
                token.type = TOKEN_EOF;
                token.text = NULL;
                lexer->position++; lexer->column++; // Consume the invalid char
            }
            break;
        case '|':
            if (next_char == '|') {
                token.type = TOKEN_OR; // Assuming || maps to TOKEN_OR
                token.text = strdup("||");
                lexer->position += 2; lexer->column += 2;
            } else {
                error("Invalid character '|' at line %d, column %d. Did you mean '||'?", 
                      token.line, token.column);
                token.type = TOKEN_EOF;
                token.text = NULL;
                lexer->position++; lexer->column++; // Consume the invalid char
            }
            break;
        default:
            error("Invalid character '%c' at line %d, column %d", 
                  c, token.line, token.column);
            token.type = TOKEN_EOF; // Or some error token type
            token.text = NULL; // No text for invalid char
            lexer->position++; lexer->column++; // Consume the invalid char
    }
    
    return token;
}

// Free lexer resources
void free_lexer(Lexer* lexer) {
    safe_free(lexer); // Assumes safe_free handles NULL
}

// Implementation of safe_malloc
void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed (size: %zu).\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Implementation of safe_free
void safe_free(void* ptr) {
    if (ptr != NULL) {
        free(ptr);
    }
}
