#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "Environment.h"
#include "AST.h"
#include <string>

// ========== FUNCTION OBJECT ==========
struct FunctionObj {
    std::vector<std::string> params;
    std::shared_ptr<Stmt> body;
    Environment closureEnv;
    bool isNative;
    std::function<Value(Interpreter&, std::vector<Value>)> nativeFunc;

    FunctionObj()
        : isNative(false), nativeFunc(nullptr) {}
};

// ========== INTERPRETER ==========
class Interpreter {
public:
    Interpreter();

    Value evaluate(const std::string& source);
    void evaluateFile(const std::string& filename);

    // Public for Value::call
    void execute(Stmt& stmt, Environment& env);

private:
    Environment env_;

    Value evaluate(Expr& expr, Environment& env);
    void initializeBuiltins();
};

#endif
