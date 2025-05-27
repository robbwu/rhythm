#include <iostream>
#include <variant>
#include "token.hpp"
#include "vm.hpp"

// #define DEBUG_TRACE_EXECUTION

InterpretResult VM::run(BeatFunction *func){
    if(func->type == BeatFunctionType::SCRIPT) {
        // reset the stack and frames
        stack.clear();
        frames.clear();
        // initialize the frame;
        frames.push_back(std::make_shared<CallFrame>(func, &func->chunk.bytecodes[0], 0));
    } else {
        throw VMRuntimeError(0, "NOT IMPLEMENTED YET");
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
        Value a = pop(); \
        if (!std::holds_alternative<double>(a) || !std::holds_alternative<double>(b)) \
            throw VMRuntimeError(0, "binary op operands must be numbers"); \
        push( std::get<double>(a) op std::get<double>(b)); \
    } while (false)


    for (;;) {
        auto &frame = frames.back();
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (auto & slot : stack) {
            printf("[ ");
            std::cout << slot;
            printf(" ]");
        }
        printf("\n");
        frame->function->chunk.disassembleInstruction((int)(frame->ip - &frame->function->chunk.bytecodes[0]));
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
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
                break;
            }
            case OP_NEGATE: {
                auto v = pop();
                if (!std::holds_alternative<double>(v)) {
                    throw new VMRuntimeError(0, "negate operand is not a number" );
                }
                push(-std::get<double>(v));
                break;
            }
            case OP_RETURN: {
                auto result = pop();
                if (frames.size()==1) { // if this is the last frame, we are done
                    return INTERPRET_OK;
                }
                stack.resize(stack.size() - 1 - frame->function->arity()); // restore stack to the frame pointer
                frames.pop_back(); // pop the current frame
                push(result);
                break;
            }
            case OP_ADD: {
                Value b = pop();
                Value a = pop();
                if (std::holds_alternative<double>(a) && std::holds_alternative<double>(b))
                    push( std::get<double>(a) + std::get<double>(b));
                else if (std::holds_alternative<std::string>(a) && std::holds_alternative<std::string>(b))
                    push( std::get<std::string>(a) + std::get<std::string>(b));
                else
                    throw VMRuntimeError(0, "binary op operands must both be numbers or strings");
                break;
            }
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;

            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(a == b);
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
                    throw VMRuntimeError(0, std::format("global variable {} not found", name));
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
            case OP_CALL: {
                int argCount = READ_BYTE();
                if (argCount > 255) {
                    throw VMRuntimeError(0, "cannot call function with more than 255 arguments");
                }
                if (argCount < 0) {
                    throw VMRuntimeError(0, "cannot call function with negative arguments");
                }
                if (argCount > stack.size() - frame->frame_pointer - 1) {
                    throw VMRuntimeError(0, "not enough arguments for function call");
                }
                auto fun = peek(argCount);
                if (!std::holds_alternative<LoxCallable*>(fun)) {
                    throw VMRuntimeError(0, "OP_CALL cannot find LoxCallable* on stack");
                }
                LoxCallable* func = std::get<LoxCallable*>(fun);
                BeatFunction* beat_func = dynamic_cast<BeatFunction*>(func);
                if ( beat_func == nullptr) {
                    throw VMRuntimeError(0, std::format("OP_CALL expected a BeatFunction but got {}", func->toString()));
                }
                if (beat_func->arity() != argCount) {
                    throw VMRuntimeError(0, std::format("function {} expected {} arguments but got {}", beat_func->toString(), beat_func->arity(), argCount));
                }
                // create a new call frame
                frames.push_back(std::make_shared<CallFrame>(beat_func, &beat_func->chunk.bytecodes[0], stack.size() - argCount));

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
