
#ifndef HILTI_CODEGEN_SYMBOLS_H
#define HILTI_CODEGEN_SYMBOLS_H

namespace hilti {
namespace codegen {

/// Namespace for the name of symbols generated/examined by code generator
/// and linker.
namespace symbols {
// Names of meta data examined by AssemblyAnnotationWriter.
static const char* MetaComment = "hlt.comment";

// Names of globals examined by custom linker pass.
static const char* MetaModule = "hlt.module";
static const char* MetaGlobals = "hlt.globals";
static const char* MetaModuleName = "hlt.module.id";
static const char* MetaModuleInit = "hlt.modules.init";
static const char* MetaGlobalsInit = "hlt.globals.init";
static const char* MetaGlobalsDtor = "hlt.globals.dtor";
static const char* MetaHookDecls = "hlt.hook.decls";
static const char* MetaHookImpls = "hlt.hook.impls";

static const char* TypeGlobals = "hlt.globals.type";
static const char* FuncGlobalsBase = "hlt.globals.base";

// Indices of fields in MetaModule.
static const int MetaModuleVersion = 0;
static const int MetaModuleID = 1;
static const int MetaModuleFile = 2;
static const int MetaModuleExecutionContextType = 3;

// Symbols created by the linker for access by libhilti.
static const char* FunctionGlobalsInit = "__hlt_globals_init";
static const char* FunctionGlobalsDtor = "__hlt_globals_dtor";
static const char* FunctionModulesInit = "__hlt_modules_init";
static const char* FunctionGlobalsSize = "__hlt_globals_size";

// Names for argument added internally for our calling conventions.
static const char* ArgExecutionContext = "__ctx";
static const char* ArgException = "__excpt";
}
}
}

#endif
