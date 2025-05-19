#pragma once
#include <utility>
#include <unordered_map>
#include <iostream>
#include <variant>

#include "expr.hpp"
#include "scanner.hpp"
#include "statement.hpp"
#include "exception.hpp"

class Environment {
private:
    Environment *enclosing = nullptr;
    std::unordered_map<std::string, Value> values;

public:

    explicit Environment(Environment *enclosing) : enclosing(enclosing) {}

    void define(const std::string& name, const Value& value) {
        values[name] = value;
    }

    void assign(const Token& token, const Value& value) {
        if (values.contains(token.lexeme)) {
            values[token.lexeme] = value;
            return;
        }
        if (enclosing) {
            enclosing->assign(token, value);
            return;
        }
        throw RuntimeError(token, "Assignment: Undefined variable '" + token.lexeme + "'.");
    }

    Value get(const Token& name) {
        if (values.contains(name.lexeme)) {
            return values[name.lexeme];
        }
        if (enclosing) {
            return enclosing->get(name);
        }
        throw RuntimeError(name, "Undefined variable '" + name.lexeme + "'.");
    }


};
class Interpreter: public ExprVisitor, public StmtVisitor {
private:
    Value _result;

    // void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
    static bool isTruthy(Value value);

public:
    Environment *env =&globals;
    Environment globals{nullptr};

    Interpreter();
    Value eval(const Expr& expr) {
        expr.accept(*this);         // result_ is filled by child
        return std::exchange(_result, {});   // grab & clear
    }
    void execute(Stmt &stmt) {
        stmt.accept(*this);
    }
    void interpret(std::vector<std::unique_ptr<Stmt>>& stmts) {
        for (auto &stmt : stmts) {
            execute(*stmt);
        }
    }
    // void print(const Expr& expr);
    void visit(const Binary& binary) override;
    void visit(const Logical&) override;
    void visit(const Grouping& group) override;
    void visit(const Literal& lit) override;
    void visit(const Unary& unary) override;
    void visit(const Variable &variable) override;
    void visit(const Assignment &assignment) override;
    void visit(const Call &call) override;

    void visit(const ExpressionStmt& exprStmt) override;
    void visit(const PrintStmt& printStmt) override;
    void visit(const VarStmt& varStmt) override;
    void visit(const BlockStmt&) override;
    void visit(const IfStmt& ifStmt) override;
    void visit(const WhileStmt& whileStmt) override;
    void visit(const FunctionStmt&) override;
    void visit(const ReturnStmt& returnStmt) override;

    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& stmts,  Environment*);

};

// Interpreter.hpp
class EnvGuard {
public:
    EnvGuard(Interpreter& I, Environment* newEnv)
        : interp(I), previous(I.env)        // remember current env
    {
        interp.env = newEnv;               // switch to function-local env
    }

    // non-copyable
    EnvGuard(const EnvGuard&)            = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;

    ~EnvGuard() { interp.env = previous; } // always restore
private:
    Interpreter&  interp;
    Environment*  previous;
};

