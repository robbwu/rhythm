#include "scanner.hpp"
#include <_ctype.h>
#include <iostream>

bool Scanner::isAtEnd() {
    return current >= source.length();
}

bool Scanner::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    return true;
}

char Scanner::peek() {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Scanner::peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source[current+1];
}

void Scanner::error(int line, std::string_view msg) {
    std::cout << std::format("line {}: error: {}", line, msg);
}

void Scanner::string() {
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

void Scanner::number() {
    while (isdigit(peek())) advance();

    if (peek() == '.' && isdigit(peekNext())) {
        advance(); // consume "."
        while(isdigit(peek())) advance();
    }

    addToken(TokenType::NUMBER, std::stof(source.substr(start, current-start)));
}

void Scanner::identifier() {
    while (isalnum(peek()) || peek() == '_') advance();
    auto text = source.substr(start, current-start);
    auto it = keywords.find(text);
    auto type = TokenType::IDENTIFIER;
    // if it's keyword, set toketype to the keyword specific
    if (it != keywords.end()) {
        type = it->second;
    }
    addToken(type);
}

void Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN); break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE); break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case '[': addToken(TokenType::LEFT_BRACKET); break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case ':': addToken(TokenType::COLON); break;
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
            } else if (isalpha(c) || c == '_') {
                identifier();
            } else {
                error(line, std::format("Unexpected character: {}", c));
            }
            break;
    }
}

char Scanner::advance() {
    return source[current++];
}

void Scanner::addToken(TokenType type) {
    addToken(type, nullptr);
}

void Scanner::addToken(TokenType type, Value literal) {
    auto text = source.substr(start, current-start);
    Token token(type, text, literal, line);
    tokens.push_back(token);
}

std::vector<Token> Scanner::scanTokens() {
    while (!isAtEnd()) {
        start = current;
        scanToken();
    }
    Token token(TokenType::END_TOKEN, "EOF", nullptr, line);
    tokens.push_back(token);
    return tokens;
}