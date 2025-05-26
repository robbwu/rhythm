#pragma once
#include "chunk.hpp"
#include "expr.hpp"
#include "statement.hpp"

class CompileException: public std::runtime_error {
    public:
    CompileException(const std::string& msg): std::runtime_error(msg) {}

};


class Compiler: ExprVisitor, StmtVisitor {
private:
    Chunk chunk;
public:
    Compiler() = default;

    Chunk compile(const Expr& expr) {
        expr.accept(*this);
        return chunk;
    }

    Chunk compile(const Stmt& stmt) {
        stmt.accept(*this);
        return chunk;
    }

    inline Chunk compile(std::vector<std::unique_ptr<Stmt>> stmts) {
        for (auto& stmt : stmts) {
            stmt->accept(*this);
        }
        return chunk;
    }

    inline void clear() {
        chunk.bytecodes.clear();
        chunk.lines.clear();
    }

    void visit(const Binary&) override;
    void visit(const Literal&) override;
    void visit(const Logical&) override;
    void visit(const Grouping&) override;
    void visit(const Unary&) override;
    void visit(const Variable&) override;
    void visit(const Assignment&) override;
    void visit(const Call&) override;
    void visit(const ArrayLiteral&) override;
    void visit(const MapLiteral&) override;
    void visit(const Subscript&) override;
    void visit(const PropertyAccess&) override;
    void visit(const SubscriptAssignment&) override;
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


