#include "Environment.h"
#include <stdexcept>

Environment::Environment() {
    pushScope();
}

void Environment::pushScope() {
    scopes_.push_back(std::make_shared<std::map<std::string, Variable>>());
}

void Environment::popScope() {
    scopes_.pop_back();
}

Variable* Environment::get(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = (*it)->find(name);
        if (found != (*it)->end()) return &found->second;
    }
    return nullptr;
}

void Environment::define(const std::string& name, Value value, bool isConst, bool isGlobal) {
    if (isGlobal) {
        (*scopes_[0])[name] = Variable{std::move(value), isConst};
    } else {
        (*scopes_.back())[name] = Variable{std::move(value), isConst};
    }
}

void Environment::assign(const std::string& name, Value value) {
    Variable* var = get(name);
    if (!var) throw std::runtime_error("Undefined variable '" + name + "'");
    if (var->isConst) throw std::runtime_error("Cannot reassign constant '" + name + "'");
    var->value = std::move(value);
}

Environment Environment::snapshot() const {
    Environment env;
    env.scopes_ = scopes_;
    return env;
}
