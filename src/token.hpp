#pragma once
#include <variant>
#include <iostream>
#include <unordered_map>
#include "magic_enum.hpp"
#include <format>

class Interpreter;
class LoxCallable;

using Value = std::variant<double, std::string, std::nullptr_t, bool, LoxCallable*>;

class LoxCallable {
public:
    virtual Value call(Interpreter *interpreter, std::vector<Value> arguments) = 0;
    virtual int arity() = 0;
    virtual std::string toString() = 0;
};

// helper to make overloaded lambdas
template<class... Fs>
struct overloaded : Fs... { using Fs::operator()...; };
template<class... Fs> overloaded(Fs...) -> overloaded<Fs...>;

// streaming operator for Value
inline std::ostream& operator<<(std::ostream& os, Value const& v) {
    std::visit(overloaded{
        [&](double   d) { os << d; },
        [&](bool     b) { os << (b ? "true" : "false"); },
        [&](std::nullptr_t) { os << "nil"; },
        [&](std::string const& s) { os << s; },
        [&](LoxCallable* fn) { os << (fn ? fn->toString() : "<null fn>"); }
    }, v);
    return os;
}

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
    int line{};

    Token() = default;
    Token(TokenType _type, std::string _lexeme, Value _literal, int _line)
    : type(_type), lexeme(_lexeme), literal(_literal), line(_line){}

    Token(const Token& other)
    : type(other.type), lexeme(other.lexeme), literal(other.literal), line(other.line) {}

    [[nodiscard]] std::string toString() const {
        return std::format("L:{} T:{:} V:{}", line, magic_enum::enum_name(type), lexeme);
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

enum class FunctionType {
    NONE, FUNCTION,
};