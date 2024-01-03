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
        co = allocCode(std::move(name_tag), 0).asCodeObject();
        addCodeObject(co);

        generate(input);

        emit(OP_HALT);

        EvaDisassembler disasm(m_globals);
        for (auto co : m_codeObjects) {
            disasm.disassemble(co);
        }

        return co;
    }

    void generate(Exp exp)
    {
        // Use a switch because it makes debugging easier
        switch (exp.type) {
        case ExpType::NUMBER:
            genNumber(exp);
            break;
        case ExpType::STRING:
            genString(exp);
            break;
        case ExpType::SYMBOL:
            genSymbol(exp);
            break;
        case ExpType::LIST:
            genList(exp);
            break;
        }
    }

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
                DIE << "[Compiler] Unkown global variable " << exp.string;
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

                    // Actually we don't need to emit a store when declaring the variable
                    // because the value is already there
                    //                    emit(OP_SET_LOCAL);
                    //                    emit(co->getLocalIndex(varName).value());
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
            // (def <name> (<parameters>) <expressions>)
            else if (op == "def") {
                auto name = exp.list[1].string;
                auto parameters = exp.list[2].list;
                auto arity = parameters.size();
                auto body = exp.list[3];

                // Allocate code object
                auto prevCo = co;
                auto newCode = allocCode(name, parameters.size());
                prevCo->addConst(newCode);
                co = newCode.asCodeObject();

                addCodeObject(co);

                co->addLocal(name);
                for (int i = 0; i < parameters.size(); ++i) {
                    co->addLocal(parameters[i].string);
                }

                // generate code
                generate(body);

                // In case we have a body with a single expression, we need
                // to generate the instruction to pop all the parameters and
                // the function name
                if (!isBlock(body)) {
                    emit(OP_SCOPE_EXIT);
                    emit(arity + 1);
                }

                emit(OP_RETURN);

                auto fn = allocFunction(newCode.asCodeObject());

                co = prevCo;
                co->addConst(fn);

                emit(OP_CONST);
                emit(co->constants.size() - 1);

                if (co->isGlobalScope()) {
                    m_globals->define(name);
                    emit(OP_SET_GLOBAL);
                    emit(m_globals->getGlobalIndex(name).value());
                } else {
                    co->addLocal(name);
                    emit(OP_SET_LOCAL);
                    emit(co->getLocalIndex(name).value());
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
                    if (i == lastElement && isLocalDeclaration)
                        DIE << "[Compiler] Blocks must end with a value, not a variable "
                               "declaration";
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
            // Function calls:
            // (square 2)
            else {
                // Push the function on the stack
                generate(exp.list[0]);

                // evaluate all parameters
                for (int i = 1; i < exp.list.size(); i++) {
                    generate(exp.list[i]);
                }

                emit(OP_CALL);
                emit(exp.list.size() - 1);
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
        if (varsCount > 0 || co->arity > 0) {
            emit(OP_SCOPE_EXIT);
            if (isFunctionBody()) {
                varsCount += co->arity + 1;
            }
            emit(varsCount);
        }
        co->exitBlock();
    }

    bool isFunctionBody() { return co->name != "main" && co->currentLevel == 1; }

    bool isVarDeclaration(Exp exp) { return isTagList(exp, "var"); }
    bool isBlock(Exp exp) { return isTagList(exp, "begin"); }

    bool isTagList(Exp exp, const std::string &tag)
    {
        return exp.type == ExpType::LIST && exp.list[0].type == ExpType::SYMBOL
               && exp.list[0].string == tag;
    }
    void addCodeObject(CodeObject *) { m_codeObjects.push_back(co); };

    CodeObject *co{nullptr};
    std::shared_ptr<Globals> m_globals;
    std::vector<CodeObject *> m_codeObjects;
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
