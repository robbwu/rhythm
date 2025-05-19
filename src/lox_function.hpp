#include "interpreter.hpp"

class LoxFunction: public LoxCallable {
private:
    const FunctionStmt* declaration;
public:
    explicit LoxFunction(const FunctionStmt* declaration): declaration(declaration) {}

    Value call(Interpreter *interpreter, std::vector<Value> arguments) {
        auto env = std::make_unique<Environment>(interpreter->globals);
        for (int i=0; i<declaration->params.size(); i++) {
            env->define(declaration->params[i].lexeme, arguments[i]);
        }
        interpreter->executeBlock(declaration->body,std::move(env));
        return nullptr;
    }

    int arity() override {
        return declaration->params.size();
    }

    std::string toString() {
        return "<fn " + declaration->name.lexeme + ">";
    }
};