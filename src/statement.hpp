#pragma once
#include <memory>

// #include "expr.hpp"
// Forward declarations
class Expr;
class Token;

class ExpressionStmt;
class PrintStmt;
class VarStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
// class ForStmt;
class FunctionStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;

class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visit(const ExpressionStmt&) = 0;
    virtual void visit(const PrintStmt&) = 0;
    virtual void visit(const VarStmt&) = 0;
    virtual void visit(const BlockStmt&) = 0;
    virtual void visit(const IfStmt&) = 0;
    virtual void visit(const WhileStmt&) = 0;
    virtual void visit(const FunctionStmt&) = 0;
    virtual void visit(const ReturnStmt&) = 0;
    virtual void visit(const BreakStmt&) = 0;
    virtual void visit(const ContinueStmt&) = 0;
};

class Stmt {
public:
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& visitor) const = 0;
};

class ExpressionStmt: public Stmt {
public:
    std::unique_ptr<Expr> expr;
    int line;
    explicit ExpressionStmt(std::unique_ptr<Expr> expr, int line): expr(std::move(expr)), line(line) {}
    static std::unique_ptr<ExpressionStmt> create(std::unique_ptr<Expr> expr, int line) {
        return std::make_unique<ExpressionStmt>(ExpressionStmt(std::move(expr), line));
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
    int line;
    std::vector<std::unique_ptr<Stmt>> statements;
    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements, int line): statements(std::move(statements)), line(line) {}
    static std::unique_ptr<BlockStmt> create(std::vector<std::unique_ptr<Stmt>> statements, int line) {
        return std::make_unique<BlockStmt>(std::move(statements), line);
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class IfStmt: public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBlock;
    std::unique_ptr<Stmt> elseBlock;
    IfStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBlock, std::unique_ptr<Stmt> elseBlock)
        : condition(std::move(condition)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}
    static std::unique_ptr<IfStmt> create(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> thenBlock, std::unique_ptr<Stmt> elseBlock) {
        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBlock), std::move(elseBlock));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class WhileStmt: public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    std::unique_ptr<Expr> increment;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, std::unique_ptr<Expr> increment = nullptr): condition(std::move(condition)), body(std::move(body)), increment(std::move(increment)) {}
    static std::unique_ptr<WhileStmt> create(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body, std::unique_ptr<Expr> increment = nullptr) {
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body), std::move(increment));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class FunctionStmt: public Stmt {
public:
    Token name;
    std::vector<Token> params;
    std::unique_ptr<BlockStmt> body;

    FunctionStmt(Token name, std::vector<Token> params, std::unique_ptr<BlockStmt> body): name(name), params(std::move(params)), body(std::move(body)) {}
    static std::unique_ptr<FunctionStmt> create(Token name, std::vector<Token> params, std::unique_ptr<BlockStmt> body) {
        return std::make_unique<FunctionStmt>(std::move(name), params, std::move(body));
    }
    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class ReturnStmt: public Stmt {
public:
    Token kw;
    std::unique_ptr<Expr> value;

    ReturnStmt(Token& kw, std::unique_ptr<Expr> value): kw(std::move(kw)), value(std::move(value)) {}
    static std::unique_ptr<ReturnStmt> create(Token& kw, std::unique_ptr<Expr> value) {
        return std::make_unique<ReturnStmt>(kw, std::move(value));
    }

    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

// Add at the end of the file, after Subscript class
class FunctionExpr : public Expr {
public:
    std::vector<Token> params;
    std::unique_ptr<BlockStmt> body;
    int line;

    FunctionExpr(std::vector<Token> params, std::unique_ptr<BlockStmt> body, int line)
        : params(std::move(params)), body(std::move(body)), line(line) {}

    static std::unique_ptr<FunctionExpr> create(
        std::vector<Token> params, std::unique_ptr<BlockStmt> body, int line) {
        return std::make_unique<FunctionExpr>(std::move(params), std::move(body), line);
    }

    void accept(ExprVisitor& visitor) const override {
        visitor.visit(*this);
    }
    int get_line() const override {
        return line;
    }
};

class BreakStmt: public Stmt {
public:
    Token kw;

    explicit BreakStmt(Token kw): kw(std::move(kw)) {}
    static std::unique_ptr<BreakStmt> create(Token kw) {
        return std::make_unique<BreakStmt>(std::move(kw));
    }

    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};

class ContinueStmt: public Stmt {
public:
    Token kw;

    explicit ContinueStmt(Token kw): kw(std::move(kw)) {}
    static std::unique_ptr<ContinueStmt> create(Token kw) {
        return std::make_unique<ContinueStmt>(std::move(kw));
    }

    void accept(StmtVisitor& visitor) const override {
        visitor.visit(*this);
    }
};
