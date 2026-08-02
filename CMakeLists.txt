cmake_minimum_required(VERSION 3.16)
project(vit_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(COMMON_SOURCES
    src/ast/ASTPrinter.cpp
    src/diagnostics/DiagnosticPrinter.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/semantics/SemanticAnalyzer.cpp
    src/semantics/Monomorphizer.cpp
    src/codegen/LLVMCodeGen.cpp
    src/codegen/NativeCompiler.cpp
    src/codegen/JITEngine.cpp
    src/codegen/EscapeAnalysis.cpp
    src/tools/LSP.cpp
    src/tools/PackageManager.cpp
    src/tools/REPL.cpp
    src/tools/Formatter.cpp
    src/tools/Linter.cpp
    src/tools/DevServer.cpp
    src/utils/Platform.cpp
)

add_executable(vit ${COMMON_SOURCES} src/main.cpp)
add_executable(vit-lsp ${COMMON_SOURCES} src/lsp_main.cpp)

if(MINGW)
    target_link_options(vit PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_options(vit-lsp PRIVATE -static -static-libgcc -static-libstdc++)
endif()

