cmake_minimum_required(VERSION 3.10)
project(EvaVM VERSION 1.0)

set(LANGUAGE CXX)

add_executable(EvaVM src/vm/main.cpp)

add_executable(test_eva src/vm/test.cpp)