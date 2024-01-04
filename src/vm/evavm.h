/**
* Eva Virtual Machine
*/

#pragma once

#include "../parser/eva_parser.h"
#include "eva_collector.h"
#include "eva_compiler.h"
#include "evavalue.h"
#include "globals.h"
#include "logger.h"
#include "opcodes.h"

#include <array>
#include <memory>
#include <set>
#include <string>
#include <vector>

// FIXME: use a largish stack because there are ops that don't pop
// for example each time we load a const
constexpr size_t STACK_LIMIT = 128;
constexpr size_t GC_THRESHOLD = 512;

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
        : m_globals(std::make_shared<Globals>())
        , parser(std::make_unique<syntax::eva_parser>())
        , m_compiler(std::make_unique<EvaCompiler>(m_globals))
        , m_collector(std::make_unique<EvaCollector>())
    {
        setGlobalVariables();
    };

    ~EvaVM() { Traceable::clear(); }

    EvaValue exec(const std::string &program)
    {
        // Add an implicit block so that all list of instructions are ok
        auto ast = parser->parse("(begin " + program + ")");

        co = m_compiler->compile(ast, "main");
        ip = &co->code[0];
        sp = stack.begin();
        return eval();
    }

    EvaValue exec(const std::vector<uint8_t> &code, std::vector<EvaValue> constants)
    {
        co = allocCode("main", 0).asCodeObject();
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
                    maybeGC();
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
                // User defined functions
                else {
                    auto callee = fn.asFunction();
                    frames.push(StackFrame{.ip = ip, .bp = bp, .co = co});

                    ip = &callee->co->code[0];
                    co = callee->co;
                    bp = sp - args - 1;
                }
                break;
            }
            case OP_RETURN: {
                auto frame = frames.top();
                frames.pop();
                ip = frame.ip;
                bp = frame.bp;
                co = frame.co;
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

    void maybeGC()
    {
        if (Traceable::bytesAllocated < GC_THRESHOLD)
            return;

        // Three sources of roots:
        // 1. constant objects
        // 2. the stack
        // 3. globals
        auto roots = getStackGCRoots();

        // Add current code to roots, so it doesn't get deleted
        roots.insert(co);

        auto constants = m_compiler->getConstantObjects();
        roots.insert(constants.begin(), constants.end());

        auto globals = getGlobalGCRoots();
        roots.insert(globals.begin(), globals.end());

        m_collector->runGC(roots);
    }

    std::set<Traceable *> getStackGCRoots()
    {
        std::set<Traceable *> ret;
        auto spCopy = sp;
        while (spCopy >= stack.begin()) {
            if (isObject(*spCopy)) {
                ret.insert(spCopy->asObject());
            }
            spCopy--;
        }
        return ret;
    }

    std::set<Traceable *> getGlobalGCRoots()
    {
        std::set<Traceable *> ret;

        for (const auto &g : m_globals->m_values) {
            if (isObject(g.value)) {
                ret.insert(g.value.asObject());
            }
        }

        return ret;
    }

    std::shared_ptr<Globals> m_globals;
    std::unique_ptr<syntax::eva_parser> parser;
    std::unique_ptr<EvaCompiler> m_compiler;
    std::unique_ptr<EvaCollector> m_collector;

    CodeObject *co = {nullptr};
    const uint8_t *ip;

    std::array<EvaValue, STACK_LIMIT> stack;

    struct StackFrame
    {
        const uint8_t *ip;
        EvaValue *bp;
        CodeObject *co;
    };

    std::stack<StackFrame> frames;
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
