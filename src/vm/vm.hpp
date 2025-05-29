#pragma once
#include <iostream>
#include <ostream>
#include <utility>

#include "chunk.hpp"
#include "compiler.hpp"
#include "vm_exception.hpp"
#include "native_func.hpp"
#include "native_func_array.hpp"
#include "native_math.hpp"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

class CallFrame {
public:
    BeatFunction* function;
    uint8_t* ip; // instruction pointer
    int frame_pointer; // where the function's stack starts in the VM stack

    CallFrame(BeatFunction* func, uint8_t* instruction_pointer, int fp)
        : function(func), ip(instruction_pointer), frame_pointer(fp) {}
};

class VM {
    std::vector<Value> stack; // stack VM
    std::vector<std::shared_ptr<CallFrame>> frames; // call frame stack

    std::unordered_map<std::string, Value> globals;
public:

    explicit VM(): stack(), frames(), globals()  {
        globals["clock"] = new ClockCallable();
        globals["printf"] = new PrintfCallable();
        globals["assert"] = new AssertCallable();
        globals["len"] = new LenCallable();
        globals["floor"] = new FloorCallable();
    };

    InterpretResult run();
    InterpretResult run(BeatFunction *func);

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
    Value peek(int i) {
        return stack[stack.size() - 1 - i];
    }
    void print_stack_trace();
    void error(int line, std::string msg);
    void define_native_function(std::string name, LoxCallable* fun);

};
