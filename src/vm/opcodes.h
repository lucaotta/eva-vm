#pragma once

#include "logger.h"
#include <cstdint>
#include <string>

#define CASE_STR(x) \
    case OP_##x: \
        return #x

constexpr uint8_t OP_HALT = 0x00;
constexpr uint8_t OP_CONST = 0x01;
constexpr uint8_t OP_ADD = 0x02;
constexpr uint8_t OP_SUB = 0x03;
constexpr uint8_t OP_MUL = 0x04;
constexpr uint8_t OP_DIV = 0x05;
constexpr uint8_t OP_COMP = 0x06;
constexpr uint8_t OP_JMP_IF_FALSE = 0x07;
constexpr uint8_t OP_JMP = 0x08;
constexpr uint8_t OP_GET_GLOBAL = 0x09;
constexpr uint8_t OP_SET_GLOBAL = 0x0A;

enum class ComparisonType : uint8_t {
    GT,
    GE,
    LT,
    LE,
    EQ,
    NEQ,
};

inline std::string opcodeToString(uint8_t opcode)
{
    switch (opcode) {
        CASE_STR(HALT);
        CASE_STR(CONST);
        CASE_STR(ADD);
        CASE_STR(SUB);
        CASE_STR(MUL);
        CASE_STR(DIV);
        CASE_STR(COMP);
        CASE_STR(JMP_IF_FALSE);
        CASE_STR(JMP);
        CASE_STR(GET_GLOBAL);
        CASE_STR(SET_GLOBAL);
    }
    DIE << "Unhandled opcodeToString " << std::hex << int(opcode);
    return "";
}
