
#include <hilti.h>

using namespace hilti;

int main(int argc, char** argv)
{
    init();

    builder::ModuleBuilder module("Main");

    module.importModule("Hilti");

    module.pushFunction("run");

    auto str = builder::string::create("Hello, world!");

    auto op1 = builder::id::create("Hilti::print");
    auto op2 = builder::tuple::create( { str } );
    module.builder()->addInstruction(instruction::flow::CallVoid, op1, op2);

    module.popFunction();

    if ( ! module.finalize() )
        return 1;

    module.print(std::cout);
}
