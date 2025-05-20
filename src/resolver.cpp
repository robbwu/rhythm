#include "resolver.hpp"

#include "exception.hpp"
#include "interpreter.hpp"

void Resolver::visit(const BlockStmt &stmts) {
    beginScope();
    for (auto &stmt : stmts.statements) {
        resolve(stmt.get());
    }
    endScope();
}

void Resolver::resolve(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto &stmt :stmts ) {
        resolve(stmt.get());
    }
}


void Resolver::resolve(Stmt* stmt) {
    stmt->accept(*this);
}

void Resolver::resolve(Expr* expr) {
    expr->accept(*this);
}

void Resolver::beginScope() {
    scopes.emplace_back();
}

void Resolver::endScope() {
    scopes.pop_back();
}

void Resolver::visit(const VarStmt& stmt) {
    declare(stmt.name);
    if (stmt.initializer != nullptr) {
        resolve(stmt.initializer.get());
    }
    define(stmt.name);
}

void Resolver::declare(Token name) {
    if (scopes.empty()) return;
    auto& scope = scopes.back();
    if (scope.contains(name.lexeme)) {
        throw RuntimeError(name, "Already a variable with this name in this scope.");
    }
    scope[name.lexeme] = false;
}

void Resolver::define(Token name) {
    if (scopes.empty()) return;
    auto& scope = scopes.back();
    scope[name.lexeme] = true;
}

void Resolver::visit(const Variable& var) {
    if (!scopes.empty()) {
        auto& scope = scopes.back();
        if (scope.contains(var.name.lexeme) && scope[var.name.lexeme] == false) {
            throw RuntimeError(var.name, "Can't read local variable in its own initializer.");
        }
    }
    resolveLocal(var, var.name);
}

void Resolver::resolveLocal(const Expr& expr, const Token& name) {
    for (int i = scopes.size() -1; i >= 0; i--) {
        if (scopes[i].contains(name.lexeme)) {
            interpreter->resolve(&expr, scopes.size() - i - 1);
            return;
        }
    }
}

void Resolver::visit(const Assignment& assignment) {
    resolve(assignment.right.get());
    resolveLocal(assignment, assignment.name);
}

void Resolver::visit(const FunctionStmt& stmt) {
    declare(stmt.name);
    define(stmt.name);

    resolveFunction(stmt, FunctionType::FUNCTION);
    // resolveFunction(stmt);
}

void Resolver::resolveFunction(const FunctionStmt& stmt, FunctionType type) {
    FunctionType enclosing_function = current_function;
    current_function = type;
    beginScope();
    for (auto& param : stmt.params) {
        declare(param);
        define(param);
    }
    resolve(stmt.body);
    endScope();
    current_function = enclosing_function;
}

void Resolver::visit(const ExpressionStmt& stmt) {
    resolve(stmt.expr.get());
}

void Resolver::visit(const IfStmt& stmt) {
    resolve(stmt.condition.get());
    resolve(stmt.thenBlock.get());
    if (stmt.elseBlock != nullptr) {
        resolve(stmt.elseBlock.get());
    }
}

void Resolver::visit(const PrintStmt& stmt) {
    resolve(stmt.expr.get());
}

void Resolver::visit(const ReturnStmt& stmt) {
    if (current_function == FunctionType::NONE) {
        throw RuntimeError(stmt.kw, "Can't return from top-level code.");
    }
    if (stmt.value != nullptr) {
        resolve(stmt.value.get());
    }
}

void Resolver::visit(const WhileStmt& stmt) {
    resolve(stmt.condition.get());
    resolve(stmt.body.get());
}


void Resolver::visit(const Binary& expr) {
    resolve(expr.left.get());
    resolve(expr.right.get());
}

void Resolver::visit(const Call& expr) {
    resolve(expr.callee.get());
    for (auto& argument : expr.arguments) {
        resolve(argument.get());
    }
}
void Resolver::visit(const Grouping& expr) {
    resolve(expr.expression.get());
}
void Resolver::visit(const Literal& expr) {
}
void Resolver::visit(const Logical& expr) {
    resolve(expr.left.get());
    resolve(expr.right.get());
}
void Resolver::visit(const Unary& expr) {
    resolve(expr.right.get());
}