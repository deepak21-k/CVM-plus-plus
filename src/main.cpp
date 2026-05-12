#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "disasm.h"
#include <iostream>
#include <fstream>
#include <sstream>
bool dumpBytecode = false;
bool dumpOnly = false;  // --dump from CLI: disassemble without executing

void run(const std::string& source, Compiler& compiler, VM& vm) {
    try {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto statements = parser.parse();

        Chunk chunk = compiler.compile(statements);

        if (dumpBytecode || dumpOnly) {
            disassembleChunk(chunk, "Compiled Bytecode");
        }

        if (!dumpOnly) {
            vm.execute(chunk);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        compiler.reset();
        vm.reset();  // Reset VM variables to stay in sync with compiler
    }
}

void repl() {
    std::cout << "CVM++ REPL v1.1\nType 'exit' or 'quit' to close.\n";
    VM vm;
    Compiler compiler;
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;
        if (line == "disasm on") {
            dumpBytecode = true;
            std::cout << "Bytecode disassembly enabled.\n";
            continue;
        }
        if (line == "disasm off") {
            dumpBytecode = false;
            std::cout << "Bytecode disassembly disabled.\n";
            continue;
        }
        if (line.empty()) continue;
        
        run(line, compiler, vm);
    }
}

void runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << path << '\n';
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    VM vm;
    Compiler compiler;
    run(buffer.str(), compiler, vm);
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        repl();
    } else {
        std::string path;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--dump") {
                dumpOnly = true;
            } else {
                path = arg;
            }
        }
        if (path.empty()) {
            std::cerr << "Usage: cvm [--dump] [path to script]\n";
            return 1;
        }
        runFile(path);
    }
    return 0;
}
