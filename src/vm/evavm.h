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

// FIXME: use a largish stack because there are ops that don't pop
// for example each time we load a const
constexpr size_t STACK_LIMIT = 128;

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
        setGlobalVariables();
    };

    EvaValue exec(const std::string &program)
    {
        // Add an implicit block so that all list of instructions are ok
        auto ast = parser->parse("(begin " + program + ")");

        EvaCompiler comp(m_globals);
        co = comp.compile(ast, "main");
        ip = &co->code[0];
        sp = stack.begin();
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
            auto opcode = read_byte();
            //            std::cout << "current opcode " << opcodeToString(opcode) << '\n';
            //            printStack();
            switch (opcode) {
            case OP_HALT: {
                return pop();
            }
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
                m_globals->set(index, peek(0));
                break;
            }
            case OP_POP:
                pop();
                break;
            case OP_GET_LOCAL: {
                // Local variables are always stored on the stack
                auto index = read_byte();
                push(bp[index]);
                break;
            }
            case OP_SET_LOCAL: {
                auto index = read_byte();
                // TODO: at the moment we are working only with a global stack.
                // It must be changed to support function calls.
                bp[index] = peek(0);
                break;
            }
            case OP_SCOPE_EXIT: {
                auto count = read_byte();
                // We need to preserve the value of the block on the top of the stack.
                // Copy it then change the stack pointer
                *(sp - count - 1) = peek(0);
                popN(count);
                break;
            }
            case OP_CALL: {
                auto args = read_byte();
                auto fn = peek(args);
                if (isNative(fn)) {
                    fn.asNativeFunction()->fn();
                    auto result = pop();
                    popN(args + 1);
                    push(result);
                }

                // TODO: handle user defined functions
                break;
            }
            default:
                DIE << "VM: Unknown opcode " << std::hex << opcode;
            }
        }
    }

private:
    void printStack()
    {
        auto csp = sp - 1;
        std::cout << "---- STACK ----\n";
        while (csp >= stack.begin()) {
            std::cout << toString(*csp) << "\n";
            csp--;
        }
        std::cout << std::endl;
    }
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
        if ((sp - stack.begin()) >= STACK_LIMIT) {
            DIE << "Stack overflow";
        }
        *sp = v;
        sp++;
    }

    EvaValue pop()
    {
        if (sp - stack.begin() == 0) {
            DIE << "VM pop: Empty stack";
        }
        sp--;
        return *sp;
    }

    void popN(size_t n)
    {
        if (sp - stack.begin() < n) {
            DIE << "VM: stack too small, requested popN " << n << " but size is "
                << sp - stack.begin();
        }

        sp -= n;
    }

    EvaValue peek(size_t number)
    {
        if (sp - stack.begin() == 0) {
            DIE << "VM peek: Empty stack";
        }
        return *(sp - 1 - number);
    }

    std::unique_ptr<syntax::eva_parser> parser;
    std::shared_ptr<Globals> m_globals;

    CodeObject *co = {nullptr};
    const uint8_t *ip;

    std::array<EvaValue, STACK_LIMIT> stack;
    EvaValue *sp{stack.begin()};
    EvaValue *bp{sp};
    void setGlobalVariables()
    {
        m_globals->addConst("PI", NUMBER(3.1415));
        m_globals->addNativeFunction(
            "square",
            [&]() {
                auto x = peek(0).asNumber();
                push(NUMBER(x * x));
            },
            1);
    }
};
