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


