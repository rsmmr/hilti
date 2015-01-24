
#include <hilti.h>
#include <binpac++.h>
#include <jit/libhilti-jit.h>

extern "C" {
#include "binpacshark.h"
}

typedef hlt_list* (*binpac_parsers_func)(hlt_exception** excpt, hlt_execution_context* ctx);
binpac_parsers_func JitParsers = nullptr;

static bool jitPac2(const std::list<string>& pac2)
{
    ::hilti::init();
    ::binpac::init();

    auto hilti_options = std::make_shared<::hilti::Options>();
    auto pac2_options = std::make_shared<::binpac::Options>();

    hilti_options->jit = true;
    hilti_options->debug = false;
    pac2_options->jit = true;
    pac2_options->debug = false;
    pac2_options->record_offsets = false; // TODO: Something crashes when this is enabled.

    auto pac2_context = std::make_shared<::binpac::CompilerContext>(pac2_options);

    std::list<llvm::Module* > llvm_modules;

    for ( auto p : pac2 ) {
        auto llvm_module = pac2_context->compile(p);

        if ( ! llvm_module ) {
            fprintf(stderr, "compiling %p failed\n", p.c_str());
            return false;
        }

        llvm_modules.push_back(llvm_module);
    }

    auto linked_module = pac2_context->linkModules("<jit analyzers>", llvm_modules);

    if ( ! linked_module ) {
        fprintf(stderr, "linking failed\n");
        return false;
    }

    auto hilti_context = pac2_context->hiltiContext();
    auto ee = hilti_context->jitModule(linked_module);

    if ( ! ee ) {
        fprintf(stderr, "jit failed");
        return false;
    }

    hlt_config cfg = *hlt_config_get();
    cfg.fiber_stack_size = 5000 * 1024;
    cfg.profiling = 0;
    cfg.num_workers = 2;
    hlt_config_set(&cfg);

    hlt_init_jit(hilti_context, linked_module, ee);
    binpac_init();
    binpac_init_jit(hilti_context, linked_module, ee);

    auto func = hilti_context->nativeFunction(linked_module, ee, "binpac_parsers");

    if ( ! func ) {
        fprintf(stderr, "jitBinpacParser error: no function 'binpac_parsers'");
        return false;
    }

    JitParsers = (binpac_parsers_func)func;

    return true;
}

hlt_list* pac2_get_parsers()
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    assert(JitParsers);
    return (*JitParsers)(&excpt, ctx);
}

void pac2_init()
{
    const char* env = getenv("BINPACSHARK");

    if ( ! (env && *env) ) {
        fprintf(stderr, "Set environment variable BINPACSHARK to space-separated list of *.pac2 to load.\n");
        return;
    }

    std::list<string> pac2 = ::util::strsplit(env);

    if ( ! jitPac2(pac2) ) {
        fprintf(stderr, "binpacshark: compile error\n");
        exit(1);
    }
}

