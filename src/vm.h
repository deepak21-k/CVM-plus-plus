#pragma once
#include "opcode.h"
#include <array>
#include <unordered_map>

class VM {
public:
    VM();
    void execute(const Chunk& chunk);

    // For testing/debugging, we can inspect stack or variables
    int32_t peekStack() const;

private:
    static constexpr size_t STACK_MAX = 2048;
    std::array<int32_t, STACK_MAX> stack;
    size_t sp; // Stack pointer

    std::unordered_map<int32_t, int32_t> globals;

    void push(int32_t value);
    int32_t pop();
};
