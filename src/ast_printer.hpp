#pragma once
#include "expr.hpp"

class AstPrinter: public ExprVisitor {
private:
    void parenthesize(const std::string& name, const std::vector<const Expr*>& exprs);
public:
    void print(const Expr& expr);
    void visit(const Binary& binary) override ;
    void visit(const Grouping& group) override ;
    void visit(const Literal& lit) override ;
    void visit(const Unary& unary) override ;
};