cmake_minimum_required(VERSION 3.10)
project(CVM_PlusPlus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cvm
    src/main.cpp
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/compiler.cpp
    src/vm.cpp
)
