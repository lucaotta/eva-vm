cmake_minimum_required(VERSION 3.10)
project(EvaVM VERSION 1.0)

set(LANGUAGE CXX)

set(CXX_STANDARD 17)

# Sanitizers are omitted because currently there's an issue with string constants
# allocated by code, they don't seem to be GC'd correctly.
# However the EvaVM destructor **should** destroy everything, so is this a false positive
# from the sanitizer that can't track the deallocation? Or is it a subtle error in my
# delete function, which doesn't cleanup properly?
#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address")

add_executable(EvaVM
    src/vm/main.cpp

    src/vm/evavalue.cpp
    src/vm/eva_compiler.h
)

add_executable(test_eva
    src/vm/test.cpp

    src/vm/evavalue.cpp
)
