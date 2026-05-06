#include "Value.h"
#include "Interpreter.h"
#include "Environment.h"
#include "AST.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

// ========== VALUE CONSTRUCTORS ==========
Value::Value() : type(Type::NONE), b(false), n(0.0), func(nullptr) {}
Value::Value(bool val) : type(Type::BOOL), b(val), n(0.0), func(nullptr) {}
Value::Value(double val) : type(Type::NUMBER), b(false), n(val), func(nullptr) {}
Value::Value(int val) : type(Type::NUMBER), b(false), n(static_cast<double>(val)), func(nullptr) {}
Value::Value(std::string val) : type(Type::STRING), b(false), n(0.0), s(std::move(val)), func(nullptr) {}
Value::Value(const char* val) : type(Type::STRING), b(false), n(0.0), s(val), func(nullptr) {}
Value::Value(TablePtr tab) : type(Type::TABLE), b(false), n(0.0), t(std::move(tab)), func(nullptr) {}

// ========== VALUE FACTORIES ==========
Value Value::makeNative(std::function<Value(Interpreter&, std::vector<Value>)> native) {
    Value v;
    v.type = Type::FUNCTION;
    v.func = std::make_shared<FunctionObj>();
    v.func->isNative = true;
    v.func->nativeFunc = std::move(native);
    return v;
}

Value Value::makeFunction(
        std::vector<std::string> params,
        std::shared_ptr<Stmt> body,
        Environment env) {
    Value v;
    v.type = Type::FUNCTION;
    v.func = std::make_shared<FunctionObj>();
    v.func->params = std::move(params);
    v.func->body = std::move(body);
    v.func->closureEnv = std::move(env);
    return v;
}

// ========== VALUE OPERATIONS ==========
bool Value::isTruthy() const {
    if (type == Type::NONE)  return false;
    if (type == Type::BOOL)  return b;
    return true;
}

std::string Value::toString() const {
    switch (type) {
        case Type::NONE:   return "none";
        case Type::BOOL:   return b ? "true" : "false";
        case Type::NUMBER: {
            std::ostringstream oss;
            oss << std::setprecision(15) << n;
            std::string str = oss.str();
            if (str.find('.') != std::string::npos) {
                str.erase(str.find_last_not_of('0') + 1, std::string::npos);
                if (str.back() == '.') str.pop_back();
            }
            return str;
        }
        case Type::STRING: return s;
        case Type::TABLE:  return "<table>";
        case Type::FUNCTION: return "<function>";
    }
    return "???";
}

Value Value::call(Interpreter& interp, const std::vector<Value>& args) const {
    if (!isCallable()) throw std::runtime_error("Trying to call non-function");
    
    if (func->isNative) {
        return func->nativeFunc(interp, args);
    } else {
        auto callEnv = func->closureEnv;
        callEnv.pushScope();
        
        if (args.size() != func->params.size()) {
            throw std::runtime_error("Function expected " +
                std::to_string(func->params.size()) + " arguments, got " +
                std::to_string(args.size()));
        }
        
        for (size_t i = 0; i < func->params.size(); ++i) {
            callEnv.define(func->params[i], args[i], false);
        }
        
        try {
            auto* block = dynamic_cast<BlockStmt*>(func->body.get());
            if (!block) throw std::runtime_error("Function body is not a block");
            for (auto& stmt : block->statements) {
                interp.execute(*stmt, callEnv);
            }
        } catch (ReturnException& ret) {
            callEnv.popScope();
            return ret.value;
        }
        callEnv.popScope();
        return Value();
    }
}

// ========== TABLE ==========
Value Table::get(const Value& key) const {
    if (key.isNumber()) {
        int idx = static_cast<int>(key.n);
        if (key.n == static_cast<double>(idx) && idx >= 0 && idx < static_cast<int>(array.size()))
            return array[idx];
        std::string keystr = key.toString();
        auto it = dict.find(keystr);
        if (it != dict.end()) return it->second;
        return Value();
    } else if (key.isString()) {
        auto it = dict.find(key.s);
        if (it != dict.end()) return it->second;
        return Value();
    }
    return Value();
}

void Table::set(const Value& key, const Value& val) {
    if (key.isNumber()) {
        int idx = static_cast<int>(key.n);
        if (key.n == static_cast<double>(idx) && idx >= 0) {
            if (idx >= static_cast<int>(array.size()))
                array.resize(idx + 1, Value());
            array[idx] = val;
            return;
        }
    }
    dict[key.toString()] = val;
}

int Table::len() const {
    return static_cast<int>(array.size());
}

// ========== AST CONSTRUCTORS ==========
LiteralExpr::LiteralExpr(Value v) : value(std::move(v)) {}
