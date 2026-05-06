#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Keywords
    VAR, VAL, GLOBAL, FUNC, IF, ELIF, ELSE, FOR, WHILE, BREAK, CONTINUE,
    IN, AND, OR, NOT, TRUE, FALSE, NONE, IMPORT, FROM, BLOCK, RETURN, IS,
    
    // Identifiers and literals
    IDENTIFIER, NUMBER, STRING,
    
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, CARET, DOTDOT,
    EQ, NEQ, LT, GT, LE, GE, ASSIGN,
    AMP, PIPE, BANG, TILDE,
    
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    COMMA, COLON, QUESTION, SEMICOLON,
    
    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string lexeme;
    double numberValue;
    int line;
    int column;
};

#endif
