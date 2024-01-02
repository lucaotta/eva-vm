/**
* Eva Virtual Machine
*/

#pragma once

#include "../parser/eva_parser.h"
#include "eva_compiler.h"
#include "evavalue.h"
#include "globals.h"
#include "logger.h"
#include "opcodes.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

constexpr size_t STACK_LIMIT = 8;

#define BINARY_OP(bin_op) \
do { \
    auto op2 = pop().asNumber(); \
    auto op1 = pop().asNumber(); \
    push(NUMBER(op1 bin_op op2)); \
} while (0)

#define COMPARE_VALUES(op, v1, v2) \
    do { \
        switch (op) { \
        case ComparisonType::GE: \
            push(BOOLEAN(v1 >= v2)); \
            break; \
        case ComparisonType::GT: \
            push(BOOLEAN(v1 > v2)); \
            break; \
        case ComparisonType::LT: \
            push(BOOLEAN(v1 < v2)); \
            break; \
        case ComparisonType::LE: \
            push(BOOLEAN(v1 <= v2)); \
            break; \
        case ComparisonType::EQ: \
            push(BOOLEAN(v1 == v2)); \
            break; \
        case ComparisonType::NEQ: \
            push(BOOLEAN(v1 != v2)); \
            break; \
        default: \
            DIE << "Unimplemented comparison opcode" << std::hex << int(op); \
        } \
    } while (0)

class EvaVM
{
public:
    EvaVM()
        : parser(std::make_unique<syntax::eva_parser>())
        , m_globals(std::make_shared<Globals>())
    {
        const auto idx = m_globals->define("PI");
        m_globals->set(idx.value(), NUMBER(3.1415));
    };

    EvaValue exec(const std::string &program)
    {
        auto ast = parser->parse(program);

        EvaCompiler comp(m_globals);
        co = comp.compile(ast, "main");
        ip = &co->code[0];
        return eval();
    }

    EvaValue exec(const std::vector<uint8_t> &code, std::vector<EvaValue> constants)
    {
        co = allocCode("main").asCodeObject();
        co->constants = std::move(constants);
        co->code = std::move(code);
        ip = &co->code[0];
        sp = stack.begin();
        return eval();
    }

    EvaValue eval()
    {
        for (;;) {
            unsigned int opcode = read_byte();
            switch (opcode) {
            case OP_HALT:
                return pop();
            case OP_CONST: {
                auto constIndex = read_byte();
                push(co->constants[constIndex]);
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
            case OP_COMP: {
                auto op = ComparisonType(read_byte());
                auto stack2 = pop();
                auto stack1 = pop();
                if (isNumber(stack1) && isNumber(stack2)) {
                    COMPARE_VALUES(op, stack1.asNumber(), stack2.asNumber());
                } else if (isString(stack1) && isString(stack2)) {
                    COMPARE_VALUES(op, stack1.asCppString(), stack2.asCppString());
                }
                break;
            }
            case OP_JMP_IF_FALSE: {
                auto addr = read_address();
                if (pop().asBool() == false) {
                    ip = &co->code[addr];
                }
                break;
            }
            case OP_JMP: {
                auto addr = read_address();
                ip = &co->code[addr];
                break;
            }
            case OP_GET_GLOBAL: {
                auto index = read_byte();
                push(m_globals->get(index));
                break;
            }
            case OP_SET_GLOBAL: {
                auto index = read_byte();
                m_globals->set(index, peek());
                break;
            }
            default:
                DIE << "VM: Unknown opcode " << std::hex << opcode;
            }
        }
    }

private:
    uint8_t read_byte() {
        auto ret = *ip;
        ++ip;
        return ret;
    }

    uint16_t read_address()
    {
        auto ret = (*ip << 8) | (*(ip + 1));
        ip += 2;
        return ret;
    }

    void push(const EvaValue &v)
    {
        if ((sp - stack.begin()) > STACK_LIMIT) {
            DIE << "Stack overflow";
        }
        *sp = v;
        sp++;
    }

    EvaValue pop()
    {
        if (sp - stack.begin() == 0) {
            DIE << "Empty stack";
        }
        sp--;
        return *sp;
    }
    EvaValue peek()
    {
        if (sp - stack.begin() == 0) {
            DIE << "Empty stack";
        }
        return *(sp - 1);
    }

    std::unique_ptr<syntax::eva_parser> parser;
    std::shared_ptr<Globals> m_globals;

    CodeObject *co = {nullptr};
    const uint8_t *ip;

    std::array<EvaValue, STACK_LIMIT> stack;
    EvaValue *sp{stack.begin()};
};
