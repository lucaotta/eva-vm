#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
//        (var x 5)
//        (set x (+ x 10))
//        x
//        (begin
//            (var x 100)
//            (begin
//                (var x 200)
//                x
//            )
//            x
//        )
//        x

//    (begin
//        (var x 100)
//        (var z 700)
//        (var xy 2)
//        z
//    )

    (begin
        (var x 100)
        (var z 200)
        (var xy 3)
        xy
    )
    5
    )");

    std::cout << toString(result) << '\n';

    return 0;
}
