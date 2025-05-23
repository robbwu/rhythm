#include "interpreter.hpp"
#include "expr.hpp"
#include <iostream>
#include <ostream>

#include "lox_function.hpp"
#include "native_func.hpp"
#include "native_func_array.hpp"
#include "native_math.hpp"

Interpreter::Interpreter() {
    globals.define("clock", new ClockCallable());
    globals.define("printf", new PrintfCallable());
    globals.define("len", new LenCallable());
    // globals.define("at", new AtCallable());
    // globals.define("set", new SetCallable());
    globals.define("push", new PushCallable());
    globals.define("pop", new PopCallable());
    globals.define("readline", new ReadlineCallable());
    globals.define("split", new SplitCallable());
    globals.define("floor", new FloorCallable());
    globals.define("ceil", new CeilCallable());
    globals.define("assert", new AssertCallable());
    globals.define("for_each", new ForEachCallable());
}


void Interpreter::visit(const Literal &lit) {
    _result = lit.value;
}

void Interpreter::visit(const Grouping &grouping) {
    _result = eval(*grouping.expression);
}

void Interpreter::visit(const Unary &unary) {
    auto right = eval(*unary.right);
    switch (unary.op.type) {
        case TokenType::MINUS:
            _result = - std::get<double>(right);
            break;
        case TokenType::BANG:
            _result = !isTruthy(right);
            break;
        default:
            _result = nullptr;
    }
}

void Interpreter::visit(const Binary &expr) {
    auto left = eval(*expr.left);
    auto right = eval(*expr.right);

    switch (expr.op.type) {
        case TokenType::MINUS:
            _result = std::get<double>(left)  - std::get<double>(right);
            break;
        case TokenType::SLASH:
            _result = std::get<double>(left)  / std::get<double>(right);
            break;
        case TokenType::STAR:
            _result = std::get<double>(left)  * std::get<double>(right);
            break;
        case TokenType::PLUS:
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right))
                _result = std::get<double>(left) + std::get<double>(right);
            else if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
                _result = std::get<std::string>(left) + std::get<std::string>(right);
            break;
        case TokenType::GREATER:
            _result = std::get<double>(left) > std::get<double>(right);
            break;
        case TokenType::GREATER_EQUAL:
            _result = std::get<double>(left) >= std::get<double>(right);
            break;
        case TokenType::LESS:
            _result = std::get<double>(left) < std::get<double>(right);
            break;
        case TokenType::LESS_EQUAL:
            _result = std::get<double>(left) <= std::get<double>(right);
            break;
        case TokenType::BANG_EQUAL:
            _result = (left != right);
            break;
        case TokenType::EQUAL_EQUAL:
            _result = (left == right);
            break;
        default:
            _result = nullptr;
    }

}

void Interpreter::visit(const Logical& logical) {
    auto left = eval(*logical.left);

    if (logical.op.type == TokenType::OR) {
        if (isTruthy(left)) {
            _result = left;
            return;
        }
    } else if (logical.op.type == TokenType::AND) {
        if (!isTruthy(left)) {
            _result = left;
            return;
        }
    } else {
        throw std::runtime_error("Invalid logical operator");
    }
    _result = eval(*logical.right);
};

void Interpreter::visit(const Call &call) {
    auto callee = eval(*call.callee);
    std::vector<Value> arguments;
    arguments.reserve(call.arguments.size());
    for (auto &arg : call.arguments) {
        arguments.push_back(eval(*arg));
    }
    if (!std::holds_alternative<LoxCallable*>(callee)) {
        throw RuntimeError(call.paren, "Can only call functions and classes.");
    }
    auto f = std::get<LoxCallable*>(callee);
    // arity -1 means variable number of arguments
    if (f->arity() != -1 && f->arity() != arguments.size()) {
        throw RuntimeError(call.paren,std::format("expected {} arguments but got {}",f->arity(),arguments.size()));
    }
    _result = f->call(this, arguments);
}

void Interpreter::visit(const ArrayLiteral &alit) {
    std::vector<Value> values;
    values.reserve(alit.elements.size());
    for (auto &expr : alit.elements) {
        values.push_back(eval(*expr));
    }
    _result =  std::make_shared<Array>(values);
}

void Interpreter::visit(const MapLiteral &mlit) {
    std::unordered_map<Value, Value> values;
    values.reserve(mlit.pairs.size());
    for (auto &pairs : mlit.pairs) {
        Value key = eval(*pairs.first);
        Value val = eval(*pairs.second);
        if (!std::holds_alternative<nullptr_t>(val)) // nil cannot be value in a map
            values[key] = val;
    }
    _result =  std::make_shared<Map>(values);
}

void Interpreter::visit(const Subscript& sub) {
    auto obj = eval(*sub.object);
    auto index = eval(*sub.index);
    if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        if (!std::holds_alternative<double>(index)) {
            throw RuntimeError(sub.bracket, "array index must be a number");
        }
        auto ind = std::get<double>(index);
        if (!is_integer(ind)) {
            throw RuntimeError(sub.bracket, "index must be an integer");
        }
        auto& array = std::get<std::shared_ptr<Array>>(obj);
        _result =  array->data.at((int)ind);
        return;
    }
    if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
        auto map = std::get<std::shared_ptr<Map>>(obj);
        auto it = map->data.find(index);
        if (it == map->data.end()) {
            _result = nullptr;
        } else {
            _result = it->second;
        }
        return;
    }
    throw RuntimeError(sub.bracket, "subscript must be of an array or map");
}



bool  Interpreter::isTruthy(const Value& value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return false;
    }
    return true;
}


void Interpreter::visit(const ExpressionStmt& exprStmt) {
    eval(*exprStmt.expr);
}

void Interpreter::visit(const PrintStmt& printStmt) {
    auto val = eval(*printStmt.expr);
    std::cout << val << std::endl;
}

void Interpreter::visit(const VarStmt& varStmt) {
    Value value = nullptr;
    if (varStmt.initializer != nullptr) {
        value = eval(*varStmt.initializer);
    }
    env->define(varStmt.name.lexeme, value);
}

void Interpreter::visit(const Variable& variable) {
    // _result = env->get(variable.name);
    // _result = lookUpVariable(variable.name,  &variable);
    auto it = varLocations.find(&variable);
    if (it != varLocations.end()) {
        // Fast path: direct index access
        _result = env->getAt(it->second.distance, it->second.index);
        return;
    }
    // Fallback for globals or unresolved variables
    _result = globals.get(variable.name);
}

void Interpreter::visit(const Assignment& assignment) {
    auto value = eval(*assignment.right);
    // env->assign(assignment.name, value);
    auto it = varLocations.find(&assignment);
    if (it != varLocations.end()) {
        env->assignAt(it->second.distance, it->second.index, value);
    } else {
        globals.assign(assignment.name, value);
    }
    _result = value; // Why? chain of assignment?
}

void Interpreter::visit(const PropertyAccess& prop) {
    auto obj = eval(*prop.object);

    // Convert property access to subscript access with string key
    if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
        auto map = std::get<std::shared_ptr<Map>>(obj);
        Value key = prop.name.lexeme;  // Use the property name as string key
        auto it = map->data.find(key);
        if (it == map->data.end()) {
            _result = nullptr;
        } else {
            _result = it->second;
        }
        return;
    }

    throw RuntimeError(prop.name, "Only maps can have properties accessed with dot notation");
}

void Interpreter::visit(const SubscriptAssignment& assignment) {
    Value obj = eval(*assignment.object);
    Value index = eval(*assignment.index);
    Value value = eval(*assignment.value);

    if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        if (!std::holds_alternative<double>(index)) {
            throw RuntimeError(assignment.bracket, "array index must be a number");
        }
        auto ind = std::get<double>(index);
        if (!is_integer(ind)) {
            throw RuntimeError(assignment.bracket, "index must be an integer");
        }
        auto idx = (int) ind;
        auto& array = std::get<std::shared_ptr<Array>>(obj);
        if (idx < 0 || idx >= array->data.size()) {
            throw RuntimeError(assignment.bracket,
                "Index out of bounds: " + std::to_string(idx) +
                " (size: " + std::to_string(array->data.size()) + ")");
        }
        array->data[idx] = value;
        _result =  value;
        return;
    }

    if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
        // value being nil means removing it from underlying Map

        auto map = std::get<std::shared_ptr<Map>>(obj);
        auto it = map->data.find(index);
        if (std::holds_alternative<nullptr_t>(value)) {
            if (it != map->data.end()) // remove key if val is nil
                map->data.erase(it);
            _result = nullptr;
            return;
        }
        map->data[index] = value;
        _result =  value;
        return;
    }

    throw RuntimeError(assignment.bracket, "Only arrays and maps can be subscripted.");
}

void Interpreter::visit(const BlockStmt& block) {
    // auto new_env = new Environment(env); // FIXME: this leaks memory
    Environment new_env(env);
    executeBlock(block.statements, &new_env);
}

void Interpreter::visit(const IfStmt& ifStmt) {
    if (isTruthy(eval(*ifStmt.condition))) {
        execute(*ifStmt.thenBlock);
    } else if (ifStmt.elseBlock != nullptr) {
        execute(*ifStmt.elseBlock);
    }
}

void Interpreter::visit(const WhileStmt& whileStmt) {
    while (isTruthy(eval(*whileStmt.condition))) {
        execute(*whileStmt.body);
    }
}

// this function owns the new environment
void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, Environment* new_env) {
    EnvGuard guard(*this, new_env);

    // could throw--need to handle env restoration in such case
    for (auto &statement : statements) {
        execute(*statement);
    }
}

void Interpreter::visit(const FunctionStmt& stmt) {
    // FIXME: who owns/deletes this function?
    LoxCallable* function = new LoxFunction(&stmt, env);
    env->define(stmt.name.lexeme, function);
}

void Interpreter::visit(const ReturnStmt& returnStmt) {
    Value value = nullptr;
    if (returnStmt.value != nullptr) {
        value = eval(*returnStmt.value);
    }
    throw Return(value);
}

// store the depth of the expr in the interpreter;
// void Interpreter::resolve(const Expr* expr, int depth) {
//     locals[expr] = depth;
// }

void Interpreter::resolveWithIndex(const Expr* expr, int distance, int index) {
    varLocations[expr] = {distance, index};
}