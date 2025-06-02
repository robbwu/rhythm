#pragma once
#include "chunk.hpp"
#include "expr.hpp"
#include "statement.hpp"
#include "token.hpp"

class CompileException: public std::runtime_error {
    public:
    CompileException(const std::string& msg): std::runtime_error(msg) {}

};

class Compiler: ExprVisitor, StmtVisitor {
private:
    Chunk chunk;
    typedef struct {
        uint8_t index;
        bool isLocal;
    } Upvalue;
    std::vector<Upvalue> upvalues;
public:
    Compiler *enclosing = nullptr;
    typedef struct {
        Token name;
        int depth;
        bool isCaptured;
    } Local;
    int scopeDepth = 0;
    struct LoopContext {
        std::vector<int> breakJumps;    // Jump locations to patch for break
        std::vector<int> continueJumps; // Jump locations to patch for continue
        int loopStart;                   // Where continue should jump to
        int numLocals;
    };
    std::vector<LoopContext> loopStack; // Stack of nested loop contexts


    // use as a stack for local variables
    // this compile-time stack will exactly mirror runtime VM stack std::vector<Value> locals
    // except that locals store meta-info about the local variable, the VM stack storing the values only
    // because statements have net-zero effect on stack
    std::vector<Local> locals;

    Compiler(Compiler* enclosing ): enclosing(enclosing) {}

    // Chunk compile(const Expr& expr) {
    //     expr.accept(*this);
    //     return chunk;
    // }
    //
    // Chunk compile(const Stmt& stmt) {
    //     stmt.accept(*this);
    //     return chunk;
    // }

    inline Chunk compile(const std::vector<std::unique_ptr<Stmt>>& stmts) {
        for (auto& stmt : stmts) {
            stmt->accept(*this);
        }
        return chunk;
    }

    BeatFunction* compileBeatFunction(const std::unique_ptr<BlockStmt> &body, std::string name, int arity, BeatFunctionType type) {
        chunk = Chunk(); // reset the chunk
        // upvalues.clear();
        // compile(std::move(stmts));
        if (type == BeatFunctionType::FUNCTION)
            body->accept(*this);
        else if (type == BeatFunctionType::SCRIPT) { // avoid block stmt; no begin scope/end scope etc for script.
            for (auto &stmt : body->statements) {
                stmt->accept(*this);
            }
        }

        chunk.write(OP_NIL, 0);
        chunk.write(OP_RETURN, 0); // add return at the end of the function
        std::cout << "Compiling BeatFunction: " << name << " with upvalue count: " << upvalues.size() << std::endl;
        return new BeatFunction(arity, name, chunk, type, upvalues.size());
    }

    inline void clear() {
        chunk.bytecodes.clear();
        chunk.lines.clear();
    }

    inline void beginScope() {
        scopeDepth++;
    }

    inline void endScope() {
        std::cout << "endScope " << scopeDepth << std::endl;
        scopeDepth--;
        while (!locals.empty()  && locals.back().depth > scopeDepth) {
            auto local = locals.back();
            locals.pop_back();
            if (local.isCaptured) {
                chunk.write(OP_CLOSE_UPVALUE, local.name.line);
            } else {
                chunk.write(OP_POP, local.name.line);
            }
        }
    }
    int resolveLocal(Token token);
    int resolveUpvalue(Token name);
    int addUpvalue( uint8_t index,bool isLocal);


    int emitJump(uint8_t instruction, int line);
    void patchJump(int offset);
    void emitLoop(int loopStart);

    void beginLoop(int loopStart);
    void endLoop();
    void addBreakJump(int jump);
    void addContinueJump(int jump);
    bool isInLoop() const { return !loopStack.empty(); }


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
