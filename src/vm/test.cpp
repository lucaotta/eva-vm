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
        auto g = std::make_shared<Globals>();
        EvaCompiler c(g);
        syntax::eva_parser p;
        auto co{c.compile(p.parse(R"#((+ 1 1))#"), "test")};
        CHECK_CPPNUMBER(co->constants.size(), 1);
    }

    {
        auto g = std::make_shared<Globals>();
        EvaCompiler c(g);
        syntax::eva_parser p;
        auto co{c.compile(p.parse(R"#((+ "hello" "hello"))#"), "test")};
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

    CHECK_NUMBER(vm.exec(R"#(
    (if (> 5 10) 1 2)
    )#"),
                 2);

    CHECK_NUMBER(vm.exec(R"#(
    (if (< 5 10) 1 2)
    )#"),
                 1);

    CHECK_NUMBER(vm.exec(R"#(
        (var x (+ PI 7))
        x
    )#"),
                 10.1415);

    CHECK_NUMBER(vm.exec(R"#(
        (+ 1 1)
        (+ 2 2)
        (+ 3 3)
    )#"),
                 6);

    CHECK_NUMBER(vm.exec(R"#(
        (var z 10)
        (- z 11)
    )#"),
                 -1);

    CHECK_NUMBER(vm.exec(R"#(
        (set x 5)
        (begin
            (var x 100)
            (set x (+ 3 5))
        )
    )#"),
                 8);

    CHECK_NUMBER(vm.exec(R"#(
    (var i 10)
    (var count 0)
    (while (> i 0)
        (begin
            (set count (+ count 2))
            (set i (- i 1))
        )
    )
    count
    )#"),
                 20);

    CHECK_NUMBER(vm.exec(R"#(
    (square 8)
    )#"),
                 64);

    CHECK_NUMBER(vm.exec(R"#(
    (def foo (a b)
        (begin
            (var x 10)
            (+ a b)
        ))
    (foo 2 3)
    )#"),
                 5);

    CHECK_NUMBER(vm.exec(R"#(
    (def factorial (x)
        (if (= x 1)
            1
            (* x (factorial(- x 1)))
        ))
    (factorial 5)
    )#"),
                 120);

    CHECK_NUMBER(vm.exec(R"#(
    (def innerFunction (x)
        (begin
            (def sum (a b) (+ a b))
            (sum x 10)
        )
    )
    (innerFunction 10)
    )#"),
                 20);

    //    CHECK_NUMBER(vm.exec(R"#(
    //    (begin
    //        (var count 0)
    //        (for (var i 0) (< i 10) (set i (+ i 1))
    //            (begin
    //                (set count (+ count 2))
    //            )
    //        )
    //        count
    //    )
    //    )#"),
    //                 20);

    std::cout << "All tests passed\n";
    Traceable::printStats();
    std::cout << std::endl;
}
