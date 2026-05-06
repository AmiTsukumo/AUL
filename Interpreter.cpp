#include "Interpreter.h"
#include "Lexer.h"
#include "Parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

Interpreter::Interpreter() {
    initializeBuiltins();
}

void Interpreter::initializeBuiltins() {
    env_.define("print", Value::makeNative([](Interpreter&, std::vector<Value> args) -> Value {
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) std::cout << "\t";
            std::cout << args[i].toString();
        }
        std::cout << std::endl;
        return Value();
    }), true, true);

    env_.define("range", Value::makeNative([](Interpreter&, std::vector<Value> args) -> Value {
        double start = 0, end = 0, step = 1;
        if (args.size() == 1) { start = 0; end = args[0].n; }
        else if (args.size() == 2) { start = args[0].n; end = args[1].n; }
        else if (args.size() >= 3) { start = args[0].n; end = args[1].n; step = args[2].n; }
        else return Value();
        auto tab = std::make_shared<Table>();
        for (double i = start; i < end; i += step)
            tab->array.push_back(Value(i));
        return Value(tab);
    }), true, true);

    env_.define("entries", Value::makeNative([](Interpreter&, std::vector<Value> args) -> Value {
        if (args.size() != 1 || !args[0].isTable()) throw std::runtime_error("entries expects a table");
        return args[0];
    }), true, true);

    env_.define("len", Value::makeNative([](Interpreter&, std::vector<Value> args) -> Value {
        if (args.size() != 1) throw std::runtime_error("len expects 1 argument");
        if (args[0].isTable()) return Value(args[0].t->len());
        if (args[0].isString()) return Value(static_cast<int>(args[0].s.length()));
        return Value(0);
    }), true, true);

    env_.define("push", Value::makeNative([](Interpreter&, std::vector<Value> args) -> Value {
        if (args.size() != 2 || !args[0].isTable()) throw std::runtime_error("push expects table and value");
        args[0].t->array.push_back(args[1]);
        return Value();
    }), true, true);
}

Value Interpreter::evaluate(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parse();
    Value result;
    try {
        for (auto& stmt : program) {
            execute(*stmt, env_);
        }
    } catch (const ReturnException&) {
        // top-level return ignored
    }
    return result;
}

void Interpreter::evaluateFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("Cannot open file: " + filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    evaluate(buffer.str());
}

void Interpreter::execute(Stmt& stmt, Environment& env) {
    if (auto* s = dynamic_cast<ExprStmt*>(&stmt)) {
        evaluate(*s->expr, env);
    } else if (auto* s = dynamic_cast<VarDeclStmt*>(&stmt)) {
        Value initVal = s->init ? evaluate(*s->init, env) : Value();
        env.define(s->name, initVal, s->isConst, s->isGlobal);
    } else if (auto* s = dynamic_cast<FuncDeclStmt*>(&stmt)) {
        auto bodyShared = std::shared_ptr<Stmt>(std::move(s->body));
        Value funcVal = Value::makeFunction(s->params, bodyShared, env.snapshot());
        env.define(s->name, funcVal, true, false);
    } else if (auto* s = dynamic_cast<IfStmt*>(&stmt)) {
        if (evaluate(*s->condition, env).isTruthy()) {
            execute(*s->thenBranch, env);
        } else if (s->elseBranch) {
            execute(*s->elseBranch, env);
        }
    } else if (auto* s = dynamic_cast<WhileStmt*>(&stmt)) {
        while (evaluate(*s->condition, env).isTruthy()) {
            try { execute(*s->body, env); }
            catch (BreakException&) { break; }
            catch (ContinueException&) { continue; }
        }
    } else if (auto* s = dynamic_cast<ForNumericStmt*>(&stmt)) {
        double start = evaluate(*s->start, env).n;
        double end   = evaluate(*s->end, env).n;
        double step  = s->step ? evaluate(*s->step, env).n : 1.0;
        for (double i = start; (step > 0 ? i < end : i > end); i += step) {
            env.pushScope();
            env.define(s->varName, Value(i), false);
            try { execute(*s->body, env); }
            catch (BreakException&) { env.popScope(); break; }
            catch (ContinueException&) { env.popScope(); continue; }
            env.popScope();
        }
    } else if (auto* s = dynamic_cast<ForInStmt*>(&stmt)) {
        Value iterable = evaluate(*s->iterable, env);
        if (!iterable.isTable()) throw std::runtime_error("For-in only supports tables");
        Table& table = *iterable.t;
        
        if (s->loopVars.size() == 1) {
            std::vector<Value> values;
            for (auto& v : table.array) values.push_back(v);
            for (auto& kv : table.dict) values.push_back(kv.second);
            for (auto& val : values) {
                env.pushScope();
                env.define(s->loopVars[0], val, false);
                try { execute(*s->body, env); }
                catch (BreakException&) { env.popScope(); break; }
                catch (ContinueException&) { env.popScope(); continue; }
                env.popScope();
            }
        } else if (s->loopVars.size() == 2) {
            struct Pair { std::string key; Value value; };
            std::vector<Pair> pairs;
            for (size_t idx = 0; idx < table.array.size(); ++idx)
                pairs.push_back({std::to_string(idx), table.array[idx]});
            for (auto& kv : table.dict)
                pairs.push_back({kv.first, kv.second});
            for (auto& p : pairs) {
                env.pushScope();
                env.define(s->loopVars[0], Value(p.key), false);
                env.define(s->loopVars[1], p.value, false);
                try { execute(*s->body, env); }
                catch (BreakException&) { env.popScope(); break; }
                catch (ContinueException&) { env.popScope(); continue; }
                env.popScope();
            }
        }
    } else if (auto* s = dynamic_cast<AssignmentStmt*>(&stmt)) {
        Value val = evaluate(*s->value, env);
        if (s->targetType == AssignmentStmt::SIMPLE) {
            env.assign(s->name, val);
        } else {
            Value obj = evaluate(*s->objectExpr, env);
            Value idx = evaluate(*s->indexExpr, env);
            if (!obj.isTable()) throw std::runtime_error("Attempt to index non-table");
            obj.t->set(idx, val);
        }
    } else if (auto* s = dynamic_cast<BlockStmt*>(&stmt)) {
        env.pushScope();
        try {
            for (auto& inner : s->statements) execute(*inner, env);
        } catch (...) { env.popScope(); throw; }
        env.popScope();
    } else if (auto* s = dynamic_cast<ReturnStmt*>(&stmt)) {
        Value val = s->value ? evaluate(*s->value, env) : Value();
        throw ReturnException(val);
    } else if (dynamic_cast<BreakStmt*>(&stmt)) {
        throw BreakException();
    } else if (dynamic_cast<ContinueStmt*>(&stmt)) {
        throw ContinueException();
    } else if (auto* s = dynamic_cast<ImportStmt*>(&stmt)) {
        std::cout << "[import " << s->moduleName;
        if (s->isFromImport && !s->specificImports.empty()) {
            std::cout << " (";
            for (size_t i = 0; i < s->specificImports.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << s->specificImports[i];
            }
            std::cout << ")";
        }
        std::cout << " - ignored]" << std::endl;
    }
}

Value Interpreter::evaluate(Expr& expr, Environment& env) {
    if (auto* e = dynamic_cast<LiteralExpr*>(&expr)) {
        return e->value;
    }
    
    if (auto* e = dynamic_cast<IdentifierExpr*>(&expr)) {
        Variable* var = env.get(e->name);
        if (!var) throw std::runtime_error("Undefined variable '" + e->name + "'");
        return var->value;
    }
    
    if (auto* e = dynamic_cast<BinaryExpr*>(&expr)) {
        // Special handling for logical AND: return boolean, short-circuit
        if (e->op == TokenType::AND) {
            Value left = evaluate(*e->left, env);
            if (!left.isTruthy()) return Value(false);
            Value right = evaluate(*e->right, env);
            return Value(right.isTruthy());
        }
        
        // Special handling for logical OR: return boolean, short-circuit
        if (e->op == TokenType::OR) {
            Value left = evaluate(*e->left, env);
            if (left.isTruthy()) return Value(true);
            Value right = evaluate(*e->right, env);
            return Value(right.isTruthy());
        }
        
        // For all other operators, evaluate both sides
        Value left = evaluate(*e->left, env);
        Value right = evaluate(*e->right, env);
        
        switch (e->op) {
            case TokenType::PLUS:
                if (left.isNumber() && right.isNumber()) return Value(left.n + right.n);
                if (left.isString() || right.isString()) return Value(left.toString() + right.toString());
                throw std::runtime_error("Invalid operands for '+'");
            
            case TokenType::MINUS: 
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot subtract non-numbers");
                return Value(left.n - right.n);
            
            case TokenType::STAR:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot multiply non-numbers");
                return Value(left.n * right.n);
            
            case TokenType::SLASH:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot divide non-numbers");
                return Value(left.n / right.n);
            
            case TokenType::PERCENT:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot modulo non-numbers");
                return Value(std::fmod(left.n, right.n));
            
            case TokenType::CARET:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot exponentiate non-numbers");
                return Value(std::pow(left.n, right.n));
            
            case TokenType::DOTDOT:
                return Value(left.toString() + right.toString());
            
            case TokenType::EQ: {
                if (left.type != right.type) return Value(false);
                switch (left.type) {
                    case Value::Type::NONE: return Value(true);
                    case Value::Type::BOOL: return Value(left.b == right.b);
                    case Value::Type::NUMBER: return Value(left.n == right.n);
                    case Value::Type::STRING: return Value(left.s == right.s);
                    case Value::Type::TABLE: return Value(left.t.get() == right.t.get());
                    case Value::Type::FUNCTION: return Value(left.func.get() == right.func.get());
                }
                return Value(false);
            }
            
            case TokenType::NEQ: {
                if (left.type != right.type) return Value(true);
                switch (left.type) {
                    case Value::Type::NONE: return Value(false);
                    case Value::Type::BOOL: return Value(left.b != right.b);
                    case Value::Type::NUMBER: return Value(left.n != right.n);
                    case Value::Type::STRING: return Value(left.s != right.s);
                    case Value::Type::TABLE: return Value(left.t.get() != right.t.get());
                    case Value::Type::FUNCTION: return Value(left.func.get() != right.func.get());
                }
                return Value(true);
            }
            
            case TokenType::LT:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot compare non-numbers");
                return Value(left.n < right.n);
            
            case TokenType::GT:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot compare non-numbers");
                return Value(left.n > right.n);
            
            case TokenType::LE:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot compare non-numbers");
                return Value(left.n <= right.n);
            
            case TokenType::GE:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Cannot compare non-numbers");
                return Value(left.n >= right.n);
            
            case TokenType::AMP:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Invalid operands for &");
                return Value(static_cast<double>(static_cast<int>(left.n) & static_cast<int>(right.n)));
            
            case TokenType::PIPE:
                if (!left.isNumber() || !right.isNumber()) throw std::runtime_error("Invalid operands for ||");
                return Value(static_cast<double>(static_cast<int>(left.n) | static_cast<int>(right.n)));
            
            default:
                throw std::runtime_error("Unknown binary operator");
        }
    }
    
    if (auto* e = dynamic_cast<UnaryExpr*>(&expr)) {
        Value operand = evaluate(*e->operand, env);
        switch (e->op) {
            case TokenType::MINUS:
                if (!operand.isNumber()) throw std::runtime_error("Cannot negate non-number");
                return Value(-operand.n);
            
            case TokenType::BANG:
            case TokenType::NOT:
                if (operand.isBool()) return Value(!operand.b);
                if (operand.isNumber()) return Value(static_cast<double>(~static_cast<int>(operand.n)));
                throw std::runtime_error("Invalid operand for not/!");
            
            default:
                throw std::runtime_error("Unknown unary operator");
        }
    }
    
    if (auto* e = dynamic_cast<CallExpr*>(&expr)) {
        Value callee = evaluate(*e->callee, env);
        if (!callee.isCallable()) throw std::runtime_error("Trying to call non-function");
        std::vector<Value> args;
        for (auto& arg : e->args) args.push_back(evaluate(*arg, env));
        return callee.call(*this, args);
    }
    
    if (auto* e = dynamic_cast<IndexExpr*>(&expr)) {
        Value obj = evaluate(*e->object, env);
        Value idx = evaluate(*e->index, env);
        if (!obj.isTable()) throw std::runtime_error("Attempt to index non-table");
        return obj.t->get(idx);
    }
    
    throw std::runtime_error("Unhandled expression");
}
