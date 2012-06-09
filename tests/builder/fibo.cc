
#include <hilti.h>

using namespace hilti;

int main(int argc, char** argv)
{
    XXXX

    builder::ModuleBuilder m("HelloWorld");

    auto main = builder::function::create(builder::id::create("main"));

    m.pushFunction(main);

    builder::tuple::element_list args = { builder::string::create("Hello, world!") };
    m->addInstruction(instruction::Call, builder::string::create("Hilti::print", args))

    m.popFunction();

    m.finalize();

    printModule(m.module());
}
