#include "evavm.h"

int main() {
    EvaVM vm;
    auto result = vm.exec(R"(
        (+ "Hello" "world")
    )");

    log(result.asNumber());

    return 0;
}
