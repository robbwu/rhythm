#include <expr.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

using string = std::string;

void AstPrinter::parenthesize(const std::string& name, const std::vector<const Expr*>& exprs) {
    std::cout << "(" << name;
    for (const auto& expr : exprs) {
        std::cout << " ";
        expr->accept(*this);
    }
    std::cout << ")";
}

void  AstPrinter::print(const Expr& expr) {
    return expr.accept(*this);
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
    return parenthesize(unary.op.lexeme, {unary.right.get()});
}
