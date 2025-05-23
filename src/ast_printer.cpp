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

void  AstPrinter::visit(const Logical& logical) {
    return parenthesize(logical.op.lexeme, {logical.left.get(), logical.right.get()});
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

void AstPrinter::visit(const ArrayLiteral& alit) {
    std::cout << "[";
    for (const auto& arg : alit.elements) {
        arg->accept(*this);
        std::cout << ", ";
    }
    std::cout << "]";
}

void AstPrinter::visit(const MapLiteral& mlit) {
    std::cout << "{";
    for (const auto& pair : mlit.pairs) {
        pair.first->accept(*this);
        std::cout << ": ";
        pair.second->accept(*this);
        std::cout << ", ";
    }
    std::cout << "}";
}

void AstPrinter::visit(const Subscript& sub) {
    // std::cout << "TODO subscript" << std::endl;
    sub.object->accept(*this);
    std::cout << "[ ";
    sub.index->accept(*this);
    std::cout << "]";
}

void AstPrinter::visit(const PropertyAccess& prop) {
    prop.object->accept(*this);
    std::cout << "." << prop.name.lexeme;
}

void AstPrinter::visit(const Variable& variable) {
    std::cout << "VAR:" << variable.name.lexeme;
}

void AstPrinter::visit(const FunctionExpr& expr) {
    std::cout << "ANON_FUN_" << expr.params.size() << "(";
    for (const auto& param : expr.params) {
        std::cout << param.lexeme << ",";
    }
    std::cout << ")";
}

void AstPrinter::visit(const Assignment& assignment) {
    std::cout << assignment.name.lexeme << " = ";
    assignment.right->accept(*this);
}

void AstPrinter::visit(const SubscriptAssignment& expr) {
    expr.object->accept(*this);
    std::cout << "[ ";
    expr.index->accept(*this);
    std::cout << "]";
    std:: cout << " = ";
    expr.value->accept(*this);
}

void AstPrinter::visit(const Call& call) {
    std::cout << "CALL ";
    call.callee->accept(*this);
    std::cout << "_" << call.arguments.size();
    std::cout << "(";
    for (const auto& arg : call.arguments) {
        arg->accept(*this);
        std::cout << ",";
    }
    std::cout << ")";
    // std::cout << std::endl;
}

// statements
void AstPrinter::visit(const ExpressionStmt& stmt) {
    std::cout << get_indent();
    print(*stmt.expr);
    std::cout << std::endl;
};
void AstPrinter::visit(const PrintStmt& stmt) {
    std::cout << get_indent();
    parenthesize("print", {stmt.expr.get()});
    std::cout << std::endl;
};
void AstPrinter::visit(const VarStmt& varStmt) {
    std::cout << get_indent();
    parenthesize(varStmt.name.lexeme, {varStmt.initializer.get()});
    std::cout << std::endl;
};

void AstPrinter::visit(const BlockStmt& blockStmt) {
    std::cout << get_indent() << "BLOCK:" << blockStmt.statements.size() << "\n";
    IndentGuard guard(*this);
    for (const auto& stmt : blockStmt.statements) {
        stmt->accept(*this);
    }
}
void AstPrinter::visit(const IfStmt& ifStmt) {
    std::cout << get_indent() << "IF ";
    ifStmt.condition->accept(*this);
    std::cout << "\n";
    {
        std::cout << get_indent() << "THEN\n";
        IndentGuard guard(*this);
        ifStmt.thenBlock->accept(*this);
        // std::cout << "\n";
    }
    if (ifStmt.elseBlock) {
        std::cout << get_indent() << "ELSE\n";
        IndentGuard guard(*this);
        ifStmt.elseBlock->accept(*this);
        // std::cout << "\n";
    }
}

void AstPrinter::visit(const WhileStmt& whileStmt) {
    std::cout << get_indent() << "WHILE ";
    whileStmt.condition->accept(*this);
    std::cout << "\n";
    IndentGuard guard(*this);
    whileStmt.body->accept(*this);
}

void AstPrinter::visit(const FunctionStmt& funStmt) {
    std::cout << get_indent() << "FUNCTION " << funStmt.name.lexeme << "(";
    for (size_t i = 0; i < funStmt.params.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << funStmt.params[i].lexeme;
    }
    std::cout << ")\n";

    IndentGuard guard(*this);
    for (const auto& stmt : funStmt.body) {
        stmt->accept(*this);
    }
}

void AstPrinter::visit(const ReturnStmt& returnStmt) {
    std::cout << get_indent();
    std::cout << "RETURN ";
    returnStmt.value->accept(*this);
    std::cout << std::endl;
}

void AstPrinter::visit(const BreakStmt& breakStmt) {
    std::cout << get_indent() << "BREAK" << std::endl;
}

void AstPrinter::visit(const ContinueStmt& continueStmt) {
    std::cout << get_indent() << "CONTINUE" << std::endl;
}