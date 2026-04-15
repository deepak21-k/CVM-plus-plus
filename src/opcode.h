#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class Opcode : uint8_t {
    PUSH_INT,
    PUSH_BOOL,
    ADD, SUB, MUL, DIV, MOD,
    EQ, NEQ, LT, GT, LTE, GTE,
    BIT_XOR, BIT_AND, SHL, SHR, BIT_NOT,
    NOT, NEG, POP,
    SET_VAR,
    GET_VAR,
    JMP,
    JMP_IF_FALSE,
    PRINT,
    INPUT,
    HALT
};

struct Chunk {
    std::vector<uint8_t> code;
    
    void write(uint8_t byte) {
        code.push_back(byte);
    }
    
    void writeInt(int32_t value) {
        code.push_back((value >> 24) & 0xFF);
        code.push_back((value >> 16) & 0xFF);
        code.push_back((value >> 8) & 0xFF);
        code.push_back(value & 0xFF);
    }

    int32_t readInt(size_t offset) const {
        return (static_cast<int32_t>(code[offset]) << 24) |
               (static_cast<int32_t>(code[offset + 1]) << 16) |
               (static_cast<int32_t>(code[offset + 2]) << 8) |
               (static_cast<int32_t>(code[offset + 3]));
    }
};
