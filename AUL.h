#ifndef AUL_H
#define AUL_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <variant>
#include <iostream>

namespace AUL {

// ----------------------------------------------------------------------
//  Runtime Value
// ----------------------------------------------------------------------
struct Value;
struct ValueHash;
using Table = std::unordered_map<Value, Value, ValueHash>;

struct Value {
    enum Type { NONE, NUMBER, BOOL, STRING, TABLE, NATIVE_FUNC, USER_FUNC };
    Type type;

    // data
    double num;
    bool boolean;
    std::shared_ptr<std::string> str;
    std::shared_ptr<Table> table;
    std::function<Value(const std::vector<Value>&)> nativeFunc;

    struct UserFunction {
        std::vector<std::string> params;
        class Stmt* body;          // points to a BlockStmt (AST node)
        std::shared_ptr<class Environment> closure;
        ~UserFunction();
    };
    std::shared_ptr<UserFunction> userFunc;

    Value() : type(NONE) {}
    Value(double n) : type(NUMBER), num(n) {}
    Value(bool b) : type(BOOL), boolean(b) {}
    Value(const char* s) : type(STRING), str(std::make_shared<std::string>(s)) {}
    Value(const std::string& s) : type(STRING), str(std::make_shared<std::string>(s)) {}
    Value(std::shared_ptr<Table> t) : type(TABLE), table(t) {}
    Value(std::function<Value(const std::vector<Value>&)> f) : type(NATIVE_FUNC), nativeFunc(f) {}
    Value(std::shared_ptr<UserFunction> f) : type(USER_FUNC), userFunc(f) {}

    bool isTruthy() const { return type != NONE && !(type == BOOL && boolean == false); }
    std::string toString() const;
};

bool operator==(const Value& a, const Value& b);
bool operator!=(const Value& a, const Value& b);

// Hash for Value (needed for Table keys)
struct ValueHash {
    std::size_t operator()(const Value& v) const;
};

// ----------------------------------------------------------------------
//  Environment (scope chain)
// ----------------------------------------------------------------------
class Environment : public std::enable_shared_from_this<Environment> {
public:
    Environment(std::shared_ptr<Environment> parent = nullptr);
    void define(const std::string& name, const Value& value, bool mutableFlag);
    void assign(const std::string& name, const Value& value);
    Value get(const std::string& name) const;
    bool isMutable(const std::string& name) const;
    void setGlobal(bool g) { isGlobal = g; }
    std::shared_ptr<Environment> getParent() const { return parent; }
private:
    std::shared_ptr<Environment> parent;
    std::unordered_map<std::string, Value> values;
    std::unordered_map<std::string, bool> mutableFlags;
    bool isGlobal = false;
};

// ----------------------------------------------------------------------
//  AST Forward Declarations
// ----------------------------------------------------------------------
struct Expr;
struct Stmt;
struct BlockStmt;

// ----------------------------------------------------------------------
//  Interpreter
// ----------------------------------------------------------------------
class Interpreter {
public:
    using ErrorHandler = std::function<void(const std::string&)>;
    Interpreter(ErrorHandler err = [](const std::string& msg){ std::cerr << msg << std::endl; });
    ~Interpreter();

    void interpret(const std::string& source);
    void addModule(const std::string& name, const std::string& source); // preload a module

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace AUL

#endif // AUL_H