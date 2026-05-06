#ifndef VALUE_H
#define VALUE_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <variant>

// Forward declarations
class Interpreter;
struct FunctionObj;
struct Stmt;

using TablePtr = std::shared_ptr<struct Table>;

// ========== VALUE TYPE ==========
struct Value {
    enum class Type { NONE, BOOL, NUMBER, STRING, TABLE, FUNCTION };
    
    Type type;
    bool   b;
    double n;
    std::string s;
    TablePtr t;
    std::shared_ptr<FunctionObj> func;

    // Constructors
    Value();
    Value(bool val);
    Value(double val);
    Value(int val);
    Value(std::string val);
    Value(const char* val);
    Value(TablePtr tab);

    // Factory methods
    static Value makeNative(std::function<Value(Interpreter&, std::vector<Value>)> native);
    static Value makeFunction(
        std::vector<std::string> params,
        std::shared_ptr<Stmt> body,
        class Environment env);

    // Type checking
    bool isNone()     const { return type == Type::NONE; }
    bool isBool()     const { return type == Type::BOOL; }
    bool isNumber()   const { return type == Type::NUMBER; }
    bool isString()   const { return type == Type::STRING; }
    bool isTable()    const { return type == Type::TABLE; }
    bool isFunction() const { return type == Type::FUNCTION; }
    bool isCallable() const { return isFunction(); }

    // Operations
    Value call(Interpreter& interp, const std::vector<Value>& args) const;
    bool isTruthy() const;
    std::string toString() const;
};

// ========== TABLE ==========
struct Table {
    std::vector<Value> array;
    std::map<std::string, Value> dict;

    Value get(const Value& key) const;
    void set(const Value& key, const Value& val);
    int len() const;
};

#endif
