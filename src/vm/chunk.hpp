#pragma once
#include <cstdint>
#include <vector>
#include "token.hpp"

typedef enum  {
    OP_RETURN, OP_CONSTANT,
    OP_NEGATE, OP_NOT,
    OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE, OP_EQUAL, OP_GREATER, OP_LESS,
    OP_PRINT, OP_NIL,
    OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL,
    OP_SET_LOCAL, OP_GET_LOCAL,
    OP_POP,
    OP_JUMP_IF_FALSE, OP_JUMP, OP_LOOP, OP_CALL,
    OP_ARRAY_LITERAL, OP_MAP_LITERAL, OP_SUBSCRIPT, OP_SUBSCRIPT_ASSIGNMENT,
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
    int simpleInstruction(const char* name, int offset);
    int jumpInstruction(const char* name, int sign, int offset);
};

enum class BeatFunctionType {
    FUNCTION, SCRIPT,
};


// Function defined in Rhythm code
class BeatFunction: public LoxCallable {
private:
public:
    std::string name;
    int arity_;
    Chunk chunk;
    BeatFunctionType type;

    BeatFunction(int _arity, const std::string &name, Chunk &chunk, BeatFunctionType type): arity_(_arity), name(name), chunk(std::move(chunk)), type(type) {}

    int arity() override { return arity_;}

    Value call(Interpreter *interpreter, std::vector<Value> arguments) override {
        return {};
    }

    std::string toString() override {
        return "<BeatFn " + name + ">";
    }
};
