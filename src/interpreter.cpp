#include "interpreter.hpp"

#include <iostream>
#include <ostream>

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

}

void Interpreter::visit(const Variable& variable) {

}