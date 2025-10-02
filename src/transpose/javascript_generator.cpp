#include "transpose/javascript_generator.hpp"

#include <iomanip>
#include <utility>

#include "token.hpp"
#include "transpose/runtime.hpp"

namespace transpose {

JavascriptGenerator::JavascriptGenerator() : current_(&builder_) {}

std::string JavascriptGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    builder_.str("");
    builder_.clear();
    indent_ = 0;
    scopeStack_.clear();
    beginScope(true);
    builder_ << runtimePrelude() << "\n\n";

    const std::vector<std::string> builtins = {
        "clock",      "printf",    "sprintf",  "len",        "push",      "pop",
        "readline",   "split",     "assert",   "for_each",   "tonumber",  "slurp",
        "keys",       "floor",     "ceil",     "sin",        "cos",       "tan",
        "asin",       "acos",      "atan",     "log",        "log10",     "sqrt",
        "exp",        "fabs",      "pow",      "atan2",      "fmod",      "from_json",
        "to_json",    "inf",       "substring", "random_int"};

    for (const auto& name : builtins) {
        builder_ << "let " << name << " = __rt.globals." << name << ";\n";
        declareInCurrentScope(name);
    }
    builder_ << "\n";

    emitLine("try {");
    indent_++;
    for (const auto& stmt : statements) {
        emitStatement(*stmt);
    }
    indent_--;
    emitLine("} catch (err) {");
    indent_++;
    emitLine("if (err && err.__isRhythmError) {");
    indent_++;
    emitLine("console.error(err.message);");
    emitLine("process.exit(1);");
    indent_--;
    emitLine("}");
    emitLine("throw err;");
    indent_--;
    emitLine("}");

    return builder_.str();
}

void JavascriptGenerator::emitLine(const std::string& line) {
    (*current_) << std::string(indent_ * 2, ' ') << line << '\n';
}

void JavascriptGenerator::emitStatement(const Stmt& stmt) {
    stmt.accept(*this);
}

void JavascriptGenerator::emitStatementBody(const Stmt& stmt) {
    indent_++;
    if (const auto* block = dynamic_cast<const BlockStmt*>(&stmt)) {
        beginScope(false);
        for (const auto& inner : block->statements) {
            emitStatement(*inner);
        }
        endScope();
    } else {
        emitStatement(stmt);
    }
    indent_--;
}

std::string JavascriptGenerator::generateExpression(const Expr& expr) {
    expr.accept(*this);
    return std::exchange(exprResult_, std::string{});
}

std::string JavascriptGenerator::renderFunctionBody(const BlockStmt& block, const std::vector<Token>& params) {
    std::ostringstream body;
    auto* previous = current_;
    int previousIndent = indent_;
    size_t previousScopeDepth = scopeStack_.size();

    current_ = &body;
    body << "{\n";
    indent_ = 1;
    beginScope(false);
    for (const auto& param : params) {
        declareInCurrentScope(param.lexeme);
    }
    for (const auto& stmt : block.statements) {
        emitStatement(*stmt);
    }
    endScope();
    indent_ = 0;
    body << "}";

    current_ = previous;
    indent_ = previousIndent;
    if (scopeStack_.size() != previousScopeDepth) {
        scopeStack_.resize(previousScopeDepth);
    }
    return body.str();
}

std::string JavascriptGenerator::escapeString(const std::string& value) const {
    std::string escaped = "\"";
    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '\"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    escaped += '"';
    return escaped;
}

void JavascriptGenerator::beginScope(bool allowRedeclare) {
    scopeStack_.push_back(Scope{allowRedeclare, {}});
}

void JavascriptGenerator::endScope() {
    if (!scopeStack_.empty()) {
        scopeStack_.pop_back();
    }
}

bool JavascriptGenerator::isRedeclarationOfCurrentScope(const std::string& name) const {
    if (scopeStack_.empty()) {
        return false;
    }
    const auto& scope = scopeStack_.back();
    return scope.allowRedeclare && scope.names.contains(name);
}

void JavascriptGenerator::declareInCurrentScope(const std::string& name) {
    if (scopeStack_.empty()) {
        return;
    }
    scopeStack_.back().names.insert(name);
}

void JavascriptGenerator::visit(const Binary& expr) {
    auto left = generateExpression(*expr.left);
    auto right = generateExpression(*expr.right);
    const auto line = std::to_string(expr.op.line);

    switch (expr.op.type) {
        case TokenType::MINUS:
            exprResult_ = "__rt.binaryMinus(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::SLASH:
            exprResult_ = "__rt.binaryDivide(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::STAR:
            exprResult_ = "__rt.binaryMultiply(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::PLUS:
            exprResult_ = "__rt.binaryPlus(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::PERCENT:
            exprResult_ = "__rt.binaryMod(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::GREATER:
            exprResult_ = "__rt.greaterThan(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::GREATER_EQUAL:
            exprResult_ = "__rt.greaterEqual(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::LESS:
            exprResult_ = "__rt.lessThan(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::LESS_EQUAL:
            exprResult_ = "__rt.lessEqual(" + left + ", " + right + ", " + line + ")";
            break;
        case TokenType::BANG_EQUAL:
            exprResult_ = "__rt.notEquals(" + left + ", " + right + ")";
            break;
        case TokenType::EQUAL_EQUAL:
            exprResult_ = "__rt.equals(" + left + ", " + right + ")";
            break;
        default:
            exprResult_ = "null";
            break;
    }
}

void JavascriptGenerator::visit(const Logical& expr) {
    auto left = generateExpression(*expr.left);
    auto right = generateExpression(*expr.right);
    if (expr.op.type == TokenType::AND) {
        exprResult_ = "__rt.logicalAnd(() => " + left + ", () => " + right + ")";
    } else {
        exprResult_ = "__rt.logicalOr(() => " + left + ", () => " + right + ")";
    }
}

void JavascriptGenerator::visit(const Grouping& expr) {
    exprResult_ = "(" + generateExpression(*expr.expression) + ")";
}

void JavascriptGenerator::visit(const Literal& expr) {
    if (std::holds_alternative<double>(expr.value)) {
        std::ostringstream oss;
        oss << std::setprecision(17) << std::get<double>(expr.value);
        exprResult_ = oss.str();
        return;
    }
    if (std::holds_alternative<std::string>(expr.value)) {
        exprResult_ = escapeString(std::get<std::string>(expr.value));
        return;
    }
    if (std::holds_alternative<std::nullptr_t>(expr.value)) {
        exprResult_ = "null";
        return;
    }
    if (std::holds_alternative<bool>(expr.value)) {
        exprResult_ = std::get<bool>(expr.value) ? "true" : "false";
        return;
    }
    exprResult_ = "null";
}

void JavascriptGenerator::visit(const Unary& expr) {
    auto right = generateExpression(*expr.right);
    const auto line = std::to_string(expr.op.line);
    if (expr.op.type == TokenType::MINUS) {
        exprResult_ = "__rt.unaryMinus(" + right + ", " + line + ")";
    } else {
        exprResult_ = "__rt.unaryNot(" + right + ")";
    }
}

void JavascriptGenerator::visit(const Variable& expr) {
    exprResult_ = expr.name.lexeme;
}

void JavascriptGenerator::visit(const Assignment& expr) {
    auto value = generateExpression(*expr.right);
    exprResult_ = "(" + expr.name.lexeme + " = " + value + ")";
}

void JavascriptGenerator::visit(const SubscriptAssignment& expr) {
    auto object = generateExpression(*expr.object);
    auto index = generateExpression(*expr.index);
    auto value = generateExpression(*expr.value);
    exprResult_ = "__rt.setIndex(" + object + ", " + index + ", " + value + ", " + std::to_string(expr.bracket.line) + ")";
}

void JavascriptGenerator::visit(const Call& expr) {
    auto callee = generateExpression(*expr.callee);
    std::vector<std::string> args;
    args.reserve(expr.arguments.size());
    for (const auto& argument : expr.arguments) {
        args.push_back(generateExpression(*argument));
    }
    std::string joined;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) joined += ", ";
        joined += args[i];
    }
    exprResult_ = "__rt.callFunction(" + callee + ", [" + joined + "], " + std::to_string(expr.paren.line) + ")";
}

void JavascriptGenerator::visit(const ArrayLiteral& expr) {
    std::vector<std::string> elements;
    elements.reserve(expr.elements.size());
    for (const auto& element : expr.elements) {
        elements.push_back(generateExpression(*element));
    }
    std::string joined;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) joined += ", ";
        joined += elements[i];
    }
    exprResult_ = "__rt.makeArray([" + joined + "])";
}

void JavascriptGenerator::visit(const MapLiteral& expr) {
    std::vector<std::string> pairs;
    pairs.reserve(expr.pairs.size());
    for (const auto& pair : expr.pairs) {
        auto key = generateExpression(*pair.first);
        auto value = generateExpression(*pair.second);
        pairs.push_back("[" + key + ", " + value + "]");
    }
    std::string joined;
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) joined += ", ";
        joined += pairs[i];
    }
    exprResult_ = "__rt.makeMap([" + joined + "])";
}

void JavascriptGenerator::visit(const Subscript& expr) {
    auto object = generateExpression(*expr.object);
    auto index = generateExpression(*expr.index);
    exprResult_ = "__rt.getIndex(" + object + ", " + index + ", " + std::to_string(expr.bracket.line) + ")";
}

void JavascriptGenerator::visit(const PropertyAccess& expr) {
    auto object = generateExpression(*expr.object);
    exprResult_ = "__rt.getProperty(" + object + ", " + escapeString(expr.name.lexeme) + ", " + std::to_string(expr.name.line) + ")";
}

void JavascriptGenerator::visit(const FunctionExpr& expr) {
    std::string params;
    for (size_t i = 0; i < expr.params.size(); ++i) {
        if (i != 0) params += ", ";
        params += expr.params[i].lexeme;
    }
    auto body = renderFunctionBody(*expr.body, expr.params);
    exprResult_ = "__rt.makeAnonFunction(function(" + params + ") " + body + ", " + std::to_string(expr.params.size()) + ")";
}

void JavascriptGenerator::visit(const ExpressionStmt& stmt) {
    emitLine(generateExpression(*stmt.expr) + ";");
}

void JavascriptGenerator::visit(const PrintStmt& stmt) {
    emitLine("__rt.print(" + generateExpression(*stmt.expr) + ");");
}

void JavascriptGenerator::visit(const VarStmt& stmt) {
    std::string value = "null";
    if (stmt.initializer) {
        value = generateExpression(*stmt.initializer);
    }
    bool redeclaration = isRedeclarationOfCurrentScope(stmt.name.lexeme);
    declareInCurrentScope(stmt.name.lexeme);
    if (redeclaration) {
        emitLine(stmt.name.lexeme + " = " + value + ";");
    } else {
        emitLine("let " + stmt.name.lexeme + " = " + value + ";");
    }
}

void JavascriptGenerator::visit(const BlockStmt& stmt) {
    emitLine("{");
    indent_++;
    beginScope(false);
    for (const auto& inner : stmt.statements) {
        emitStatement(*inner);
    }
    endScope();
    indent_--;
    emitLine("}");
}

void JavascriptGenerator::visit(const IfStmt& stmt) {
    auto condition = "__rt.isTruthy(" + generateExpression(*stmt.condition) + ")";
    emitLine("if (" + condition + ") {");
    emitStatementBody(*stmt.thenBlock);
    emitLine("}");
    if (stmt.elseBlock) {
        emitLine("else {");
        emitStatementBody(*stmt.elseBlock);
        emitLine("}");
    }
}

void JavascriptGenerator::visit(const WhileStmt& stmt) {
    auto condition = "__rt.isTruthy(" + generateExpression(*stmt.condition) + ")";
    if (stmt.increment) {
        auto increment = generateExpression(*stmt.increment);
        emitLine("for (; " + condition + "; " + increment + ") {");
        emitStatementBody(*stmt.body);
        emitLine("}");
    } else {
        emitLine("while (" + condition + ") {");
        emitStatementBody(*stmt.body);
        emitLine("}");
    }
}

void JavascriptGenerator::visit(const FunctionStmt& stmt) {
    std::string params;
    for (size_t i = 0; i < stmt.params.size(); ++i) {
        if (i != 0) params += ", ";
        params += stmt.params[i].lexeme;
    }
    auto body = renderFunctionBody(*stmt.body, stmt.params);
    bool redeclaration = isRedeclarationOfCurrentScope(stmt.name.lexeme);
    declareInCurrentScope(stmt.name.lexeme);
    std::string rhs = "__rt.makeFunction(" + escapeString(stmt.name.lexeme) + ", function " + stmt.name.lexeme + "(" + params + ") " + body + ", " + std::to_string(stmt.params.size()) + ")";
    if (redeclaration) {
        emitLine(stmt.name.lexeme + " = " + rhs + ";");
    } else {
        emitLine("let " + stmt.name.lexeme + " = " + rhs + ";");
    }
}

void JavascriptGenerator::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        emitLine("return " + generateExpression(*stmt.value) + ";");
    } else {
        emitLine("return null;");
    }
}

void JavascriptGenerator::visit(const BreakStmt&) {
    emitLine("break;");
}

void JavascriptGenerator::visit(const ContinueStmt&) {
    emitLine("continue;");
}

}  // namespace transpose
