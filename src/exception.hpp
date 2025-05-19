#pragma once
#include <stdexcept>   // std::runtime_error
#include <string>
#include <utility>
#include "scanner.hpp"

class RuntimeError : public std::runtime_error {
private:
    Token name;
    std::string msg;
public:
    explicit RuntimeError(Token name, const std::string& msg)
        : std::runtime_error(msg), name(name), msg(msg) {}          // forward message to base

    const char* what() const noexcept override {
        auto m = new std::string(name.toString());
        *m += ": " + msg;
        return m->c_str();
    }
};

class Return: public std::runtime_error {
public:
    Value value;
    explicit Return(Value value): value(std::move(value)), std::runtime_error("return") {}
};