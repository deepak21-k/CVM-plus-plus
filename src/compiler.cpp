#include "compiler.h"
#include "vm.h"
#include <optional>
#include <stdexcept>

Compiler::Compiler() : variablesCount(0), currentDepth(0) {}

Chunk Compiler::compile(const std::vector<std::unique_ptr<Statement>>& statements) {
    chunk = Chunk();
    chunk.code.reserve(1024);
    // Do not clear variables here; REPL relies on persisting local variables table.
    for (const auto& stmt : statements) {
        stmt->accept(*this);
    }
    chunk.write(static_cast<uint8_t>(Opcode::HALT));
    return chunk;
}

int32_t Compiler::resolveVariable(const std::string& name, bool declare) {
    if (declare) {
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->depth < currentDepth) break;
            if (it->name == name) throw std::runtime_error("Variable '" + name + "' already declared in this scope.");
        }
        // Needs declaring
        int32_t id;
        if (!freeIds.empty()) {
            id = freeIds.back();
            freeIds.pop_back();
        } else {
            id = variablesCount++;
            if (id >= VM::MAX_VARIABLES) {
                throw std::runtime_error("Compile error: Variable limit exceeded (max " + std::to_string(VM::MAX_VARIABLES) + ")");
            }
        }
        locals.push_back({name, currentDepth, id});
        return id;
    }
    for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
        if (it->name == name) return it->id;
    }
    throw std::runtime_error("Undeclared variable '" + name + "'");
}

void Compiler::beginScope() {
    currentDepth++;
}

void Compiler::endScope() {
    currentDepth--;
    while (!locals.empty() && locals.back().depth > currentDepth) {
        freeIds.push_back(locals.back().id);
        locals.pop_back();
    }
}

int Compiler::emitJmp(Opcode instruction) {
    chunk.write(static_cast<uint8_t>(instruction));
    chunk.writeInt(0); 
    return chunk.code.size() - 4;
}

void Compiler::patchJmp(int offsetIndex) {
    int32_t jump = chunk.code.size() - offsetIndex - 4;
    chunk.code[offsetIndex] = (jump >> 24) & 0xFF;
    chunk.code[offsetIndex + 1] = (jump >> 16) & 0xFF;
    chunk.code[offsetIndex + 2] = (jump >> 8) & 0xFF;
    chunk.code[offsetIndex + 3] = jump & 0xFF;
}

void Compiler::emitLoop(int loopStart) {
    chunk.write(static_cast<uint8_t>(Opcode::JMP));
    // After the VM reads the 4-byte operand, ip will be at (chunk.code.size() + 4).
    // We need to jump back to loopStart, so offset = loopStart - (here + 4).
    int32_t offset = chunk.code.size() - loopStart + 4;
    chunk.writeInt(-offset);
}

void Compiler::visitBinaryExpr(BinaryExpr& expr) {
    // Attempt constant folding for two literal operands
    if (expr.left->nodeType == NodeType::LiteralExpr && expr.right->nodeType == NodeType::LiteralExpr) {
        auto* leftLit = static_cast<LiteralExpr*>(expr.left.get());
        auto* rightLit = static_cast<LiteralExpr*>(expr.right.get());
        if (leftLit->value.type == TokenType::NUMBER && rightLit->value.type == TokenType::NUMBER) {
            auto tryFold = [&]() -> std::optional<int64_t> {
                int32_t a = std::stoi(leftLit->value.value);
                int32_t b = std::stoi(rightLit->value.value);
                switch (expr.op.type) {
                    case TokenType::PLUS:  return static_cast<int64_t>(a) + static_cast<int64_t>(b);
                    case TokenType::MINUS: return static_cast<int64_t>(a) - static_cast<int64_t>(b);
                    case TokenType::STAR:  return static_cast<int64_t>(a) * static_cast<int64_t>(b);
                    case TokenType::SLASH:
                        if (b == 0) throw std::runtime_error("Division by zero in constant expression");
                        if (a == INT32_MIN && b == -1) throw std::runtime_error("Integer overflow in constant division");
                        return static_cast<int64_t>(a) / b;
                    case TokenType::MOD:
                        if (b == 0) throw std::runtime_error("Modulo by zero in constant expression");
                        if (a == INT32_MIN && b == -1) throw std::runtime_error("Integer overflow in constant modulo");
                        return static_cast<int64_t>(a) % b;
                    case TokenType::BIT_XOR: return static_cast<int64_t>(a ^ b);
                    case TokenType::BIT_AND: return static_cast<int64_t>(a & b);
                    case TokenType::BIT_OR:  return static_cast<int64_t>(a | b);
                    case TokenType::SHL:
                        if (b < 0 || b >= 32) throw std::runtime_error("Invalid shift amount");
                        return static_cast<int64_t>(static_cast<int32_t>(static_cast<uint32_t>(a) << b));
                    case TokenType::SHR: {
                        if (b < 0 || b >= 32) throw std::runtime_error("Invalid shift amount");
                        uint32_t ua = static_cast<uint32_t>(a);
                        uint32_t shifted = ua >> b;
                        if (a < 0 && b > 0) shifted |= ~(UINT32_MAX >> b);
                        return static_cast<int64_t>(static_cast<int32_t>(shifted));
                    }
                    case TokenType::EQ_EQ:     return (a == b) ? 1LL : 0LL;
                    case TokenType::BANG_EQ:    return (a != b) ? 1LL : 0LL;
                    case TokenType::LESS:       return (a <  b) ? 1LL : 0LL;
                    case TokenType::LESS_EQ:    return (a <= b) ? 1LL : 0LL;
                    case TokenType::GREATER:    return (a >  b) ? 1LL : 0LL;
                    case TokenType::GREATER_EQ: return (a >= b) ? 1LL : 0LL;
                    default: return std::nullopt;
                }
            };
            if (auto result = tryFold()) {
                if (*result > INT32_MAX || *result < INT32_MIN)
                    throw std::runtime_error("Integer overflow in constant expression");
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(static_cast<int32_t>(*result));
                return;
            }
        }
    }

    // Runtime evaluation
    expr.left->accept(*this);
    expr.right->accept(*this);

    switch (expr.op.type) {
        case TokenType::PLUS: chunk.write(static_cast<uint8_t>(Opcode::ADD)); break;
        case TokenType::MINUS: chunk.write(static_cast<uint8_t>(Opcode::SUB)); break;
        case TokenType::STAR: chunk.write(static_cast<uint8_t>(Opcode::MUL)); break;
        case TokenType::SLASH: chunk.write(static_cast<uint8_t>(Opcode::DIV)); break;
        case TokenType::MOD: chunk.write(static_cast<uint8_t>(Opcode::MOD)); break;
        case TokenType::EQ_EQ: chunk.write(static_cast<uint8_t>(Opcode::EQ)); break;
        case TokenType::BANG_EQ: chunk.write(static_cast<uint8_t>(Opcode::NEQ)); break;
        case TokenType::LESS: chunk.write(static_cast<uint8_t>(Opcode::LT)); break;
        case TokenType::LESS_EQ: chunk.write(static_cast<uint8_t>(Opcode::LTE)); break;
        case TokenType::GREATER: chunk.write(static_cast<uint8_t>(Opcode::GT)); break;
        case TokenType::GREATER_EQ: chunk.write(static_cast<uint8_t>(Opcode::GTE)); break;
        case TokenType::BIT_XOR: chunk.write(static_cast<uint8_t>(Opcode::BIT_XOR)); break;
        case TokenType::BIT_AND: chunk.write(static_cast<uint8_t>(Opcode::BIT_AND)); break;
        case TokenType::SHL: chunk.write(static_cast<uint8_t>(Opcode::SHL)); break;
        case TokenType::SHR: chunk.write(static_cast<uint8_t>(Opcode::SHR)); break;
        case TokenType::BIT_OR: chunk.write(static_cast<uint8_t>(Opcode::BIT_OR)); break;
        default: throw std::runtime_error("Unknown binary operator");
    }
}

void Compiler::visitLogicalExpr(LogicalExpr& expr) {
    // --- O2: Constant folding for logical expressions with literal operands ---
    if (expr.left->nodeType == NodeType::LiteralExpr && expr.right->nodeType == NodeType::LiteralExpr) {
        auto* leftLit = static_cast<LiteralExpr*>(expr.left.get());
        auto* rightLit = static_cast<LiteralExpr*>(expr.right.get());
        auto toBool = [](const LiteralExpr* lit) -> int {
            if (lit->value.type == TokenType::TRUE_LIT) return 1;
            if (lit->value.type == TokenType::FALSE_LIT) return 0;
            if (lit->value.type == TokenType::NUMBER) {
                try { return std::stoi(lit->value.value) != 0 ? 1 : 0; }
                catch (...) { return -1; }
            }
            return -1;
        };
        int lv = toBool(leftLit);
        int rv = toBool(rightLit);
        if (lv >= 0 && rv >= 0) {
            int result;
            if (expr.op.type == TokenType::AND_AND) {
                result = (lv && rv) ? 1 : 0;
            } else {
                result = (lv || rv) ? 1 : 0;
            }
            chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
            chunk.writeInt(result);
            return;
        }
    }

    expr.left->accept(*this);
    
    if (expr.op.type == TokenType::AND_AND) {
        int endJump = emitJmp(Opcode::JMP_IF_FALSE);
        
        expr.right->accept(*this);
        chunk.write(static_cast<uint8_t>(Opcode::NORMALIZE));
        int skipJump = emitJmp(Opcode::JMP);
        
        patchJmp(endJump);
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        chunk.writeInt(0);
        
        patchJmp(skipJump);
    } else if (expr.op.type == TokenType::OR_OR) {
        int elseJump = emitJmp(Opcode::JMP_IF_FALSE);
        
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        chunk.writeInt(1); // pushes true 
        int endJump = emitJmp(Opcode::JMP);
        
        patchJmp(elseJump);
        expr.right->accept(*this);
        chunk.write(static_cast<uint8_t>(Opcode::NORMALIZE));
        
        patchJmp(endJump);
    } else {
        throw std::runtime_error("Unknown logical operator");
    }
}

void Compiler::visitUnaryExpr(UnaryExpr& expr) {
    // --- Constant folding for literal operands ---
    if (expr.right->nodeType == NodeType::LiteralExpr) {
        auto* lit = static_cast<LiteralExpr*>(expr.right.get());
        if (lit->value.type == TokenType::NUMBER) {
            long long val;
            try { val = std::stoll(lit->value.value); }
            catch (...) { goto runtime; }

            if (expr.op.type == TokenType::MINUS) {
                long long result = -val;
                if (result > INT32_MAX || result < INT32_MIN)
                    throw std::runtime_error("Integer overflow in constant negation");
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(static_cast<int32_t>(result));
                return;
            } else if (expr.op.type == TokenType::BIT_NOT) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw std::runtime_error("Integer literal out of range");
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(~static_cast<int32_t>(val));
                return;
            } else if (expr.op.type == TokenType::NOT) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw std::runtime_error("Integer literal out of range");
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(val == 0 ? 1 : 0);
                return;
            } else if (expr.op.type == TokenType::PLUS) {
                if (val > INT32_MAX || val < INT32_MIN)
                    throw std::runtime_error("Integer literal out of range");
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(static_cast<int32_t>(val));
                return;
            }
        }
        if (lit->value.type == TokenType::TRUE_LIT || lit->value.type == TokenType::FALSE_LIT) {
            int32_t boolVal = (lit->value.type == TokenType::TRUE_LIT) ? 1 : 0;
            if (expr.op.type == TokenType::NOT) {
                chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
                chunk.writeInt(boolVal == 0 ? 1 : 0);
                return;
            }
        }
    }
    // --- Runtime evaluation for non-literal operands ---
runtime:
    expr.right->accept(*this);
    if (expr.op.type == TokenType::BIT_NOT) {
        chunk.write(static_cast<uint8_t>(Opcode::BIT_NOT));
    } else if (expr.op.type == TokenType::NOT) {
        chunk.write(static_cast<uint8_t>(Opcode::NOT));
    } else if (expr.op.type == TokenType::MINUS) {
        chunk.write(static_cast<uint8_t>(Opcode::NEG));
    } else if (expr.op.type == TokenType::PLUS) {
        // Unary plus is a no-op — value is already on the stack
    } else {
        throw std::runtime_error("Unknown unary operator");
    }
}

void Compiler::visitLiteralExpr(LiteralExpr& expr) {
    if (expr.value.type == TokenType::NUMBER) {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        try {
            long long val = std::stoll(expr.value.value);
            if (val > INT32_MAX || val < INT32_MIN) {
                throw std::runtime_error("Integer literal out of range: " + expr.value.value);
            }
            chunk.writeInt(static_cast<int32_t>(val));
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Integer literal out of range: " + expr.value.value);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Invalid integer literal: " + expr.value.value);
        }
    } else if (expr.value.type == TokenType::TRUE_LIT) {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_BOOL));
        chunk.write(1);
    } else if (expr.value.type == TokenType::FALSE_LIT) {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_BOOL));
        chunk.write(0);
    } else {
        throw std::runtime_error("Unknown literal type");
    }
}

void Compiler::visitVariableExpr(VariableExpr& expr) {
    int32_t id = resolveVariable(expr.name.value, false);
    chunk.write(static_cast<uint8_t>(Opcode::GET_VAR));
    chunk.writeInt(id);
}

void Compiler::visitInputExpr(InputExpr&) {
    chunk.write(static_cast<uint8_t>(Opcode::INPUT));
}

void Compiler::visitAssignExpr(AssignExpr& expr) {
    expr.value->accept(*this);
    int32_t id = resolveVariable(expr.name.value, false);
    chunk.write(static_cast<uint8_t>(Opcode::SET_VAR_PUSH));
    chunk.writeInt(id);
}

void Compiler::visitUpdateExpr(UpdateExpr& expr) {
    int32_t id = resolveVariable(expr.name.value, false);
    if (expr.isPrefix) {
        // Prefix (++x / --x): update variable, leave NEW value on stack.
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR));
        chunk.writeInt(id);
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        chunk.writeInt(1);
        chunk.write(static_cast<uint8_t>(expr.isIncrement ? Opcode::ADD : Opcode::SUB));
        chunk.write(static_cast<uint8_t>(Opcode::SET_VAR_PUSH));
        chunk.writeInt(id);
    } else {
        // Postfix (x++ / x--): leave OLD value on stack, then update variable.
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR));
        chunk.writeInt(id); // old value (result)
        chunk.write(static_cast<uint8_t>(Opcode::GET_VAR));
        chunk.writeInt(id); // old value (for computation)
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        chunk.writeInt(1);
        chunk.write(static_cast<uint8_t>(expr.isIncrement ? Opcode::ADD : Opcode::SUB));
        chunk.write(static_cast<uint8_t>(Opcode::SET_VAR));
        chunk.writeInt(id);
    }
}

void Compiler::visitBlockStmt(BlockStmt& stmt) {
    beginScope();
    for (const auto& s : stmt.statements) {
        s->accept(*this);
    }
    endScope();
}

void Compiler::visitExpressionStmt(ExpressionStmt& stmt) {
    // O3 simplified: skip compilation for literal-only expression statements
    // (no side effects, result is immediately discarded)
    if (stmt.expr->nodeType == NodeType::LiteralExpr) return;
    stmt.expr->accept(*this);
    chunk.write(static_cast<uint8_t>(Opcode::POP)); 
}

void Compiler::visitLetStmt(LetStmt& stmt) {
    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    } else {
        chunk.write(static_cast<uint8_t>(Opcode::PUSH_INT));
        chunk.writeInt(0);
    }
    int32_t id = resolveVariable(stmt.name.value, true);
    chunk.write(static_cast<uint8_t>(Opcode::SET_VAR));
    chunk.writeInt(id);
}

void Compiler::visitIfStmt(IfStmt& stmt) {
    stmt.condition->accept(*this);
    int thenJmp = emitJmp(Opcode::JMP_IF_FALSE);
    
    stmt.thenBranch->accept(*this);
    
    if (stmt.elseBranch) {
        int elseJmp = emitJmp(Opcode::JMP);
        patchJmp(thenJmp);
        stmt.elseBranch->accept(*this);
        patchJmp(elseJmp);
    } else {
        patchJmp(thenJmp);
    }
}

void Compiler::visitWhileStmt(WhileStmt& stmt) {
    LoopContext ctx;
    ctx.loopStart = static_cast<int>(chunk.code.size());
    ctx.continuePatchesToIncrement = false;
    loopStack.push_back(ctx);

    int loopStart = chunk.code.size();
    stmt.condition->accept(*this);
    
    int exitJmp = emitJmp(Opcode::JMP_IF_FALSE);
    stmt.body->accept(*this);
    
    emitLoop(loopStart);
    patchJmp(exitJmp);

    // Patch any `break;` jumps to loop end.
    for (int br : loopStack.back().breakJumps) {
        patchJmp(br);
    }
    // `continue;` in while loops is emitted as a backward jump immediately.
    loopStack.pop_back();
}

void Compiler::visitForStmt(ForStmt& stmt) {
    // Keep `let` variables inside the for-loop initializer scoped to the loop.
    beginScope();

    LoopContext ctx;
    ctx.continuePatchesToIncrement = true; // continue jumps forward to increment start
    loopStack.push_back(ctx);

    if (stmt.initializer) {
        stmt.initializer->accept(*this);
    }

    int loopStart = chunk.code.size();
    loopStack.back().loopStart = loopStart;
    
    int exitJmp = -1;
    if (stmt.condition) {
        stmt.condition->accept(*this);
        exitJmp = emitJmp(Opcode::JMP_IF_FALSE);
    }

    stmt.body->accept(*this);

    // `continue;` inside the body should jump to the increment section (forward).
    for (int cont : loopStack.back().continueJumps) {
        patchJmp(cont);
    }

    if (stmt.increment) {
        stmt.increment->accept(*this);
        // INVARIANT: All valid increment expressions (assignments, ++/-- updates,
        // binary ops, literals) push exactly one value onto the stack. This POP
        // discards that unused result. If new expression types are added that
        // don't push a value, this must be revisited.
        chunk.write(static_cast<uint8_t>(Opcode::POP));
    }

    emitLoop(loopStart);
    if (exitJmp != -1) {
        patchJmp(exitJmp);
    }

    // Patch any `break;` jumps to loop end (after condition fails / after loop).
    for (int br : loopStack.back().breakJumps) {
        patchJmp(br);
    }
    loopStack.pop_back();

    endScope();
}

void Compiler::visitBreakStmt(BreakStmt&) {
    if (loopStack.empty()) {
        throw std::runtime_error("Cannot use 'break' outside of a loop");
    }
    int j = emitJmp(Opcode::JMP);
    loopStack.back().breakJumps.push_back(j);
}

void Compiler::visitContinueStmt(ContinueStmt&) {
    if (loopStack.empty()) {
        throw std::runtime_error("Cannot use 'continue' outside of a loop");
    }
    LoopContext& ctx = loopStack.back();
    if (ctx.continuePatchesToIncrement) {
        int j = emitJmp(Opcode::JMP);
        ctx.continueJumps.push_back(j);
    } else {
        emitLoop(ctx.loopStart);
    }
}

void Compiler::visitPrintStmt(PrintStmt& stmt) {
    stmt.expr->accept(*this);
    chunk.write(static_cast<uint8_t>(Opcode::PRINT));
}
