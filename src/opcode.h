#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>

enum class Opcode : uint8_t {
    PUSH_INT,
    PUSH_BOOL,
    ADD, SUB, MUL, DIV, MOD,
    EQ, NEQ, LT, GT, LTE, GTE,
    BIT_XOR, BIT_AND, BIT_OR, SHL, SHR, BIT_NOT,
    NOT, NORMALIZE, NEG, POP,
    SET_VAR,
    GET_VAR,
    SET_VAR_PUSH,
    JMP,
    JMP_IF_FALSE,
    PRINT,
    INPUT,
    HALT
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<int> lines;
    
    inline void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }
    
    inline void writeInt(int32_t value, int line) {
        code.push_back((value >> 24) & 0xFF); lines.push_back(line);
        code.push_back((value >> 16) & 0xFF); lines.push_back(line);
        code.push_back((value >> 8) & 0xFF); lines.push_back(line);
        code.push_back(value & 0xFF); lines.push_back(line);
    }

    int getLine(size_t offset) const {
        if (offset < lines.size()) {
            return lines[offset];
        }
        return -1;
    }

    int32_t readInt(size_t offset) const {
        if (code.size() < 4 || offset > code.size() - 4) {
            throw std::runtime_error("Bytecode truncated: attempted read past end");
        }
        return static_cast<int32_t>(
               (static_cast<uint32_t>(code[offset]) << 24) |
               (static_cast<uint32_t>(code[offset + 1]) << 16) |
               (static_cast<uint32_t>(code[offset + 2]) << 8) |
               (static_cast<uint32_t>(code[offset + 3]))
        );
    }
};
