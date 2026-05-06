#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "AST.h"
#include <vector>

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::vector<StmtPtr> parse();

private:
    Lexer& lexer_;
    Token current_;

    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& msg);

    // Statements
    StmtPtr parseStatement();
    StmtPtr parseVariableDeclaration(bool isConst, bool isGlobal);
    StmtPtr parseFuncDecl();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseForStmt();
    std::vector<StmtPtr> parseBlock();
    void parseOptionalSemicolon();

    // Expressions
    ExprPtr parseExpression();
    ExprPtr parseAssignment();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseConcat();
    ExprPtr parseAdditive();
    ExprPtr parseMultiplicative();
    ExprPtr parseUnary();
    ExprPtr parsePostfix();
    ExprPtr parsePrimary();
    ExprPtr parseTableLiteral();

    // Helpers
    ExprPtr finishCall(ExprPtr callee);
    Value evaluateExprConst(Expr* expr);
    [[noreturn]] void throwError(const std::string& msg);
};

#endif
