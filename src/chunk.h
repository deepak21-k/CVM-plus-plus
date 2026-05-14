#pragma once
#include <cstdint>
#include <vector>
#include "error.h"

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<int> lines;
    
    inline void reserve(size_t capacity) {
        code.reserve(capacity);
        lines.reserve(capacity);
    }

    inline void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    inline void writeByte(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }
    
    inline void writeInt(int32_t value, int line) {
        code.push_back((value >> 24) & 0xFF); lines.push_back(line);
        code.push_back((value >> 16) & 0xFF); lines.push_back(line);
        code.push_back((value >> 8) & 0xFF); lines.push_back(line);
        code.push_back(value & 0xFF); lines.push_back(line);
    }

    [[nodiscard]] int getLine(size_t offset) const {
        if (offset < lines.size()) {
            return lines[offset];
        }
        return -1;
    }

    [[nodiscard]] int32_t readInt(size_t offset) const {
        if (code.size() < 4 || offset > code.size() - 4) {
            throw RuntimeError("Bytecode truncated: attempted read past end");
        }
        return static_cast<int32_t>(
               (static_cast<uint32_t>(code[offset]) << 24) |
               (static_cast<uint32_t>(code[offset + 1]) << 16) |
               (static_cast<uint32_t>(code[offset + 2]) << 8) |
               (static_cast<uint32_t>(code[offset + 3]))
        );
    }
};
