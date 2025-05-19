#pragma once

#include <string>
#include <variant>
#include <format>
#include <vector>
#include <unordered_map>

#include "magic_enum.hpp"
#include "token.hpp"
#include "interpreter.hpp"

class LoxCallable;




class Scanner {
private:
    int start = 0;
    int current = 0;
    int line = 1;
    std::string source;
    std::vector<Token> tokens = {};

    bool isAtEnd();
    bool match(char expected);
    char peek();
    char peekNext();
    void error(int line, std::string_view msg);
    void string();
    void number();
    void identifier();
    void scanToken();
    char advance();
    void addToken(TokenType type);
    void addToken(TokenType type, Value literal);

public:
    explicit Scanner(std::string _source): source(std::move(_source)) {}
    std::vector<Token> scanTokens();
};
