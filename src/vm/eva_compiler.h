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

    CodeObject compile(const Value &input, std::string name_tag)
    {
        CodeObject co(std::move(name_tag));

        if (input.type == ExpType::NUMBER) {
            co.constants.push_back(NUMBER(input.number));
            co.code.push_back(OP_CONST);
            co.code.push_back(0);
        }

        if (input.type == ExpType::STRING) {
            co.constants.push_back(allocString(input.string));
            co.code.push_back(OP_CONST);
            co.code.push_back(0);
        }

        co.code.push_back(OP_HALT);
        return co;
    }
};
