#pragma once
#include <memory>

#include "expr.hpp"

class ExpressionStmt;
class PrintStmt;
class VarStmt;
class BlockStmt;

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visit(const ExpressionStmt&) = 0;
    virtual void visit(const PrintStmt&) = 0;
    virtual void visit(const VarStmt&) = 0;
    virtual void visit(const BlockStmt&) = 0;
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

class VarStmt: public Stmt {
public:
    Token name;
    std::unique_ptr<Expr> initializer;
    VarStmt(const Token& _name, std::unique_ptr<Expr> _initializer): name(_name), initializer(std::move(_initializer)) {}

    static std::unique_ptr<VarStmt> create(Token name, std::unique_ptr<Expr> initilializer) {
        return std::make_unique<VarStmt>(VarStmt(std::move(name), std::move(initilializer)));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class BlockStmt: public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements): statements(std::move(statements)) {}
    static std::unique_ptr<BlockStmt> create(std::vector<std::unique_ptr<Stmt>> statements) {
        return std::make_unique<BlockStmt>(std::move(statements));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};