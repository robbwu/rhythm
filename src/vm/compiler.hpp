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
    typedef struct {
        Token name;
        int depth;
        bool isCaptured;
    } Local;
    int scopeDepth = 0;


    // use as a stack for local variables
    // this compile-time stack will exactly mirror runtime VM stack std::vector<Value> locals
    // except that locals store meta-info about the local variable, the VM stack storing the values only
    // because statements have net-zero effect on stack
    std::vector<Local> locals;

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

    inline void beginScope() {
        scopeDepth++;
    }

    inline void endScope() {
        scopeDepth--;
        while (!locals.empty()  && locals.back().depth > scopeDepth) {
            auto local = locals.back();
            locals.pop_back();
            chunk.write(OP_POP, local.name.line);
        }
    }
    int resolveLocal(Token token);

    int emitJump(uint8_t instruction, int line);
    void patchJump(int offset);
    void emitLoop(int loopStart);


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


