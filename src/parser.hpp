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
        return equality();
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
    // primary        → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" ;
    std::unique_ptr<Expr> primary() {
        if (match({TokenType::FALSE})) return Literal::create(false);
        if (match({TokenType::TRUE})) return Literal::create(true);
        if (match({TokenType::NIL})) return Literal::create(nullptr);

        if (match({TokenType::NUMBER, TokenType::STRING})) {
            return Literal::create(previous().literal);
        }

        if (match({TokenType::LEFT_PAREN})) {
            auto expr = expression();
            consume(TokenType::RIGHT_PAREN, "expected ')'");
            return Grouping::create(std::move(expr));
        }
        error(peek(), "expect expression");
    }

    // helper functions
    Token consume(TokenType type, std::string msg) {
        if (check(type))  return  advance();
        error(peek(), msg);
    }
    bool match(std::vector<TokenType> types) {
        for (auto type : types) {
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

    static void error(Token token, std::string msg) {
        if (token.type == TokenType::END_TOKEN) {
            std::cout << token.line << " at end, " << msg << std::endl;
        } else {
            std::cout << token.line << " at " << token.lexeme << msg << std::endl;
        }
        throw std::runtime_error(token.lexeme);
    }

public:
    Parser(std::vector<Token> tokens) : tokens(tokens) {}

    std::unique_ptr<Expr> parse() {
        try {
            return expression();
        } catch (const std::exception& e) {
            return nullptr;
        }
    }
};
