#include <complex>
#include <utility>

#include "interpreter.hpp"
#include "token.hpp"

class LoxFunction: public LoxCallable {
private:
    const FunctionStmt* declaration;
    std::shared_ptr<Environment> closure;
public:
    explicit LoxFunction(const FunctionStmt* declaration, std::shared_ptr<Environment> closure): declaration(declaration), closure(std::move(closure)) {}

    Value call(RuntimeContext *ctxt, std::vector<Value> arguments) override {
        auto env = std::make_shared<Environment>(closure);
        for (int i=0; i<declaration->params.size(); i++) {
            env->define(declaration->params[i].lexeme, arguments[i]);
        }
        try {
            auto interpreter = dynamic_cast<Interpreter*>(ctxt);
            interpreter->executeBlock(declaration->body->statements, env);
        } catch (const Return& ret) {
            return ret.value;
        }
        return nullptr;
    }

    int arity() override {
        return declaration->params.size();
    }

    std::string toString() override {
        return "<fn " + declaration->name.lexeme + ">";
    }
};

// Add after the existing LoxFunction class:
class LoxFunctionExpr: public LoxCallable {
private:
    const FunctionExpr* declaration;
    std::shared_ptr<Environment> closure;
public:
    explicit LoxFunctionExpr(const FunctionExpr* declaration, std::shared_ptr<Environment> closure): declaration(declaration), closure(std::move(closure)) {}

    Value call(RuntimeContext *ctxt, std::vector<Value> arguments) override {
        auto env = std::make_shared<Environment>(closure);
        for (int i=0; i<declaration->params.size(); i++) {
            env->define(declaration->params[i].lexeme, arguments[i]);
        }
        try {
            auto interpreter = dynamic_cast<Interpreter*>(ctxt);
            interpreter->executeBlock(declaration->body->statements, env);
        } catch (const Return& ret) {
            return ret.value;
        }
        return nullptr;
    }

    int arity() override {
        return declaration->params.size();
    }

    std::string toString() override {
        return "<anonymous fn>";
    }
};
