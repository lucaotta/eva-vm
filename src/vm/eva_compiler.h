#pragma once

#include "../disassemble/eva_disassembler.h"
#include "../parser/eva_parser.h"
#include "evavalue.h"
#include "globals.h"
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
    EvaCompiler(std::shared_ptr<Globals> g)
        : m_globals(g)
    {}

    CodeObject *compile(const Value &input, std::string name_tag)
    {
        co = allocCode(std::move(name_tag)).asCodeObject();

        generate(input);

        emit(OP_HALT);

        EvaDisassembler disasm(m_globals);
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
        // Handle variables
        else {
            // Handle local variables first
            if (const auto localIndex = co->getLocalIndex(exp.string); localIndex) {
                emit(OP_GET_LOCAL);
                emit(localIndex.value());
            }
            // Then try if it is a global variable
            else if (const auto globalIndex = m_globals->getGlobalIndex(exp.string); globalIndex) {
                emit(OP_GET_GLOBAL);
                emit(globalIndex.value());
            } else
                DIE << "Unkown global variable " << exp.string;
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
            // (var <variable <value) Define a variable
            else if (op == "var") {
                generate(exp.list[2]);
                const auto &varName = exp.list[1].string;
                if (!co->isGlobalScope()) {
                    co->addLocal(varName);
                    // The trick here is that it's not possible to have something on the stack
                    // that are not variables, we use this fact to store the index in the bytecode.
                    // TODO: probably this will need a fix for user defined functions
                    emit(OP_SET_LOCAL);
                    emit(co->getLocalIndex(varName).value());
                } else {
                    const auto idx = m_globals->define(varName);
                    if (idx) {
                        emit(OP_SET_GLOBAL);
                        emit(idx.value());
                    }
                }
            }
            // (set <variable> <value>)
            else if (op == "set") {
                const auto &varName = exp.list[1].string;
                if (auto idx = co->getLocalIndex(varName); idx) {
                    generate(exp.list[2]);
                    emit(OP_SET_LOCAL);
                    emit(idx.value());
                } else {
                    // Global variables
                    const auto index = m_globals->getGlobalIndex(varName);
                    if (index) {
                        generate(exp.list[2]);
                        emit(OP_SET_GLOBAL);
                        emit(index.value());
                    }
                }
            }
            // (begin <expression>)
            else if (op == "begin") {
                const auto lastElement = exp.list.size() - 1;
                co->enterBlock();
                for (size_t i = 1; i < exp.list.size(); ++i) {
                    generate(exp.list[i]);
                    const bool isLocalDeclaration = isVarDeclaration(exp.list[i])
                                                    && !co->isGlobalScope();
                    // We have generated a value on the stack, now
                    // we need to pop it except for the last one
                    if (i != lastElement && !isLocalDeclaration)
                        emit(OP_POP);
                }
                exitBlock();
            }
            // (while <test> <expression)
            else if (op == "while") {
                auto loopStart = getCurrentOffset();

                generate(exp.list[1]);
                emit(OP_JMP_IF_FALSE);

                // placeholder bytes for 16-bit address
                emit(0);
                emit(0);

                // get the address where the placeholder bytes are
                auto loopEndJumpAddress = getCurrentOffset() - 2;

                // generate code for <expression>
                generate(exp.list[2]);

                emit(OP_JMP);
                // Go back to loop start
                emit(0);
                emit(0);
                patchAddress(getCurrentOffset() - 2, loopStart);
                patchAddress(loopEndJumpAddress, getCurrentOffset() + 1);
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

    void exitBlock()
    {
        auto varsCount = co->variableNumberInCurrentBlock();
        if (varsCount > 0) {
            emit(OP_SCOPE_EXIT);
            emit(varsCount);
        }
        co->exitBlock();
    }

    bool isVarDeclaration(Exp exp) { return isTagList(exp, "var"); }

    bool isTagList(Exp exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL
               && exp.list[0].string == tag;
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
    std::shared_ptr<Globals> m_globals;
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
