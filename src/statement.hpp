#pragma once
#include <memory>

#include "expr.hpp"

class ExpressionStmt;
class PrintStmt;


class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visit(const ExpressionStmt&) = 0;
    virtual void visit(const PrintStmt&) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& visitor) const = 0;
};

class ExpressionStmt: public Stmt {
public:
    std::unique_ptr<Expr> expr;
    explicit ExpressionStmt(std::unique_ptr<Expr> expr): expr(std::move(expr)) {}
    static std::unique_ptr<ExpressionStmt> create(std::unique_ptr<Expr> expr) {
        return std::make_unique<ExpressionStmt>(ExpressionStmt(std::move(expr)));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class PrintStmt: public Stmt {
public:
    std::unique_ptr<Expr> expr;
    explicit PrintStmt(std::unique_ptr<Expr> expr): expr(std::move(expr)) {}

    static std::unique_ptr<PrintStmt> create(std::unique_ptr<Expr> expr) {
        return std::make_unique<PrintStmt>(PrintStmt(std::move(expr)));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};