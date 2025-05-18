#pragma once
#include <iostream>
#include <ostream>
#include <vector>

#include "expr.hpp"
#include "scanner.hpp"
/*

expression     → equality ;
equality       → comparison ( ( "!=" | "==" ) comparison )* ;
comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term           → factor ( ( "-" | "+" ) factor )* ;
factor         → unary ( ( "/" | "*" ) unary )* ;
unary          → ( "!" | "-" ) unary
               | primary ;
primary        → NUMBER | STRING | "true" | "false" | "nil"
               | "(" expression ")" ;
*/
class Parser {
private:
    std::vector<Token> tokens;
    int current = 0; // next token index to consume

    // expression     → equality ;
    std::unique_ptr<Expr> expression() {
        return assignment();
    }

    std::unique_ptr<Expr> assignment() {
        auto expr = equality(); // may match l-value if in assignment

        if (match({TokenType::EQUAL})) {
            Token equals = previous();
            auto value = assignment();
            if (auto var = dynamic_cast<Variable*>(expr.get())) {
                Token name = var->name;
                return Assignment::create(name, std::move(value));
            }
            error(equals, "Invalid assignment target");
        }
        return expr;
    }

    // equality       → comparison ( ( "!=" | "==" ) comparison )* ;
    std::unique_ptr<Expr>  equality() {
        std::unique_ptr<Expr> expr = comparison();

        while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
            Token op = previous();
            auto right = comparison();
            expr = Binary::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    // comparison     → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
    std::unique_ptr<Expr> comparison() {
        std::unique_ptr<Expr> expr = term();
        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
            Token op = previous();
            auto right = term();
            expr = Binary::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    // term           → factor ( ( "-" | "+" ) factor )* ;
    std::unique_ptr<Expr> term() {
        std::unique_ptr<Expr> expr = factor();
        while (match({TokenType::MINUS, TokenType::PLUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = factor();
            expr = Binary::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    // factor         → unary ( ( "/" | "*" ) unary )* ;
    std::unique_ptr<Expr> factor() {
        auto expr = unary();

        while (match({TokenType::SLASH,TokenType::STAR})) {
            auto op = previous();
            auto right = unary();
            expr = Binary::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    // unary          → ( "!" | "-" ) unary | primary ;
    std::unique_ptr<Expr> unary() {
        if (match({TokenType::BANG, TokenType::MINUS})) {
            Token op = previous();
            auto right = unary();
            return Unary::create(op, std::move(right));
        }
        return primary();
    }
    // primary        → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER ;
    std::unique_ptr<Expr> primary() {
        if (match({TokenType::FALSE})) return Literal::create(false);
        if (match({TokenType::TRUE})) return Literal::create(true);
        if (match({TokenType::NIL})) return Literal::create(nullptr);

        if (match({TokenType::NUMBER, TokenType::STRING})) {
            return Literal::create(previous().literal);
        }
        if (match({TokenType::IDENTIFIER})) return Variable::create(previous());

        if (match({TokenType::LEFT_PAREN})) {
            auto expr = expression();
            consume(TokenType::RIGHT_PAREN, "expected ')'");
            return Grouping::create(std::move(expr));
        }

        error(peek(), "expect expression");
        return nullptr;
    }

    // statements
    std::unique_ptr<Stmt> declaration() {
        if (match({TokenType::VAR}))
            return varDeclaration();
        return statement();
    }
    std::unique_ptr<Stmt> varDeclaration() {
        Token name = consume(TokenType::IDENTIFIER, "expected variable name");
        std::unique_ptr<Expr> initializer{};
        if (match({TokenType::EQUAL})) {
             initializer = expression();
        }
        consume(TokenType::SEMICOLON, "expected ';'");
        return VarStmt::create(name, std::move(initializer));
    }

    std::unique_ptr<Stmt> statement() {
        if (match({TokenType::PRINT})) return printStatement();

        return expressionStatement();
    }
    std::unique_ptr<Stmt> printStatement() {
        auto value = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after value.");
        return PrintStmt::create(std::move(value));
    }
    std::unique_ptr<Stmt> expressionStatement() {
        auto value = expression();
        consume(TokenType::SEMICOLON, "Expect ';' after value.");
        return ExpressionStmt::create(std::move(value));
    }

    // helper functions
    Token consume(TokenType type, std::string msg) {
        if (check(type))  return  advance();
        error(peek(), msg);
    }
    bool match(const std::vector<TokenType>& types) {
        for (const auto type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    bool check(TokenType type) {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool isAtEnd() {
        return peek().type == TokenType::END_TOKEN;
    }

    Token peek() {
        return tokens[current];
    }

    Token previous() {
        return tokens[current-1];
    }

    static void error(const Token& token, const std::string& msg) {
        if (token.type == TokenType::END_TOKEN) {
            std::cout << token.line << " at end, " << msg << std::endl;
        } else {
            std::cout << token.line << " at " << token.lexeme << msg << std::endl;
        }
        throw std::runtime_error(token.lexeme);
    }

public:
    explicit Parser(const std::vector<Token> &tokens) : tokens(tokens) {}

    std::unique_ptr<Expr> parseExpr() {
        try {
            return expression();
        } catch (const std::exception& e) {
            return nullptr;
        }
    }

    std::vector<std::unique_ptr<Stmt>> parse() {
        std::vector<std::unique_ptr<Stmt>> stmts;
        while (!isAtEnd()) {
            stmts.push_back(declaration());
        }
        return stmts;
    }
};
