#include <iostream>
#include <variant>
#include "token.hpp"
#include "vm.hpp"
#include "lox_function.hpp"

extern bool debug_trace_exeuction;

#define ASSERT_MSG(expr, msg) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << #expr << ", message: " << msg << ", in file " << __FILE__ << ", line " << __LINE__ << '\n'; \
        std::abort(); \
    }


InterpretResult VM::run(BeatFunction *func){
    if(func->type == BeatFunctionType::SCRIPT) {
        // reset the stack and frames
        stack.clear();
        frames.clear();
        // initialize the frame;
        frames.push_back(std::make_shared<CallFrame>(func, &func->chunk.bytecodes[0], 0));
    } else {
        error(0, "NOT IMPLEMENTED YET");
    }
    return run();
}


InterpretResult VM::run() {

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->function->chunk.constants[READ_BYTE()])
#define READ_STRING() std::get<std::string>(READ_CONSTANT())
#define BINARY_OP(op) \
    do { \
        Value b = pop(); \
        if (!std::holds_alternative<double>(b)) \
            error(0, "binary op operands must be numbers"); \
        stack.back() = std::get<double>(stack.back()) op std::get<double>(b); \
    } while (false)

    auto frame = frames.back();
    for (;;) {
        if (debug_trace_exeuction) {
            printf("          ");
            for (auto & slot : stack) {
                printf("[ ");
                std::cout << slot;
                printf(" ]");
            }
            printf("\n");
            frame->function->chunk.disassembleInstruction((int)(frame->ip - &frame->function->chunk.bytecodes[0]));
        }

        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_CONSTANT: {
                Value &constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_PRINT: {
                auto a = pop();
                std:: cout << a << std::endl;
                break;
            }
            case OP_NIL: {
                push(nullptr);
                break;
            }
            case OP_POP: {
                pop();
                break;
            }
            case OP_NOT: {
                push(! std::get<bool>(pop()));
                stack.back() = !std::get<bool>(stack.back());
                break;
            }
            case OP_NEGATE: {
                if (!std::holds_alternative<double>(stack.back())) {
                    throw new VMRuntimeError(0, "negate operand is not a number" );
                }
                stack.back() = -std::get<double>(stack.back());
                break;
            }
            // stack: [a, b] => [a+b]
            case OP_ADD: {
                Value b = pop();
                // Value a = pop();
                if (std::holds_alternative<double>(b)) {
                    stack.back() = std::get<double>(stack.back()) + std::get<double>(b);
                } else if (std::holds_alternative<std::string>(b)) {
                    stack.back() = std::get<std::string>(stack.back()) + std::get<std::string>(b);
                } else {
                    error(0, "binary op operands must both be numbers or strings");
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;

            case OP_EQUAL: {
                Value b = pop();
                stack.back() = (stack.back() == b);
                break;
            }
            case OP_GREATER:  BINARY_OP(>); break;
            case OP_LESS:     BINARY_OP(<); break;
            case OP_DEFINE_GLOBAL: {
                auto name = READ_STRING();
                globals[name] = pop();
                break;
            }
            case OP_GET_GLOBAL: {
                auto name = READ_STRING();
                auto it = globals.find(name);
                if (it == globals.end()) {
                    error(0, std::format("global variable {} not found", name));
                }
                push(globals[name]);
                break;
            }
            case OP_SET_GLOBAL: {
                auto name = READ_STRING();
                globals[name] = peek();
                break;
            }
            case OP_SET_LOCAL: {
                auto slot = READ_BYTE();
                stack[frame->frame_pointer+slot] = peek();
                break;
            }
            case OP_GET_LOCAL: {
                auto slot = READ_BYTE();
                push(stack[frame->frame_pointer+slot]);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!is_truthy(peek())) {
                    frame->ip += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_RETURN: {
                // std::cout << "XXX OP_RETURN frames left" << frames.size() << std::endl;
                auto result = pop();
                if (frames.size()==1) { // if this is the last frame, we are done
                    // assert(stack.size() == 0);
                    // pop();
                    ASSERT_MSG(stack.size() == 0, std::format("stack size {}, wanted 0", stack.size()));
                    return INTERPRET_OK;
                }
                stack.resize(frame->frame_pointer-1); // restore stack to the frame pointer
                frames.pop_back(); // pop the current frame
                frame = frames.back();
                push(result);
                break;
            }
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (argCount > 255) {
                    error(0, "cannot call function with more than 255 arguments");
                }
                if (argCount < 0) {
                    error(0, "cannot call function with negative arguments");
                }
                if (argCount > stack.size() - frame->frame_pointer - 1) {
                    error(0, "not enough arguments for function call");
                }
                auto fun = peek(argCount);
                if (!std::holds_alternative<LoxCallable*>(fun)) {
                    error(0, "OP_CALL cannot find LoxCallable* on stack");
                }
                LoxCallable* func = std::get<LoxCallable*>(fun);

                BeatFunction* beat_func = dynamic_cast<BeatFunction*>(func);
                if ( beat_func != nullptr) {
                    if (beat_func->arity() != argCount) {
                        error(0, std::format("function {} expected {} arguments but got {}", beat_func->toString(), beat_func->arity(), argCount));
                    }
                    // create a new call frame; frame pointer points to the first of the arguments
                    frames.push_back(std::make_shared<CallFrame>(beat_func, &beat_func->chunk.bytecodes[0], stack.size() - argCount));
                    frame = frames.back();
                    break;
                } else if (func != nullptr) { // not BeatFunction, must be subclass of LoxCallable, native functions
                    if (func->arity() != -1 && func->arity() != argCount) {
                        error(0, std::format("function {} expected {} arguments but got {}", func->toString(), func->arity(), argCount));
                    }
                    std::vector<Value> arguments;
                    arguments.reserve(argCount);
                    for (int i = 0; i < argCount; i++) {
                        arguments.push_back(pop());
                    }
                    std::reverse(arguments.begin(), arguments.end()); // reverse the order of arguments
                    Value result = func->call(nullptr, arguments);
                    pop(); // pop the function from the stack
                    push(result); // push the result of the function call
                    break;
                } else {
                    error(0, "OP_CALL cannot find LoxCallable* on stack");
                }
                break;
            }
            case OP_ARRAY_LITERAL: {
                int size = READ_BYTE();
                auto array = std::make_shared<Array>(std::vector<Value>());
                array->data.reserve(size);
                // array->reserve(size);
                for (int i = 0; i < size; i++) {
                    array->data.push_back(pop());
                }
                std::reverse(array->data.begin(), array->data.end()); // reverse the order of elements
                push(array);
                break;
            }
            case OP_SUBSCRIPT: {
                auto i = pop();
                auto obj = pop();
                if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
                    push(std::get<std::shared_ptr<Array>>(obj)->data.at((int)std::get<double>(i)));
                } else if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
                    // TODO
                } else {
                    std::cout << "obj " << obj << std::endl;
                    error(0, std::format("OP_SUBSCRIPT obj can only be Array or Map"));
                }
                break;
            }
            default:
                throw std::runtime_error("Unknown instruction");
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP

}


void VM::print_stack_trace() {
    for (int i = frames.size() - 1; i >= 0; i--) {
        auto& frame = frames[i];
        auto  function = frame->function;
        size_t instruction = frame->ip - &function->chunk.bytecodes[0] - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk.lines[instruction]);
        if (function->name == "") {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name.c_str());
        }
    }
}

void VM::error(int line, std::string msg) {
    if (line == 0) {
        std::cerr << "Runtime error: " << msg << std::endl;
    } else {
        std::cerr << "Runtime error at line " << line << ": " << msg << std::endl;
    }
    print_stack_trace();
    throw VMRuntimeError(line, msg);
}
