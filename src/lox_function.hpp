#include <complex>

#include "interpreter.hpp"

class LoxFunction: public LoxCallable {
private:
    const FunctionStmt* declaration;
public:
    explicit LoxFunction(const FunctionStmt* declaration): declaration(declaration) {}

    Value call(Interpreter *interpreter, std::vector<Value> arguments) override {
        // std::cout <<"calling function with parameters: ";
        // for (const auto & argument : arguments) {
        //     std::cout << argument << ",";
        // }
        // std::cout << std::endl;
        auto env = new Environment(interpreter->globals); // FIXME: this is probably leaking!
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