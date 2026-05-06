#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <string>
#include <map>

class Lexer {
public:
    explicit Lexer(std::string source);
    Token next();

private:
    std::string source_;
    size_t pos_;
    int line_, col_;

    void skipWhitespaceAndComments();
    Token scanNumber();
    Token scanIdentifier();
    Token scanString(char quote);
    char peek(size_t ahead = 1) const;
    void advance();
    Token makeToken(TokenType type, std::string lexeme);
    [[noreturn]] void throwError(const std::string& msg);
};

#endif
