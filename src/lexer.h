#pragma once
#include "token.h"
#include <vector>
#include <string>

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos;
    int line;
    int column;

    char peek() const;
    char peekNext() const;
    char advance();
    bool isAtEnd() const;

    void skipWhitespace();
    Token createToken(TokenType type, const std::string& value);
};
