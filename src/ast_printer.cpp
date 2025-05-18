#include "expr.hpp"
#include "scanner.hpp"
#include <iostream>
#include <memory>
#include "ast_printer.hpp"


using string = std::string;

void AstPrinter::parenthesize(const std::string& name, const std::vector<const Expr*>& exprs) {
    std::cout << "(" << name;
    for (const auto& expr : exprs) {
        std::cout << " ";
        if (expr) expr->accept(*this);
    }
    std::cout << ")";
}

void  AstPrinter::print(const Expr& expr) {
    return expr.accept(*this);
}

void AstPrinter::print(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        stmt->accept(*this);
    }
}

void  AstPrinter::visit(const Binary& binary) {
    return parenthesize(binary.op.lexeme, {binary.left.get(), binary.right.get()});
}

void AstPrinter::visit(const Grouping& group) {
    return parenthesize("group", {group.expression.get()});
}

void AstPrinter::visit(const Literal& lit) {
    if (std::holds_alternative<std::nullptr_t>(lit.value)) {
        std::cout <<  "nil";
    } else if (std::holds_alternative<double>(lit.value)) {
        std::cout <<  std::to_string(std::get<double>(lit.value));
    } else if (std::holds_alternative<std::string>(lit.value)) {
        std::cout <<  "\"" + std::get<std::string>(lit.value) + "\"";
    } else {
        std::cout <<  "unknown";
    }
}

void AstPrinter::visit(const Unary& unary) {
    parenthesize(unary.op.lexeme, {unary.right.get()});
}

void AstPrinter::visit(const Variable& variable) {
    std::cout << "VAR:" << variable.name.lexeme;
}

void AstPrinter::visit(const Assignment& assignment) {
    parenthesize(assignment.name.lexeme, {assignment.right.get()});
}

void AstPrinter::visit(const ExpressionStmt& stmt) {
    print(*stmt.expr);
};
void AstPrinter::visit(const PrintStmt& stmt) {
    parenthesize("print", {stmt.expr.get()});
};
void AstPrinter::visit(const VarStmt& varStmt) {
    parenthesize(varStmt.name.lexeme, {varStmt.initializer.get()});
};

void AstPrinter::visit(const BlockStmt&blockStmt) {

}