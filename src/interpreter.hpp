#pragma once
#include <utility>
#include <unordered_map>
#include <map>
#include <variant>

#include "expr.hpp"
#include "scanner.hpp"
#include "statement.hpp"
#include "exception.hpp"
#include "robin_hood.h"

class Environment {
private:
    std::shared_ptr<Environment> enclosing = nullptr;
    // std::unordered_map<std::string, Value> values;
    std::vector<Value> values; // Flat array for direct indexing
    robin_hood::unordered_flat_map<std::string, int> nameToIndex; // Maps variable names to indices
    // Keep map only for globals; only active when enclosing==NULL
    robin_hood::unordered_flat_map<std::string, Value> globals;

public:

    explicit Environment(std::shared_ptr<Environment> enclosing) : enclosing(std::move(enclosing)) {}

    int define(const std::string& name, const Value& value) {
        if (!enclosing) { // Global scope
            globals[name] = value;
            return -1;
        }
        int index = values.size();
        nameToIndex[name] = index;
        values.push_back(value);
        return index;
    }

    [[nodiscard]] Value get_by_index(int index) const {
        return values[index];
    }

    void assign_by_index(int index, const Value& value) {
        values[index] = value;
    }

    Value getAt(int distance, int index) {
        return ancestor(distance)->get_by_index(index);
    }

    void assignAt(int distance, int index, const Value& value) {
        ancestor(distance)->assign_by_index(index, value);
    }


    void assign(const Token& token, const Value& value) {
        if (enclosing != nullptr) {
            throw RuntimeError(token, "not in global environment");
        }

        globals[token.lexeme] = value;
    }

    Value get(const Token& name) {
        auto it = globals.find(name.lexeme);
        if (it != globals.end()) {
            return it->second;
        }

        throw RuntimeError(name, "Undefined global variable '" + name.lexeme + "'.");
    }

    Environment* ancestor(int distance) {
        Environment *env = this;
        for (int i=0; i<distance; i++) {
            env = env->enclosing.get();
        }
        if (env == nullptr) {
            throw std::runtime_error("Environment nullptr!");
        }
        return env;
    }
};

class Interpreter: public ExprVisitor, public StmtVisitor, public  RuntimeContext{
private:
    Value _result;
    // Store both distance and index for each variable
    struct VarLocation {
        int distance;
        int index;
    };

    // void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
    static bool isTruthy(const Value&);

public:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> env;
    // std::unordered_map<const Expr*, int> locals;
    std::map<const Expr*, VarLocation> varLocations;


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
    Value callFunction(LoxCallable* func, const std::vector<Value>& args) override {
        return func->call(this, args);  // Delegate to the function
    }
    // void print(const Expr& expr);
    void visit(const Binary& binary) override;
    void visit(const Logical&) override;
    void visit(const Ternary&) override;
    void visit(const Grouping& group) override;
    void visit(const Literal& lit) override;
    void visit(const Unary& unary) override;
    void visit(const Postfix& postfix) override;
    void visit(const Variable &variable) override;
    void visit(const Assignment &) override;
    void visit(const SubscriptAssignment &) override;
    void visit(const Call &call) override;
    void visit(const ArrayLiteral&) override;
    void visit(const MapLiteral&) override;
    void visit(const Subscript&) override;
    void visit(const PropertyAccess&) override;
    void visit(const FunctionExpr&) override;


    void visit(const ExpressionStmt& exprStmt) override;
    void visit(const PrintStmt& printStmt) override;
    void visit(const VarStmt& varStmt) override;
    void visit(const BlockStmt&) override;
    void visit(const IfStmt& ifStmt) override;
    void visit(const WhileStmt& whileStmt) override;
    void visit(const FunctionStmt&) override;
    void visit(const ReturnStmt& returnStmt) override;
    void visit(const ContinueStmt& continueStmt) override;
    void visit(const BreakStmt& breakStmt) override;

    Value lookUpVariable(const Token & name, const Variable * variable);
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& stmts,  std::shared_ptr<Environment> env);

    // void resolve(const Expr*, int);
    void resolveWithIndex(const Expr*, int distance, int index);
};

// Interpreter.hpp
class EnvGuard {
public:
    EnvGuard(Interpreter& I, std::shared_ptr<Environment> newEnv)
        : interp(I), previous(I.env)        // remember current env
    {
        interp.env = std::move(newEnv);               // switch to function-local env
    }

    // non-copyable
    EnvGuard(const EnvGuard&)            = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;

    ~EnvGuard() { interp.env = previous; } // always restore
private:
    Interpreter&  interp;
    std::shared_ptr<Environment>  previous;
};
