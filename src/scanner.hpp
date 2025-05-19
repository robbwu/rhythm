#pragma once

#include <string>
#include <variant>
#include <format>
#include <vector>
#include <unordered_map>
#include "magic_enum.hpp"
// #include "interpreter.hpp"


class LoxCallable;
using Value = std::variant<double, std::string, std::nullptr_t, bool, LoxCallable*>;


enum class TokenType {
    // Single-character tokens.
      LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
      COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

      // One or two character tokens.
      BANG, BANG_EQUAL,
      EQUAL, EQUAL_EQUAL,
      GREATER, GREATER_EQUAL,
      LESS, LESS_EQUAL,

      // Literals.
      IDENTIFIER, STRING, NUMBER,

      // Keywords.
      AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
      PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

      END_TOKEN
};

// A global (or namespace-scope) constant map of keyword strings to TokenType
static const std::unordered_map<std::string, TokenType> keywords = {
    {"and",    TokenType::AND},
    {"class",  TokenType::CLASS},
    {"else",   TokenType::ELSE},
    {"false",  TokenType::FALSE},
    {"for",    TokenType::FOR},
    {"fun",    TokenType::FUN},
    {"if",     TokenType::IF},
    {"nil",    TokenType::NIL},
    {"or",     TokenType::OR},
    {"print",  TokenType::PRINT},
    {"return", TokenType::RETURN},
    {"super",  TokenType::SUPER},
    {"this",   TokenType::THIS},
    {"true",   TokenType::TRUE},
    {"var",    TokenType::VAR},
    {"while",  TokenType::WHILE},
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    Value literal;
    int line;

    Token(TokenType _type, std::string _lexeme, Value _literal, int _line)
    : type(_type), lexeme(_lexeme), literal(_literal), line(_line){}

    Token(const Token& other)
    : type(other.type), lexeme(other.lexeme), literal(other.literal), line(other.line) {}

    [[nodiscard]] std::string toString() const {
        return std::format("T:{:} V:{}", magic_enum::enum_name(type), lexeme);
    }
};

template<>
struct std::formatter<Token> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();  // Simple parser that doesn't handle format specifiers
    }

    static auto format(const Token& token, format_context& ctx) {
        return format_to(ctx.out(), "{}", token.toString());  // Format using Token's data
    }
};

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
