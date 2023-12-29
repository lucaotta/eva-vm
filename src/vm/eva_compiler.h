#pragma once

#include "../disassemble/eva_disassembler.h"
#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "opcodes.h"

#include <map>
#include <string>

#define GEN_BINARY_OP(op) \
    do { \
        generate(exp.list[1]); \
        generate(exp.list[2]); \
        emit(op); \
    } while (0)

#define GEN_COMPARISON_OP(op) \
    do { \
        generate(exp.list[1]); \
        generate(exp.list[2]); \
        emit(OP_COMP); \
        emit(uint8_t(op)); \
    } while (0)

class EvaCompiler
{
public:
    EvaCompiler() = default;

    CodeObject *compile(const Value &input, std::string name_tag)
    {
        co = allocCode(std::move(name_tag)).asCodeObject();

        generate(input);

        emit(OP_HALT);

        EvaDisassembler disasm;
        disasm.disassemble(co);

        return co;
    }

    void generate(Exp exp) { handlers[exp.type](exp); }

private:
    void emit(uint8_t opcode) { co->code.push_back(opcode); }
    void genNumber(const Exp &exp)
    {
        emit(OP_CONST);
        emit(getNumericConstant(exp.number));
    }
    void genString(const Exp &exp)
    {
        emit(OP_CONST);
        emit(getStringConstant(exp.string));
    }
    /*
     * Handles variables and built-in
     */
    void genSymbol(const Exp &exp)
    {
        if (exp.string == "true" || exp.string == "false") {
            emit(OP_CONST);
            emit(getBoolConstant(exp.string == "true" ? true : false));
        }
    }
    void genList(const Exp &exp)
    {
        if (exp.list[0].type == ExpType::SYMBOL) {
            auto op = exp.list[0].string;
            if (op == "+") {
                GEN_BINARY_OP(OP_ADD);
            } else if (op == "-") {
                GEN_BINARY_OP(OP_SUB);
            } else if (op == "*") {
                GEN_BINARY_OP(OP_MUL);
            } else if (op == "/") {
                GEN_BINARY_OP(OP_DIV);
            }
            // Handle comparison operators
            // eg. (< 5 10)
            else if (comparison.count(op) > 0) {
                GEN_COMPARISON_OP(comparison[op]);
            }
            // (if <test> <true_branch> <false_branch>)
            else if (op == "if") {
                generate(exp.list[1]);

                emit(OP_JMP_IF_FALSE);

                // placeholder bytes for 16-bit address
                emit(0);
                emit(0);

                // get the address where the placeholder bytes are
                auto jmpIfFalseAddress = getCurrentOffset() - 2;

                // generate code for true_branch
                generate(exp.list[2]);

                emit(OP_JMP);
                // placeholder to jump over false_branch code
                emit(0);
                emit(0);
                auto jmpAddress = getCurrentOffset() - 2;

                auto falseBranchAddress = getCurrentOffset();

                // generate false_branch code
                generate(exp.list[3]);

                patchAddress(jmpIfFalseAddress, falseBranchAddress);
                patchAddress(jmpAddress, getCurrentOffset());
            }
        }
    }

    uint16_t getCurrentOffset() { return co->code.size(); }

    void patchAddress(uint16_t address, uint16_t value)
    {
        // write `value` at `address` in code
        co->code[address] = uint8_t(value >> 8);
        address++;
        co->code[address] = uint8_t(value & 0x00FF);
    }

    int getNumericConstant(double value)
    {
        for (int i = 0; i < co->constants.size(); i++) {
            const auto c = co->constants[i];
            if (isNumber(c) && c.asNumber() == value) {
                return i;
            }
        }
        co->constants.push_back(NUMBER(value));
        return co->constants.size() - 1;
    }

    int getBoolConstant(bool value)
    {
        for (int i = 0; i < co->constants.size(); i++) {
            const auto c = co->constants[i];
            if (isBool(c) && c.asBool() == value) {
                return i;
            }
        }
        co->constants.push_back(BOOLEAN(value));
        return co->constants.size() - 1;
    }

    int getStringConstant(const std::string &value)
    {
        for (int i = 0; i < co->constants.size(); i++) {
            const auto c = co->constants[i];
            if (isString(c) && c.asCppString() == value) {
                return i;
            }
        }
        co->constants.push_back(allocString(value));
        return co->constants.size() - 1;
    }

    std::map<ExpType, std::function<void(const Exp &)>> handlers{
        {
            ExpType::NUMBER,
            [this](const Exp &e) { genNumber(e); },
        },
        {
            ExpType::STRING,
            [this](const Exp &e) { genString(e); },
        },
        {
            ExpType::SYMBOL,
            [this](const Exp &e) { genSymbol(e); },
        },
        {
            ExpType::LIST,
            [this](const Exp &e) { genList(e); },
        },
    };

    CodeObject *co{nullptr};
    static std::map<std::string, ComparisonType> comparison;
};

std::map<std::string, ComparisonType> EvaCompiler::comparison{
    {">", ComparisonType::GT},
    {">=", ComparisonType::GE},
    {"<", ComparisonType::LT},
    {"<=", ComparisonType::LE},
    {"=", ComparisonType::EQ},
    {"!=", ComparisonType::NEQ},
};
