cmake_minimum_required(VERSION 3.16)
project(vit_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/ast/ASTPrinter.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/codegen/LLVMCodeGen.cpp
    src/codegen/NativeCompiler.cpp
    src/main.cpp
)

add_executable(vit ${SOURCES})

if(MINGW)
    target_link_options(vit PRIVATE -static -static-libgcc -static-libstdc++)
endif()
