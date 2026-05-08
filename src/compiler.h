#pragma once
#include "ast.h"
#include "opcode.h"
#include <string>
#include <vector>

struct Local {
    std::string name;
    int depth;
    int32_t id;
};

class Compiler : public ASTVisitor {
public:
    Compiler();
    Chunk compile(const std::vector<std::unique_ptr<Statement>>& statements);

    void visitBinaryExpr(BinaryExpr& expr) override;
    void visitUnaryExpr(UnaryExpr& expr) override;
    void visitLiteralExpr(LiteralExpr& expr) override;
    void visitLogicalExpr(LogicalExpr& expr) override;
    void visitVariableExpr(VariableExpr& expr) override;
    void visitInputExpr(InputExpr& expr) override;
    void visitAssignExpr(AssignExpr& expr) override;

    void visitBlockStmt(BlockStmt& stmt) override;
    void visitExpressionStmt(ExpressionStmt& stmt) override;
    void visitLetStmt(LetStmt& stmt) override;
    void visitIfStmt(IfStmt& stmt) override;
    void visitWhileStmt(WhileStmt& stmt) override;
    void visitForStmt(ForStmt& stmt) override;
    void visitBreakStmt(BreakStmt& stmt) override;
    void visitContinueStmt(ContinueStmt& stmt) override;
    void visitPrintStmt(PrintStmt& stmt) override;

private:
    Chunk chunk;
    std::vector<Local> locals;
    int variablesCount;
    std::vector<int32_t> freeIds;
    int currentDepth;
    
    int32_t resolveVariable(const std::string& name, bool declare);
    void beginScope();
    void endScope();
    
    int emitJmp(Opcode instruction);
    void patchJmp(int offsetIndex);
    void emitLoop(int loopStart);

    struct LoopContext {
        int loopStart = 0;                 // for backward continue (while)
        bool continuePatchesToIncrement = false; // for forward continue (for)
        std::vector<int> breakJumps;       // operand indices to patch to loop end
        std::vector<int> continueJumps;    // operand indices to patch to increment start
    };
    std::vector<LoopContext> loopStack;
};
