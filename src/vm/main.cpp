#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
        (var i 10)
        (var count 0)
        (while (> i 0)
            (begin
                (set count (+ count 2))
                (set i (- i 1))
            )
        )
        count
    )");

    std::cout << toString(result) << '\n';

    return 0;
}
