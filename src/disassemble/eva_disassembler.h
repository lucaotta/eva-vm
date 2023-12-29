#pragma once
#include "../vm/evavalue.h"
#include "../vm/opcodes.h"

#include <cstdio>
#include <iostream>

class EvaDisassembler
{
public:
    EvaDisassembler() = default;

    void disassemble(CodeObject *co)
    {
        std::cout << "----------- Opcodes for " << co->name << " ------------------\n";
        printf("%.4s %.2s %20s %s\n", "addr", "op", "name", "value");
        size_t offset = 0;
        while (offset < co->code.size()) {
            offset = disassembleInstruction(co, offset);
        }
    }

private:
    size_t disassembleInstruction(CodeObject *co, size_t offset)
    {
        auto op = co->code[offset];
        printf("%04zX %02X %20s ", offset, op, opcodeToString(op).c_str());
        switch (op) {
        case OP_HALT:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            break;
        case OP_CONST: {
            auto index = co->code[++offset];
            printf("%4d (%s)", index, toString(co->constants[index]).c_str());
            break;
        }
        case OP_COMP:
            printf("%4d", co->code[++offset]);
            break;
        case OP_JMP: {
            uint16_t address = (co->code[offset + 1] << 8) | (co->code[offset + 2]);
            offset += 2;
            printf("%04X", address);
            break;
        }
        case OP_JMP_IF_FALSE: {
            uint16_t address = (co->code[offset + 1] << 8) | (co->code[offset + 2]);
            offset += 2;
            printf("%04X", address);
            break;
        }
        }
        printf("\n");
        offset++;
        return offset;
    }
};
