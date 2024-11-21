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
    {NULL, TOKEN_EOF}
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
            error("Unterminated string literal at line %d", lexer->line);
        }
        lexer->position++;
        lexer->column++;
    }
    
    if (lexer->input[lexer->position] == '\0') {
        error("Unterminated string literal at line %d", lexer->line);
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
    
    lexer->position++;
    lexer->column++;
    
    switch (c) {
        case '+': token.type = TOKEN_PLUS; break;
        case '-': token.type = TOKEN_MINUS; break;
        case '*': token.type = TOKEN_STAR; break;
        case '/': token.type = TOKEN_SLASH; break;
        case '(': token.type = TOKEN_LPAREN; break;
        case ')': token.type = TOKEN_RPAREN; break;
        case '{': token.type = TOKEN_LBRACE; break;
        case '}': token.type = TOKEN_RBRACE; break;
        case ';': token.type = TOKEN_SEMICOLON; break;
        case ',': token.type = TOKEN_COMMA; break;
        case '=':
            if (lexer->input[lexer->position] == '=') {
                token.type = TOKEN_EQEQ;
                lexer->position++;
                lexer->column++;
            } else {
                token.type = TOKEN_EQ;
            }
            break;
        case '<':
            if (lexer->input[lexer->position] == '=') {
                token.type = TOKEN_LTEQ;
                lexer->position++;
                lexer->column++;
            } else {
                token.type = TOKEN_LT;
            }
            break;
        case '>':
            if (lexer->input[lexer->position] == '=') {
                token.type = TOKEN_GTEQ;
                lexer->position++;
                lexer->column++;
            } else {
                token.type = TOKEN_GT;
            }
            break;
        case '!':
            if (lexer->input[lexer->position] == '=') {
                token.type = TOKEN_NOTEQ;
                lexer->position++;
                lexer->column++;
            } else {
                token.type = TOKEN_NOT;
            }
            break;
        default:
            error("Invalid character '%c' at line %d, column %d", 
                  c, token.line, token.column);
            token.type = TOKEN_EOF;
    }
    
    token.text = NULL;
    return token;
}

// Free lexer resources
void free_lexer(Lexer* lexer) {
    safe_free(lexer);
}
