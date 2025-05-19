#pragma once
#include "expr.hpp"
#include "statement.hpp"

class AstPrinter: public ExprVisitor, StmtVisitor {
private:
    void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
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

    void visit(const ExpressionStmt&) override;
    void visit(const PrintStmt&) override;
    void visit(const VarStmt&) override;
    void visit(const BlockStmt&) override;
    void visit(const IfStmt&) override;
    void visit(const WhileStmt&) override;
};
