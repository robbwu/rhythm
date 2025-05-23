#pragma once
#include <functional>
#include <iostream>
#include <ostream>
#include <vector>

#include "expr.hpp"
#include "scanner.hpp"
#include "statement.hpp"
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

    // expression     → assignment ;
    std::unique_ptr<Expr> expression() {
        return assignment();
    }
    // assignment     → IDENTIFIER "=" assignment
    //                | logic_or ;
    std::unique_ptr<Expr> assignment() {
        auto expr = logic_or(); // may match l-value if in assignment

        if (match({TokenType::EQUAL})) {
            Token equals = previous();
            auto value = assignment();
            if (auto var = dynamic_cast<Variable*>(expr.get())) {
                Token name = var->name;
                return Assignment::create(name, std::move(value));
            }
            if (auto subscript = dynamic_cast<Subscript *>(expr.get())) {
                return SubscriptAssignment::create(
                    std::move(subscript->object),
                    std::move(subscript->index),
                    std::move(value),
                    subscript->bracket);
            }
            if (auto prop = dynamic_cast<PropertyAccess*>(expr.get())) {
                // Convert property assignment to subscript assignment
                auto stringKey = Literal::create(Value(prop->name.lexeme));
                return SubscriptAssignment::create(
                    std::move(prop->object),
                    std::move(stringKey),
                    std::move(value),
                    prop->name);  // Use property name token for error reporting
            }
            error(equals, "Invalid assignment target");
        }
        return expr;
    }
    // logic_or       → logic_and ( "or" logic_and )* ;
    std::unique_ptr<Expr> logic_or() {
        auto expr = logic_and();
        while (match({TokenType::OR})) {
            Token op = previous();
            auto right = logic_and();
            expr = Logical::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> logic_and() {
        auto expr = equality();

        while (match({TokenType::AND})) {
            Token op = previous();
            auto right = equality();
            expr = Logical::create(std::move(expr), op, std::move(right));
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

        while (match({TokenType::SLASH,TokenType::STAR, TokenType::PERCENT})) {
            auto op = previous();
            auto right = unary();
            expr = Binary::create(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    // unary          → ( "!" | "-" ) unary | call ;
    std::unique_ptr<Expr> unary() {
        if (match({TokenType::BANG, TokenType::MINUS})) {
            Token op = previous();
            auto right = unary();
            return Unary::create(op, std::move(right));
        }
        return call();
    }

    // call           → primary ( "(" arguments? ")" )* ;
    // arguments      → expression ( "," expression )* ;
    std::unique_ptr<Expr> call() {
        auto expr = primary();
        while (true) {
            if (match({TokenType::LEFT_PAREN})) {
                expr = finishCall(std::move(expr));
            } else if (match({TokenType::LEFT_BRACKET})) {
                Token bracket = previous();
                auto index = expression();
                consume(TokenType::RIGHT_BRACKET, "Expect ']' after index.");
                expr = Subscript::create(std::move(expr), std::move(index), bracket);
            } else if (match({TokenType::DOT})) {
                Token name = consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
                expr = PropertyAccess::create(std::move(expr), name);
            }else {
                break;
            }
        }

        return expr;
    }
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee) {
        std::vector<std::unique_ptr<Expr>> args;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                args.push_back(expression());
            } while (match({TokenType::COMMA}));
        }

        Token paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
        return Call::create(std::move(callee), paren, std::move(args));
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

        if (match({TokenType::FUN})) {
            return functionExpression();
        }

        if (match({TokenType::LEFT_BRACKET})) { // array literal
            return arrayLiteral();
        }
        if (match({TokenType::LEFT_BRACE})) { // map literal
            return mapLiteral();
        }

        error(peek(), "expect expression");
        return nullptr;
    }

    std::unique_ptr<FunctionExpr> functionExpression() {
        // parse parameters
        std::vector<Token> parameters;
        consume(TokenType::LEFT_PAREN, "expected '('");
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                parameters.push_back(consume(TokenType::IDENTIFIER, "expect parameter name"));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "expected ')'");

        // parse body
        consume(TokenType::LEFT_BRACE, "expected '{' before body");
        std::vector<std::unique_ptr<Stmt>> body = block();

        return FunctionExpr::create(std::move(parameters), std::move(body));
    }

    std::unique_ptr<Expr> arrayLiteral() {
        std::vector<std::unique_ptr<Expr>> elements;

        if (!check(TokenType::RIGHT_BRACKET)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RIGHT_BRACKET, "Expect ']' after array elements.");
        return ArrayLiteral::create(std::move(elements));
    }

    std::unique_ptr<Expr> mapLiteral() {
        std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> pairs;

        if (!check(TokenType::RIGHT_BRACE)) {
            do {
                // Parse key: value pairs
                auto key = expression();
                consume(TokenType::COLON, "Expect ':' after map key.");
                auto value = expression();
                pairs.emplace_back(std::move(key), std::move(value));
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RIGHT_BRACE, "Expect '}' after map elements.");
        return MapLiteral::create(std::move(pairs));
    }

    // statements
    std::unique_ptr<Stmt> declaration() {
        if (match({TokenType::FUN})) return function("function");
        if (match({TokenType::VAR})) return varDeclaration();
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

    std::unique_ptr<FunctionStmt> function(std::string kind) {
        Token name = consume(TokenType::IDENTIFIER, std::format("expected {} name", kind));

        // parse parameters
        std::vector<Token> parameters;
        consume(TokenType::LEFT_PAREN, "expected '('");
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                parameters.push_back(consume(TokenType::IDENTIFIER, "expect parameter name"));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "expected ')'");

        // parse body
        consume(TokenType::LEFT_BRACE, "expected '{' before body");
        std::vector<std::unique_ptr<Stmt>> body = block();

        return FunctionStmt::create(name, parameters, std::move(body));
    }

    std::unique_ptr<Stmt> statement() {
        if (match({TokenType::BREAK})) return breakStatement();
        if (match({TokenType::CONTINUE})) return continueStatement();
        if (match({TokenType::FOR})) return forStatement();
        if (match({TokenType::IF})) return ifStatement();
        if (match({TokenType::PRINT})) return printStatement();
        if (match({TokenType::RETURN})) return returnStatement();
        if (match({TokenType::WHILE})) return whileStatement();
        if (match({TokenType::LEFT_BRACE})) return BlockStmt::create(block());

        return expressionStatement();
    }

    std::unique_ptr<BreakStmt> breakStatement() {
        Token kw = previous();
        consume(TokenType::SEMICOLON, "Expect ';' after 'break'.");
        return BreakStmt::create(kw);
    }

    std::unique_ptr<ContinueStmt> continueStatement() {
        Token kw = previous();
        consume(TokenType::SEMICOLON, "Expect ';' after 'continue'.");
        return ContinueStmt::create(kw);
    }

    // parse sequence of Stmts up until }
    std::vector<std::unique_ptr<Stmt>> block() {
        std::vector<std::unique_ptr<Stmt>> statements;
        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
            statements.push_back(declaration());
        }

        consume(TokenType::RIGHT_BRACE, "expected '}'");
        return statements;
    }

    // desugar for into while statement
    std::unique_ptr<Stmt> forStatement() {
        consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");

        std::unique_ptr<Stmt> initializer;
        if (match({TokenType::SEMICOLON})) {
            initializer = nullptr;
        } else if (match({TokenType::VAR})) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        std::unique_ptr<Expr> condition{nullptr};
        if (!check(TokenType::SEMICOLON)) {
            condition = expression();
        }
        consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

        std::unique_ptr<Expr> increment{nullptr};
        if (!check(TokenType::RIGHT_PAREN)) {
            increment = expression();
        }
        consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses");

        std::unique_ptr<Stmt> body = statement();
        // de-sugar for into while
        // if (increment) {
        //     std::unique_ptr<Stmt> incrStmt = ExpressionStmt::create(std::move(increment));
        //     std::vector<std::unique_ptr<Stmt>> block;
        //     block.push_back(std::move(body));
        //     block.push_back(std::move(incrStmt));
        //     body = BlockStmt::create(std::move(block));
        //
        // }

        if (condition == nullptr) condition = Literal::create(true);
        // body = WhileStmt::create(std::move(condition), std::move(body));
        auto while_loop_stmt = WhileStmt::create(std::move(condition), std::move(body), std::move(increment));


        if (initializer) {
            std::vector<std::unique_ptr<Stmt>> block_stmts;
            block_stmts.push_back(std::move(initializer));
            block_stmts.push_back(std::move(while_loop_stmt));
            return BlockStmt::create(std::move(block_stmts));
        }
        return while_loop_stmt;
    }

    std::unique_ptr<Stmt> ifStatement() {
        consume(TokenType::LEFT_PAREN, "expected '(' after if");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "expected ')' after if condition");

        std::unique_ptr<Stmt> thenBranch = statement();
        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (match({TokenType::ELSE}))
            elseBranch = statement();

        return IfStmt::create(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    std::unique_ptr<Stmt> whileStatement() {
        consume(TokenType::LEFT_PAREN, "expected '(' after if");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "expected ')' after if condition");
        std::unique_ptr<Stmt> body = statement();

        return WhileStmt::create(std::move(condition), std::move(body));
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

    std::unique_ptr<ReturnStmt> returnStatement() {
        Token kw = previous(); // just for the position reporting in error
        std::unique_ptr<Expr> value{nullptr};
        if (!check(TokenType::SEMICOLON)) {
            value = expression();
        }
        consume(TokenType::SEMICOLON, "Expect ';' after return value.");
        return ReturnStmt::create(kw, std::move(value));
    }

    // helper functions
    Token consume(TokenType type, std::string msg) {
        if (check(type))  return  advance();
        error(peek(), msg);
        return {};
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
