#include <complex>

#include "interpreter.hpp"

class LoxFunction: public LoxCallable {
private:
    const FunctionStmt* declaration;
    Environment* closure;
public:
    explicit LoxFunction(const FunctionStmt* declaration, Environment* closure): declaration(declaration), closure(closure) {}

    Value call(Interpreter *interpreter, std::vector<Value> arguments) override {
        auto env = new Environment(closure); // FIXME: this is probably leaking!
        for (int i=0; i<declaration->params.size(); i++) {
            env->define(declaration->params[i].lexeme, arguments[i]);
        }
        try {
            interpreter->executeBlock(declaration->body, env);
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
    Environment* closure;
public:
    explicit LoxFunctionExpr(const FunctionExpr* declaration, Environment* closure): declaration(declaration), closure(closure) {}

    Value call(Interpreter *interpreter, std::vector<Value> arguments) override {
        auto env = new Environment(closure); // FIXME: this is probably leaking!
        for (int i=0; i<declaration->params.size(); i++) {
            env->define(declaration->params[i].lexeme, arguments[i]);
        }
        try {
            interpreter->executeBlock(declaration->body, env);
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