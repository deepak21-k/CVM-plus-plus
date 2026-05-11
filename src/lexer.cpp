#include "lexer.h"
#include <cctype>
#include <unordered_map>
#include <stdexcept>
#include <string>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), column(1) {}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[pos];
}

char Lexer::peekNext() const {
    if (pos + 1 >= source.length()) return '\0';
    return source[pos + 1];
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Lexer::isAtEnd() const {
    return pos >= source.length();
}

void Lexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
        } else if (c == '/' && peekNext() == '/') {
            // Single-line comment
            while (!isAtEnd() && peek() != '\n') {
                advance();
            }
        } else if (c == '/' && peekNext() == '*') {
            // Block comment (non-nesting, matching C behavior)
            advance(); // skip '/'
            advance(); // skip '*'
            while (!isAtEnd()) {
                if (peek() == '*' && peekNext() == '/') {
                    advance(); // skip '*'
                    advance(); // skip '/'
                    break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::createToken(TokenType type, const std::string& value) {
    return {type, value, line, column - static_cast<int>(value.length())};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    tokens.reserve(source.length() / 8 + 1);  // ~8 chars/token average
    
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let", TokenType::LET},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"for", TokenType::FOR},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"print", TokenType::PRINT},
        {"input", TokenType::INPUT},
        {"true", TokenType::TRUE_LIT},
        {"false", TokenType::FALSE_LIT}
    };

    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;

        char c = peek();
        
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::string ident;
            ident.reserve(16);
            while (!isAtEnd() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
                ident += advance();
            }
            auto it = keywords.find(ident);
            if (it != keywords.end()) {
                tokens.push_back(createToken(it->second, ident));
            } else {
                tokens.push_back(createToken(TokenType::IDENTIFIER, ident));
            }
            continue;
        }

        if (std::isdigit(c)) {
            // I2: Support 0x (hex), 0b (binary), 0o (octal) prefixes
            if (c == '0' && !isAtEnd()) {
                char next = peekNext();
                if (next == 'x' || next == 'X') {
                    advance(); advance(); // skip '0x'
                    std::string hex;
                    while (!isAtEnd() && std::isxdigit(static_cast<unsigned char>(peek()))) hex += advance();
                    if (hex.empty()) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Expected hex digits after '0x'");
                    long long val = std::stoll(hex, nullptr, 16);
                    if (val > INT32_MAX) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Hex literal out of int32 range");
                    tokens.push_back(createToken(TokenType::NUMBER, std::to_string(val)));
                    continue;
                } else if (next == 'b' || next == 'B') {
                    advance(); advance(); // skip '0b'
                    std::string bin;
                    while (!isAtEnd() && (peek() == '0' || peek() == '1')) bin += advance();
                    if (bin.empty()) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Expected binary digits after '0b'");
                    long long val = std::stoll(bin, nullptr, 2);
                    if (val > INT32_MAX) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Binary literal out of int32 range");
                    tokens.push_back(createToken(TokenType::NUMBER, std::to_string(val)));
                    continue;
                } else if (next == 'o' || next == 'O') {
                    advance(); advance(); // skip '0o'
                    std::string oct;
                    while (!isAtEnd() && peek() >= '0' && peek() <= '7') oct += advance();
                    if (oct.empty()) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Expected octal digits after '0o'");
                    long long val = std::stoll(oct, nullptr, 8);
                    if (val > INT32_MAX) throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Octal literal out of int32 range");
                    tokens.push_back(createToken(TokenType::NUMBER, std::to_string(val)));
                    continue;
                }
            }
            std::string num;
            num.reserve(16);
            while (!isAtEnd() && std::isdigit(peek())) {
                num += advance();
            }
            tokens.push_back(createToken(TokenType::NUMBER, num));
            continue;
        }

        char advanced = advance();
        
        switch (advanced) {
            case '+':
                if (peek() == '+') {
                    advance();
                    tokens.push_back(createToken(TokenType::PLUS_PLUS, "++"));
                } else {
                    tokens.push_back(createToken(TokenType::PLUS, "+"));
                }
                break;
            case '-':
                if (peek() == '-') {
                    advance();
                    tokens.push_back(createToken(TokenType::MINUS_MINUS, "--"));
                } else {
                    tokens.push_back(createToken(TokenType::MINUS, "-"));
                }
                break;
            case '*': tokens.push_back(createToken(TokenType::STAR, "*")); break;
            case '/': tokens.push_back(createToken(TokenType::SLASH, "/")); break;
            case '%': tokens.push_back(createToken(TokenType::MOD, "%")); break;
            case '(': tokens.push_back(createToken(TokenType::LPAREN, "(")); break;
            case ')': tokens.push_back(createToken(TokenType::RPAREN, ")")); break;
            case '{': tokens.push_back(createToken(TokenType::LBRACE, "{")); break;
            case '}': tokens.push_back(createToken(TokenType::RBRACE, "}")); break;
            case ';': tokens.push_back(createToken(TokenType::SEMICOLON, ";")); break;
            case '^': tokens.push_back(createToken(TokenType::BIT_XOR, "^")); break;
            case '&':
                if (peek() == '&') {
                    advance();
                    tokens.push_back(createToken(TokenType::AND_AND, "&&"));
                } else {
                    tokens.push_back(createToken(TokenType::BIT_AND, "&"));
                }
                break;
            case '|':
                if (peek() == '|') {
                    advance();
                    tokens.push_back(createToken(TokenType::OR_OR, "||"));
                } else {
                    tokens.push_back(createToken(TokenType::BIT_OR, "|"));
                }
                break;
            case '~': tokens.push_back(createToken(TokenType::BIT_NOT, "~")); break;
            case '=':
                if (peek() == '=') {
                    advance();
                    tokens.push_back(createToken(TokenType::EQ_EQ, "=="));
                } else {
                    tokens.push_back(createToken(TokenType::EQUAL, "="));
                }
                break;
            case '<':
                if (peek() == '=') {
                    advance();
                    tokens.push_back(createToken(TokenType::LESS_EQ, "<="));
                } else if (peek() == '<') {
                    advance();
                    tokens.push_back(createToken(TokenType::SHL, "<<"));
                } else {
                    tokens.push_back(createToken(TokenType::LESS, "<"));
                }
                break;
            case '>':
                if (peek() == '=') {
                    advance();
                    tokens.push_back(createToken(TokenType::GREATER_EQ, ">="));
                } else if (peek() == '>') {
                    advance();
                    tokens.push_back(createToken(TokenType::SHR, ">>"));
                } else {
                    tokens.push_back(createToken(TokenType::GREATER, ">"));
                }
                break;
            case '!':
                if (peek() == '=') {
                    advance();
                    tokens.push_back(createToken(TokenType::BANG_EQ, "!="));
                } else {
                    tokens.push_back(createToken(TokenType::NOT, "!"));
                }
                break;
            default: {
                std::string errText(1, advanced);
                throw std::runtime_error("Lexer Error [line " + std::to_string(line) + "]: Unexpected character '" + errText + "'");
            }
        }
    }
    
    tokens.push_back(createToken(TokenType::END_OF_FILE, ""));
    return tokens;
}
