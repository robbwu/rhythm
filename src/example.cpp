#include "expr.hpp"
#include "scanner.hpp"
#include <iostream>
#include <memory>

// Example of building an expression tree using the factory methods
// Equivalent to the Java code:
// Expr expression = new Expr.Binary(
//     new Expr.Unary(
//         new Token(TokenType.MINUS, "-", null, 1),
//         new Expr.Literal(123)),
//     new Token(TokenType.STAR, "*", null, 1),
//     new Expr.Grouping(
//         new Expr.Literal(45.67)));

int main() {
    // Example usage of the expression factory methods
    auto expression = Binary::create(
        Unary::create(
            Token(TokenType::MINUS, "-", nullptr, 1),
            Literal::create(123.0)),
        Token(TokenType::STAR, "*", nullptr, 1),
        Grouping::create(
            Literal::create(45.67)));

    // Print the expression using AstPrinter
    AstPrinter printer;
    printer.print(*expression);
    std::cout << std::endl;

    return 0;
}