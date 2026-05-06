#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "Value.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

// ========== EXCEPTIONS ==========
class BreakException : public std::exception {};
class ContinueException : public std::exception {};

class ReturnException : public std::exception {
public:
    Value value;
    explicit ReturnException(Value v) : value(std::move(v)) {}
};

// ========== VARIABLE ==========
struct Variable {
    Value value;
    bool isConst;
};

// ========== ENVIRONMENT (Scoping) ==========
class Environment {
public:
    Environment();

    void pushScope();
    void popScope();

    Variable* get(const std::string& name);
    void define(const std::string& name, Value value, bool isConst, bool isGlobal = false);
    void assign(const std::string& name, Value value);
    
    // For closures
    Environment snapshot() const;

private:
    std::vector<std::shared_ptr<std::map<std::string, Variable>>> scopes_;
};

#endif
