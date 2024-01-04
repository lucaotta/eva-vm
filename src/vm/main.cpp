#include "evavm.h"

int main()
{
    EvaValue result = BOOLEAN(false);
    {
        EvaVM vm;
        Traceable::printStats();
        /*
         * 1. Code object in VM
         * 2. "hello" string
         * 3. ", world" string
         * 4. "hello, world" string
         * 5. Native function `square`
         */
        result = vm.exec(R"(
    (+ "hello" ", world")
    )");
        Traceable::printStats();
        std::cout << "Exit correctly, value " << toString(result) << '\n';
    }
    Traceable::printStats();

    return 0;
}
