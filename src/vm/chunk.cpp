#include "chunk.hpp"

#include <iostream>

#include "lox_function.hpp"

void Chunk::disassembleChunk( const std::string& name) {
    printf("== %s ==\n", name.c_str());
    for (int offset = 0; offset < m_bytecodes.size();) {
        offset = disassembleInstruction(offset);
    }
}

int Chunk::disassembleInstruction(int offset) {
    printf("%04d ", offset);
    if (offset > 0 && m_lines[offset] == m_lines[offset - 1]) {
        printf("   | ");
     } else {
         printf("%4d ", m_lines[offset]);
     }
    uint8_t instruction = m_bytecodes[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", offset);
        case OP_GET_LOCAL:
            return byteInstruction("OP_GET_LOCAL", offset);
        case OP_SET_LOCAL:
            return byteInstruction("OP_SET_LOCAL", offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_MODULO:
            return simpleInstruction("OP_MODULO", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, offset);
        case OP_CALL:
            return byteInstruction("OP_CALL", offset);
        case OP_ARRAY_LITERAL:
            return byteInstruction("OP_ARRAY_LITERAL", offset);
        case OP_MAP_LITERAL:
            return byteInstruction("OP_MAP_LITERAL", offset);
        case OP_SUBSCRIPT:
            return simpleInstruction("OP_SUBSCRIPT", offset);
        case OP_SUBSCRIPT_ASSIGNMENT:
            return simpleInstruction("OP_SUBSCRIPT_ASSIGNMENT", offset);
        case OP_POSTFIX_INC_LOCAL:
            return byteInstruction("OP_POSTFIX_INC_LOCAL", offset);
        case OP_POSTFIX_DEC_LOCAL:
            return byteInstruction("OP_POSTFIX_DEC_LOCAL", offset);
        case OP_POSTFIX_INC_GLOBAL:
            return constantInstruction("OP_POSTFIX_INC_GLOBAL", offset);
        case OP_POSTFIX_DEC_GLOBAL:
            return constantInstruction("OP_POSTFIX_DEC_GLOBAL", offset);
        case OP_POSTFIX_INC_UPVALUE:
            return byteInstruction("OP_POSTFIX_INC_UPVALUE", offset);
        case OP_POSTFIX_DEC_UPVALUE:
            return byteInstruction("OP_POSTFIX_DEC_UPVALUE", offset);
        case OP_POSTFIX_INC_SUBSCRIPT:
            return simpleInstruction("OP_POSTFIX_INC_SUBSCRIPT", offset);
        case OP_POSTFIX_DEC_SUBSCRIPT:
            return simpleInstruction("OP_POSTFIX_DEC_SUBSCRIPT", offset);
        case OP_CLOSURE: {
            offset++;
            uint16_t constant = (uint16_t)(m_bytecodes[offset] << 8);
            constant |= m_bytecodes[offset + 1];
            offset += 2;
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            auto callable = std::get<LoxCallable*>(m_constants[constant]);
            auto function = dynamic_cast<BeatFunction*>(callable);
            if (!function) {
                throw std::runtime_error("OP_CLOSURE must consume a BeatFunction on stack");
            }
            std::cout << m_constants[constant] << std::endl;
            for (int j = 0; j < function->upvalueCount; j++) {
                int isLocal = m_bytecodes[offset++];
                int index = m_bytecodes[offset++];
                printf("%04d      |                     %s %d\n",
                       offset - 2, isLocal ? "local" : "upvalue", index);
            }
            return offset;
        }
        case OP_GET_UPVALUE:
            return byteInstruction("OP_GET_UPVALUE", offset);
        case OP_SET_UPVALUE:
            return byteInstruction("OP_SET_UPVALUE", offset);
        case OP_CLOSE_UPVALUE:
            return simpleInstruction("OP_CLOSE_UPVALUE", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}


int Chunk::simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int Chunk::addConstant(const Value& value) {
    int offset = m_constants.size();
    m_constants.push_back(value);
    return offset;
}

 int Chunk::constantInstruction(const char* name, int offset) {
    uint16_t constant = (uint16_t)(m_bytecodes[offset + 1] << 8);
    constant |= m_bytecodes[offset + 2];
    printf("%-16s %4d '", name, constant);
    std::cout << m_constants[constant];
    printf("'\n");
    return offset + 3;
}

int Chunk::byteInstruction(const char* name,int offset) {
    uint8_t slot = m_bytecodes[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}
int Chunk::jumpInstruction(const char* name, int sign, int offset) {
    uint16_t jump = (uint16_t)(m_bytecodes[offset + 1] << 8);
    jump |= m_bytecodes[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}
