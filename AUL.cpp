#include "AUL.h"
#include <cctype>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <fstream>

namespace AUL {

// ----------------------------------------------------------------------
//  Value implementation
// ----------------------------------------------------------------------
std::string Value::toString() const {
    switch (type) {
        case NONE: return "none";
        case NUMBER: {
            // Format number without trailing zeros
            if (num == (double)(int)num) {
                // It's a whole number, print without decimal
                return std::to_string((int)num);
            } else {
                // Format with some precision and remove trailing zeros
                std::stringstream ss;
                ss << std::fixed << std::setprecision(6) << num;
                std::string result = ss.str();
                // Remove trailing zeros after decimal point
                size_t dot_pos = result.find('.');
                if (dot_pos != std::string::npos) {
                    result.erase(result.find_last_not_of('0') + 1);
                    if (result.back() == '.') result.pop_back();
                }
                return result;
            }
        }
        case BOOL: return boolean ? "true" : "false";
        case STRING: return *str;
        case TABLE: return "<table>";
        case NATIVE_FUNC: return "<native fn>";
        case USER_FUNC: return "<function>";
    }
    return "?";
}

bool operator==(const Value& a, const Value& b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case Value::NONE: return true;
        case Value::NUMBER: return a.num == b.num;
        case Value::BOOL: return a.boolean == b.boolean;
        case Value::STRING: return *a.str == *b.str;
        case Value::TABLE: return a.table == b.table;
        default: return false;
    }
}
bool operator!=(const Value& a, const Value& b) { return !(a == b); }

std::size_t ValueHash::operator()(const Value& v) const {
    switch (v.type) {
        case Value::NONE: return 0;
        case Value::NUMBER: return std::hash<double>{}(v.num);
        case Value::BOOL: return std::hash<bool>{}(v.boolean);
        case Value::STRING: return std::hash<std::string>{}(*v.str);
        case Value::TABLE: return std::hash<Table*>{}(v.table.get());
        default: return std::hash<int>{}(v.type);
    }
}

// ----------------------------------------------------------------------
//  Environment
// ----------------------------------------------------------------------
Environment::Environment(std::shared_ptr<Environment> parent) : parent(parent) {}

void Environment::define(const std::string& name, const Value& value, bool mutableFlag) {
    values[name] = value;
    mutableFlags[name] = mutableFlag;
}

void Environment::assign(const std::string& name, const Value& value) {
    if (values.find(name) != values.end()) {
        if (!mutableFlags[name]) {
            throw std::runtime_error("Error: cannot reassign immutable variable '" + name + "' (declared with 'val')");
        }
        values[name] = value;
        return;
    }
    if (parent) {
        parent->assign(name, value);
        return;
    }
    throw std::runtime_error("Error: undefined variable '" + name + "' - variable must be declared before assignment");
}

Value Environment::get(const std::string& name) const {
    auto it = values.find(name);
    if (it != values.end()) return it->second;
    if (parent) return parent->get(name);
    throw std::runtime_error("Error: undefined variable '" + name + "' - variable must be declared before use");
}

bool Environment::isMutable(const std::string& name) const {
    auto it = mutableFlags.find(name);
    if (it != mutableFlags.end()) return it->second;
    if (parent) return parent->isMutable(name);
    return true; // default mutable if not found (should not happen)
}

// ----------------------------------------------------------------------
//  Lexer & Tokens
// ----------------------------------------------------------------------
enum class TokenType {
    IDENT, NUMBER, STRING, KEYWORD,
    PLUS, MINUS, STAR, SLASH, PERCENT, CARET, DOT, DOTDOT,   // arithmetic & concat
    EQ, EQEQ, BANG, BANGEQ, LT, LTE, GT, GTE,           // comparison
    AND, OR, NOT, AND_SYM, OR_SYM, NOT_SYM,              // logical/bitwise
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACK, RBRACK,
    COMMA, SEMICOLON, TILDE,
    END
};

struct Token {
    TokenType type;
    std::string lexeme;
    double number;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& src) : source(src), pos(0), line(1) {}
    Token nextToken();

private:
    std::string source;
    size_t pos;
    int line;
    void skipWhitespace();
    Token readNumber();
    Token readString();
    Token readIdentifier();
};

static bool isKeyword(const std::string& id) {
    static const std::vector<std::string> keywords = {
        "var", "val", "global", "func", "if", "elif", "else", "for", "while",
        "break", "continue", "in", "and", "or", "not", "true", "false", "none",
        "import", "from", "block", "return", "is", "print"
    };
    return std::find(keywords.begin(), keywords.end(), id) != keywords.end();
}

static TokenType keywordType(const std::string& kw) {
    if (kw == "var") return TokenType::KEYWORD;
    if (kw == "val") return TokenType::KEYWORD;
    if (kw == "global") return TokenType::KEYWORD;
    if (kw == "func") return TokenType::KEYWORD;
    if (kw == "if") return TokenType::KEYWORD;
    if (kw == "elif") return TokenType::KEYWORD;
    if (kw == "else") return TokenType::KEYWORD;
    if (kw == "for") return TokenType::KEYWORD;
    if (kw == "while") return TokenType::KEYWORD;
    if (kw == "break") return TokenType::KEYWORD;
    if (kw == "continue") return TokenType::KEYWORD;
    if (kw == "in") return TokenType::KEYWORD;
    if (kw == "and") return TokenType::AND;
    if (kw == "or") return TokenType::OR;
    if (kw == "not") return TokenType::NOT;
    if (kw == "true") return TokenType::KEYWORD;
    if (kw == "false") return TokenType::KEYWORD;
    if (kw == "none") return TokenType::KEYWORD;
    if (kw == "import") return TokenType::KEYWORD;
    if (kw == "from") return TokenType::KEYWORD;
    if (kw == "block") return TokenType::KEYWORD;
    if (kw == "return") return TokenType::KEYWORD;
    if (kw == "is") return TokenType::KEYWORD;
    if (kw == "print") return TokenType::KEYWORD;
    return TokenType::IDENT;
}

void Lexer::skipWhitespace() {
    while (pos < source.size() && std::isspace(source[pos])) {
        if (source[pos] == '\n') line++;
        pos++;
    }
}

Token Lexer::readNumber() {
    size_t start = pos;
    while (pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.')) pos++;
    std::string numStr = source.substr(start, pos - start);
    double val = std::stod(numStr);
    return {TokenType::NUMBER, numStr, val, line};
}

Token Lexer::readString() {
    pos++; // skip opening "
    size_t start = pos;
    while (pos < source.size() && source[pos] != '"') pos++;
    std::string str = source.substr(start, pos - start);
    pos++; // skip closing "
    return {TokenType::STRING, str, 0, line};
}

Token Lexer::readIdentifier() {
    size_t start = pos;
    while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_')) pos++;
    std::string id = source.substr(start, pos - start);
    if (isKeyword(id)) {
        TokenType tt = keywordType(id);
        if (tt == TokenType::KEYWORD) return {TokenType::KEYWORD, id, 0, line};
        else return {tt, id, 0, line};
    }
    return {TokenType::IDENT, id, 0, line};
}

Token Lexer::nextToken() {
    skipWhitespace();
    while (pos < source.size() && source[pos] == '/' && pos + 1 < source.size() && source[pos+1] == '/') {
        // Skip line comment
        pos += 2;
        while (pos < source.size() && source[pos] != '\n') {
            pos++;
        }
        skipWhitespace();
    }
    
    if (pos >= source.size()) return {TokenType::END, "", 0, line};
    char c = source[pos];
    if (std::isdigit(c)) return readNumber();
    if (c == '"') return readString();
    if (std::isalpha(c) || c == '_') return readIdentifier();
    // operators and punctuation
    pos++;
    switch (c) {
        case '+': return {TokenType::PLUS, "+", 0, line};
        case '-': return {TokenType::MINUS, "-", 0, line};
        case '*': return {TokenType::STAR, "*", 0, line};
        case '/': return {TokenType::SLASH, "/", 0, line};
        case '%': return {TokenType::PERCENT, "%", 0, line};
        case '^': return {TokenType::CARET, "^", 0, line};
        case '=':
            if (pos < source.size() && source[pos] == '=') { pos++; return {TokenType::EQEQ, "==", 0, line}; }
            return {TokenType::EQ, "=", 0, line};
        case '!':
            if (pos < source.size() && source[pos] == '=') { pos++; return {TokenType::BANGEQ, "!=", 0, line}; }
            return {TokenType::BANG, "!", 0, line};
        case '<':
            if (pos < source.size() && source[pos] == '=') { pos++; return {TokenType::LTE, "<=", 0, line}; }
            return {TokenType::LT, "<", 0, line};
        case '>':
            if (pos < source.size() && source[pos] == '=') { pos++; return {TokenType::GTE, ">=", 0, line}; }
            return {TokenType::GT, ">", 0, line};
        case '&': return {TokenType::AND_SYM, "&", 0, line};
        case '|':
            if (pos < source.size() && source[pos] == '|') { pos++; return {TokenType::OR_SYM, "||", 0, line}; }
            return {TokenType::OR_SYM, "|", 0, line};
        case '(': return {TokenType::LPAREN, "(", 0, line};
        case ')': return {TokenType::RPAREN, ")", 0, line};
        case '{': return {TokenType::LBRACE, "{", 0, line};
        case '}': return {TokenType::RBRACE, "}", 0, line};
        case '[': return {TokenType::LBRACK, "[", 0, line};
        case ']': return {TokenType::RBRACK, "]", 0, line};
        case ',': return {TokenType::COMMA, ",", 0, line};
        case ';': return {TokenType::SEMICOLON, ";", 0, line};
        case '~': return {TokenType::TILDE, "~", 0, line};
        case '.': 
            if (pos < source.size() && source[pos] == '.') { pos++; return {TokenType::DOTDOT, "..", 0, line}; }
            return {TokenType::DOT, ".", 0, line};
    }
    std::stringstream ss;
    ss << "Lexical error on line " << line << ": unexpected character '" << c << "' (ASCII " << (int)c << ")";
    throw std::runtime_error(ss.str());
}

// ----------------------------------------------------------------------
//  AST nodes
// ----------------------------------------------------------------------
struct Expr {
    virtual ~Expr() = default;
    virtual Value evaluate(std::shared_ptr<Environment> env) = 0;
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void execute(std::shared_ptr<Environment> env) = 0;
};

// Expressions
struct LiteralExpr : Expr {
    Value value;
    LiteralExpr(const Value& v) : value(v) {}
    Value evaluate(std::shared_ptr<Environment> /* env */) override { return value; }
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(const std::string& n) : name(n) {}
    Value evaluate(std::shared_ptr<Environment> env) override { return env->get(name); }
};

struct BinaryExpr : Expr {
    std::unique_ptr<Expr> left, right;
    TokenType op;
    BinaryExpr(Expr* l, Expr* r, TokenType o) : left(l), right(r), op(o) {}
    Value evaluate(std::shared_ptr<Environment> env) override;
};

struct UnaryExpr : Expr {
    std::unique_ptr<Expr> expr;
    TokenType op;
    UnaryExpr(Expr* e, TokenType o) : expr(e), op(o) {}
    Value evaluate(std::shared_ptr<Environment> env) override;
};

struct AssignExpr : Expr {
    std::string name;
    std::unique_ptr<Expr> value;
    AssignExpr(const std::string& n, Expr* v) : name(n), value(v) {}
    Value evaluate(std::shared_ptr<Environment> env) override {
        Value val = value->evaluate(env);
        env->assign(name, val);
        return val;
    }
};

struct IndexExpr : Expr {
    std::unique_ptr<Expr> table;
    std::unique_ptr<Expr> index;
    IndexExpr(Expr* t, Expr* i) : table(t), index(i) {}
    Value evaluate(std::shared_ptr<Environment> env) override;
};

struct CallExpr : Expr {
    std::unique_ptr<Expr> callee;
    std::vector<std::unique_ptr<Expr>> args;
    CallExpr(Expr* c, std::vector<Expr*> a) : callee(c) {
        for (auto* e : a) args.emplace_back(e);
    }
    Value evaluate(std::shared_ptr<Environment> env) override;
};

struct TableLiteralExpr : Expr {
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> pairs;
    TableLiteralExpr(std::vector<std::pair<Expr*, Expr*>> p) {
        for (auto& pair : p) {
            pairs.emplace_back(pair.first, pair.second);
        }
    }
    Value evaluate(std::shared_ptr<Environment> env) override {
        auto tbl = std::make_shared<Table>();
        for (auto& pair : pairs) {
            Value key = pair.first->evaluate(env);
            Value val = pair.second->evaluate(env);
            (*tbl)[key] = val;
        }
        return Value(tbl);
    }
};

// Statements
struct ExprStmt : Stmt {
    std::unique_ptr<Expr> expr;
    ExprStmt(Expr* e) : expr(e) {}
    void execute(std::shared_ptr<Environment> env) override { expr->evaluate(env); }
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Expr> value;
    ReturnStmt(Expr* v) : value(v) {}
    void execute(std::shared_ptr<Environment> env) override {
        throw value->evaluate(env);  // special exception to unwind
    }
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> initializer;
    bool mutableFlag;
    bool isGlobal;
    VarDeclStmt(const std::string& n, Expr* init, bool mut, bool global)
        : name(n), initializer(init), mutableFlag(mut), isGlobal(global) {}
    void execute(std::shared_ptr<Environment> env) override {
        Value val;
        try {
            val = initializer ? initializer->evaluate(env) : Value();
        } catch (const std::exception& e) {
            throw std::runtime_error("Error initializing variable '" + name + "': " + e.what());
        }
        if (isGlobal) {
            // find or create global environment (walk to top)
            auto top = env;
            while (top->getParent()) top = top->getParent();
            top->define(name, val, mutableFlag);
        } else {
            env->define(name, val, mutableFlag);
        }
    }
};

struct BlockStmt : Stmt {
    std::vector<std::unique_ptr<Stmt>> statements;
    void addStmt(Stmt* s) { statements.emplace_back(s); }
    void execute(std::shared_ptr<Environment> env) override {
        auto blockEnv = std::make_shared<Environment>(env);
        for (auto& stmt : statements) {
            try {
                stmt->execute(blockEnv);
            } catch (Value& retVal) {
                throw retVal; // propagate return
            }
        }
    }
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;
    IfStmt(Expr* cond, Stmt* thenS, Stmt* elseS) : condition(cond), thenBranch(thenS), elseBranch(elseS) {}
    void execute(std::shared_ptr<Environment> env) override {
        if (condition->evaluate(env).isTruthy()) {
            thenBranch->execute(env);
        } else if (elseBranch) {
            elseBranch->execute(env);
        }
    }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    WhileStmt(Expr* cond, Stmt* b) : condition(cond), body(b) {}
    void execute(std::shared_ptr<Environment> env) override {
        while (condition->evaluate(env).isTruthy()) {
            try {
                body->execute(env);
            } catch (const std::string& ctrl) {
                if (ctrl == "break") break;
                if (ctrl == "continue") continue;
                throw; // rethrow unexpected
            }
        }
    }
};

struct ForNumericStmt : Stmt {
    std::string varName;
    std::unique_ptr<Expr> start, end, step;
    std::unique_ptr<Stmt> body;
    ForNumericStmt(const std::string& v, Expr* s, Expr* e, Expr* st, Stmt* b)
        : varName(v), start(s), end(e), step(st), body(b) {}
    void execute(std::shared_ptr<Environment> env) override {
        double from = start->evaluate(env).num;
        double to = end->evaluate(env).num;
        double stepVal = step ? step->evaluate(env).num : 1.0;
        auto loopEnv = std::make_shared<Environment>(env);
        for (double i = from; (stepVal > 0 ? i < to : i > to); i += stepVal) {
            loopEnv->define(varName, Value(i), true);
            try {
                body->execute(loopEnv);
            } catch (const std::string& ctrl) {
                if (ctrl == "break") break;
                if (ctrl == "continue") continue;
                throw;
            }
        }
    }
};

struct ForInStmt : Stmt {
    std::vector<std::string> vars; // one or two (key, value)
    std::unique_ptr<Expr> iterable;
    std::unique_ptr<Stmt> body;
    ForInStmt(const std::vector<std::string>& v, Expr* it, Stmt* b) : vars(v), iterable(it), body(b) {}
    void execute(std::shared_ptr<Environment> env) override;
};

struct FuncDeclStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    bool isGlobal;
    FuncDeclStmt(const std::string& n, const std::vector<std::string>& p, BlockStmt* b, bool global)
        : name(n), params(p), body(b), isGlobal(global) {}
    void execute(std::shared_ptr<Environment> env) override {
        auto funcVal = Value(std::make_shared<Value::UserFunction>());
        funcVal.userFunc->params = params;
        funcVal.userFunc->body = body.get(); // careful: body stored as unique_ptr, we keep raw pointer
        funcVal.userFunc->closure = env;
        if (isGlobal) {
            auto top = env;
            while (top->getParent()) top = top->getParent();
            top->define(name, funcVal, false);
        } else {
            env->define(name, funcVal, false);
        }
    }
};

struct ImportStmt : Stmt {
    std::string module;
    std::vector<std::string> symbols;
    bool fromForm;
    ImportStmt(const std::string& m, const std::vector<std::string>& syms, bool from)
        : module(m), symbols(syms), fromForm(from) {}
    void execute(std::shared_ptr<Environment> env) override;
};

struct BlockKeywordStmt : Stmt {
    std::unique_ptr<BlockStmt> block;
    BlockKeywordStmt(BlockStmt* b) : block(b) {}
    void execute(std::shared_ptr<Environment> env) override {
        block->execute(env);
    }
};

// Helper for pattern matching (is)
struct IsExpr : Expr {
    std::unique_ptr<Expr> target;
    std::string typeName;
    std::string bindVar;
    IsExpr(Expr* t, const std::string& type, const std::string& bind) : target(t), typeName(type), bindVar(bind) {}
    Value evaluate(std::shared_ptr<Environment> env) override {
        Value v = target->evaluate(env);
        bool matches = false;
        if (typeName == "String" && v.type == Value::STRING) matches = true;
        else if (typeName == "Number" && v.type == Value::NUMBER) matches = true;
        // could extend for Table, etc.
        if (matches && !bindVar.empty()) {
            env->define(bindVar, v, true); // bind in current scope
        }
        return Value(matches);
    }
};

// Expression evaluators that need full definitions
Value BinaryExpr::evaluate(std::shared_ptr<Environment> env) {
    Value l = left->evaluate(env);
    Value r = right->evaluate(env);
    auto binOp = [](double a, double b, TokenType op) -> double {
        switch (op) {
            case TokenType::PLUS: return a + b;
            case TokenType::MINUS: return a - b;
            case TokenType::STAR: return a * b;
            case TokenType::SLASH: {
                if (b == 0) throw std::runtime_error("Division by zero");
                return a / b;
            }
            case TokenType::PERCENT: {
                if (b == 0) throw std::runtime_error("Modulo by zero");
                return std::fmod(a, b);
            }
            case TokenType::CARET: return std::pow(a, b);
            default: return 0;
        }
    };
    if (op == TokenType::DOTDOT) {
        if (l.type == Value::STRING && r.type == Value::STRING) {
            return Value(*l.str + *r.str);
        }
        std::string leftType = (l.type == Value::NONE) ? "none" : 
                              (l.type == Value::NUMBER) ? "number" :
                              (l.type == Value::BOOL) ? "bool" :
                              (l.type == Value::STRING) ? "string" : "table/function";
        std::string rightType = (r.type == Value::NONE) ? "none" : 
                               (r.type == Value::NUMBER) ? "number" :
                               (r.type == Value::BOOL) ? "bool" :
                               (r.type == Value::STRING) ? "string" : "table/function";
        throw std::runtime_error("Type error: cannot concatenate " + leftType + " and " + rightType + " (both must be strings)");
    }
    if (op == TokenType::EQEQ) return Value(l == r);
    if (op == TokenType::BANGEQ) return Value(l != r);
    if (op == TokenType::LT || op == TokenType::LTE || op == TokenType::GT || op == TokenType::GTE) {
        if (l.type == Value::NUMBER && r.type == Value::NUMBER) {
            switch(op) {
                case TokenType::LT: return Value(l.num < r.num);
                case TokenType::LTE: return Value(l.num <= r.num);
                case TokenType::GT: return Value(l.num > r.num);
                case TokenType::GTE: return Value(l.num >= r.num);
                default: break;
            }
        }
        std::string leftType = (l.type == Value::NUMBER) ? "number" : 
                              (l.type == Value::STRING) ? "string" : "other";
        std::string rightType = (r.type == Value::NUMBER) ? "number" : 
                               (r.type == Value::STRING) ? "string" : "other";
        throw std::runtime_error("Type error: cannot compare " + leftType + " and " + rightType + " (both must be numbers or strings)");
    }
    if (op == TokenType::AND || op == TokenType::AND_SYM) {
        if (l.type == Value::NUMBER && r.type == Value::NUMBER)
            return Value((double)((int)l.num & (int)r.num));
        return Value(l.isTruthy() && r.isTruthy());
    }
    if (op == TokenType::OR || op == TokenType::OR_SYM) {
        if (l.type == Value::NUMBER && r.type == Value::NUMBER)
            return Value((double)((int)l.num | (int)r.num));
        return Value(l.isTruthy() || r.isTruthy());
    }
    // arithmetic
    if (l.type == Value::NUMBER && r.type == Value::NUMBER)
        return Value(binOp(l.num, r.num, op));
    
    std::string opStr = "+";
    switch(op) {
        case TokenType::PLUS: opStr = "+"; break;
        case TokenType::MINUS: opStr = "-"; break;
        case TokenType::STAR: opStr = "*"; break;
        case TokenType::SLASH: opStr = "/"; break;
        case TokenType::PERCENT: opStr = "%"; break;
        case TokenType::CARET: opStr = "^"; break;
        default: opStr = "?"; break;
    }
    std::string leftType = (l.type == Value::NUMBER) ? "number" : 
                          (l.type == Value::STRING) ? "string" :
                          (l.type == Value::BOOL) ? "bool" :
                          (l.type == Value::TABLE) ? "table" : "other";
    std::string rightType = (r.type == Value::NUMBER) ? "number" : 
                           (r.type == Value::STRING) ? "string" :
                           (r.type == Value::BOOL) ? "bool" :
                           (r.type == Value::TABLE) ? "table" : "other";
    throw std::runtime_error("Type error: cannot " + opStr + " " + leftType + " and " + rightType);
}

Value UnaryExpr::evaluate(std::shared_ptr<Environment> env) {
    Value v = expr->evaluate(env);
    if (op == TokenType::MINUS) {
        if (v.type != Value::NUMBER) {
            throw std::runtime_error("Type error: cannot negate non-numeric type");
        }
        return Value(-v.num);
    }
    if (op == TokenType::BANG || op == TokenType::NOT)
        return Value(!v.isTruthy());
    if (op == TokenType::NOT_SYM) { // bitwise NOT
        if (v.type == Value::NUMBER) return Value((double)~(int)v.num);
        throw std::runtime_error("Type error: bitwise NOT (~) only works on numbers");
    }
    throw std::runtime_error("Internal error: invalid unary operator");
}

Value IndexExpr::evaluate(std::shared_ptr<Environment> env) {
    Value tbl = table->evaluate(env);
    Value idx = index->evaluate(env);
    
    // Special handling for method calls on primitive types
    if (idx.type == Value::STRING && tbl.type != Value::TABLE) {
        std::string methodName = *idx.str;
        
        // Add toString method for all types
        if (methodName == "toString") {
            return Value(std::function<Value(const std::vector<Value>&)>([tbl](const std::vector<Value>& args) -> Value {
                return Value(tbl.toString());
            }));
        }
        
        // Add type method
        if (methodName == "type") {
            return Value(std::function<Value(const std::vector<Value>&)>([tbl](const std::vector<Value>& args) -> Value {
                std::string typeStr;
                switch(tbl.type) {
                    case Value::NONE: typeStr = "none"; break;
                    case Value::NUMBER: typeStr = "number"; break;
                    case Value::BOOL: typeStr = "bool"; break;
                    case Value::STRING: typeStr = "string"; break;
                    case Value::TABLE: typeStr = "table"; break;
                    case Value::NATIVE_FUNC: typeStr = "native_func"; break;
                    case Value::USER_FUNC: typeStr = "function"; break;
                }
                return Value(typeStr);
            }));
        }
        
        // Unknown method
        throw std::runtime_error("Error: unknown method '" + methodName + "' on " + tbl.toString());
    }
    
    if (tbl.type != Value::TABLE) {
        std::string typeStr = (tbl.type == Value::NUMBER) ? "number" : 
                             (tbl.type == Value::STRING) ? "string" :
                             (tbl.type == Value::BOOL) ? "bool" :
                             (tbl.type == Value::NONE) ? "none" : "function";
        throw std::runtime_error("Type error: cannot index " + typeStr + " (only tables can be indexed)");
    }
    auto it = tbl.table->find(idx);
    if (it != tbl.table->end()) return it->second;
    return Value(); // none
}

Value CallExpr::evaluate(std::shared_ptr<Environment> env) {
    Value calleeVal = callee->evaluate(env);
    std::vector<Value> argVals;
    for (auto& a : args) argVals.push_back(a->evaluate(env));
    
    if (calleeVal.type == Value::NATIVE_FUNC) {
        try {
            return calleeVal.nativeFunc(argVals);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error in native function call: " + std::string(e.what()));
        }
    } else if (calleeVal.type == Value::USER_FUNC) {
        auto func = calleeVal.userFunc;
        auto callEnv = std::make_shared<Environment>(func->closure);
        if (argVals.size() > func->params.size()) {
            throw std::runtime_error("Error: function takes " + std::to_string(func->params.size()) + 
                                   " parameters but " + std::to_string(argVals.size()) + " arguments were provided");
        }
        for (size_t i = 0; i < func->params.size(); ++i) {
            callEnv->define(func->params[i], i < argVals.size() ? argVals[i] : Value(), true);
        }
        try {
            func->body->execute(callEnv);
        } catch (Value& ret) {
            return ret;
        }
        return Value();
    }
    
    std::string typeStr = (calleeVal.type == Value::NUMBER) ? "number" : 
                         (calleeVal.type == Value::STRING) ? "string" :
                         (calleeVal.type == Value::BOOL) ? "bool" :
                         (calleeVal.type == Value::TABLE) ? "table" :
                         (calleeVal.type == Value::NONE) ? "none" : "unknown";
    throw std::runtime_error("Type error: cannot call " + typeStr + " (not a function)");
}

void ForInStmt::execute(std::shared_ptr<Environment> env) {
    Value iter = iterable->evaluate(env);
    if (iter.type == Value::TABLE) {
        if (vars.size() == 1) {
            for (auto& [k, v] : *iter.table) {
                auto loopEnv = std::make_shared<Environment>(env);
                loopEnv->define(vars[0], v, true);
                try { 
                    // Create a new block environment for each iteration
                    auto blockEnv = std::make_shared<Environment>(loopEnv);
                    for (auto& stmt : iter.table->empty() ? std::vector<std::shared_ptr<Stmt>>() : std::vector<std::shared_ptr<Stmt>>()) {
                        // This is handled in the body
                    }
                    body->execute(loopEnv);
                }
                catch (const std::string& ctrl) {
                    if (ctrl == "break") break;
                    if (ctrl == "continue") continue;
                    throw;
                }
            }
        } else if (vars.size() == 2) {
            for (auto& [k, v] : *iter.table) {
                auto loopEnv = std::make_shared<Environment>(env);
                loopEnv->define(vars[0], k, true);
                loopEnv->define(vars[1], v, true);
                try { 
                    body->execute(loopEnv);
                }
                catch (const std::string& ctrl) {
                    if (ctrl == "break") break;
                    if (ctrl == "continue") continue;
                    throw;
                }
            }
        } else {
            throw std::runtime_error("Error: for-in loop supports 1 or 2 variables, got " + std::to_string(vars.size()));
        }
    } else {
        std::string typeStr = (iter.type == Value::NUMBER) ? "number" : 
                             (iter.type == Value::STRING) ? "string" :
                             (iter.type == Value::BOOL) ? "bool" :
                             (iter.type == Value::NONE) ? "none" : "function";
        throw std::runtime_error("Type error: for-in loop requires a table, got " + typeStr);
    }
}

void ImportStmt::execute(std::shared_ptr<Environment> env) {
    // In a full implementation, load module from pre‑registered sources.
    // Here we provide a hook: addModule() stores source strings.
    // For brevity, we simulate a module as an empty table.
    Value moduleTable = Value(std::make_shared<Table>());
    if (fromForm) {
        for (auto& sym : symbols) {
            env->define(sym, moduleTable, true); // simplistic: all symbols refer to same table
        }
    } else {
        env->define(module, moduleTable, true);
    }
}

Value::UserFunction::~UserFunction() { /* body owned by unique_ptr in FuncDeclStmt, we don't delete here */ }

// ----------------------------------------------------------------------
//  Parser
// ----------------------------------------------------------------------
class Parser {
public:
    Parser(const std::string& src) : lexer(src), current(Token{TokenType::END,"",0,0}) { advance(); }
    std::vector<std::unique_ptr<Stmt>> parse();

private:
    Lexer lexer;
    Token current;
    Token previous;
    
    void advance() { 
        previous = current;
        current = lexer.nextToken(); 
    }
    
    bool check(TokenType t) const { return current.type == t; }
    
    bool match(TokenType t) { 
        if (check(t)) { 
            advance(); 
            return true; 
        } 
        return false; 
    }
    
    void expect(TokenType t, const std::string& msg) { 
        if (!match(t)) {
            std::stringstream ss;
            ss << "Parse error on line " << current.line << ": " << msg;
            if (current.type != TokenType::END) {
                ss << " (found '" << current.lexeme << "' instead)";
            } else {
                ss << " (found end of file)";
            }
            throw std::runtime_error(ss.str());
        }
    }

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> logicalOr();
    std::unique_ptr<Expr> logicalAnd();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> call();

    std::unique_ptr<Stmt> statement();
    std::unique_ptr<BlockStmt> block();
    std::unique_ptr<Stmt> varDecl(bool globalFlag);
    std::unique_ptr<Stmt> funcDecl(bool globalFlag);
    std::unique_ptr<Stmt> ifStmt();
    std::unique_ptr<Stmt> whileStmt();
    std::unique_ptr<Stmt> forStmt();
    std::unique_ptr<Stmt> returnStmt();
    std::unique_ptr<Stmt> importStmt();
    std::unique_ptr<Stmt> blockKeyword();
};

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    while (!check(TokenType::END)) {
        stmts.push_back(statement());
        if (match(TokenType::SEMICOLON)) continue;
        // allow newline separation
    }
    return stmts;
}

// Expression precedence helpers (simplified)
std::unique_ptr<Expr> Parser::expression() { 
    return logicalOr();
}
std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();
    while (check(TokenType::OR) || check(TokenType::OR_SYM)) {
        TokenType op = current.type;
        advance();
        auto right = logicalAnd();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = equality();
    while (check(TokenType::AND) || check(TokenType::AND_SYM)) {
        TokenType op = current.type;
        advance();
        auto right = equality();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    while (check(TokenType::EQEQ) || check(TokenType::BANGEQ)) {
        TokenType op = current.type;
        advance();
        auto right = comparison();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::comparison() {
    auto expr = term();
    while (check(TokenType::LT) || check(TokenType::LTE) || check(TokenType::GT) || check(TokenType::GTE)) {
        TokenType op = current.type;
        advance();
        auto right = term();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op = current.type;
        advance();
        auto right = factor();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT) ||
           check(TokenType::CARET) || check(TokenType::DOTDOT)) {
        TokenType op = current.type;
        advance();
        auto right = unary();
        expr = std::make_unique<BinaryExpr>(expr.release(), right.release(), op);
    }
    return expr;
}
std::unique_ptr<Expr> Parser::unary() {
    if (check(TokenType::MINUS) || check(TokenType::BANG) || check(TokenType::NOT) || check(TokenType::NOT_SYM)) {
        TokenType op = current.type;
        advance();
        auto expr = unary();
        return std::make_unique<UnaryExpr>(expr.release(), op);
    }
    return call();
}
std::unique_ptr<Expr> Parser::call() {
    auto expr = primary();
    while (true) {
        if (check(TokenType::LPAREN)) {
            advance();
            std::vector<Expr*> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(expression().release());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')' after arguments");
            expr = std::make_unique<CallExpr>(expr.release(), args);
        }
        else if (check(TokenType::LBRACK)) {
            advance();
            auto idx = expression();
            expect(TokenType::RBRACK, "Expected ']'");
            expr = std::make_unique<IndexExpr>(expr.release(), idx.release());
        }
        else if (check(TokenType::DOT)) {
            advance();
            expect(TokenType::IDENT, "Expected identifier after '.'");
            auto ident = std::make_unique<LiteralExpr>(Value(previous.lexeme));
            expr = std::make_unique<IndexExpr>(expr.release(), ident.release());
        }
        else {
            break;
        }
    }
    return expr;
}
std::unique_ptr<Expr> Parser::primary() {
    if (check(TokenType::NUMBER)) {
        double val = current.number;
        advance();
        return std::make_unique<LiteralExpr>(Value(val));
    }
    if (check(TokenType::STRING)) {
        std::string str = current.lexeme;
        advance();
        return std::make_unique<LiteralExpr>(Value(str));
    }
    if (check(TokenType::KEYWORD)) {
        std::string kw = current.lexeme;
        if (kw == "true") { advance(); return std::make_unique<LiteralExpr>(Value(true)); }
        if (kw == "false") { advance(); return std::make_unique<LiteralExpr>(Value(false)); }
        if (kw == "none") { advance(); return std::make_unique<LiteralExpr>(Value()); }
        if (kw == "print") {
            std::string name = kw;
            advance();
            if (check(TokenType::EQ)) {
                advance();
                auto val = expression();
                return std::make_unique<AssignExpr>(name, val.release());
            }
            return std::make_unique<VarExpr>(name);
        }
        throw std::runtime_error("Unexpected keyword in expression: " + kw);
    }
    if (check(TokenType::IDENT)) {
        std::string name = current.lexeme;
        advance();
        if (check(TokenType::EQ)) {
            advance();
            auto val = expression();
            return std::make_unique<AssignExpr>(name, val.release());
        }
        return std::make_unique<VarExpr>(name);
    }
    if (check(TokenType::LPAREN)) {
        advance();
        auto expr = expression();
        expect(TokenType::RPAREN, "Expected ')'");
        return expr;
    }
    if (check(TokenType::LBRACE)) { // table literal
        advance();
        std::vector<std::pair<Expr*, Expr*>> pairs;
        while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
            auto key = expression();
            expect(TokenType::EQ, "Expected '=' in table literal");
            auto val = expression();
            pairs.push_back({key.release(), val.release()});
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::RBRACE, "Expected '}'");
        return std::make_unique<TableLiteralExpr>(pairs);
    }
    std::stringstream ss;
    ss << "Parse error on line " << current.line << ": unexpected token in expression";
    if (current.type != TokenType::END) {
        ss << " (found '" << current.lexeme << "')";
    } else {
        ss << " (found end of file)";
    }
    throw std::runtime_error(ss.str());
}

std::unique_ptr<Stmt> Parser::statement() {
    if (check(TokenType::KEYWORD)) {
        std::string kw = current.lexeme;
        if (kw == "print") {
            // print is a builtin function, treat as expression
            auto expr = expression();
            if (match(TokenType::TILDE)) {
                return std::make_unique<ReturnStmt>(expr.release());
            }
            return std::make_unique<ExprStmt>(expr.release());
        }
        if (kw == "var" || kw == "val") {
            return varDecl(false);
        }
        if (kw == "global") {
            advance();
            if (check(TokenType::KEYWORD) && (current.lexeme == "var" || current.lexeme == "val")) {
                auto stmt = varDecl(true);
                return stmt;
            } else if (check(TokenType::KEYWORD) && current.lexeme == "func") {
                advance();
                return funcDecl(true);
            }
            throw std::runtime_error("global must be followed by var, val, or func");
        }
        if (kw == "func") { advance(); return funcDecl(false); }
        if (kw == "if") {
            advance();
            return ifStmt();
        }
        if (kw == "while") {
            advance();
            return whileStmt();
        }
        if (kw == "for") {
            advance();
            return forStmt();
        }
        if (kw == "return") return returnStmt();
        if (kw == "import") {
            advance();
            return importStmt();
        }
        if (kw == "from") {
            advance();
            auto fromStmt = importStmt();
            return fromStmt;
        }
        if (kw == "block") {
            advance();
            return blockKeyword();
        }
        if (kw == "break") {
            advance();
            struct BreakStmt : Stmt {
                void execute(std::shared_ptr<Environment> /* env */) override {
                    throw std::string("break");
                }
            };
            return std::make_unique<BreakStmt>();
        }
        if (kw == "continue") {
            advance();
            struct ContinueStmt : Stmt {
                void execute(std::shared_ptr<Environment> /* env */) override {
                    throw std::string("continue");
                }
            };
            return std::make_unique<ContinueStmt>();
        }
        throw std::runtime_error("Unexpected keyword: " + kw);
    }
    auto expr = expression();
    if (match(TokenType::TILDE)) {
        return std::make_unique<ReturnStmt>(expr.release());
    }
    return std::make_unique<ExprStmt>(expr.release());
}

std::unique_ptr<BlockStmt> Parser::block() {
    expect(TokenType::LBRACE, "Expected '{'");
    auto blockStmt = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBRACE) && !check(TokenType::END)) {
        blockStmt->addStmt(statement().release());
        match(TokenType::SEMICOLON);
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return blockStmt;
}

std::unique_ptr<Stmt> Parser::varDecl(bool globalFlag) {
    bool mutableFlag = (current.lexeme == "var");
    advance();  // Move past var/val keyword
    if (current.type != TokenType::IDENT) throw std::runtime_error("Expected variable name");
    std::string name = current.lexeme;
    advance();
    std::unique_ptr<Expr> init;
    if (match(TokenType::EQ)) {
        init = expression();
    }
    return std::make_unique<VarDeclStmt>(name, init.release(), mutableFlag, globalFlag);
}

std::unique_ptr<Stmt> Parser::funcDecl(bool globalFlag) {
    if (current.type != TokenType::IDENT) {
        std::stringstream ss;
        ss << "Parse error on line " << current.line << ": expected function name";
        if (current.type != TokenType::END) {
            ss << " (found '" << current.lexeme << "' instead)";
        } else {
            ss << " (found end of file)";
        }
        throw std::runtime_error(ss.str());
    }
    std::string name = current.lexeme;
    advance();
    if (current.type != TokenType::LPAREN) {
        throw std::runtime_error("Parse error on line " + std::to_string(current.line) + ": expected '(' after function name '" + name + "'");
    }
    advance();
    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            if (current.type != TokenType::IDENT) {
                throw std::runtime_error("Parse error on line " + std::to_string(current.line) + ": expected parameter name");
            }
            params.push_back(current.lexeme);
            advance();
        } while (match(TokenType::COMMA));
    }
    if (current.type != TokenType::RPAREN) {
        throw std::runtime_error("Parse error on line " + std::to_string(current.line) + ": expected ')' after parameters");
    }
    advance();
    auto bodyBlock = block();
    return std::make_unique<FuncDeclStmt>(name, params, bodyBlock.release(), globalFlag);
}

std::unique_ptr<Stmt> Parser::ifStmt() {
    auto cond = expression();
    auto thenStmt = block();
    std::unique_ptr<Stmt> elseStmt;
    if (check(TokenType::KEYWORD)) {
        if (current.lexeme == "elif") {
            advance();
            // Recursively parse elif as nested if-else
            auto elifCond = expression();
            auto elifThen = block();
            std::unique_ptr<Stmt> nestedElse;
            // Check for more elif/else chains
            if (check(TokenType::KEYWORD)) {
                if (current.lexeme == "elif") {
                    // Recursively handle nested elif
                    advance();
                    auto restOfChain = ifStmt();
                    nestedElse = std::move(restOfChain);
                } else if (current.lexeme == "else") {
                    advance();
                    nestedElse = std::move(block());
                }
            }
            elseStmt = std::make_unique<IfStmt>(elifCond.release(), elifThen.release(), nestedElse.release());
        } else if (current.lexeme == "else") {
            advance();
            elseStmt = std::move(block());
        }
    }
    return std::make_unique<IfStmt>(cond.release(), thenStmt.release(), elseStmt.release());
}

std::unique_ptr<Stmt> Parser::whileStmt() {
    auto cond = expression();
    auto body = block();
    return std::make_unique<WhileStmt>(cond.release(), body.release());
}

std::unique_ptr<Stmt> Parser::forStmt() {
    // numeric: for i = 0, 10, 2  or for i in range(...)
    // or: for k, v in table
    
    // First, collect variable names
    std::vector<std::string> vars;
    if (check(TokenType::IDENT)) {
        vars.push_back(current.lexeme);
        advance();
        while (match(TokenType::COMMA)) {
            if (current.type != TokenType::IDENT) {
                throw std::runtime_error("Expected identifier in for loop");
            }
            vars.push_back(current.lexeme);
            advance();
        }
    } else {
        throw std::runtime_error("Expected loop variable in for statement");
    }
    
    // Now check what kind of for loop
    if (vars.size() == 1) {
        if (match(TokenType::EQ)) {
            // numeric for: for i = start, end, step
            auto start = expression();
            expect(TokenType::COMMA, "Expected ',' after start value in for loop");
            auto end = expression();
            std::unique_ptr<Expr> step;
            if (match(TokenType::COMMA)) {
                step = expression();
            }
            auto body = block();
            return std::make_unique<ForNumericStmt>(vars[0], start.release(), end.release(), step.release(), body.release());
        } else if (check(TokenType::KEYWORD) && current.lexeme == "in") {
            // for i in iterable
            advance();
            auto iterable = expression();
            auto body = block();
            return std::make_unique<ForInStmt>(vars, iterable.release(), body.release());
        } else {
            throw std::runtime_error("Expected '=' or 'in' in for loop");
        }
    } else {
        // Multi-variable for loop: for k, v in table
        if (check(TokenType::KEYWORD) && current.lexeme == "in") {
            advance();
            auto iterable = expression();
            auto body = block();
            return std::make_unique<ForInStmt>(vars, iterable.release(), body.release());
        } else {
            throw std::runtime_error("Error: multiple variables in for loop require 'in' keyword");
        }
    }
}

std::unique_ptr<Stmt> Parser::returnStmt() {
    advance();
    std::unique_ptr<Expr> val;
    if (!check(TokenType::RBRACE) && !check(TokenType::END)) {
        val = expression();
    }
    return std::make_unique<ReturnStmt>(val.release());
}

std::unique_ptr<Stmt> Parser::importStmt() {
    bool fromForm = false;
    if (current.lexeme == "from") {
        fromForm = true;
        advance();
    }
    std::string module;
    if (current.type == TokenType::STRING) {
        module = current.lexeme;
        advance();
    } else if (current.type == TokenType::IDENT) {
        module = current.lexeme;
        advance();
    } else {
        throw std::runtime_error("Expected module name");
    }
    std::vector<std::string> symbols;
    if (fromForm) {
        if (current.type != TokenType::KEYWORD || current.lexeme != "import") throw std::runtime_error("Expected 'import'");
        advance();
        do {
            if (current.type != TokenType::IDENT) throw std::runtime_error("Expected symbol name");
            symbols.push_back(current.lexeme);
            advance();
        } while (match(TokenType::COMMA));
    }
    return std::make_unique<ImportStmt>(module, symbols, fromForm);
}

std::unique_ptr<Stmt> Parser::blockKeyword() {
    advance();
    auto b = block();
    return std::make_unique<BlockKeywordStmt>(b.release());
}

// ----------------------------------------------------------------------
//  Interpreter Implementation (PIMPL)
// ----------------------------------------------------------------------
struct Interpreter::Impl {
    std::shared_ptr<Environment> globalEnv;
    ErrorHandler errorHandler;
    std::unordered_map<std::string, std::string> modules;

    Impl(ErrorHandler err) : globalEnv(std::make_shared<Environment>()), errorHandler(err) {
        // define builtins
        globalEnv->define("print", Value(std::function<Value(const std::vector<Value>&)>([](const std::vector<Value>& args) -> Value {
            for (auto& a : args) std::cout << a.toString();
            std::cout << std::endl;
            return Value();
        })), false);
        globalEnv->define("range", Value(std::function<Value(const std::vector<Value>&)>([](const std::vector<Value>& args) -> Value {
            double start = 0, end = 0, step = 1;
            if (args.size() == 1) { end = args[0].num; }
            else if (args.size() == 2) { start = args[0].num; end = args[1].num; }
            else if (args.size() == 3) { start = args[0].num; end = args[1].num; step = args[2].num; }
            auto tbl = std::make_shared<Table>();
            for (double i = start; (step > 0 ? i < end : i > end); i += step) {
                (*tbl)[Value((double)(*tbl).size())] = Value(i);
            }
            return Value(tbl);
        })), false);
        globalEnv->define("entries", Value(std::function<Value(const std::vector<Value>&)>([](const std::vector<Value>& args) -> Value {
            if (args.empty() || args[0].type != Value::TABLE) throw std::runtime_error("entries expects a table");
            return args[0]; // for-in will treat the table directly
        })), false);
    }

    void run(const std::vector<std::unique_ptr<Stmt>>& stmts) {
        for (auto& stmt : stmts) {
            try {
                stmt->execute(globalEnv);
            } catch (Value& ret) {
                // return at top-level ignored
            } catch (const std::string& ctrl) {
                if (ctrl == "break" || ctrl == "continue")
                    errorHandler("break/continue outside loop");
                else throw;
            } catch (std::exception& e) {
                errorHandler(e.what());
            }
        }
    }
};

Interpreter::Interpreter(ErrorHandler err) : pImpl(std::make_unique<Impl>(err)) {}
Interpreter::~Interpreter() = default;

void Interpreter::interpret(const std::string& source) {
    Parser parser(source);
    auto stmts = parser.parse();
    pImpl->run(stmts);
}

void Interpreter::addModule(const std::string& name, const std::string& source) {
    pImpl->modules[name] = source;
}

} // namespace AUL

// Main program to test the interpreter
int main(int argc, char* argv[]) {
    std::string source;
    
    if (argc > 1) {
        // Read from file
        std::ifstream file(argv[1]);
        if (!file) {
            std::cerr << "Error: Could not open file " << argv[1] << std::endl;
            return 1;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    } else {
        std::cerr << "Usage: " << argv[0] << " <script.aul>" << std::endl;
        return 1;
    }
    
    AUL::Interpreter interpreter;
    try {
        interpreter.interpret(source);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}