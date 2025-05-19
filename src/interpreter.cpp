#include "interpreter.hpp"
#include "expr.hpp"
#include <iostream>
#include <ostream>

// ── Native “clock” ──────────────────────────────────────────────────────────────
class ClockCallable final : public LoxCallable {
public:
    // zero arguments
    int arity() override { return 0; }

    // return seconds since Unix epoch, as a double
    Value call(Interpreter*, std::vector<Value>) override {
        using namespace std::chrono;
        auto now_ms = duration_cast<milliseconds>(
                          system_clock::now().time_since_epoch()).count();
        return static_cast<double>(now_ms) / 1000.0;
    }

    std::string toString() const { return "<native fn>"; }
};

Interpreter::Interpreter() {
    globals.define("clock", new ClockCallable());
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
            if (std::holds_alternative<std::string>(left) && std::holds_alternative<std::string>(right))
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
    _result = f->call(this, arguments);
}


bool  Interpreter::isTruthy(Value value) {
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
    std::visit([&](const auto& x) { std::cout << x; }, val);
    std::cout << std::endl;
}

void Interpreter::visit(const VarStmt& varStmt) {
    Value value = nullptr;
    if (varStmt.initializer != nullptr) {
        value = eval(*varStmt.initializer);
    }
    env->define(varStmt.name.lexeme, value);
}

void Interpreter::visit(const Variable& variable) {
    _result = env->get(variable.name);
}

void Interpreter::visit(const Assignment& assignment) {
    auto value = eval(*assignment.right);
    env->assign(assignment.name, value);
    _result = value; // Why? associated assignment?
}

void Interpreter::visit(const BlockStmt& block) {
    executeBlock(block.statements, std::move(std::make_unique<Environment>(env)));
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
void Interpreter::executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, std::unique_ptr<Environment> new_env) {
    auto previous = env;
    env = new_env.get();
    for (auto &statement : statements) {
        execute(*statement);
    }
    env = previous;
}