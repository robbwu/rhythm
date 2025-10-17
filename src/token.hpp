#pragma once
#include <variant>
#include <iostream>
#include <unordered_map>
#include <format>
#include <memory>
#include <cfloat>
#include <cmath>

#include "magic_enum.hpp"


class Interpreter;
class LoxCallable;

struct Array;
struct Map;

using Value = std::variant<double, std::string, std::nullptr_t, bool, LoxCallable*, std::shared_ptr<Array>, std::shared_ptr<Map>>;

struct Array {
    std::vector<Value> data;
    explicit Array(const std::vector<Value>& data) : data(data) {}
};

struct Map {
    std::unordered_map<Value, Value> data;
    explicit Map(const std::unordered_map<Value, Value>& data) : data(data) {}
};

class RuntimeContext {
public:
    virtual ~RuntimeContext() = default;
    virtual Value callFunction(LoxCallable* func, const std::vector<Value>& args) = 0;
};

class LoxCallable {
public:
    virtual ~LoxCallable() = default;

    virtual Value call(RuntimeContext *context, std::vector<Value> arguments) = 0;
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
        [&](LoxCallable* fn) { os << (fn ? fn->toString() : "<null fn>"); },
        [&](std::shared_ptr<Array> a) {
            os << '[';
            for (auto &f : a->data) {
                os << f << ", ";
            }
            os << ']';
        },
        [&](std::shared_ptr<Map> map) {
            os << '{';
            for (auto &f : map->data) {
                os << f.first << ": " << f.second << ", ";
            }
            os << '}';
        }
    }, v);
    return os;
}

enum class TokenType {
    // Single-character tokens.
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, COLON, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR, PERCENT,
    QUESTION,

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
    BREAK, CONTINUE,

    END_TOKEN
};

// A global (or namespace-scope) constant map of keyword strings to TokenType
static const std::unordered_map<std::string, TokenType> keywords = {
    {"and",    TokenType::AND},
    {"break",    TokenType::BREAK},
    {"class",  TokenType::CLASS},
    {"continue", TokenType::CONTINUE},
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


// false, nil are falsy; everything else is truthy
inline bool is_truthy(const Value& value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    }
    if (std::holds_alternative<std::nullptr_t>(value)) {
        return false;
    }
    return true;
}

inline bool is_integer(double x) {
    if (!std::isfinite(x))       // exclude NaN and ±∞
        return false;
    // truncate toward zero, then compare exactly
    return x == std::trunc(x);
    // —or—
    // return std::fmod(x, 1.0) == 0.0;
}
