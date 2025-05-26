#include "chunk.hpp"

#include <iostream>

void Chunk::disassembleChunk( const std::string& name) {
    printf("== %s ==\n", name.c_str());
    for (int offset = 0; offset < bytecodes.size();) {
        offset = disassembleInstruction(offset);
    }
}

int Chunk::disassembleInstruction(int offset) {
    printf("%04d ", offset);
    if (offset > 0 && lines[offset] == lines[offset - 1]) {
        printf("   | ");
     } else {
         printf("%4d ", lines[offset]);
     }
    uint8_t instruction = bytecodes[offset];
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
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, offset);
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
    int offset = constants.size();
    constants.push_back(value);
    return offset;
}

 int Chunk::constantInstruction(const char* name, int offset) {
    uint8_t constant = bytecodes[offset + 1];
    printf("%-16s %4d '", name, constant);
    std::cout << constants[constant];
    printf("'\n");
    return offset + 2;
}

int Chunk::byteInstruction(const char* name,int offset) {
    uint8_t slot = bytecodes[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}
int Chunk::jumpInstruction(const char* name, int sign, int offset) {
    uint16_t jump = (uint16_t)(bytecodes[offset + 1] << 8);
    jump |= bytecodes[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset,
           offset + 3 + sign * jump);
    return offset + 3;
}