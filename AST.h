#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include <string>
#include "Token.h"
#include "Value.h"

// Forward declarations
struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ========== EXPRESSIONS ==========
struct Expr { virtual ~Expr() = default; };

struct LiteralExpr : Expr {
    Value value;
    explicit LiteralExpr(Value v);
};

struct IdentifierExpr : Expr {
    std::string name;
    explicit IdentifierExpr(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : Expr {
    ExprPtr left;
    TokenType op;
    ExprPtr right;
    BinaryExpr(ExprPtr l, TokenType o, ExprPtr r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    TokenType op;
    ExprPtr operand;
    UnaryExpr(TokenType o, ExprPtr opnd) : op(o), operand(std::move(opnd)) {}
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    IndexExpr(ExprPtr obj, ExprPtr idx)
        : object(std::move(obj)), index(std::move(idx)) {}
};

struct PatternMatchExpr : Expr {
    ExprPtr value;
    std::string typeName;
    std::vector<std::string> captureNames;
    PatternMatchExpr(ExprPtr v, std::string t, std::vector<std::string> c)
        : value(std::move(v)), typeName(std::move(t)), captureNames(std::move(c)) {}
};

// ========== STATEMENTS ==========
struct Stmt { virtual ~Stmt() = default; };

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {}
};

struct VarDeclStmt : Stmt {
    std::string name;
    bool isConst;
    bool isGlobal;
    std::string typeAnnotation;  // NEW: for type checking
    bool isOptional;             // NEW: for option types
    ExprPtr init;
    
    VarDeclStmt(std::string n, bool c, bool g, ExprPtr i)
        : name(std::move(n)), isConst(c), isGlobal(g), isOptional(false), init(std::move(i)) {}
};

struct AssignmentStmt : Stmt {
    enum TargetType { SIMPLE, INDEX };
    TargetType targetType;
    std::string name;
    ExprPtr indexExpr;
    ExprPtr objectExpr;
    ExprPtr value;
    
    AssignmentStmt(std::string n, ExprPtr v)
        : targetType(SIMPLE), name(std::move(n)), value(std::move(v)) {}
    AssignmentStmt(ExprPtr obj, ExprPtr idx, ExprPtr v)
        : targetType(INDEX), indexExpr(std::move(idx)), objectExpr(std::move(obj)), value(std::move(v)) {}
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    explicit BlockStmt(std::vector<StmtPtr> stmts) : statements(std::move(stmts)) {}
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    
    IfStmt(ExprPtr cond, StmtPtr t, StmtPtr e)
        : condition(std::move(cond)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    
    WhileStmt(ExprPtr cond, StmtPtr b)
        : condition(std::move(cond)), body(std::move(b)) {}
};

struct ForNumericStmt : Stmt {
    std::string varName;
    ExprPtr start;
    ExprPtr end;
    ExprPtr step;
    StmtPtr body;
    
    ForNumericStmt(std::string vn, ExprPtr s, ExprPtr e, ExprPtr st, StmtPtr b)
        : varName(std::move(vn)), start(std::move(s)), end(std::move(e)), step(std::move(st)), body(std::move(b)) {}
};

struct ForInStmt : Stmt {
    std::vector<std::string> loopVars;
    ExprPtr iterable;
    StmtPtr body;
    
    ForInStmt(std::vector<std::string> vars, ExprPtr iter, StmtPtr b)
        : loopVars(std::move(vars)), iterable(std::move(iter)), body(std::move(b)) {}
};

struct BreakStmt : Stmt {};
struct ContinueStmt : Stmt {};

struct ReturnStmt : Stmt {
    ExprPtr value;
    explicit ReturnStmt(ExprPtr v) : value(std::move(v)) {}
};

struct FuncDeclStmt : Stmt {
    std::string name;
    std::vector<std::string> params;
    StmtPtr body;
    
    FuncDeclStmt(std::string n, std::vector<std::string> p, StmtPtr b)
        : name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

struct ImportStmt : Stmt {
    std::string moduleName;
    std::vector<std::string> specificImports;
    bool isFromImport;
    
    explicit ImportStmt(std::string m, bool from = false)
        : moduleName(std::move(m)), isFromImport(from) {}
};

#endif
