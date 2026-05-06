#include "Parser.h"
#include <stdexcept>

Parser::Parser(Lexer& lexer) : lexer_(lexer) {
    advance();
}

std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> statements;
    while (!check(TokenType::END_OF_FILE))
        statements.push_back(parseStatement());
    return statements;
}

void Parser::advance() {
    current_ = lexer_.next();
}

bool Parser::check(TokenType type) const {
    return current_.type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

Token Parser::consume(TokenType type, const std::string& msg) {
    if (check(type)) { Token t = current_; advance(); return t; }
    throwError(msg);
}

// ========== STATEMENTS ==========

StmtPtr Parser::parseStatement() {
    if (check(TokenType::VAR) || check(TokenType::VAL) || check(TokenType::GLOBAL)) {
        bool isConst = false, isGlobal = false;
        if (match(TokenType::GLOBAL)) {
            isGlobal = true;
            if (match(TokenType::VAR)) isConst = false;
            else if (match(TokenType::VAL)) isConst = true;
            else throwError("Expected 'var' or 'val' after 'global'");
        } else if (match(TokenType::VAR)) {
            isConst = false;
            isGlobal = false;
        } else if (match(TokenType::VAL)) {
            isConst = true;
            isGlobal = false;
        }
        return parseVariableDeclaration(isConst, isGlobal);
    }
    
    if (match(TokenType::FUNC)) return parseFuncDecl();
    if (match(TokenType::IF)) return parseIfStmt();
    if (match(TokenType::WHILE)) return parseWhileStmt();
    if (match(TokenType::FOR)) return parseForStmt();
    if (match(TokenType::BREAK)) { parseOptionalSemicolon(); return std::make_unique<BreakStmt>(); }
    if (match(TokenType::CONTINUE)) { parseOptionalSemicolon(); return std::make_unique<ContinueStmt>(); }
    
    if (match(TokenType::RETURN)) {
        ExprPtr val = nullptr;
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) val = parseExpression();
        parseOptionalSemicolon();
        return std::make_unique<ReturnStmt>(std::move(val));
    }
    
    if (match(TokenType::BLOCK)) {
        consume(TokenType::LBRACE, "Expected '{' after block");
        auto stmts = parseBlock();
        return std::make_unique<BlockStmt>(std::move(stmts));
    }
    
    if (match(TokenType::IMPORT)) {
        std::string mod = consume(TokenType::IDENTIFIER, "Expected module name").lexeme;
        parseOptionalSemicolon();
        return std::make_unique<ImportStmt>(mod, false);
    }
    
    if (match(TokenType::FROM)) {
        std::string mod = consume(TokenType::IDENTIFIER, "Expected module name").lexeme;
        consume(TokenType::IMPORT, "Expected 'import' after module name");
        auto stmt = std::make_unique<ImportStmt>(mod, true);
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
            do {
                stmt->specificImports.push_back(consume(TokenType::IDENTIFIER, "Expected import name").lexeme);
            } while (match(TokenType::COMMA));
        }
        parseOptionalSemicolon();
        return stmt;
    }
    
    ExprPtr expr = parseExpression();
    if (match(TokenType::ASSIGN)) {
        ExprPtr value = parseExpression();
        parseOptionalSemicolon();
        if (auto* idExpr = dynamic_cast<IdentifierExpr*>(expr.get())) {
            return std::make_unique<AssignmentStmt>(idExpr->name, std::move(value));
        } else if (auto* idxExpr = dynamic_cast<IndexExpr*>(expr.get())) {
            return std::make_unique<AssignmentStmt>(std::move(idxExpr->object), std::move(idxExpr->index), std::move(value));
        } else throwError("Invalid left-hand side in assignment");
    }
    if (match(TokenType::TILDE)) {
        parseOptionalSemicolon();
        return std::make_unique<ReturnStmt>(std::move(expr));
    }
    parseOptionalSemicolon();
    return std::make_unique<ExprStmt>(std::move(expr));
}

StmtPtr Parser::parseVariableDeclaration(bool isConst, bool isGlobal) {
    std::string name = consume(TokenType::IDENTIFIER, "Expected variable name").lexeme;
    std::string typeAnnotation;
    bool isOptional = false;
    
    if (match(TokenType::COLON)) {
        typeAnnotation = consume(TokenType::IDENTIFIER, "Expected type name").lexeme;
        if (match(TokenType::QUESTION)) {
            isOptional = true;
        }
    }
    
    ExprPtr init = nullptr;
    if (match(TokenType::ASSIGN)) init = parseExpression();
    parseOptionalSemicolon();
    
    auto stmt = std::make_unique<VarDeclStmt>(name, isConst, isGlobal, std::move(init));
    stmt->typeAnnotation = typeAnnotation;
    stmt->isOptional = isOptional;
    return stmt;
}

StmtPtr Parser::parseFuncDecl() {
    std::string name = consume(TokenType::IDENTIFIER, "Expected function name").lexeme;
    consume(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters");
    consume(TokenType::LBRACE, "Expected '{' before function body");
    auto body = parseBlock();
    return std::make_unique<FuncDeclStmt>(name, params, std::make_unique<BlockStmt>(std::move(body)));
}

StmtPtr Parser::parseIfStmt() {
    ExprPtr cond = parseExpression();
    consume(TokenType::LBRACE, "Expected '{' after if condition");
    auto thenBlock = std::make_unique<BlockStmt>(parseBlock());
    StmtPtr elseBlock = nullptr;
    
    if (check(TokenType::ELIF)) {
        advance();
        elseBlock = parseIfStmt();
    } else if (check(TokenType::ELSE)) {
        advance();
        if (check(TokenType::IF)) {
            advance();
            elseBlock = parseIfStmt();
        } else {
            consume(TokenType::LBRACE, "Expected '{' after else");
            elseBlock = std::make_unique<BlockStmt>(parseBlock());
        }
    }
    
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBlock), std::move(elseBlock));
}

StmtPtr Parser::parseWhileStmt() {
    ExprPtr cond = parseExpression();
    consume(TokenType::LBRACE, "Expected '{' after while condition");
    auto body = std::make_unique<BlockStmt>(parseBlock());
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

StmtPtr Parser::parseForStmt() {
    std::string name = consume(TokenType::IDENTIFIER, "Expected loop variable").lexeme;
    
    if (match(TokenType::ASSIGN)) {
        ExprPtr start = parseExpression();
        consume(TokenType::COMMA, "Expected ',' after start value");
        ExprPtr end = parseExpression();
        ExprPtr step = nullptr;
        if (match(TokenType::COMMA)) step = parseExpression();
        consume(TokenType::LBRACE, "Expected '{' before for body");
        return std::make_unique<ForNumericStmt>(name, std::move(start), std::move(end), std::move(step),
                                                std::make_unique<BlockStmt>(parseBlock()));
    } else if (match(TokenType::COMMA)) {
        std::vector<std::string> vars = {name};
        vars.push_back(consume(TokenType::IDENTIFIER, "Expected second variable name in for-in").lexeme);
        consume(TokenType::IN, "Expected 'in' after for-in variables");
        ExprPtr iter = parseExpression();
        consume(TokenType::LBRACE, "Expected '{' before for-in body");
        return std::make_unique<ForInStmt>(std::move(vars), std::move(iter), 
                                           std::make_unique<BlockStmt>(parseBlock()));
    } else if (check(TokenType::IN)) {
        consume(TokenType::IN, "Expected 'in'");
        ExprPtr iter = parseExpression();
        consume(TokenType::LBRACE, "Expected '{'");
        return std::make_unique<ForInStmt>(std::vector<std::string>{name}, std::move(iter),
                                           std::make_unique<BlockStmt>(parseBlock()));
    } else {
        throwError("Invalid for loop syntax");
    }
}

std::vector<StmtPtr> Parser::parseBlock() {
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE))
        stmts.push_back(parseStatement());
    consume(TokenType::RBRACE, "Expected '}' after block");
    return stmts;
}

void Parser::parseOptionalSemicolon() {
    if (match(TokenType::SEMICOLON)) {}
}

// ========== EXPRESSIONS ==========

ExprPtr Parser::parseExpression() {
    return parseAssignment();
}

ExprPtr Parser::parseAssignment() {
    return parseOr();
}

ExprPtr Parser::parseOr() {
    ExprPtr expr = parseAnd();
    while (check(TokenType::OR) || check(TokenType::PIPE)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseAnd() {
    ExprPtr expr = parseEquality();
    while (check(TokenType::AND) || check(TokenType::AMP)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseEquality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseEquality() {
    ExprPtr expr = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseComparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseComparison() {
    ExprPtr expr = parseConcat();
    while (check(TokenType::LT) || check(TokenType::GT) || check(TokenType::LE) || check(TokenType::GE)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseConcat();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseConcat() {
    ExprPtr expr = parseAdditive();
    while (check(TokenType::DOTDOT)) {
        advance();
        ExprPtr right = parseAdditive();
        expr = std::make_unique<BinaryExpr>(std::move(expr), TokenType::DOTDOT, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseAdditive() {
    ExprPtr expr = parseMultiplicative();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseMultiplicative();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseMultiplicative() {
    ExprPtr expr = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        TokenType op = current_.type;
        advance();
        ExprPtr right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

ExprPtr Parser::parseUnary() {
    // FIX: Capture operator BEFORE advancing, not after
    if (match(TokenType::MINUS)) {
        ExprPtr operand = parseUnary();
        return std::make_unique<UnaryExpr>(TokenType::MINUS, std::move(operand));
    }
    if (match(TokenType::BANG)) {
        ExprPtr operand = parseUnary();
        return std::make_unique<UnaryExpr>(TokenType::BANG, std::move(operand));
    }
    if (match(TokenType::NOT)) {
        ExprPtr operand = parseUnary();
        return std::make_unique<UnaryExpr>(TokenType::NOT, std::move(operand));
    }
    return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
    ExprPtr expr = parsePrimary();
    while (true) {
        if (match(TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(TokenType::LBRACKET)) {
            ExprPtr idx = parseExpression();
            consume(TokenType::RBRACKET, "Expected ']'");
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(idx));
        } else break;
    }
    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> args;
    if (!check(TokenType::RPAREN)) {
        do {
            args.push_back(parseExpression());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after arguments");
    return std::make_unique<CallExpr>(std::move(callee), std::move(args));
}

ExprPtr Parser::parsePrimary() {
    if (check(TokenType::NUMBER)) {
        Token numToken = consume(TokenType::NUMBER, "Expected number");
        return std::make_unique<LiteralExpr>(Value(numToken.numberValue));
    }
    if (check(TokenType::STRING)) {
        Token strToken = consume(TokenType::STRING, "Expected string");
        return std::make_unique<LiteralExpr>(Value(strToken.lexeme));
    }
    if (match(TokenType::TRUE))  return std::make_unique<LiteralExpr>(Value(true));
    if (match(TokenType::FALSE)) return std::make_unique<LiteralExpr>(Value(false));
    if (match(TokenType::NONE))  return std::make_unique<LiteralExpr>(Value());
    
    if (check(TokenType::IDENTIFIER)) {
        Token idToken = consume(TokenType::IDENTIFIER, "Expected identifier");
        return std::make_unique<IdentifierExpr>(idToken.lexeme);
    }
    
    if (match(TokenType::LPAREN)) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    if (match(TokenType::LBRACE)) return parseTableLiteral();
    
    throwError("Unexpected token " + current_.lexeme);
}

ExprPtr Parser::parseTableLiteral() {
    auto table = std::make_shared<Table>();
    if (!check(TokenType::RBRACE)) {
        do {
            if (check(TokenType::IDENTIFIER) || check(TokenType::STRING)) {
                ExprPtr keyExpr = parsePrimary();
                if (match(TokenType::ASSIGN)) {
                    Value keyVal;
                    if (auto* id = dynamic_cast<IdentifierExpr*>(keyExpr.get())) keyVal = Value(id->name);
                    else if (auto* lit = dynamic_cast<LiteralExpr*>(keyExpr.get())) keyVal = lit->value;
                    else throwError("Invalid table key");
                    ExprPtr valExpr = parseExpression();
                    table->set(keyVal, evaluateExprConst(valExpr.get()));
                } else {
                    table->array.push_back(evaluateExprConst(keyExpr.get()));
                }
            } else {
                ExprPtr expr = parseExpression();
                table->array.push_back(evaluateExprConst(expr.get()));
            }
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RBRACE, "Expected '}' at end of table literal");
    return std::make_unique<LiteralExpr>(Value(table));
}

Value Parser::evaluateExprConst(Expr* expr) {
    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) return lit->value;
    if (auto* id = dynamic_cast<IdentifierExpr*>(expr)) return Value(id->name);
    throwError("Invalid expression in table literal");
}

void Parser::throwError(const std::string& msg) {
    throw std::runtime_error("Parse error at line " + std::to_string(current_.line) + ":"
                             + std::to_string(current_.column) + " - " + msg);
}
