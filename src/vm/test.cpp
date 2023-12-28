#include "evavm.h"

#define CHECK_NUMBER(evaVal, expected) \
do { \
  if (evaVal.asNumber() != expected) { \
    DIE << __LINE__ << " Test failed, actual: " << evaVal.asNumber() << " expected: " << expected << '\n'; \
  } \
} while (0)

#define CHECK_STRING(evaVal, expected) \
    do { \
        if (evaVal.asCppString() != expected) { \
            DIE << __LINE__ << " Test failed, actual: " << evaVal.asCppString() \
                << " expected: " << expected << '\n'; \
        } \
    } while (0)

int main() {
    EvaVM vm;
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT}, {NUMBER(10), NUMBER(3)}), 13);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_SUB, OP_HALT}, {NUMBER(10), NUMBER(3)}), 7);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_MUL, OP_HALT}, {NUMBER(5), NUMBER(2)}), 10);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_DIV, OP_HALT}, {NUMBER(5), NUMBER(2)}), 2.5);

    CHECK_STRING(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT},
                         {STRING("Hello"), STRING(" world")}),
                 "Hello world");

    std::cout << "All tests passed\n";
}
