#pragma once
#include "token.h"
#include <memory>
#include <vector>

class ASTVisitor;

enum class NodeType {
    // Expressions
    BinaryExpr, UnaryExpr, LogicalExpr, LiteralExpr,
    VariableExpr, InputExpr, AssignExpr, UpdateExpr,
    // Statements
    BlockStmt, ExpressionStmt, LetStmt, IfStmt,
    WhileStmt, ForStmt, BreakStmt, ContinueStmt, PrintStmt
};

struct ASTNode {
    const NodeType nodeType;
    explicit ASTNode(NodeType type) : nodeType(type) {}
    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;
};

struct Expression : public ASTNode {
    using ASTNode::ASTNode;
};
struct Statement : public ASTNode {
    using ASTNode::ASTNode;
};

struct BinaryExpr : public Expression {
    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
    
    BinaryExpr(std::unique_ptr<Expression> left_, Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::BinaryExpr), left(std::move(left_)), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct UnaryExpr : public Expression {
    Token op;
    std::unique_ptr<Expression> right;

    UnaryExpr(Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::UnaryExpr), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LogicalExpr : public Expression {
    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;

    LogicalExpr(std::unique_ptr<Expression> left_, Token op_, std::unique_ptr<Expression> right_)
        : Expression(NodeType::LogicalExpr), left(std::move(left_)), op(std::move(op_)), right(std::move(right_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LiteralExpr : public Expression {
    Token value;

    explicit LiteralExpr(Token value_) : Expression(NodeType::LiteralExpr), value(std::move(value_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct VariableExpr : public Expression {
    Token name;

    explicit VariableExpr(Token name_) : Expression(NodeType::VariableExpr), name(std::move(name_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct InputExpr : public Expression {
    InputExpr() : Expression(NodeType::InputExpr) {}
    void accept(ASTVisitor& visitor) override;
};

struct AssignExpr : public Expression {
    Token name;
    std::unique_ptr<Expression> value;

    AssignExpr(Token name_, std::unique_ptr<Expression> value_)
        : Expression(NodeType::AssignExpr), name(std::move(name_)), value(std::move(value_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct UpdateExpr : public Expression {
    Token name;
    bool isIncrement;
    bool isPrefix;

    UpdateExpr(Token name_, bool isIncrement_, bool isPrefix_)
        : Expression(NodeType::UpdateExpr), name(std::move(name_)), isIncrement(isIncrement_), isPrefix(isPrefix_) {}
    void accept(ASTVisitor& visitor) override;
};

struct BlockStmt : public Statement {
    std::vector<std::unique_ptr<Statement>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Statement>> statements_)
        : Statement(NodeType::BlockStmt), statements(std::move(statements_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ExpressionStmt : public Statement {
    std::unique_ptr<Expression> expr;

    explicit ExpressionStmt(std::unique_ptr<Expression> expr_) : Statement(NodeType::ExpressionStmt), expr(std::move(expr_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct LetStmt : public Statement {
    Token name;
    std::unique_ptr<Expression> initializer;

    LetStmt(Token name_, std::unique_ptr<Expression> initializer_)
        : Statement(NodeType::LetStmt), name(std::move(name_)), initializer(std::move(initializer_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct IfStmt : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch;

    IfStmt(std::unique_ptr<Expression> condition_, std::unique_ptr<Statement> thenBranch_, std::unique_ptr<Statement> elseBranch_)
        : Statement(NodeType::IfStmt), condition(std::move(condition_)), thenBranch(std::move(thenBranch_)), elseBranch(std::move(elseBranch_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct WhileStmt : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStmt(std::unique_ptr<Expression> condition_, std::unique_ptr<Statement> body_)
        : Statement(NodeType::WhileStmt), condition(std::move(condition_)), body(std::move(body_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct ForStmt : public Statement {
    // C-like: for (initializer ; condition ; increment) body
    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> increment;
    std::unique_ptr<Statement> body;

    ForStmt(std::unique_ptr<Statement> initializer_,
            std::unique_ptr<Expression> condition_,
            std::unique_ptr<Expression> increment_,
            std::unique_ptr<Statement> body_)
        : Statement(NodeType::ForStmt),
          initializer(std::move(initializer_)),
          condition(std::move(condition_)),
          increment(std::move(increment_)),
          body(std::move(body_)) {}
    void accept(ASTVisitor& visitor) override;
};

struct BreakStmt : public Statement {
    BreakStmt() : Statement(NodeType::BreakStmt) {}
    void accept(ASTVisitor& visitor) override;
};

struct ContinueStmt : public Statement {
    ContinueStmt() : Statement(NodeType::ContinueStmt) {}
    void accept(ASTVisitor& visitor) override;
};

struct PrintStmt : public Statement {
    std::unique_ptr<Expression> expr;

    explicit PrintStmt(std::unique_ptr<Expression> expr_) : Statement(NodeType::PrintStmt), expr(std::move(expr_)) {}
    void accept(ASTVisitor& visitor) override;
};

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visitBinaryExpr(BinaryExpr& expr) = 0;
    virtual void visitUnaryExpr(UnaryExpr& expr) = 0;
    virtual void visitLiteralExpr(LiteralExpr& expr) = 0;
    virtual void visitLogicalExpr(LogicalExpr& expr) = 0;
    virtual void visitVariableExpr(VariableExpr& expr) = 0;
    virtual void visitInputExpr(InputExpr& expr) = 0;
    virtual void visitAssignExpr(AssignExpr& expr) = 0;
    virtual void visitUpdateExpr(UpdateExpr& expr) = 0;

    virtual void visitBlockStmt(BlockStmt& stmt) = 0;
    virtual void visitExpressionStmt(ExpressionStmt& stmt) = 0;
    virtual void visitLetStmt(LetStmt& stmt) = 0;
    virtual void visitIfStmt(IfStmt& stmt) = 0;
    virtual void visitWhileStmt(WhileStmt& stmt) = 0;
    virtual void visitForStmt(ForStmt& stmt) = 0;
    virtual void visitBreakStmt(BreakStmt& stmt) = 0;
    virtual void visitContinueStmt(ContinueStmt& stmt) = 0;
    virtual void visitPrintStmt(PrintStmt& stmt) = 0;
};
