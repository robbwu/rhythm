#pragma once
#include <cstdint>
#include <vector>
#include "token.hpp"

typedef enum  {
    OP_RETURN, OP_CONSTANT,
    OP_NEGATE, OP_NOT,
    OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE,
    OP_PRINT, OP_NIL,
    OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL,
    OP_SET_LOCAL, OP_GET_LOCAL,
    OP_POP,
} OpCode;

// using Chunk = std::vector<uint8_t>;
class Chunk {
public:
    std::vector<uint8_t> bytecodes;
    std::vector<Value> constants;
    std::vector<int> lines;

    void disassembleChunk(const std::string& name);
    int disassembleInstruction(int offset);
    int addConstant(const Value& value);
    inline void write(uint8_t byte, int line) {
        bytecodes.push_back(byte);
        lines.push_back(line);
    }

    int constantInstruction(const char* name, int offset);
    int byteInstruction(const char* name,int offset);
    static int simpleInstruction(const char* name, int offset);
};


