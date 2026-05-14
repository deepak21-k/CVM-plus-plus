#pragma once
#include "token.h"
#include <vector>
#include <string>

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    const std::string& source;
    size_t pos;
    int line;
    int column;

    [[nodiscard]] char peek() const;
    [[nodiscard]] char peekNext() const;
    char advance();
    [[nodiscard]] bool isAtEnd() const;

    void skipWhitespace();
    Token createToken(TokenType type, const std::string& value);
};
