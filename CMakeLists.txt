cmake_minimum_required(VERSION 3.10)
project(AUL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(aul
    main.cpp
    Lexer.cpp
    Parser.cpp
    Value.cpp
    Environment.cpp
    Interpreter.cpp
)

target_include_directories(aul PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
