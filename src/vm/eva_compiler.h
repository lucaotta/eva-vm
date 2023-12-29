#pragma once

#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "opcodes.h"

#include <map>
#include <string>

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
        //co->constants.push_back(NUMBER(exp.number));
        emit(OP_CONST);
        emit(getNumericConstant(exp));
    }
    void genString(const Exp &exp)
    {
        //        co->constants.push_back(allocString(exp.string));
        emit(OP_CONST);
        emit(getStringConstant(exp));
    }
    void genSymbol(const std::vector<Exp> &list)
    {
        if (list[0].string == "+") {
            generate(list[1]);
            generate(list[2]);
            emit(OP_ADD);
        }
    }
    void genList(const Exp &exp)
    {
        if (exp.list[0].type == ExpType::SYMBOL) {
            genSymbol(exp.list);
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
