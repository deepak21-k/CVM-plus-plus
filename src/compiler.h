#pragma once
#include "ast.h"
#include "opcode.h"
#include <unordered_map>
#include <string>

class Compiler : public ASTVisitor {
public:
    Compiler();
    Chunk compile(const std::vector<std::unique_ptr<Statement>>& statements);

    void visitBinaryExpr(BinaryExpr& expr) override;
    void visitUnaryExpr(UnaryExpr& expr) override;
    void visitLiteralExpr(LiteralExpr& expr) override;
    void visitVariableExpr(VariableExpr& expr) override;
    void visitInputExpr(InputExpr& expr) override;
    void visitAssignExpr(AssignExpr& expr) override;

    void visitBlockStmt(BlockStmt& stmt) override;
    void visitExpressionStmt(ExpressionStmt& stmt) override;
    void visitLetStmt(LetStmt& stmt) override;
    void visitIfStmt(IfStmt& stmt) override;
    void visitWhileStmt(WhileStmt& stmt) override;
    void visitPrintStmt(PrintStmt& stmt) override;

private:
    Chunk chunk;
    std::unordered_map<std::string, int32_t> variables;
    
    int32_t resolveVariable(const std::string& name, bool declare);
    
    int emitJmp(Opcode instruction);
    void patchJmp(int offsetIndex);
    void emitLoop(int loopStart);
};
