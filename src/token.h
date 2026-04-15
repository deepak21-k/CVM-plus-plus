#pragma once
#include <string>

enum class TokenType {
    // Keywords
    LET, IF, ELSE, WHILE, PRINT, INPUT, TRUE_LIT, FALSE_LIT,

    // Operators
    PLUS, MINUS, STAR, SLASH, MOD,
    EQUAL, EQ_EQ, BANG_EQ, LESS, GREATER, LESS_EQ, GREATER_EQ,
    BIT_XOR, BIT_AND, SHL, SHR, BIT_NOT, NOT,

    // Symbols
    LPAREN, RPAREN, LBRACE, RBRACE, SEMICOLON,

    // Literals & Identifiers
    NUMBER, IDENTIFIER,

    // End of file
    END_OF_FILE,
    INVALID
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};
