#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
        (< 3 5)
    )");

    std::cout << toString(result) << '\n';

    return 0;
}
