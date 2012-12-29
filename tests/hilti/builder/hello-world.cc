//
// @TEST-EXEC: ${CXX} -o a.out `${HILTI_CONFIG} --compiler --cxxflags --ldflags --libs` tests/hilti/builder/hello-world.cc 
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output
//

#include <memory>

#include <hilti/hilti.h>

using namespace hilti;

int main(int argc, char** argv)
{
    auto ctx = std::make_shared<CompilerContext>();
    auto m = std::make_shared<builder::ModuleBuilder>(ctx, "HelloWorld");

    m->importModule("Hilti");

    m->pushFunction("main");
    auto args = builder::tuple::create( { builder::string::create("Hello, world!") } );
    m->builder()->addInstruction(instruction::flow::CallVoid, builder::string::create("Hilti::print"), args);
    m->popFunction();

    m->finalize();
    ctx->print(m->module(), std::cout);
}
