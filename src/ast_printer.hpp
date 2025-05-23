#pragma once
#include "exception.hpp"
#include "expr.hpp"
#include "statement.hpp"

class AstPrinter: public ExprVisitor, StmtVisitor {
private:
    void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
    int indent_level = 0;
    std::string get_indent() const { return std::string(indent_level * 2, ' '); }
    class IndentGuard {
    private:
        AstPrinter& printer;
    public:
        IndentGuard(AstPrinter& p) : printer(p) { printer.indent_level++; }
        ~IndentGuard() { printer.indent_level--; }
    };

public:
    void print(const Expr& expr);
    void print(const std::vector<std::unique_ptr<Stmt>> &statements);

    void visit(const Binary& binary) override ;
    void visit(const Logical&) override ;
    void visit(const Grouping& group) override ;
    void visit(const Literal& lit) override ;
    void visit(const Unary& unary) override ;
    void visit(const Variable&) override;
    void visit(const Assignment&) override;
    void visit(const Call&) override;
    void visit(const ArrayLiteral&) override;
    void visit(const MapLiteral&) override;
    void visit(const Subscript&) override;
    void visit(const SubscriptAssignment&) override;
    void visit(const PropertyAccess&) override;
    void visit(const FunctionExpr&) override;


    void visit(const ExpressionStmt&) override;
    void visit(const PrintStmt&) override;
    void visit(const VarStmt&) override;
    void visit(const BlockStmt&) override;
    void visit(const IfStmt&) override;
    void visit(const WhileStmt&) override;
    void visit(const FunctionStmt&) override;
    void visit(const ReturnStmt&) override;
    void visit(const BreakStmt&) override;
    void visit(const ContinueStmt&) override;
};
