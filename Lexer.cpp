#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <map>

Lexer::Lexer(std::string source) : source_(std::move(source)), pos_(0), line_(1), col_(1) {}

Token Lexer::next() {
    skipWhitespaceAndComments();
    if (pos_ >= source_.size()) return makeToken(TokenType::END_OF_FILE, "");

    char c = source_[pos_];
    if (std::isdigit(c) || (c == '.' && pos_+1 < source_.size() && std::isdigit(source_[pos_+1])))
        return scanNumber();
    if (std::isalpha(c) || c == '_')
        return scanIdentifier();
    if (c == '"' || c == '\'')
        return scanString(c);

    switch (c) {
        case '+': advance(); return makeToken(TokenType::PLUS, "+");
        case '-': advance(); return makeToken(TokenType::MINUS, "-");
        case '*': advance(); return makeToken(TokenType::STAR, "*");
        case '/': advance(); return makeToken(TokenType::SLASH, "/");
        case '%': advance(); return makeToken(TokenType::PERCENT, "%");
        case '^': advance(); return makeToken(TokenType::CARET, "^");
        case '~': advance(); return makeToken(TokenType::TILDE, "~");
        case '(': advance(); return makeToken(TokenType::LPAREN, "(");
        case ')': advance(); return makeToken(TokenType::RPAREN, ")");
        case '{': advance(); return makeToken(TokenType::LBRACE, "{");
        case '}': advance(); return makeToken(TokenType::RBRACE, "}");
        case '[': advance(); return makeToken(TokenType::LBRACKET, "[");
        case ']': advance(); return makeToken(TokenType::RBRACKET, "]");
        case ',': advance(); return makeToken(TokenType::COMMA, ",");
        case ':': advance(); return makeToken(TokenType::COLON, ":");
        case ';': advance(); return makeToken(TokenType::SEMICOLON, ";");
        case '?': advance(); return makeToken(TokenType::QUESTION, "?");

        case '=':
            if (peek(1) == '=') { advance(); advance(); return makeToken(TokenType::EQ, "=="); }
            advance(); return makeToken(TokenType::ASSIGN, "=");

        case '!':
            if (peek(1) == '=') { advance(); advance(); return makeToken(TokenType::NEQ, "!="); }
            advance(); return makeToken(TokenType::BANG, "!");

        case '<':
            if (peek(1) == '=') { advance(); advance(); return makeToken(TokenType::LE, "<="); }
            advance(); return makeToken(TokenType::LT, "<");

        case '>':
            if (peek(1) == '=') { advance(); advance(); return makeToken(TokenType::GE, ">="); }
            advance(); return makeToken(TokenType::GT, ">");

        case '.':
            if (peek(1) == '.') { advance(); advance(); return makeToken(TokenType::DOTDOT, ".."); }
            throwError("Unexpected '.'");
            break;

        case '&': advance(); return makeToken(TokenType::AMP, "&");
        case '|': 
            if (peek(1) == '|') { advance(); advance(); return makeToken(TokenType::PIPE, "||"); }
            throwError("Expected '||', got '|' alone");
            break;

        default:
            throwError(std::string("Unexpected character '") + c + "'");
    }
    return makeToken(TokenType::END_OF_FILE, "");
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < source_.size()) {
        char c = source_[pos_];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (c == '\n') { line_++; col_ = 1; }
            else col_++;
            pos_++;
        } else if (c == '/' && pos_+1 < source_.size()) {
            if (source_[pos_+1] == '/') {
                pos_ += 2; col_ += 2;
                while (pos_ < source_.size() && source_[pos_] != '\n') { pos_++; col_++; }
            } else if (source_[pos_+1] == '*') {
                pos_ += 2; col_ += 2;
                while (pos_ < source_.size()) {
                    if (source_[pos_] == '*' && pos_+1 < source_.size() && source_[pos_+1] == '/') {
                        pos_ += 2; col_ += 2;
                        break;
                    }
                    if (source_[pos_] == '\n') { line_++; col_ = 1; }
                    else col_++;
                    pos_++;
                }
            } else break;
        } else break;
    }
}

Token Lexer::scanNumber() {
    size_t start = pos_;
    while (pos_ < source_.size() && (std::isdigit(source_[pos_]) || source_[pos_] == '.')) {
        pos_++; col_++;
    }
    std::string numStr = source_.substr(start, pos_ - start);
    double val = std::stod(numStr);
    Token tok = makeToken(TokenType::NUMBER, numStr);
    tok.numberValue = val;
    return tok;
}

Token Lexer::scanIdentifier() {
    size_t start = pos_;
    while (pos_ < source_.size() && (std::isalnum(source_[pos_]) || source_[pos_] == '_')) {
        pos_++; col_++;
    }
    std::string ident = source_.substr(start, pos_ - start);
    
    static const std::map<std::string, TokenType> keywords = {
        {"var", TokenType::VAR}, {"val", TokenType::VAL}, {"global", TokenType::GLOBAL},
        {"func", TokenType::FUNC}, {"if", TokenType::IF}, {"elif", TokenType::ELIF},
        {"else", TokenType::ELSE}, {"for", TokenType::FOR}, {"while", TokenType::WHILE},
        {"break", TokenType::BREAK}, {"continue", TokenType::CONTINUE},
        {"in", TokenType::IN}, {"and", TokenType::AND}, {"or", TokenType::OR},
        {"not", TokenType::NOT}, {"true", TokenType::TRUE}, {"false", TokenType::FALSE},
        {"none", TokenType::NONE}, {"import", TokenType::IMPORT}, {"from", TokenType::FROM},
        {"block", TokenType::BLOCK}, {"return", TokenType::RETURN}, {"is", TokenType::IS}
    };
    
    auto it = keywords.find(ident);
    if (it != keywords.end()) return makeToken(it->second, ident);
    return makeToken(TokenType::IDENTIFIER, ident);
}

Token Lexer::scanString(char quote) {
    pos_++; col_++;
    std::string str;
    while (pos_ < source_.size() && source_[pos_] != quote) {
        char c = source_[pos_];
        if (c == '\\' && pos_+1 < source_.size()) {
            pos_++; col_++;
            switch (source_[pos_]) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '"': c = '"'; break;
                case '\'': c = '\''; break;
                default: c = source_[pos_]; break;
            }
        }
        str += c;
        pos_++; col_++;
    }
    if (pos_ >= source_.size()) throwError("Unterminated string");
    pos_++; col_++;
    return makeToken(TokenType::STRING, str);
}

char Lexer::peek(size_t ahead) const {
    if (pos_ + ahead < source_.size()) return source_[pos_ + ahead];
    return '\0';
}

void Lexer::advance() {
    if (source_[pos_] == '\n') { line_++; col_ = 1; }
    else col_++;
    pos_++;
}

Token Lexer::makeToken(TokenType type, std::string lexeme) {
    Token t;
    t.type = type;
    t.lexeme = std::move(lexeme);
    t.line = line_;
    t.column = col_;
    return t;
}

void Lexer::throwError(const std::string& msg) {
    throw std::runtime_error("Lexer error at line " + std::to_string(line_) + ":"
                             + std::to_string(col_) + " - " + msg);
}
