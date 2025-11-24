cmake_minimum_required(VERSION 4.0)
project(ProyectoCompi)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(ProyectoCompi
        ast.cpp
        ast.h
        main.cpp
        parser.cpp
        parser.h
        scanner.cpp
        scanner.h
        token.cpp
        token.h
        visitor.cpp
        visitor.h)
