#include <unordered_map>
#include <vector>

#include "expr.hpp"
#include "statement.hpp"


// Resolving pass after AST parser pass;
// walk the tree and determine static references of each variables
// specifically, how many hops to go to enclosing env from current
// env to find the declaration env?
class Resolver: ExprVisitor, StmtVisitor {
private:
    struct VarInfo {
        bool defined;
        int index;
    };
    struct Scope {
        std::unordered_map<std::string, VarInfo> variables;
        int nextIndex = 0; // Each scope tracks its own next available index
    };

    Interpreter* interpreter;
    std::vector<Scope> scopes; // used as stack
    FunctionType current_function = FunctionType::NONE;

    void resolve(Expr* expr);
    void resolve(Stmt* stmt);

    void beginScope();
    void endScope();
    void declare(Token name);
    void define(Token name);
    void resolveLocal(const Expr& expr, const Token& name);
    void resolveFunction(const FunctionStmt&, FunctionType);
    void resolveFunction(const FunctionExpr&, FunctionType);


public:
    explicit Resolver(Interpreter* interpreter) : interpreter(interpreter) {}
    void resolve(const std::vector<std::unique_ptr<Stmt>>&);


    void visit(const VarStmt&) override;
    void visit(const Variable&) override;
    void visit(const Assignment&) override;
    void visit(const SubscriptAssignment&) override;
    void visit(const Binary&) override;
    void visit(const Call&) override;
    void visit(const ArrayLiteral&) override;
    void visit(const MapLiteral& mlit) override;
    void visit(const Subscript&) override;
    void visit(const Grouping&) override;
    void visit(const Literal&) override;
    void visit(const Logical&) override;
    void visit(const Unary&) override;
    void visit(const PropertyAccess&) override;
    void visit(const FunctionExpr&) override;


    void visit(const BlockStmt&) override;
    void visit(const FunctionStmt&) override;
    void visit(const ExpressionStmt&) override;
    void visit(const IfStmt&) override;
    void visit(const PrintStmt&) override;
    void visit(const ReturnStmt&) override;
    void visit(const WhileStmt&) override;
    void visit(const BreakStmt&) override;
    void visit(const ContinueStmt&) override;
};
