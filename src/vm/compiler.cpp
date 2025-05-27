#include <filesystem>

#include "compiler.hpp"


void Compiler::visit(const Binary &expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    OpCode opcode;
    auto line = expr.op.line;
    switch (expr.op.type) {
        case TokenType::PLUS: chunk.write(OP_ADD, line); break;
        case TokenType::MINUS: chunk.write(OP_SUBTRACT, line); break;
        case TokenType::STAR: chunk.write(OP_MULTIPLY, line); break;
        case TokenType::SLASH: chunk.write(OP_DIVIDE, line); break;

        case TokenType::EQUAL_EQUAL: chunk.write(OP_EQUAL, line); break;
        case TokenType::BANG_EQUAL:
            chunk.write(OP_EQUAL, line);
            chunk.write(OP_NOT, line);
            break;
        case TokenType::GREATER: chunk.write(OP_GREATER, line); break;
        case TokenType::LESS: chunk.write(OP_LESS, line); break;
        case TokenType::GREATER_EQUAL:
            chunk.write(OP_LESS, line);
            chunk.write(OP_NOT, line);
            break;
        case TokenType::LESS_EQUAL:
            chunk.write(OP_GREATER, line);
            chunk.write(OP_NOT, line);
            break;
        default:
            throw CompileException("binary op type not in +,-,*,/");
            break;
    }
}


void Compiler::visit(const Literal &expr) {
    int constant = chunk.addConstant(expr.value);
    if (constant >= 256) {
        throw CompileException("cannot compile >= 256 constants");
    }
    chunk.write(OP_CONSTANT, 0);
    chunk.write(constant, 0);
}


void Compiler::visit(const Logical &expr) {
    if (expr.op.type ==  TokenType::AND){
        expr.left->accept(*this);
        int endJump = emitJump(OP_JUMP_IF_FALSE, expr.op.line);
        chunk.write(OP_POP, expr.op.line);
        expr.right->accept(*this);
        patchJump(endJump);
    } else if (expr.op.type ==  TokenType::OR) {
        expr.left->accept(*this);
        int elseJump = emitJump(OP_JUMP_IF_FALSE, expr.op.line);
        int endJump = emitJump(OP_JUMP, expr.op.line);
        patchJump(elseJump);
        chunk.write(OP_POP, expr.op.line);
        expr.right->accept(*this);
        patchJump(endJump);
    } else {
        throw CompileException("logical expr op type must be OR or AND");
    }
};

void Compiler::visit(const Grouping &expr) {
    expr.expression->accept(*this);
};

void Compiler::visit(const Unary &expr) {
    expr.right->accept(*this);
    OpCode opcode;
    switch (expr.op.type) {
        case TokenType::MINUS:
            opcode = OP_NEGATE;
            break;
        case TokenType::BANG:
            opcode = OP_NOT;
            break;
        default:
            throw CompileException("unary op type not supported");
    }
    chunk.write(opcode, expr.op.line);
};

void Compiler::visit(const Variable &expr) {
    int arg = resolveLocal(expr.name);
    if (arg == -1) { // global scope
        // TODO: this is wasteful--should deduplicate strings in globals contants
        int constant = chunk.addConstant(expr.name.lexeme);
        chunk.write(OP_GET_GLOBAL, expr.name.line);
        chunk.write(constant, expr.name.line);
    } else {
        chunk.write(OP_GET_LOCAL, expr.name.line);
        chunk.write(arg, expr.name.line);
    }
};

void Compiler::visit(const Assignment &expr) {
    expr.right->accept(*this);

    int arg = resolveLocal(expr.name);
    if (arg == -1) { // global var assignment
        int constant = chunk.addConstant(expr.name.lexeme);
        chunk.write(OP_SET_GLOBAL, expr.name.line);
        chunk.write(constant, expr.name.line);
    } else {
        chunk.write(OP_SET_LOCAL, expr.name.line);
        chunk.write(arg, expr.name.line);
    }
};

// returns the slot index in the locals stack in both Compiler/VM (as they mirror)
// return -1 if no local varialbel found; assume to be global
int Compiler::resolveLocal(Token token) {
    for (int i= locals.size()-1; i>= 0; i--) {
        if (locals[i].name.lexeme == token.lexeme) {
            // declared but not defined; should error out; this is to prevent self-referential
            // VarDef: var a = a;
            if (locals[i].depth == -1) {
                throw CompileException(std::format("local variable {} declared but not defined", token.lexeme));
            }
            return i;
        }
    }
    return -1;
}

void Compiler::visit(const Call &) {
};

void Compiler::visit(const ArrayLiteral &) {
};

void Compiler::visit(const MapLiteral &) {
};

void Compiler::visit(const Subscript &) {
};

void Compiler::visit(const PropertyAccess &) {
};

void Compiler::visit(const SubscriptAssignment &) {
};

void Compiler::visit(const FunctionExpr &) {
};


// statements
void Compiler::visit(const ExpressionStmt &stmt) {
    stmt.expr->accept(*this);
};

void Compiler::visit(const PrintStmt &stmt) {
    stmt.expr->accept(*this);
    chunk.write(OP_PRINT, 0); // FIXME: there is no line info
};

void Compiler::visit(const VarStmt &stmt) {
    if (scopeDepth > 0) {
        locals.push_back({stmt.name, -1, false});
    }
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        chunk.write(OP_NIL, stmt.name.line);
    }

    if (scopeDepth == 0) { // global variable declaration
        int constant = chunk.addConstant(stmt.name.lexeme);
        if (constant >= 256) {
            throw CompileException("cannot compile >= 256 constants");
        }
        chunk.write(OP_DEFINE_GLOBAL, stmt.name.line);
        chunk.write(constant, stmt.name.line);
    } else { // local variable declaration
        locals.back().depth = scopeDepth;
        // no need to emit any opcodes; just bookkeep the position of the local variables on
        // compile stack (and therefore the runtime VM stack because they mirror each other).
    }
};

void Compiler::visit(const BlockStmt &stmt) {
    beginScope();
    for (const auto &stmt : stmt.statements) {
        stmt->accept(*this);
    }
    endScope();
};

int Compiler::emitJump(uint8_t instruction, int line) {
    chunk.write(instruction, line);
    chunk.write(0xff, line);
    chunk.write(0xff, line);
    return chunk.bytecodes.size() - 2;
}

void Compiler::patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = chunk.bytecodes.size() - offset - 2;

    if (jump > UINT16_MAX) {
        throw CompileException("Too much code to jump over.");
    }

    chunk.bytecodes[offset] = (jump >> 8) & 0xff;
    chunk.bytecodes[offset + 1] = jump & 0xff;
}

void Compiler::visit(const IfStmt &stmt) {
    stmt.condition->accept(*this);
    int thenJump = emitJump(OP_JUMP_IF_FALSE, 0);
    chunk.write(OP_POP, 0);
    stmt.thenBlock->accept(*this);
    int elseJump = emitJump(OP_JUMP, 0);
    patchJump(thenJump);
    chunk.write(OP_POP, 0);
    if (stmt.elseBlock) {
        stmt.elseBlock->accept(*this);
        patchJump(elseJump);
    }
};

void Compiler::emitLoop(int loopStart) {
    chunk.write(OP_LOOP, 0);

    int offset = chunk.bytecodes.size() - loopStart + 2;
    if (offset > UINT16_MAX)
        throw CompileException("Loop body too large.");

    chunk.write((offset >> 8) & 0xff, 0);
    chunk.write(offset & 0xff, 0);
}

void Compiler::visit(const WhileStmt &stmt) {
    int loopStart = chunk.bytecodes.size();
    stmt.condition->accept(*this);
    int exitJump = emitJump(OP_JUMP_IF_FALSE, 0);
    chunk.write(OP_POP, 0);
    stmt.body->accept(*this);
    if (stmt.increment) {
        stmt.increment->accept(*this);
        chunk.write(OP_POP, 0);
    }
    emitLoop(loopStart);
    patchJump(exitJump);
    chunk.write(OP_POP, 0);
};

void Compiler::visit(const FunctionStmt &) {};

void Compiler::visit(const ReturnStmt &) {};

void Compiler::visit(const BreakStmt &) {};

void Compiler::visit(const ContinueStmt &) {};
