#pragma once

#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "opcodes.h"

#include <string>
#include <string_view>

class EvaCompiler
{
public:
    EvaCompiler() = default;

    CodeObject *compile(const Value &input, std::string name_tag)
    {
        co = allocCode(std::move(name_tag)).asCodeObject();

        if (input.type == ExpType::NUMBER) {
            co->constants.push_back(NUMBER(input.number));
            emit(OP_CONST);
            emit(0);
        }

        if (input.type == ExpType::STRING) {
            co->constants.push_back(allocString(input.string));
            emit(OP_CONST);
            emit(0);
        }

        emit(OP_HALT);
        return co;
    }

private:
    void emit(uint8_t opcode) { co->code.push_back(opcode); }

    CodeObject *co{nullptr};
};
