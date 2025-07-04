#pragma once

#include "chunk.hpp"
#include "compiler.hpp"
#include "native_func.hpp"
#include "native_func_array.hpp"
#include "native_func_math.hpp"
#include "vm_exception.hpp"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;



class CallFrame {
public:
    BeatClosure* closure;
    uint8_t* ip; // instruction pointer
    int frame_pointer; // where the function's stack starts in the VM stack

    CallFrame(BeatClosure* closure, uint8_t* instruction_pointer, int fp)
        : closure(closure), ip(instruction_pointer), frame_pointer(fp) {}
};

class VM: public RuntimeContext {
    std::vector<Value> stack; // stack VM
    std::vector<std::shared_ptr<CallFrame>> frames; // call frame stack

    std::unordered_map<std::string, Value> globals;
public:
    Upvalue* openUpvalues = nullptr;

    explicit VM(): stack(), frames(), globals()  {
        stack.reserve(256); // this is to prevent dynamicly enlarging stack that invalidates its pointers

        globals["clock"] = new ClockCallable();
        globals["printf"] = new PrintfCallable();
        globals["sprintf"] = new SprintfCallable();
        globals["len"] = new LenCallable();
        globals["push"] = new PushCallable();
        globals["pop"] = new PopCallable();
        globals["readline"] = new ReadlineCallable();
        globals["split"] = new SplitCallable();
        globals["assert"] = new AssertCallable();
        globals["for_each"] = new ForEachCallable();
        globals["tonumber"] = new ToNumberCallable();
        globals["slurp"] = new SlurpCallable();
        globals["from_json"] = new FromJsonCallable();
        globals["to_json"] = new ToJsonCallable();

        using namespace native::NativeMathFunctionNames;
        using namespace native;
        // math 1arg
        globals[floor_name] = new NativeMath1ArgCallable<std::floor, floor_name>();
        globals[ceil_name] =  new NativeMath1ArgCallable<std::ceil, ceil_name>();
        globals[sin_name] =   new NativeMath1ArgCallable<std::sin, sin_name>();
        globals[cos_name] =   new NativeMath1ArgCallable<std::cos, cos_name>();
        globals[tan_name] =   new NativeMath1ArgCallable<std::tan, tan_name>();
        globals[asin_name] =  new NativeMath1ArgCallable<std::asin, asin_name>();
        globals[acos_name] =  new NativeMath1ArgCallable<std::acos, acos_name>();
        globals[atan_name] =  new NativeMath1ArgCallable<std::atan, atan_name>();
        globals[log_name] =   new NativeMath1ArgCallable<std::log, log_name>();
        globals[log10_name] = new NativeMath1ArgCallable<std::log10, log10_name>();
        globals[sqrt_name] =  new NativeMath1ArgCallable<std::sqrt, sqrt_name>();
        globals[exp_name] =   new NativeMath1ArgCallable<std::exp, exp_name>();
        globals[fabs_name] =  new NativeMath1ArgCallable<std::fabs, fabs_name>();

        // math 2 arg
        globals[pow_name] =   new NativeMath2ArgsCallable<std::pow, pow_name>();
        globals[atan2_name] = new NativeMath2ArgsCallable<std::atan2, atan2_name>();
        globals[fmod_name] =  new NativeMath2ArgsCallable<std::fmod, fmod_name>();

        // random
        globals["random_real"] = new UniformRandomRealCallable();
        globals["random_int"] = new UniformRandomIntegerCallable();
    };

    InterpretResult run(int ret_frame = 0);
    InterpretResult run(BeatClosure*);

    Value callFunction(LoxCallable* func, const std::vector<Value>& args) override;

    Upvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);
    void printOpenUpvalues();


    void push(const Value& v) {stack.push_back(v);}
    Value pop() {
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
