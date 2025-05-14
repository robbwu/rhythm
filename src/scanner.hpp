#include <string>
#include <variant>
#include <format>

using Value = std::variant<double, std::string>;

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

      LOX_EOF
};



class Token {
public:
    TokenType type;
    std::string lexeme;
    Value literal;

    std::string toString() const {
        return std::format("token {}", lexeme);
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
public:
    Scanner(std::string source) {

    }

    std::vector<Token> scanTokens() {
        return {};
    }
};
