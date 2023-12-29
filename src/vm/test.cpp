#include "evavm.h"

#define CHECK_NUMBER(evaVal, expected) \
do { \
  if (evaVal.asNumber() != expected) { \
    DIE << __LINE__ << " Test failed, actual: " << evaVal.asNumber() << " expected: " << expected << '\n'; \
  } \
} while (0)

#define CHECK_CPPNUMBER(number, expected) \
    do { \
        if (number != expected) { \
            DIE << __LINE__ << " Test failed, actual: " << number << " expected: " << expected \
                << '\n'; \
        } \
    } while (0)

#define CHECK_STRING(evaVal, expected) \
    do { \
        if (evaVal.asCppString() != expected) { \
            DIE << __LINE__ << " Test failed, actual: " << evaVal.asCppString() \
                << " expected: " << expected << '\n'; \
        } \
    } while (0)

#define CHECK_BOOL(evaVal, expected) \
    do { \
        if (evaVal.asBool() != expected) { \
            DIE << __LINE__ << " Test failed, actual: " << evaVal.asBool() \
                << " expected: " << expected << '\n'; \
        } \
    } while (0)

int main()
{
    EvaVM vm;
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT}, {NUMBER(10), NUMBER(3.5)}),
                 13.5);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_SUB, OP_HALT}, {NUMBER(10), NUMBER(3)}), 7);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_MUL, OP_HALT}, {NUMBER(5), NUMBER(2)}), 10);
    CHECK_NUMBER(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_DIV, OP_HALT}, {NUMBER(5), NUMBER(2)}), 2.5);

    CHECK_STRING(vm.exec({OP_CONST, 0, OP_CONST, 1, OP_ADD, OP_HALT},
                         {allocString("Hello"), allocString(" world")}),
                 "Hello world");

    // Test compiler
    CHECK_NUMBER(vm.exec(R"#(
    42.2
    )#"),
                 42.2);

    CHECK_STRING(vm.exec(R"#(
    "Hello world"
    )#"),
                 "Hello world");

    {
        EvaCompiler c;
        syntax::eva_parser p;
        std::unique_ptr<CodeObject> co{c.compile(p.parse(R"#((+ 1 1))#"), "test")};
        CHECK_CPPNUMBER(co->constants.size(), 1);
    }

    {
        EvaCompiler c;
        syntax::eva_parser p;
        std::unique_ptr<CodeObject> co{c.compile(p.parse(R"#((+ "hello" "hello"))#"), "test")};
        CHECK_CPPNUMBER(co->constants.size(), 1);
    }

    CHECK_NUMBER(vm.exec(R"#(
    (+ 1 3)
    )#"),
                 4);
    CHECK_NUMBER(vm.exec(R"#(
    (* 2 3)
    )#"),
                 6);
    CHECK_NUMBER(vm.exec(R"#(
    (- 1.5 3)
    )#"),
                 -1.5);
    CHECK_NUMBER(vm.exec(R"#(
    (/ 3 2)
    )#"),
                 1.5);

    CHECK_STRING(vm.exec(R"#(
    (+ "Hello" "Hello")
    )#"),
                 "HelloHello");

    CHECK_BOOL(vm.exec(R"#(
    true
    )#"),
               true);

    CHECK_BOOL(vm.exec(R"#(
    (> 5 10)
    )#"),
               false);
    CHECK_BOOL(vm.exec(R"#(
    (>= 5 5)
    )#"),
               true);
    CHECK_BOOL(vm.exec(R"#(
    (= 5 10)
    )#"),
               false);
    CHECK_BOOL(vm.exec(R"#(
    (= 5 5)
    )#"),
               true);
    CHECK_BOOL(vm.exec(R"#(
    (!= 5 10)
    )#"),
               true);
    CHECK_BOOL(vm.exec(R"#(
    (!= "hello" "world")
    )#"),
               true);
    CHECK_BOOL(vm.exec(R"#(
    (< "abc" "def")
    )#"),
               true);

    std::cout << "All tests passed\n";
}
