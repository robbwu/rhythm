#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "token.hpp"

typedef enum  {
    OP_RETURN, OP_CONSTANT,
    OP_NEGATE, OP_NOT,
    OP_ADD, OP_SUBTRACT, OP_MULTIPLY, OP_DIVIDE, OP_EQUAL, OP_GREATER, OP_LESS, OP_MODULO,
    OP_PRINT, OP_NIL,
    OP_DEFINE_GLOBAL, OP_GET_GLOBAL, OP_SET_GLOBAL,
    OP_SET_LOCAL, OP_GET_LOCAL, OP_SET_UPVALUE, OP_GET_UPVALUE,
    OP_POP,
    OP_JUMP_IF_FALSE, OP_JUMP, OP_LOOP, OP_CALL,
    OP_ARRAY_LITERAL, OP_MAP_LITERAL, OP_SUBSCRIPT, OP_SUBSCRIPT_ASSIGNMENT,
    OP_POSTFIX_INC_LOCAL, OP_POSTFIX_DEC_LOCAL,
    OP_POSTFIX_INC_GLOBAL, OP_POSTFIX_DEC_GLOBAL,
    OP_POSTFIX_INC_UPVALUE, OP_POSTFIX_DEC_UPVALUE,
    OP_POSTFIX_INC_SUBSCRIPT, OP_POSTFIX_DEC_SUBSCRIPT,
    OP_CLOSURE, OP_CLOSE_UPVALUE,
    OP_END // not used, just for counting the number of opcodes
} OpCode;

// using Chunk = std::vector<uint8_t>;
class Chunk {
public:
    std::vector<uint8_t> m_bytecodes; // TODO: make this private

    void disassembleChunk(const std::string& name);
    int disassembleInstruction(int offset);
    int addConstant(const Value& value);
    void write(uint8_t byte, int line) {
        m_bytecodes.push_back(byte);
        m_lines.push_back(line);
    }

    int constantInstruction(const char* name, int offset);
    int byteInstruction(const char* name,int offset);
    int simpleInstruction(const char* name, int offset);
    int jumpInstruction(const char* name, int sign, int offset);

    void clear_byteclodes() {
        m_bytecodes.clear();
    }
    void clear_lines()
    {
        m_lines.clear();
    }
    [[nodiscard]] const std::vector<uint8_t>& bytecodes() const {
        return m_bytecodes;
    }
    [[nodiscard]] const std::vector<Value>& constants() const {
        return m_constants;
    }
    [[nodiscard]] const std::vector<int>& lines() const {
        return m_lines;
    }
private:
    std::vector<Value> m_constants;
    std::vector<int> m_lines;
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
    int upvalueCount = 0;

    BeatFunction(int _arity, const std::string &name, Chunk &chunk, BeatFunctionType type, int cnt): arity_(_arity), name(name), chunk(std::move(chunk)), type(type), upvalueCount(cnt) {}

    int arity() override { return arity_;}

    Value call(RuntimeContext *ctxt, std::vector<Value> arguments) override {
        return {};
    }

    std::string toString() override {
        return "<BeatFn " + name + ">";
    }
};

class Upvalue {
public:
    Value*  location; //FIXME: make this ref counted?
    Upvalue*  next = nullptr;
    Value   closed = nullptr; // in OP_CLOSE_UPVALUE, the stack value is moved to here on heap
    explicit Upvalue(Value* location): location(location) {}

    ~Upvalue() {
        std::cout << "XXXXXXXXXXX Upvalue destructor called for value: " << closed << std::endl;
    }
};


class BeatClosure: public LoxCallable {
public:
    BeatFunction* function;
    std::vector<Upvalue*> upvalues; // FIXME: this is leaking

    explicit BeatClosure(BeatFunction* beatFunction) : function(beatFunction) {
        for (int i = 0; i < function->upvalueCount; i++) {
            upvalues.push_back((nullptr));
        }
    }
    int arity() override { return function->arity();}
    Value call(RuntimeContext *ctxt, std::vector<Value> arguments) override {
        return function->call(ctxt, arguments);
    }
    std::string toString() override {
        return "<BeatClosure " + function->name + ">";
    }

    // static BeatClosure * create(BeatFunction * script);
};
