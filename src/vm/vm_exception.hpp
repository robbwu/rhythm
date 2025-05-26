#pragma once
#include <exception>

class VMRuntimeError: public std::exception {
private:
    int line;
    std::string msg;
public:
    explicit VMRuntimeError(int line, const std::string& msg)
        : line(line), msg(msg) {}          // forward message to base

    [[nodiscard]] const char* what() const noexcept override {
        auto m = std::format("line {}: {}", line, msg);
        return (new std::string(m))->c_str();
    }
};
