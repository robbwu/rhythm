#include <_ctype.h>
#include <string>
#include <variant>
#include <format>
#include <iostream>
#include "magic_enum.hpp"

using Value = std::variant<double, std::string, std::nullptr_t>;

// Specialize std::formatter for Value
template <>
struct std::formatter<Value> {
    // Parse function (usually just returns the end iterator)
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin(); // No format specifiers handled yet
    }

    // Format function
    auto format(const Value& val, std::format_context& ctx) const {
        return std::visit([&](const auto& v) {
            return std::format_to(ctx.out(), "{}", v);
        }, val);
    }
};

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

      SCAN_END
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

    std::string toString() const {
        return std::format("T:{:} V:{}", magic_enum::enum_name(type), lexeme);
    }
};

template<>
struct std::formatter<Token> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();  // Simple parser that doesn't handle format specifiers
    }

    auto format(const Token& token, format_context& ctx) const {
        return format_to(ctx.out(), "{}", token.toString());  // Format using Token's data
    }
};

class Scanner {
private:
    int start = 0;
    int current = 0;
    int line = 1;
    std::string source;
    std::vector<Token*> tokens = {};

    bool isAtEnd() {
        return current >= source.length();
    }

    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }

    char peek() {
        if (isAtEnd()) return '\0';
        return source[current];
    }

    char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source[current+1];
    }

    void error(int line, std::string_view msg) {
        std::cout << std::format("line {}: error: {}", line, msg);
    }

    void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            error(line, "Unterminated string.");
        }

        advance();

        auto value = source.substr(start+1, current-start-2);
        addToken(TokenType::STRING, value);
    }

    void number() {
        while (isdigit(peek())) advance();

        if (peek() == '.' && isdigit(peekNext())) {
            advance(); // consume "."
            while(isdigit(peek())) advance();
        }

        addToken(TokenType::NUMBER, std::stof(source.substr(start, current-start)));
    }

    void identifier() {
        while (isalnum(peek())) advance();
        auto text = source.substr(start, current-start);
        auto it = keywords.find(text);
        auto type = TokenType::IDENTIFIER;
        // if it's keyword, set toketype to the keyword specific
        if (it == keywords.end()) {
            type = it->second;
        }
        addToken(type);
    }

    void scanToken() {
        char c  = advance();
        switch (c) {
            case '(': addToken(TokenType::LEFT_PAREN); break;
            case ')': addToken(TokenType::RIGHT_PAREN); break;
            case '{': addToken(TokenType::LEFT_BRACE); break;
            case '}': addToken(TokenType::RIGHT_BRACE); break;
            case ',': addToken(TokenType::COMMA); break;
            case '.': addToken(TokenType::DOT); break;
            case '-': addToken(TokenType::MINUS); break;
            case '+': addToken(TokenType::PLUS); break;
            case ';': addToken(TokenType::SEMICOLON); break;
            case '*': addToken(TokenType::STAR); break;
            case '!': addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG); break;
            case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL); break;
            case '<': addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS); break;
            case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
            case '/':
                if (match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(TokenType::SLASH);
                }
                break;
            case ' ':
            case '\r':
            case '\t':
                break;
            case '\n':
                line++;
                break;
            // literals
            case '"': string(); break;


            default:
                if (isdigit(c)) {
                    number();
                } else if (isalpha(c)) {
                    identifier();
                } else {
                    error(line, std::format("Unexpected character: {}", c));
                }
                break;
        }
    }

    char advance() {
        return source[current++];
    }

    void addToken(TokenType type) {
        addToken(type, nullptr);
    }

    void addToken(TokenType type, Value literal) {
        auto text = source.substr(start, current-start);
        tokens.push_back(new Token(type, text, literal, line));
    }

public:
    Scanner(std::string _source): source(_source) {}

    std::vector<Token*> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }
        tokens.push_back(new Token(TokenType::SCAN_END, "EOF", nullptr, line));
        return tokens;
    }

};
