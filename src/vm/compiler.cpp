#include "compiler.hpp"

void Compiler::visit(const Binary &expr) {
    expr.left->accept(*this);
    expr.right->accept(*this);
    OpCode opcode;
    switch (expr.op.type) {
        case TokenType::PLUS:
            opcode = OP_ADD;
            break;
        case TokenType::MINUS:
            opcode = OP_SUBTRACT;
            break;
        case TokenType::STAR:
            opcode = OP_MULTIPLY;
            break;
        case TokenType::SLASH:
            opcode = OP_DIVIDE;
            break;
        default:
            throw CompileException("binary op type not in +,-,*,/");
            break;
    }
    chunk.write(opcode, expr.op.line);
}


void Compiler::visit(const Literal &expr) {
    int constant = chunk.addConstant(expr.value);
    if (constant >= 256) {
        throw CompileException("cannot compile >= 256 constants");
    }
    chunk.write(OP_CONSTANT, 0);
    chunk.write(constant, 0);
}


void Compiler::visit(const Logical &) {
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
    // TODO: this is wasteful--should deduplicate strings in globals contants
    int constant = chunk.addConstant(expr.name.lexeme);
    chunk.write(OP_GET_GLOBAL, expr.name.line);
    chunk.write(constant, expr.name.line);
};

void Compiler::visit(const Assignment &expr) {
    expr.right->accept(*this);
    int constant = chunk.addConstant(expr.name.lexeme);
    chunk.write(OP_SET_GLOBAL, expr.name.line);
    chunk.write(constant, expr.name.line);
};

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
    chunk.write(OP_PRINT, 0);
};

void Compiler::visit(const VarStmt &stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        chunk.write(OP_NIL, stmt.name.line);
    }
    int constant = chunk.addConstant(stmt.name.lexeme);
    if (constant >= 256) {
        throw CompileException("cannot compile >= 256 constants");
    }
    chunk.write(OP_DEFINE_GLOBAL, stmt.name.line);
    chunk.write(constant, stmt.name.line);
};

void Compiler::visit(const BlockStmt &) {};

void Compiler::visit(const IfStmt &) {};

void Compiler::visit(const WhileStmt &) {};

void Compiler::visit(const FunctionStmt &) {};

void Compiler::visit(const ReturnStmt &) {};

void Compiler::visit(const BreakStmt &) {};

void Compiler::visit(const ContinueStmt &) {};
