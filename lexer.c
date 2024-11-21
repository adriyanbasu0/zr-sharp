#include "compiler.h"
#include <ctype.h>
#include <string.h>

// Read the next character
static void read_char(Lexer* lexer) {
    if (lexer->read_position >= strlen(lexer->source)) {
        lexer->current_char = '\0';
    } else {
        lexer->current_char = lexer->source[lexer->read_position];
    }
    
    lexer->position = lexer->read_position;
    lexer->read_position++;
    
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 0;
    } else {
        lexer->column++;
    }
}

Lexer* init_lexer(const char* source) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->position = 0;
    lexer->read_position = 0;
    lexer->current_char = '\0';
    lexer->line = 1;
    lexer->column = 0;
    
    // Read first character
    read_char(lexer);
    
    return lexer;
}

// Skip whitespace and comments
static void skip_whitespace_and_comments(Lexer* lexer) {
    while (1) {
        // Skip whitespace
        while (isspace(lexer->current_char)) {
            read_char(lexer);
        }
        
        // Skip single-line comments
        if (lexer->current_char == '/' && lexer->source[lexer->read_position] == '/') {
            while (lexer->current_char != '\n' && lexer->current_char != '\0') {
                read_char(lexer);
            }
            continue;
        }
        break;
    }
}

// Read an identifier
static char* read_identifier(Lexer* lexer) {
    int start_pos = lexer->position;
    while (isalnum(lexer->current_char) || lexer->current_char == '_') {
        read_char(lexer);
    }
    
    int length = lexer->position - start_pos;
    char* identifier = malloc(length + 1);
    strncpy(identifier, &lexer->source[start_pos], length);
    identifier[length] = '\0';
    
    return identifier;
}

// Read a number
static char* read_number(Lexer* lexer) {
    int start_pos = lexer->position;
    while (isdigit(lexer->current_char) || lexer->current_char == '.') {
        read_char(lexer);
    }
    
    int length = lexer->position - start_pos;
    char* number = malloc(length + 1);
    strncpy(number, &lexer->source[start_pos], length);
    number[length] = '\0';
    
    return number;
}

// Get token type for identifier
static TokenType get_identifier_type(const char* identifier) {
    if (strcmp(identifier, "let") == 0) return TOKEN_LET;
    if (strcmp(identifier, "if") == 0) return TOKEN_IF;
    if (strcmp(identifier, "else") == 0) return TOKEN_ELSE;
    if (strcmp(identifier, "print") == 0) return TOKEN_PRINT;
    return TOKEN_IDENTIFIER;
}

// Get the next token from the source code
Token get_next_token(Lexer* lexer) {
    Token token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    skip_whitespace_and_comments(lexer);
    
    switch (lexer->current_char) {
        case '\0':
            token.type = TOKEN_EOF;
            token.value = NULL;
            break;
            
        case '+':
            token.type = TOKEN_PLUS;
            token.value = strdup("+");
            read_char(lexer);
            break;
            
        case '-':
            token.type = TOKEN_MINUS;
            token.value = strdup("-");
            read_char(lexer);
            break;
            
        case '*':
            token.type = TOKEN_MULTIPLY;
            token.value = strdup("*");
            read_char(lexer);
            break;
            
        case '/':
            token.type = TOKEN_DIVIDE;
            token.value = strdup("/");
            read_char(lexer);
            break;
            
        case '=':
            if (lexer->source[lexer->read_position] == '=') {
                token.type = TOKEN_EQ;
                token.value = strdup("==");
                read_char(lexer);
                read_char(lexer);
            } else {
                token.type = TOKEN_ASSIGN;
                token.value = strdup("=");
                read_char(lexer);
            }
            break;
            
        case '>':
            token.type = TOKEN_GT;
            token.value = strdup(">");
            read_char(lexer);
            break;
            
        case '<':
            token.type = TOKEN_LT;
            token.value = strdup("<");
            read_char(lexer);
            break;
            
        case '(':
            token.type = TOKEN_LPAREN;
            token.value = strdup("(");
            read_char(lexer);
            break;
            
        case ')':
            token.type = TOKEN_RPAREN;
            token.value = strdup(")");
            read_char(lexer);
            break;
            
        case '{':
            token.type = TOKEN_LBRACE;
            token.value = strdup("{");
            read_char(lexer);
            break;
            
        case '}':
            token.type = TOKEN_RBRACE;
            token.value = strdup("}");
            read_char(lexer);
            break;
            
        case ';':
            token.type = TOKEN_SEMICOLON;
            token.value = strdup(";");
            read_char(lexer);
            break;
            
        default:
            if (isalpha(lexer->current_char) || lexer->current_char == '_') {
                token.value = read_identifier(lexer);
                token.type = get_identifier_type(token.value);
            } else if (isdigit(lexer->current_char)) {
                token.value = read_number(lexer);
                token.type = TOKEN_NUMBER;
            } else {
                token.type = TOKEN_EOF;
                token.value = NULL;
                read_char(lexer);
            }
            break;
    }
    
    return token;
}
