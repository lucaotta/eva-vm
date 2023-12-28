/**
* Eva Virtual Machine
*/

#pragma once

#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "logger.h"
#include "opcodes.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

constexpr size_t STACK_LIMIT = 512;

#define BINARY_OP(bin_op) \
do { \
    auto op2 = pop().asNumber(); \
    auto op1 = pop().asNumber(); \
    push(NUMBER(op1 bin_op op2)); \
} while (0)

class EvaVM {
public:
    EvaVM() : parser(std::make_unique<syntax::eva_parser>()) {};

    EvaValue exec(const std::string &program) {
        auto ast = parser->parse(program);

        code = {OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT};

        constants.push_back(NUMBER(3));
        constants.push_back(NUMBER(2));

        ip = &code[0];

        return eval();
    }

    EvaValue exec(const std::vector<uint8_t> &code, std::vector<EvaValue> constants) {
        this->constants = std::move(constants);
        ip = &code[0];
        sp = stack.begin();
        return eval();
    }

    EvaValue eval() {
        for (;;) {
            unsigned int opcode = read_byte();
            switch (opcode) {
                case OP_HALT:
                    return pop();
                case OP_CONST: {
                    auto constIndex = read_byte();
                    push(constants[constIndex]);
                    break;
                }
                case OP_ADD: {
                    auto stack2 = pop();
                    auto stack1 = pop();
                    if (isNumber(stack2) && isNumber(stack1)) {
                        push(NUMBER(stack1.asNumber() + stack2.asNumber()));
                    }
                    if (isObjectType(stack2, ObjectType::STRING)
                        && isObjectType(stack1, ObjectType::STRING)) {
                        push(allocString(stack1.asCppString() + stack2.asCppString()));
                    }
                    break;
                }
                case OP_SUB: {
                    BINARY_OP(-);
                    break;
                }
                case OP_MUL: {
                    BINARY_OP(*);
                    break;
                }
                case OP_DIV: {
                    BINARY_OP(/);
                    break;
                }
                default:
                    DIE << "Unknown opcode " << std::hex << opcode;
            }
        }

    }

private:
    uint8_t read_byte() {
        auto ret = *ip;
        ++ip;
        return ret;
    }

    void push(const EvaValue &v) {
        if ((sp - stack.begin()) > STACK_LIMIT) {
            DIE << "Stack overflow";
        }
        *sp = v;
        sp++;
    }

    EvaValue pop() {
        if (sp - stack.begin() == 0) {
            DIE << "Empty stack";
        }
        sp--;
        return *sp;
    }

    std::unique_ptr<syntax::eva_parser> parser;

    std::vector<uint8_t> code;
    const uint8_t *ip;

    std::array<EvaValue, STACK_LIMIT> stack;
    EvaValue *sp { stack.begin() };

    std::vector<EvaValue> constants;
};
