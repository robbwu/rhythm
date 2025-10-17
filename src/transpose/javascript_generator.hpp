#pragma once

#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "expr.hpp"
#include "statement.hpp"

namespace transpose {

class JavascriptGenerator : public ExprVisitor, public StmtVisitor {
public:
    JavascriptGenerator();
    std::string generate(const std::vector<std::unique_ptr<Stmt>>& statements);
    std::string generateUserCodeOnly(const std::vector<std::unique_ptr<Stmt>>& statements, size_t skipCoreLibStatements);

private:
    std::ostringstream builder_;
    std::ostringstream* current_;
    int indent_ = 0;
    std::string exprResult_;
    struct Scope {
        bool allowRedeclare;
        std::unordered_set<std::string> names;
    };
    std::vector<Scope> scopeStack_;

    void emitLine(const std::string& line);
    void emitStatement(const Stmt& stmt);
    void emitStatementBody(const Stmt& stmt);
    std::string generateExpression(const Expr& expr);
    std::string renderFunctionBody(const BlockStmt& block, const std::vector<Token>& params);
    std::string escapeString(const std::string& value) const;
    void beginScope(bool allowRedeclare);
    void endScope();
    bool isRedeclarationOfCurrentScope(const std::string& name) const;
    void declareInCurrentScope(const std::string& name);

    // ExprVisitor overrides
    void visit(const Binary& expr) override;
    void visit(const Logical& expr) override;
    void visit(const Ternary& expr) override;
    void visit(const Grouping& expr) override;
    void visit(const Literal& expr) override;
    void visit(const Unary& expr) override;
    void visit(const Variable& expr) override;
    void visit(const Assignment& expr) override;
    void visit(const SubscriptAssignment& expr) override;
    void visit(const Call& expr) override;
    void visit(const ArrayLiteral& expr) override;
    void visit(const MapLiteral& expr) override;
    void visit(const Subscript& expr) override;
    void visit(const PropertyAccess& expr) override;
    void visit(const FunctionExpr& expr) override;

    // StmtVisitor overrides
    void visit(const ExpressionStmt& stmt) override;
    void visit(const PrintStmt& stmt) override;
    void visit(const VarStmt& stmt) override;
    void visit(const BlockStmt& stmt) override;
    void visit(const IfStmt& stmt) override;
    void visit(const WhileStmt& stmt) override;
    void visit(const FunctionStmt& stmt) override;
    void visit(const ReturnStmt& stmt) override;
    void visit(const BreakStmt& stmt) override;
    void visit(const ContinueStmt& stmt) override;
};

}  // namespace transpose
