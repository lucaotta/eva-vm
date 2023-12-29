#pragma once

#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "logger.h"
#include "opcodes.h"

#include <map>
#include <string>

#define GEN_BINARY_OP(op) \
    do { \
        generate(exp.list[1]); \
        generate(exp.list[2]); \
        emit(op); \
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
        return co;
    }

    void generate(Exp exp) { handlers[exp.type](exp); }

private:
    void emit(uint8_t opcode) { co->code.push_back(opcode); }
    void genNumber(const Exp &exp)
    {
        emit(OP_CONST);
        emit(getNumericConstant(exp));
    }
    void genString(const Exp &exp)
    {
        emit(OP_CONST);
        emit(getStringConstant(exp));
    }
    void genSymbol(const Exp &exp) { DIE << "Unimplemented Sybol"; }
    void genList(const Exp &exp)
    {
        if (exp.list[0].type == ExpType::SYMBOL) {
            auto op = exp.list[0].string;
            if (op == "+") {
                GEN_BINARY_OP(OP_ADD);
            }
            if (op == "-") {
                GEN_BINARY_OP(OP_SUB);
            }
            if (op == "*") {
                GEN_BINARY_OP(OP_MUL);
            }
            if (op == "/") {
                GEN_BINARY_OP(OP_DIV);
            }
        }
    }

    int getNumericConstant(const Exp &exp)
    {
        for (int i = 0; i < co->constants.size(); i++) {
            const auto c = co->constants[i];
            if (exp.type == ExpType::NUMBER && isNumber(c) && c.asNumber() == exp.number) {
                return i;
            }
        }
        co->constants.push_back(NUMBER(exp.number));
        return co->constants.size() - 1;
    }

    int getStringConstant(const Exp &exp)
    {
        for (int i = 0; i < co->constants.size(); i++) {
            const auto c = co->constants[i];
            if (exp.type == ExpType::STRING && isString(c) && c.asCppString() == exp.string) {
                return i;
            }
        }
        co->constants.push_back(allocString(exp.string));
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
            ExpType::LIST,
            [this](const Exp &e) { genList(e); },
        },
    };

    CodeObject *co{nullptr};
};
