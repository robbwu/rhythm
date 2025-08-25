#include <iostream>
#include <variant>
#include "token.hpp"
#include "vm.hpp"
#include "lox_function.hpp"

extern bool debug_trace_exeuction;
extern bool op_counters_flag;

#define ASSERT_MSG(expr, msg) \
    if (!(expr)) { \
        std::cerr << "Assertion failed: " << #expr << ", message: " << msg << ", in file " << __FILE__ << ", line " << __LINE__ << '\n'; \
        std::abort(); \
    }


InterpretResult VM::run(BeatClosure *closure){
    if(closure->function->type == BeatFunctionType::SCRIPT) {
        // reset the stack and frames
        stack.clear();
        frames.clear();
        // initialize the frame;
        frames.push_back(std::make_shared<CallFrame>(closure, &closure->function->chunk.m_bytecodes[0], 0));
    } else {
        error(0, "NOT IMPLEMENTED YET");
    }
    auto result = run();

    if (op_counters_flag) {
        int64_t total_ops = 0;
        for (int i=0; i<OP_END; i++) {
            total_ops += op_counters[i];
        }
        std::cout << "=== total opcodes executed: " << total_ops << std::endl;
    }

    return result;
}


InterpretResult VM::run(int ret_frame) {

#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants()[READ_BYTE()])
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
            frame->closure->function->chunk.disassembleInstruction((int)(frame->ip - &frame->closure->function->chunk.m_bytecodes[0]));
        }

        uint8_t instruction = READ_BYTE();
        if (instruction < OP_END) op_counters[instruction]++;
        switch (instruction) {
            case OP_CONSTANT: {
                auto constant = READ_CONSTANT();
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
                if (std::holds_alternative<bool>(stack.back()))
                    stack.back() = !std::get<bool>(stack.back());
                else if (std::holds_alternative<nullptr_t>(stack.back()))
                    stack.back() = true;
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
            case OP_MODULO:   {
                Value b = pop();
                if (!std::holds_alternative<double>(b) || !std::holds_alternative<double>(stack.back())) {
                    error(0, "binary % operands must be numbers");
                }
                if (!is_integer((double)std::get<double>(b)) || !is_integer((double)std::get<double>(stack.back()))) {
                    error(0, "binary % operands must be integers");
                }
                stack.back() = (double)((int)std::get<double>(stack.back()) % (int) std::get<double>(b));
                break;
            }

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
                if (frames.size() > 1){
                    closeUpvalues(&stack[frame->frame_pointer]);
                    printOpenUpvalues();
                }
                auto result = pop(); // save the return value before popping the frame

                if (frames.size()>1) {
                    stack.resize(frame->frame_pointer-1); // restore stack to the frame pointer
                }
                frames.pop_back(); // pop the current frame
                if (!frames.empty())
                    frame = frames.back();
                push(result); // push the return value back to the stack
                if (frames.size() <= ret_frame) {
                    return INTERPRET_OK;
                }
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

                BeatClosure* beat_closure = dynamic_cast<BeatClosure*>(func);
                if ( beat_closure != nullptr) {
                    if (beat_closure->arity() != argCount) {
                        error(0, std::format("function {} expected {} arguments but got {}", beat_closure->toString(), beat_closure->arity(), argCount));
                    }
                    // create a new call frame; frame pointer points to the first of the arguments
                    frames.push_back(std::make_shared<CallFrame>(beat_closure, &beat_closure->function->chunk.m_bytecodes[0], stack.size() - argCount));
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
                    Value result = func->call(this, arguments);
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
            case OP_MAP_LITERAL: {
                int size = READ_BYTE();
                auto map = std::make_shared<Map>(std::unordered_map<Value, Value>());
                for (int i = 0; i < size; i++) {
                    auto value = pop();
                    auto key = pop();
                    map->data[key] = value;
                }
                push(map);
                break;
            }
            case OP_SUBSCRIPT: {
                auto i = pop();
                auto obj = pop();
                if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
                    push(std::get<std::shared_ptr<Array>>(obj)->data.at((int)std::get<double>(i)));
                } else if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
                    auto it = std::get<std::shared_ptr<Map>>(obj)->data.find(i);
                    if (it == std::get<std::shared_ptr<Map>>(obj)->data.end())
                        push(nullptr); // if the key is not found, return nil
                    else
                        push(it->second);
                } else {
                    std::cout << "obj " << obj << std::endl;
                    error(0, std::format("OP_SUBSCRIPT obj can only be Array or Map"));
                }
                break;
            }
            case OP_SUBSCRIPT_ASSIGNMENT: {
                auto value = pop();
                auto i = pop();
                auto obj = pop();
                if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
                    std::get<std::shared_ptr<Array>>(obj)->data.at((int)std::get<double>(i)) = value;
                    push(value);
                } else if (std::holds_alternative<std::shared_ptr<Map>>(obj)) {
                    auto& map = std::get<std::shared_ptr<Map>>(obj)->data;
                    auto it = map.find(i);
                    if (std::holds_alternative<nullptr_t>(value)) { // remove key if value is nil
                        map.erase(i);
                        push(nullptr);
                        break;
                    }
                    map[i] = value; // assign the value to the key
                    push(value);
                } else {
                    std::cout << "obj " << obj << std::endl;
                    error(0, std::format("OP_SUBSCRIPT obj can only be Array or Map"));
                }
                break;
            }
            case OP_CLOSURE: {
                auto func = std::get<LoxCallable*>(READ_CONSTANT());
                auto beat_func = dynamic_cast<BeatFunction*>(func);
                if (!beat_func) {
                    error(0, "OP_CLOSURE operand must be a BeatFunction");
                }
                auto closure = new BeatClosure(beat_func); //FIXME: this leaks?
                push(closure);
                for (int i=0; i<closure->upvalues.size(); i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        // printf("XXX OP_CLOSURE %s:captured local index %d\n", closure->toString().c_str(), index);
                        closure->upvalues[i] = captureUpvalue(&stack[frame->frame_pointer+index]);
                    } else {
                        // printf("XXX OP_CLOSURE %s:captured non-local index %d\n", closure->toString().c_str(), index);
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                    printOpenUpvalues();
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                // printf("slot %d\n", slot);
                // std::cout << "size of current upvalues " << frame->closure->upvalues.size() << std::endl;
                // auto upvalue = frame->closure->upvalues[slot];
                // printf("upvalue: location %p\n", upvalue->location );
                // std::cout << "getupvalue "<< *frame->closure->upvalues[slot]->location << std::endl;
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                // no pop() here because assignment is an expression; need to leave sth on stack top.
                break;
            }
            case OP_CLOSE_UPVALUE: {
                // std::cout << "upvalue count of current frame" << frame->closure->upvalues.size() << std::endl;
                // throw std::runtime_error("OP_CLOSE_UPVALUE not implemented");
                closeUpvalues(&stack.back());
                printOpenUpvalues();
                pop();
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
// for debuggin purpose
void VM::printOpenUpvalues() {
    if (!debug_trace_exeuction) return;
    auto upvalue =  openUpvalues;
    std::cout << "[[";
    while (upvalue != nullptr) {
        std::cout <<  *upvalue->location << ", ";
        upvalue = upvalue->next;
    }
    std::cout << "]]" << std::endl;
}

Upvalue* VM::captureUpvalue(Value* local) {
    // has this local already be captured as upvalue?
    Upvalue* prev = NULL;
    Upvalue* upvalue = openUpvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        // already captured!
        // std::shared_ptr<Upvalue> upvalue_adopter(upvalue); // TODO: maybe not using Value* as intermdiary?
        return upvalue;
    }

    // if (upvalue != nullptr) std::cout <<  *upvalue->location << std::endl;
    // else std::cout <<  "current upvalue is null" << std::endl;
    auto createdUpvalue = new Upvalue(local);
    createdUpvalue->next = upvalue;
    // std::cout << "next         " << upvalue << std::endl;
    // std::cout << "openUpvalues " << openUpvalues << std::endl;
    if (prev == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prev->next = createdUpvalue;
    }
    // std::cout << "XXX created upvalue for " << *createdUpvalue->location << std::endl;
    // std::cout << "new openUpvalues " << openUpvalues << std::endl;
    return createdUpvalue;
}

void VM::closeUpvalues(Value* last) {
    // std::cout << "### closeUpvalue " << *last << std::endl;
    // std::cout << "  openUpvalues  location " << openUpvalues->location << std::endl;
    // std::cout << "  last                   " << last << std::endl;
    while (openUpvalues != NULL &&
        openUpvalues->location >= last) {
        auto upvalue = openUpvalues;
        upvalue->closed = *upvalue->location; // copied Value from stack to heap
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
        // std::cout << "### closed an open upvalue " << upvalue->closed << std::endl;
    }
}


void VM::print_stack_trace() {
    for (int i = frames.size() - 1; i >= 0; i--) {
        auto& frame = frames[i];
        auto  function = frame->closure->function;
        size_t instruction = frame->ip - &function->chunk.m_bytecodes[0] - 1;
        fprintf(stderr, "[line %d] in ",
                function->chunk.lines()[instruction]);
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


Value VM::callFunction(LoxCallable* func, const std::vector<Value>& args) {
    // Push function onto stack
    push(func);

    // Push arguments onto stack
    for (const auto& arg : args) {
        push(arg);
    }

    // Handle the call similar to OP_CALL
    int argCount = args.size();

    BeatClosure* closure = dynamic_cast<BeatClosure*>(func);
    if (closure != nullptr) {
        if (closure->arity() != argCount) {
            error(0, std::format("function {} expected {} arguments but got {}",
                    closure->toString(), closure->arity(), argCount));
        }

        // Create a new call frame and execute
        // auto currentFrame = frames.back();
        int num_frames = frames.size();
        frames.push_back(std::make_shared<CallFrame>(closure, &closure->function->chunk.m_bytecodes[0], stack.size() - argCount));

        // Execute the function by running until return when the given frame is run;
        InterpretResult result = run(num_frames);

        // The result should be on top of the stack
        return peek();
    } else {
        error(0, "not implemented--native function cannot call native function yet");
        // Native function - call directly
        std::vector<Value> arguments;
        arguments.reserve(argCount);
        for (int i = 0; i < argCount; i++) {
            arguments.push_back(pop());
        }
        std::reverse(arguments.begin(), arguments.end());
        Value result = func->call(this, arguments);
        pop(); // pop the function
        return result;
    }
}
