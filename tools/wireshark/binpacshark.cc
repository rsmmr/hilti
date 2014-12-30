
#include <hilti.h>
#include <binpac++.h>
#include <jit/libhilti-jit.h>

extern "C" {
#include "binpacshark.h"
}

typedef hlt_list* (*binpac_parsers_func)(hlt_exception** excpt, hlt_execution_context* ctx);
binpac_parsers_func JitParsers = nullptr;
binpac::CompilerContext* PacContext = nullptr;

static bool jitPac2(const std::list<string>& pac2, std::shared_ptr<binpac::Options> options)
{
    hilti::init();
    binpac::init();

    PacContext = new binpac::CompilerContext(options);
    options->record_offsets = true;
    options->debug = true;

    std::list<llvm::Module* > llvm_modules;

    for ( auto p : pac2 ) {
        auto llvm_module = PacContext->compile(p);

        if ( ! llvm_module ) {
            fprintf(stderr, "compiling %p failed\n", p.c_str());
            return false;
        }

        llvm_modules.push_back(llvm_module);
    }

    auto linked_module = PacContext->linkModules("<jit analyzers>", llvm_modules);

    if ( ! linked_module ) {
        fprintf(stderr, "linking failed\n");
        return false;
    }

    auto ee = PacContext->hiltiContext()->jitModule(linked_module);

    if ( ! ee ) {
        fprintf(stderr, "jit failed");
        return false;
    }

    // TODO: This should be done by jitModule, which however then needs to
    // move into hilti-jit.
    hlt_init_jit(PacContext->hiltiContext(), linked_module, ee);
    binpac_init();
    binpac_init_jit(PacContext->hiltiContext(), linked_module, ee);

    auto func = PacContext->hiltiContext()->nativeFunction(linked_module, ee, "binpac_parsers");

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
    hlt_config cfg = *hlt_config_get();
    hlt_config_set(&cfg);

    const char* env = getenv("BINPACSHARK");

    if ( ! (env && *env) ) {
        fprintf(stderr, "Set environment variable BINPACSHARK to space-separated list of *.pac2 to load.\n");
        return;
    }

    std::list<string> pac2 = ::util::strsplit(env);

    auto options = std::make_shared<binpac::Options>();

    options->jit = true;

    if ( ! jitPac2(pac2, options) ) {
        fprintf(stderr, "binpacshark: compile error\n");
        exit(1);
    }
}

