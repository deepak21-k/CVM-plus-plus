#pragma once
#include "opcode.h"
#include <string>

void disassembleChunk(const Chunk& chunk, const std::string& name);
size_t disassembleInstruction(const Chunk& chunk, size_t offset);
