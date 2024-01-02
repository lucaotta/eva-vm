#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
        (set PI (+ PI 7))
    )");

    std::cout << toString(result) << '\n';

    return 0;
}
