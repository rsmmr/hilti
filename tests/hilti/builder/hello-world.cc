//
// @TEST-EXEC: ${CXX} -g -O0 -o a.out %INPUT `${HILTI_CONFIG} --compiler --cxxflags --ldflags --libs`
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output
//

#include <memory>

#include <hilti/hilti.h>

int main(int argc, char** argv)
{
    hilti::init();

    auto options = std::make_shared<hilti::Options>();
    shared_ptr<hilti::CompilerContext> ctx = std::make_shared<hilti::CompilerContext>(options);
    auto m = std::make_shared<hilti::builder::ModuleBuilder>(ctx, "Main");

    m->importModule("Hilti");

    m->pushFunction("run");
    auto args = hilti::builder::tuple::create( { hilti::builder::string::create("Hello, world!") } );
    m->builder()->addInstruction(hilti::instruction::flow::CallVoid, hilti::builder::id::create("Hilti::print"), args);
    m->popFunction();

    m->finalize();
    ctx->print(m->module(), std::cout);
}
