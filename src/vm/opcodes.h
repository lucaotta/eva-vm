#pragma once

#include <cstdint>


constexpr uint8_t OP_HALT = 0x00;
constexpr uint8_t OP_CONST = 0x01;
constexpr uint8_t OP_ADD = 0x02;
constexpr uint8_t OP_SUB = 0x03;
constexpr uint8_t OP_MUL = 0x04;
constexpr uint8_t OP_DIV = 0x05;
constexpr uint8_t OP_COMP = 0x06;

enum class ComparisonType : uint8_t {
    GT,
    GE,
    LT,
    LE,
    EQ,
    NEQ,
};
