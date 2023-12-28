#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
        42
    )");

    log(result.asNumber());

    return 0;
}