#include <expr.hpp>
#include <utility>


class Interpreter: public ExprVisitor {
private:
    Value _result;

    // void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
    bool isTruthy(Value value);

public:
    Value eval(const Expr& expr) {
        expr.accept(*this);         // result_ is filled by child
        return std::exchange(_result, {});   // grab & clear
    }
    // void print(const Expr& expr);
    void visit(const Binary& binary) override ;
    void visit(const Grouping& group) override ;
    void visit(const Literal& lit) override ;
    void visit(const Unary& unary) override ;
};