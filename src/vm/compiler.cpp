#include <filesystem>
#include <format>

#include "compiler.hpp"
#include "token.hpp"
#include "vm/chunk.hpp"

extern bool disassemble;

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
        case TokenType::PERCENT: chunk.write(OP_MODULO, line); break;

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
    chunk.write(OP_CONSTANT, expr.line);
    chunk.write(constant, expr.line);
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

void Compiler::visit(const Ternary &expr) {
    // Compile: condition ? thenBranch : elseBranch
    // 1. Evaluate condition
    expr.condition->accept(*this);

    // 2. If false, jump to else branch
    int elseJump = emitJump(OP_JUMP_IF_FALSE, expr.question.line);

    // 3. Condition was true: pop condition, evaluate then branch
    chunk.write(OP_POP, expr.question.line);
    expr.thenBranch->accept(*this);

    // 4. Jump over else branch
    int endJump = emitJump(OP_JUMP, expr.question.line);

    // 5. Else branch: pop condition, evaluate else branch
    patchJump(elseJump);
    chunk.write(OP_POP, expr.question.line);
    expr.elseBranch->accept(*this);

    // 6. End of ternary
    patchJump(endJump);
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
    if (arg != -1) {
        chunk.write(OP_GET_LOCAL, expr.name.line);
        chunk.write(arg, expr.name.line);
        return;
    }
    arg = resolveUpvalue(expr.name);
    if (arg != -1 ) {
        chunk.write(OP_GET_UPVALUE, expr.name.line);
        chunk.write(arg, expr.name.line);
        return;
    }
    // TODO: this is wasteful--should deduplicate strings in globals contants
    int constant = chunk.addConstant(expr.name.lexeme);
    chunk.write(OP_GET_GLOBAL, expr.name.line);
    chunk.write(constant, expr.name.line);

};

void Compiler::visit(const Assignment &expr) {
    expr.right->accept(*this);

    int arg = resolveLocal(expr.name);
    if (arg != -1) { // global var assignment
        chunk.write(OP_SET_LOCAL, expr.name.line);
        chunk.write(arg, expr.name.line);
        return;
    }
    arg = resolveUpvalue(expr.name);
    if (arg != -1) {
        chunk.write(OP_SET_UPVALUE, expr.name.line);
        chunk.write(arg, expr.name.line);
        return;
    }

    int constant = chunk.addConstant(expr.name.lexeme);
    chunk.write(OP_SET_GLOBAL, expr.name.line);
    chunk.write(constant, expr.name.line);
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

int Compiler::resolveUpvalue(Token name) {
    if (enclosing == NULL) return -1;

    int local = enclosing->resolveLocal( name);
    if (local != -1) {
        // std::cout << "resolving upvalue -- found in enclsoing local "  << local << std::endl;
        enclosing->locals[local].isCaptured = true;
        return addUpvalue((uint8_t)local, true);
    }

    int upvalue = enclosing->resolveUpvalue(name);
    if (upvalue != -1) {
        // std::cout << "resolving upvalue -- found in enclsoing upvalue "  << upvalue << std::endl;
        return addUpvalue((uint8_t)upvalue, false);
    }

    return -1;
}

int Compiler::addUpvalue( uint8_t index,bool isLocal) {
    // std::cout << "adding upvalue " << (int)index << " isLocal: " << isLocal;
    int upvalueCount = upvalues.size();
    upvalues.push_back({index, isLocal});
    // std::cout << " upvalue count: " << upvalues.size() << std::endl;
    return upvalueCount;
}

void Compiler::visit(const Call &expr) {
    expr.callee->accept(*this);
    for (const auto &arg : expr.arguments) {
        arg->accept(*this);
    }
    int argCount = expr.arguments.size();
    if (argCount >= 256) {
        throw CompileException("cannot compile >= 256 arguments");
    }
    chunk.write(OP_CALL, expr.paren.line);
    chunk.write(argCount, expr.paren.line);
};

void Compiler::visit(const ArrayLiteral &expr) {
    for (const auto &elem : expr.elements) {
        elem->accept(*this);
    }
    chunk.write(OP_ARRAY_LITERAL, expr.get_line());
    chunk.write(expr.elements.size(), expr.get_line());
};

void Compiler::visit(const MapLiteral &expr) {
    for (const auto &pair : expr.pairs) {
        pair.first->accept(*this);
        pair.second->accept(*this);
    }
    chunk.write(OP_MAP_LITERAL, expr.get_line());
    chunk.write(expr.pairs.size(), expr.get_line());
};

void Compiler::visit(const Subscript &expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);
    chunk.write(OP_SUBSCRIPT, expr.index->get_line());
};

// obj.name := obj["name"]
void Compiler::visit(const PropertyAccess &expr) {
    expr.object->accept(*this);
    int constant = chunk.addConstant(expr.name.lexeme);
    if (constant >= 256) {
        throw CompileException("cannot compile >= 256 constants");
    }
    chunk.write(OP_CONSTANT, expr.name.line);
    chunk.write(constant, expr.name.line);
    chunk.write(OP_SUBSCRIPT, expr.object->get_line());
};

void Compiler::visit(const SubscriptAssignment &expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);
    expr.value->accept(*this);
    chunk.write(OP_SUBSCRIPT_ASSIGNMENT, expr.index->get_line());
};

void Compiler::visit(const FunctionExpr &expr) {
    Compiler functionCompiler{this};
    functionCompiler.beginScope();
    for (const auto &param : expr.params) {
        functionCompiler.locals.push_back({param, functionCompiler.scopeDepth, false});
    }
    auto func = functionCompiler.compileBeatFunction(expr.body, "anon", expr.params.size(), BeatFunctionType::FUNCTION);
    if (disassemble)
        func->chunk.disassembleChunk(std::format("BeatFunc: {}", func->name));
    int constant = chunk.addConstant((LoxCallable*)func);
    // chunk.write(OP_CONSTANT, expr.get_line());
    // chunk.write(constant, expr.get_line());
    chunk.write(OP_CLOSURE, expr.get_line());
    chunk.write(constant, expr.get_line());
    for (int i=0; i<functionCompiler.upvalues.size(); i++) {
        chunk.write(functionCompiler.upvalues[i].isLocal ? 1 : 0, expr.get_line());
        chunk.write(functionCompiler.upvalues[i].index, expr.get_line());
    }
};


// statements
void Compiler::visit(const ExpressionStmt &stmt) {
    stmt.expr->accept(*this);
    chunk.write(OP_POP, stmt.line); // pop the result of the expression
};

void Compiler::visit(const PrintStmt &stmt) {
    stmt.expr->accept(*this);
    chunk.write(OP_PRINT, stmt.expr->get_line()); // FIXME: there is no line info
};

void Compiler::visit(const VarStmt &stmt) {
    if (scopeDepth > 0) {
        for (int i = locals.size() - 1; i >= 0; --i) {
            const auto &local = locals[i];
            if (local.depth != -1 && local.depth < scopeDepth) {
                break;
            }
            if (local.name.lexeme == stmt.name.lexeme) {
                throw CompileException(std::format("L:{} T:IDENTIFIER V:{}: Already a variable with this name in this scope.",
                                                 stmt.name.line,
                                                 stmt.name.lexeme));
            }
        }
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
        // std::cout << "LocalV " << stmt.name.lexeme << " at depth " << scopeDepth << std::endl;
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
    return chunk.bytecodes().size() - 2;
}

void Compiler::patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = chunk.bytecodes().size() - offset - 2;

    if (jump > UINT16_MAX) {
        throw CompileException("Too much code to jump over.");
    }

    chunk.m_bytecodes[offset] = (jump >> 8) & 0xff;
    chunk.m_bytecodes[offset + 1] = jump & 0xff;
}

void Compiler::visit(const IfStmt &stmt) {
    stmt.condition->accept(*this);
    int thenJump = emitJump(OP_JUMP_IF_FALSE, stmt.condition->get_line());
    chunk.write(OP_POP, stmt.condition->get_line());
    stmt.thenBlock->accept(*this);
    int elseJump = emitJump(OP_JUMP, stmt.condition->get_line());
    patchJump(thenJump);
    chunk.write(OP_POP, stmt.condition->get_line());
    if (stmt.elseBlock) {
        stmt.elseBlock->accept(*this);
    }
    patchJump(elseJump);
};

void Compiler::emitLoop(int loopStart) {
    chunk.write(OP_LOOP, 0);

    int offset = chunk.m_bytecodes.size() - loopStart + 2;
    if (offset > UINT16_MAX)
        throw CompileException("Loop body too large.");

    chunk.write((offset >> 8) & 0xff, 0);
    chunk.write(offset & 0xff, 0);
}

void Compiler::visit(const WhileStmt &stmt) {
    int loopStart = chunk.m_bytecodes.size();
    beginLoop(loopStart);

    stmt.condition->accept(*this);
    int exitJump = emitJump(OP_JUMP_IF_FALSE, stmt.condition->get_line());
    chunk.write(OP_POP, stmt.condition->get_line());
    stmt.body->accept(*this);

    if (!loopStack.empty()) {
        for (int jumpLoc : loopStack.back().continueJumps) {
            patchJump(jumpLoc);
        }
        loopStack.back().continueJumps.clear();
    }
    if (stmt.increment) {
        stmt.increment->accept(*this);
        chunk.write(OP_POP, stmt.increment->get_line());
    }
    emitLoop(loopStart);
    patchJump(exitJump);
    chunk.write(OP_POP, stmt.condition->get_line());

    endLoop();  // End loop context and patch break/continue jumps
};

void Compiler::beginLoop(int loopStart) {
    loopStack.push_back(LoopContext{});
    loopStack.back().loopStart = loopStart;
    loopStack.back().numLocals = locals.size();
}

void Compiler::endLoop() {
    if (loopStack.empty()) {
        throw CompileException("endLoop called without beginLoop");
    }
    LoopContext& context = loopStack.back();
    for (int jumpLoc : context.breakJumps) {
        patchJump(jumpLoc);
    }

    // continue: TODO

    loopStack.pop_back();
    // pop locals that are out of scope from stack
}

void Compiler::addBreakJump(int jump) {
    if (loopStack.empty()) {
        throw CompileException("break statement not in loop");
    }
    loopStack.back().breakJumps.push_back(jump);
}

void Compiler::addContinueJump(int jump) {
    if (loopStack.empty()) {
        throw CompileException("continue statement not in loop");
    }
    loopStack.back().continueJumps.push_back(jump);
}

void Compiler::visit(const BreakStmt &stmt) {
    if (!isInLoop()) {
        throw CompileException("break statement not in loop");
    }
    // need to pop the stack of local variables that will immediately
    // be out of scope after the break jump. But how many pops?
    int numPops = (locals.size() - loopStack.back().numLocals);
    for (int i = 0; i < numPops; i++) {
        chunk.write(OP_POP, stmt.kw.line);
    }
    int jump = emitJump(OP_JUMP, stmt.kw.line);
    addBreakJump(jump);
};

void Compiler::visit(const ContinueStmt &stmt) {
    if (!isInLoop()) {
        throw CompileException("continue statement not in loop");
    }
    // Pop locals declared in the loop body that will go out of scope
    int numPops = (locals.size() - loopStack.back().numLocals);
    for (int i = 0; i < numPops; i++) {
        chunk.write(OP_POP, stmt.kw.line);
    }
    // Jump to the loop’s continue target (patched later)
    int jump = emitJump(OP_JUMP, stmt.kw.line);
    addContinueJump(jump);
};

void Compiler::visit(const FunctionStmt &stmt) {
    if (scopeDepth > 0) {
        locals.push_back({stmt.name, -1, false});
    }

    Compiler functionCompiler{this};
    functionCompiler.beginScope();
    for (const auto &param : stmt.params) {
        functionCompiler.locals.push_back({param, functionCompiler.scopeDepth, false});
    }
    auto func = functionCompiler.compileBeatFunction(stmt.body, stmt.name.lexeme, stmt.params.size(), BeatFunctionType::FUNCTION);
    if (disassemble)
        func->chunk.disassembleChunk(std::format("BeatFunc: {}", func->name));

    int constant = chunk.addConstant((LoxCallable*)func);
    // chunk.write(OP_CONSTANT, stmt.name.line);
    // chunk.write(constant, stmt.name.line);
    // chunk.write(OP_POP, stmt.name.line);
    chunk.write(OP_CLOSURE, stmt.name.line);
    chunk.write(constant, stmt.name.line);
    // std::cout << "upvalue count: " << func->upvalueCount << std::endl;
    // std::cout << "upvalues size: " << functionCompiler.upvalues.size() << std::endl;
    for (int i=0; i<functionCompiler.upvalues.size(); i++) {
        chunk.write(functionCompiler.upvalues[i].isLocal ? 1 : 0, stmt.name.line);
        chunk.write(functionCompiler.upvalues[i].index, stmt.name.line);
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

void Compiler::visit(const ReturnStmt &stmt) {
    if (stmt.value)
        stmt.value->accept(*this);
    else
        chunk.write(OP_NIL, stmt.kw.line); // return nil if no value is provided
    chunk.write(OP_RETURN, stmt.kw.line);
};
