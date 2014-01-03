//
// @TEST-EXEC: ${CXX} -g -O0 -o a.out %INPUT `${HILTI_CONFIG} --compiler --runtime --cxxflags --ldflags --libs` -lhilti-jit -std=c++11
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output
//

#include <memory>

#include <hilti/hilti.h>
#include <hilti/jit/libhilti-jit.h>

extern "C" {
#include <libhilti/libhilti.h>
}

int main(int argc, char** argv)
{
    hilti::init();

    hilti::Options options;
    shared_ptr<hilti::CompilerContext> ctx = std::make_shared<hilti::CompilerContext>(options);
    auto m = std::make_shared<hilti::builder::ModuleBuilder>(ctx, "Main");

    m->importModule("Hilti");

    m->pushFunction("run");
    auto args = hilti::builder::tuple::create( { hilti::builder::string::create("Hello, world!") } );
    m->builder()->addInstruction(hilti::instruction::flow::CallVoid, hilti::builder::id::create("Hilti::print"), args);
    m->popFunction();

    m->finalize();

    // Compile and link into LLVM.

    auto llvm_module = ctx->compile(m->module());

    if ( ! llvm_module ) {
        fprintf(stderr, "compile failed\n");
        return 1;
    }

    auto linked_module = ctx->linkModules("a.out", { llvm_module });

    if ( ! linked_module ) {
        fprintf(stderr, "link failed\n");
        return 1;
    }

    // JIT to native code.

    auto ee = ctx->jitModule(linked_module);

    if ( ! ee ) {
        fprintf(stderr, "jit failed\n");
        return 1;
    }

    // Execute.

    hlt_init_jit(ctx, linked_module, ee);

    auto main_run = (void (*)(hlt_exception**, hlt_execution_context*)) ctx->nativeFunction(linked_module, ee, "main_run");

    if ( ! main_run ) {
        fprintf(stderr, "nativeFunction failed\n");
        return 1;
    }

    hlt_exception* excpt = 0;
    hlt_execution_context* ectx = hlt_global_execution_context();
    (*main_run)(&excpt, ectx);

    return 0;
}
