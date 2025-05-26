#include <iostream>
#include "vm.hpp"

#define DEBUG_TRACE_EXECUTION

InterpretResult VM::run(Chunk chunk) {
    ip = &chunk.bytecodes[0];
#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
#define READ_CONSTANT() (chunk.constants[READ_BYTE()])
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
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (auto & slot : stack) {
            printf("[ ");
            std::cout << slot;
            printf(" ]");
        }
        printf("\n");
        chunk.disassembleInstruction((int)(ip - &chunk.bytecodes[0]));
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
                // std::cout << pop() << std::endl;
                return INTERPRET_OK;
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
                globals[name] = pop();
                break;
            }
            case OP_SET_LOCAL: {
                auto slot = READ_BYTE();
                stack[slot] = pop();
                break;
            }
            case OP_GET_LOCAL: {
                auto slot = READ_BYTE();
                push(stack[slot]);
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (!is_truthy(peek())) {
                    ip += offset;
                }
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                ip -= offset;
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
