#pragma once
#include <iostream>
#include <ostream>
#include <utility>

#include "chunk.hpp"
#include "compiler.hpp"
#include "vm_exception.hpp"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

class VM {
    // Chunk chunk;
    uint8_t* ip;
    std::vector<Value> stack; // stack VM

    std::unordered_map<std::string, Value> globals;
public:

    explicit VM(): ip(0) {};

    InterpretResult run(Chunk chunk);
    inline InterpretResult interpret(const std::string& source, Compiler compiler) {
        // compiler.compile(source);
        return INTERPRET_OK;
    };


    inline void push(const Value& v) {stack.push_back(v);}
    inline Value pop() {
        auto result = stack.back();
        stack.pop_back();
        return result;
    }
    Value peek() {
        return stack.back();
    }


};