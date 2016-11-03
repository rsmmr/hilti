
// TODO: This is getting very messy. The Manager needs a refactoring
// to split out the Spicy part and get rid of the PIMPLing.

#include <memory>

#include <glob.h>

extern "C" {
#include <libspicy/libspicy.h>
}

#undef DBG_LOG

// Bro includes.
#include <Desc.h>
#include <Func.h>
#include <ID.h>
#include <NetVar.h>
#include <RE.h>
#include <Scope.h>
#include <analyzer/Analyzer.h>
#include <analyzer/Manager.h>
#include <bro-config.h>
#include <net_util.h>

#undef List

// HILTI/Spicy includes.
#include <ast/declaration.h>
#include <spicy/spicy.h>
#include <spicy/declaration.h>
#include <spicy/expression.h>
#include <spicy/function.h>
#include <spicy/jit.h>
#include <spicy/scope.h>
#include <spicy/statement.h>
#include <spicy/type.h>
#include <hilti/hilti.h>
#include <hilti/jit.h>

// LLVM includes.
#include <llvm/ExecutionEngine/ExecutionEngine.h>

// Plugin includes.
#include "Converter.h"
#include "LocalReporter.h"
#include "Manager.h"
#include "SpicyAST.h"
#include "SpicyAnalyzer.h"
#include "SpicyFileAnalyzer.h"
#include "Plugin.h"
#include "compiler/Compiler.h"
#include "compiler/ConversionBuilder.h"
#include "compiler/ModuleBuilder.h"

#include "consts.bif.h"
#include "events.bif.h"
#include "functions.bif.h"

using namespace bro::hilti;
using namespace spicy;

using std::shared_ptr;
using std::string;

static string transportToString(TransportProto p)
{
    switch ( p ) {
    case TRANSPORT_TCP:
        return "tcp";
    case TRANSPORT_UDP:
        return "udp";
    case TRANSPORT_ICMP:
        return "icmp";
    case TRANSPORT_UNKNOWN:
        return "<unknown>";
    default:
        return "<not supported>";
    }
}

struct Port {
    uint32 port;
    TransportProto proto;

    Port(uint32 port = 0, TransportProto proto = TRANSPORT_UNKNOWN) : port(port), proto(proto)
    {
    }
    operator string() const
    {
        return ::util::fmt("%u/%s", port, transportToString(proto));
    }
};

// Description of a Spicy protocol analyzer.
struct bro::hilti::SpicyAnalyzerInfo {
    string location;            // Location where the analyzer was defined.
    string name;                // Name of the analyzer.
    analyzer::Tag tag;          // analyzer::Tag for this analyzer.
    TransportProto proto;       // The transport layer the analyzer uses.
    std::list<Port> ports;      // The ports associated with the analyzer.
    string replaces;            // Name of another analyzer this one replaces.
    analyzer::Tag replaces_tag; // Name of the analyzer replaced translated into a tag.
    string
        unit_name_orig; // The fully-qualified name of the unit type to parse the originator side.
    string
        unit_name_resp; // The fully-qualified name of the unit type to parse the originator side.
    shared_ptr<SpicyAST::UnitInfo> unit_orig; // The type of the unit to parse the originator side.
    shared_ptr<SpicyAST::UnitInfo> unit_resp; // The type of the unit to parse the originator side.
    spicy_parser* parser_orig; // The parser for the originator side (coming out of JIT).
    spicy_parser* parser_resp; // The parser for the responder side (coming out of JIT).
};

// Description of a Spicy file analyzer.
struct bro::hilti::SpicyFileAnalyzerInfo {
    string location;                    // Location where the analyzer was defined.
    string name;                        // Name of the analyzer.
    file_analysis::Tag tag;             // file_analysis::Tag for this analyzer.
    std::list<string> mime_types;       // The mime_types associated with the analyzer.
    string unit_name;                   // The fully-qualified name of the unit type to parse with.
    shared_ptr<SpicyAST::UnitInfo> unit; // The type of the unit to parse the originator side.
    spicy_parser* parser;              // The parser coming out of JIT.
};

// XXX
struct bro::hilti::SpicyExpressionAccessor {
    int nr;                                     // Position of this expression in argument list.
    string expr;                                // The string representation of the expression.
    bool dollar_id;                             // True if this is a "magic" $-ID.
    shared_ptr<::spicy::Type> btype = nullptr; // The Spicy type of the expression.
    shared_ptr<::hilti::Type> htype = nullptr;  // The corresponding HILTI type of the expression.
    std::shared_ptr<::spicy::declaration::Function> spicy_func =
        nullptr; // Implementation of function that evaluates the expression.
    std::shared_ptr<::hilti::declaration::Function> hlt_func =
        nullptr; // Declaration of a function that evaluates the expression.
};

// Description of a Spicy module.
struct bro::hilti::SpicyModuleInfo {
    string path;                                   // The path the module was read from.
    shared_ptr<::spicy::CompilerContext> context; // The context used for the module.
    shared_ptr<::spicy::Module> module;           // The module itself.
    shared_ptr<ValueConverter> value_converter;
    std::list<shared_ptr<::hilti::ID>> dep_types; // Types we need to import into the HILTI module.

    // The BroFunc_*.hlt module.
    shared_ptr<::hilti::CompilerContext> hilti_context;
    shared_ptr<compiler::ModuleBuilder> hilti_mbuilder;

    // The BroHooks_*.spicy module.
    shared_ptr<::spicy::Module> spicy_module;
    shared_ptr<::hilti::Module> spicy_hilti_module;
};

// Description of an event defined by an *.evt file
struct bro::hilti::SpicyEventInfo {
    typedef std::list<shared_ptr<SpicyExpressionAccessor>> accessor_list;

    // Information parsed directly from the *.evt file.
    string file;             // The path of the *.evt file we parsed this from.
    string name;             // The name of the event.
    string path;             // The hook path as specified in the evt file.
    string condition;        // Condition that must be true for the event to trigger.
    std::list<string> exprs; // The argument expressions.
    string hook;             // The name of the hook triggering the event.
    int priority;            // Event/hook priority.
    string location;         // Location string where event is defined.

    // Computed information.
    string unit;       // The fully qualified name of the unit type.
    string hook_local; // The local part of the triggering hook (i.e., w/o the unit name).
    shared_ptr<::spicy::type::Unit> unit_type; // The Spicy type of referenced unit.
    shared_ptr<::spicy::Module> unit_module;   // The module the referenced unit is defined in.
    shared_ptr<::spicy::declaration::Hook> spicy_hook;      // The generated Spicy hook.
    shared_ptr<::hilti::declaration::Function> hilti_raise; // The generated HILTI raise() function.
    shared_ptr<SpicyModuleInfo> minfo;                       // The module the event was defined in.
    BroType* bro_event_type;                                // The type of the Bro event.
    EventHandlerPtr bro_event_handler; // The type of the corresponding Bro event. Set only if we
                                       // have a handler.
    accessor_list expr_accessors;      // One HILTI function per expression to access the value.
};

// Implementation of the Manager class attributes.
struct Manager::PIMPL {
    typedef std::list<shared_ptr<SpicyModuleInfo>> spicy_module_list;
    typedef std::list<shared_ptr<SpicyEventInfo>> spicy_event_list;
    typedef std::list<shared_ptr<SpicyAnalyzerInfo>> spicy_analyzer_list;
    typedef std::list<shared_ptr<SpicyFileAnalyzerInfo>> spicy_file_analyzer_list;
    typedef std::vector<shared_ptr<SpicyAnalyzerInfo>> spicy_analyzer_vector;
    typedef std::vector<shared_ptr<SpicyFileAnalyzerInfo>> spicy_file_analyzer_vector;
    typedef std::list<shared_ptr<::hilti::Module>> hilti_module_list;
    typedef std::list<std::unique_ptr<llvm::Module>> llvm_module_list;
    typedef std::set<std::string> path_set;

    std::shared_ptr<::hilti::Options> hilti_options = nullptr;
    std::shared_ptr<::spicy::Options> spicy_options = nullptr;

    bool compile_all;            // Compile all event code, even if no handler, set from
                                 // BifConst::Hilti::compile_all.
    bool compile_scripts;        // Activate the Bro script compiler.
    bool dump_debug;             // Output debug summary, set from BifConst::Hilti::dump_debug.
    bool dump_code;              // Output generated code, set from BifConst::Hilti::dump_code.
    bool dump_code_all;          // Output all code, set from BifConst::Hilti::dump_code_all.
    bool dump_code_pre_finalize; // Output generated code before finalizing the module, set from
                                 // BifConst::Hilti::dump_code_pre_finalize.
    bool save_spicy;              // Saves all generated Spicy modules into a file, set from
                                 // BifConst::Hilti::save_spicy.
    bool save_hilti; // Saves all HILTI modules into a file, set from BifConst::Hilti::save_hilti.
    bool save_llvm;  // Saves the final linked LLVM code into a file, set from
                     // BifConst::Hilti::save_llvm.
    bool spicy_to_compiler; // If compiling scripts, raise event hooks from Spicy code directly.
    unsigned int profile;  // True to enable run-time profiling.
    unsigned int hilti_workers; // Number of HILTI worker threads to spawn.

    std::list<string> import_paths;
    SpicyAST* spicy_ast;
    string libbro_path;
    compiler::Compiler* compiler;

    spicy_module_list spicy_modules;     // All loaded modules. Indexed by their paths.
    spicy_event_list spicy_events;       // All events found in the *.evt files.
    spicy_analyzer_list spicy_analyzers; // All analyzers found in the *.evt files.
    spicy_analyzer_vector
        spicy_analyzers_by_subtype; // All analyzers indexed by their analyzer::Tag subtype.
    spicy_file_analyzer_list spicy_file_analyzers; // All file analyzers found in the *.evt files.
    spicy_file_analyzer_vector spicy_file_analyzers_by_subtype; // All file analyzers indexed by their
                                                              // file_analysis::Tag subtype.
    path_set evt_files;                                       // All loaded *.evt files.
    path_set spicy_files;                                      // All loaded *.spicy files.
    path_set hlt_files; // All loaded *.hlt files specified by the user.

    shared_ptr<::hilti::CompilerContextJIT<::spicy::JIT>> hilti_context = nullptr;
    shared_ptr<::spicy::CompilerContext> spicy_context = nullptr;

    // All compiled LLVM modules. These will eventually be linked into
    // the final code.
    llvm_module_list llvm_modules;

    // All HILTI modules (loaded and compiled, including
    // intermediaries.). This is for debugging/printing only, they won't
    // be used further.
    hilti_module_list hilti_modules;
    shared_ptr<TypeConverter> type_converter;

    // The execution engine used for JITing llvm_linked_module.
    std::unique_ptr<::hilti::JIT> jit;

    // Pointers to compiled script functions indxed by their unique ID.
    std::vector<void*> native_functions;
};

Manager::Manager()
{
    pimpl = new PIMPL;
    pre_scripts_init_run = false;
    post_scripts_init_run = false;

    pimpl->spicy_ast = new SpicyAST;

    char* dir = getenv("BRO_SPICY_PATH");
    if ( dir )
        AddLibraryPath(dir);
}

Manager::~Manager()
{
    delete pimpl->spicy_ast;
    delete pimpl;
}

void Manager::AddLibraryPath(const char* dirs)
{
    assert(! pre_scripts_init_run);
    assert(! post_scripts_init_run);

    for ( auto dir : ::util::strsplit(dirs, ":") ) {
        pimpl->import_paths.push_back(dir);
        add_to_bro_path(dir);
    }
}

void Manager::InitMembers()
{
}

bool Manager::InitPreScripts()
{
    PLUGIN_DBG_LOG(HiltiPlugin, "Beginning pre-script initialization");

    assert(! pre_scripts_init_run);
    assert(! post_scripts_init_run);

    ::hilti::init();
    ::spicy::init_spicy();

    add_input_file("init-bare-hilti.bro");

    pre_scripts_init_run = true;

    pimpl->hilti_options = std::make_shared<::hilti::Options>();
    pimpl->spicy_options = std::make_shared<::spicy::Options>();

    for ( auto dir : pimpl->import_paths ) {
        pimpl->hilti_options->libdirs_hlt.push_back(dir);
        pimpl->spicy_options->libdirs_hlt.push_back(dir);
        pimpl->spicy_options->libdirs_spicy.push_back(dir);
    }

    pimpl->spicy_context = std::make_shared<::spicy::CompilerContext>(pimpl->spicy_options);
    pimpl->hilti_context = pimpl->spicy_context->hiltiContext();
    pimpl->compiler = new compiler::Compiler(pimpl->hilti_context);
    pimpl->type_converter = std::make_shared<TypeConverter>(pimpl->compiler);

    pimpl->libbro_path = pimpl->hilti_context->searchModule(::hilti::builder::id::node("LibBro"));

    PLUGIN_DBG_LOG(HiltiPlugin, "Done with pre-script initialization");

    return true;
}

bool Manager::InitPostScripts()
{
    PLUGIN_DBG_LOG(HiltiPlugin, "Beginning post-script initialization");

    assert(pre_scripts_init_run);
    assert(! post_scripts_init_run);

    std::set<string> cg_debug;

    for ( auto t : ::util::strsplit(BifConst::Hilti::cg_debug->CheckString(), ":") )
        cg_debug.insert(t);

    pimpl->compile_all = BifConst::Hilti::compile_all;
    pimpl->compile_scripts = BifConst::Hilti::compile_scripts;
    pimpl->profile = BifConst::Hilti::profile;
    pimpl->dump_debug = BifConst::Hilti::dump_debug;
    pimpl->dump_code = BifConst::Hilti::dump_code;
    pimpl->dump_code_pre_finalize = BifConst::Hilti::dump_code_pre_finalize;
    pimpl->dump_code_all = BifConst::Hilti::dump_code_all;
    pimpl->save_spicy = BifConst::Hilti::save_spicy;
    pimpl->save_hilti = BifConst::Hilti::save_hilti;
    pimpl->save_llvm = BifConst::Hilti::save_llvm;
    pimpl->spicy_to_compiler = BifConst::Hilti::spicy_to_compiler;
    pimpl->hilti_workers = BifConst::Hilti::hilti_workers;

    pimpl->hilti_options->jit = true;
    pimpl->hilti_options->debug = true;
    pimpl->hilti_options->optimize = BifConst::Hilti::optimize;
    pimpl->hilti_options->profile = BifConst::Hilti::profile;
    pimpl->hilti_options->verify = ! BifConst::Hilti::no_verify;
    pimpl->hilti_options->cg_debug = cg_debug;
    pimpl->hilti_options->module_cache = BifConst::Hilti::use_cache ? ".cache" : "";

    pimpl->spicy_options->jit = true;
    pimpl->spicy_options->debug = BifConst::Hilti::debug;
    pimpl->spicy_options->optimize = BifConst::Hilti::optimize;
    pimpl->spicy_options->profile = BifConst::Hilti::profile;
    pimpl->spicy_options->verify = ! BifConst::Hilti::no_verify;
    pimpl->spicy_options->cg_debug = cg_debug;
    pimpl->spicy_options->module_cache = BifConst::Hilti::use_cache ? ".cache" : "";

    pimpl->jit = nullptr;

    for ( auto a : pimpl->spicy_analyzers ) {
        if ( a->replaces.empty() )
            continue;

        analyzer::Tag tag = analyzer_mgr->GetAnalyzerTag(a->replaces.c_str());

        if ( tag ) {
            PLUGIN_DBG_LOG(HiltiPlugin, "Disabling %s for %s", a->replaces.c_str(),
                           a->name.c_str());

            analyzer_mgr->DisableAnalyzer(tag);
            a->replaces_tag = tag;
        }

        else {
            PLUGIN_DBG_LOG(HiltiPlugin, "%s replaces %s, but that does not exist", a->name.c_str(),
                           a->replaces.c_str());
        }
    }

    post_scripts_init_run = true;

    PLUGIN_DBG_LOG(HiltiPlugin, "Done with post-script initialization");

    return true;
}

bool Manager::FinishLoading()
{
    assert(pre_scripts_init_run);
    assert(post_scripts_init_run);

    pre_scripts_init_run = true;

    return true;
}

bool Manager::LoadFile(const std::string& file)
{
    std::string path = SearchFile(file);

    if ( path.empty() ) {
        reporter::error(::util::fmt("cannot find file %s", file));
        return false;
    }

    if ( path.size() > 6 && path.substr(path.size() - 6) == ".spicy" ) {
        if ( pimpl->spicy_files.find(path) != pimpl->spicy_files.end() )
            // Already loaded.
            return true;

        pimpl->spicy_files.insert(path);
        return LoadSpicyModule(path);
    }

    if ( path.size() > 4 && path.substr(path.size() - 4) == ".evt" ) {
        if ( pimpl->evt_files.find(path) != pimpl->evt_files.end() )
            // Already loaded.
            return true;

        pimpl->evt_files.insert(path);
        return LoadSpicyEvents(path);
    }

    if ( path.size() > 4 && path.substr(path.size() - 4) == ".hlt" ) {
        if ( pimpl->hlt_files.find(path) != pimpl->hlt_files.end() )
            // Already loaded.
            return true;

        pimpl->hlt_files.insert(path);
        return pimpl->compiler->LoadExternalHiltiCode(path);
    }

    reporter::internal_error(::util::fmt("unknown file type passed to HILTI loader: %s", path));
    return false;
}

#if 0
bool Manager::SearchFiles(const char* ext, std::function<bool (std::istream& in, const string& path)> const & callback)
	{
	for ( auto dir : pimpl->import_paths )
		{
		glob_t g;
		string p = dir + "/*." + ext;

		PLUGIN_DBG_LOG(HiltiPlugin, "Searching %s", p.c_str());

		if ( glob(p.c_str(), 0, 0, &g) < 0 )
			continue;

		for ( int i = 0; i < g.gl_pathc; i++ )
			{
			string path = g.gl_pathv[i];

			std::ifstream in(path);

			if ( ! in )
				{
				reporter::error(::util::fmt("Cannot open %s", path));
				return false;
				}

			if ( ! callback(in, path) )
				return false;
			}
		}

	return true;
	}
#endif

std::string Manager::SearchFile(const std::string& file, const std::string& relative_to) const
{
    char cwd[PATH_MAX] = "\0";
    char rpath[PATH_MAX];
    string result = "";

    if ( file.empty() )
        goto done;

    if ( ! getcwd(cwd, PATH_MAX) ) {
        reporter::error("cannot get current working directory");
        return "";
    }

    if ( relative_to.size() ) {
        if ( chdir(relative_to.c_str()) < 0 )
            reporter::error(::util::fmt("cannot chdir to %s", relative_to));
    }

    if ( is_file(file) ) {
        result = realpath(file.c_str(), rpath) ? rpath : "";
        goto done;
    }

    if ( file[0] == '/' || is_dir(file) )
        goto done;

    for ( auto dir : pimpl->import_paths ) {
        std::string path = dir + "/" + file;

        if ( ! realpath(path.c_str(), rpath) )
            continue;

        if ( is_file(rpath) ) {
            result = rpath;
            goto done;
        }
    }

done:
    if ( cwd[0] ) {
        if ( chdir(cwd) < 0 )
            reporter::error(::util::fmt("cannot chdir back to %s", relative_to));
    }

    return result;
}

bool Manager::PopulateEvent(shared_ptr<SpicyEventInfo> ev)
{
    // If we find the path directly, it's a unit type; then add a "%done"
    // to form the hook name.
    auto uinfo = pimpl->spicy_ast->LookupUnit(ev->path);

    string hook;
    string hook_local;
    string unit;

    if ( uinfo ) {
        hook += ev->path + "::%done";
        hook_local = "%done";
        unit = ev->path;
    }

    else {
        // Strip the last element of the path, the remainder must
        // refer to a unit.
        auto p = ::util::strsplit(ev->path, "::");
        if ( p.size() ) {
            hook_local = p.back();
            p.pop_back();
            unit = ::util::strjoin(p, "::");
            uinfo = pimpl->spicy_ast->LookupUnit(unit);
            hook = ev->path;
        }
    }

    if ( ! uinfo ) {
        reporter::error(::util::fmt("unknown unit type in %s", hook));
        return 0;
    }

    ev->hook = hook;
    ev->hook_local = hook_local;
    ev->unit = unit;
    ev->unit_type = uinfo->unit_type;
    ev->unit_module = uinfo->unit_type->firstParent<::spicy::Module>();
    ev->minfo = uinfo->minfo;

    assert(ev->unit_module);

    if ( ! CreateExpressionAccessors(ev) )
        return false;

    return true;
}

bool Manager::Compile()
{
    PLUGIN_DBG_LOG(HiltiPlugin, "Beginning compilation");

    assert(pre_scripts_init_run);
    assert(post_scripts_init_run);

    if ( ! CompileBroScripts() )
        return false;

    for ( auto a : pimpl->spicy_analyzers ) {
        if ( a->unit_name_orig.size() ) {
            a->unit_orig = pimpl->spicy_ast->LookupUnit(a->unit_name_orig);

            if ( ! a->unit_orig ) {
                reporter::error(::util::fmt("unknown unit type %s with analyzer %s",
                                            a->unit_name_orig, a->name));
                return false;
            }
        }

        if ( a->unit_name_resp.size() ) {
            a->unit_resp = pimpl->spicy_ast->LookupUnit(a->unit_name_resp);

            if ( ! a->unit_resp ) {
                reporter::error(
                    ::util::fmt("unknown unit type with analyzer %s", a->unit_name_resp, a->name));
                return false;
            }
        }
    }

    for ( auto a : pimpl->spicy_file_analyzers ) {
        if ( a->unit_name.size() ) {
            a->unit = pimpl->spicy_ast->LookupUnit(a->unit_name);

            if ( ! a->unit ) {
                reporter::error(::util::fmt("unknown unit type %s with file analyzer %s",
                                            a->unit_name, a->name));
                return false;
            }
        }

        for ( auto mt : a->mime_types ) {
            val_list* vals = new val_list;
            vals->append(a->tag.AsEnumVal()->Ref());
            vals->append(new ::StringVal(mt));
            EventHandlerPtr handler = internal_handler("spicy_analyzer_for_mime_type");
            mgr.QueueEvent(handler, vals);
        }
    }

    // Create the Spicy hooks and accessor functions.
    for ( auto ev : pimpl->spicy_events ) {
        if ( ! CreateSpicyHook(ev.get()) )
            return false;
    }

    // Compile all the *.spicy modules.
    for ( auto m : pimpl->spicy_modules ) {
        // Compile the *.spicy module itself.
        shared_ptr<::hilti::Module> hilti_module_out;
        auto llvm_module = m->context->compile(m->module, &hilti_module_out);

        if ( ! llvm_module )
            return false;

        if ( pimpl->save_hilti && hilti_module_out ) {
            ofstream out(::util::fmt("bro.spicy.%s.hlt", hilti_module_out->id()->name()));
            pimpl->hilti_context->print(hilti_module_out, out);
            out.close();
        }

        pimpl->llvm_modules.push_back(std::move(llvm_module));
        // m->llvm_modules.push_back(llvm_module);

        // Compile the generated hooks *.spicy module.
        if ( pimpl->dump_code_pre_finalize ) {
            std::cerr << ::util::fmt("\n=== Pre-finalize AST: %s.spicy\n",
                                     m->spicy_module->id()->name())
                      << std::endl;
            pimpl->spicy_context->dump(m->spicy_module, std::cerr);
            std::cerr << ::util::fmt("\n=== Pre-finalize code: %s.spicy\n",
                                     m->spicy_module->id()->name())
                      << std::endl;
            pimpl->spicy_context->print(m->spicy_module, std::cerr);
        }

        if ( ! pimpl->spicy_context->finalize(m->spicy_module) )
            return false;

        if ( pimpl->dump_code_pre_finalize ) {
            std::cerr << ::util::fmt("\n=== Post-finalize, pre-compile AST: %s.spicy\n",
                                     m->spicy_module->id()->name())
                      << std::endl;
            pimpl->spicy_context->dump(m->spicy_module, std::cerr);
            std::cerr << ::util::fmt("\n=== Post-finalize, pre-compile code: %s.spicy\n",
                                     m->spicy_module->id()->name())
                      << std::endl;
            pimpl->spicy_context->print(m->spicy_module, std::cerr);
        }

        llvm_module = pimpl->spicy_context->compile(m->spicy_module, &m->spicy_hilti_module);

        if ( ! llvm_module )
            return false;

        pimpl->llvm_modules.push_back(std::move(llvm_module));
        // m->llvm_modules.push_back(llvm_module);

        if ( m->spicy_hilti_module )
            pimpl->hilti_modules.push_back(m->spicy_hilti_module);

        if ( pimpl->save_spicy ) {
            ofstream out(::util::fmt("bro.%s.spicy", m->spicy_module->id()->name()));
            pimpl->spicy_context->print(m->spicy_module, out);
            out.close();
        }

        AddHiltiTypesForModule(m);
    }

    for ( auto ev : pimpl->spicy_events )
        AddHiltiTypesForEvent(ev);

    for ( auto minfo : pimpl->spicy_modules )
        minfo->hilti_context->resolveTypes(minfo->hilti_mbuilder->module());

    // Create the HILTi raise functions().
    for ( auto ev : pimpl->spicy_events ) {
        BuildBroEventSignature(ev);
        RegisterBroEvent(ev);

        if ( ! CreateHiltiEventFunction(ev.get()) )
            return false;
    }

    // Add the standard LibBro module.

    if ( ! pimpl->libbro_path.size() ) {
        reporter::error("LibBro library module not found");
        return false;
    }

    PLUGIN_DBG_LOG(HiltiPlugin, "Loading %s", pimpl->libbro_path.c_str());

    auto libbro = pimpl->hilti_context->loadModule(pimpl->libbro_path);

    if ( ! libbro ) {
        reporter::error("loading LibBro library module failed");
        return false;
    }

    pimpl->hilti_modules.push_back(libbro);

    // Compile all the *.hlt modules.
    for ( auto m : pimpl->spicy_modules ) {
        AddHiltiTypesForModule(m);

        if ( pimpl->dump_code_pre_finalize ) {
            auto hilti_module = m->hilti_mbuilder->module();
            std::cerr << ::util::fmt("\n=== Pre-finalize AST: %s.hlt\n", hilti_module->id()->name())
                      << std::endl;
            m->hilti_context->dump(hilti_module, std::cerr);
            std::cerr << ::util::fmt("\n=== Pre-finalize code: %s.hlt\n",
                                     hilti_module->id()->name())
                      << std::endl;
            m->hilti_context->print(hilti_module, std::cerr);
        }

        auto hilti_module = m->hilti_mbuilder->Finalize();

        if ( ! hilti_module )
            return false;

        pimpl->hilti_modules.push_back(hilti_module);

        // TODO: Not sure why we need this import here.
        pimpl->hilti_context->importModule(std::make_shared<::hilti::ID>("LibBro"));
        pimpl->hilti_context->finalize(hilti_module);

        auto llvm_hilti_module = m->hilti_context->compile(hilti_module);

        if ( ! llvm_hilti_module ) {
            reporter::error("compiling LibBro library module failed");
            return false;
        }

        pimpl->llvm_modules.push_back(std::move(llvm_hilti_module));

        // m->llvm_modules.push_back(llvm_hilti_module);
        // pimpl->hilti_context->updateCache(m->key, m->llvm_modules);
    }

    if ( pimpl->save_hilti ) {
        for ( auto m : pimpl->hilti_modules ) {
            ofstream out(::util::fmt("bro.%s.hlt", m->id()->name()));
            pimpl->hilti_context->print(m, out);
            out.close();
        }
    }


    auto glue = pimpl->compiler->FinalizeGlueBuilder();

    if ( ! CompileHiltiModule(glue) )
        return false;

    // Compile and link all the HILTI modules into LLVM. We use the
    // Spicy context here to make sure we gets its additional
    // libraries linked.
    //
    PLUGIN_DBG_LOG(HiltiPlugin, "Compiling & linking all HILTI code into a single LLVM module");

    auto llvm_module =
        pimpl->spicy_context->linkModules("__bro_linked__", std::move(pimpl->llvm_modules));

    if ( ! llvm_module ) {
        reporter::error("linking failed");
        return false;
    }

    if ( pimpl->save_llvm ) {
        ofstream out("bro.ll");
        pimpl->hilti_context->printBitcode(llvm_module.get(), out);
        out.close();
    }

    llvm_module->setModuleIdentifier("__bro_linked__");

    auto result = RunJIT(std::move(llvm_module));
    PLUGIN_DBG_LOG(HiltiPlugin, "Done with compilation");

    PLUGIN_DBG_LOG(HiltiPlugin, "Registering analyzers through events");

    for ( auto a : pimpl->spicy_analyzers ) {
        for ( auto p : a->ports ) {
            val_list* vals = new val_list;
            vals->append(a->tag.AsEnumVal()->Ref());
            vals->append(new ::PortVal(p.port, p.proto));
            EventHandlerPtr handler = internal_handler("spicy_analyzer_for_port");
            mgr.QueueEvent(handler, vals);
        }
    }

    for ( auto a : pimpl->spicy_file_analyzers ) {
        for ( auto mt : a->mime_types ) {
            val_list* vals = new val_list;
            vals->append(a->tag.AsEnumVal()->Ref());
            vals->append(new ::StringVal(mt));
            EventHandlerPtr handler = internal_handler("spicy_analyzer_for_mime_type");
            mgr.QueueEvent(handler, vals);
        }
    }

    return result;
}

bool Manager::CompileBroScripts()
{
    compiler::Compiler::module_list modules = pimpl->compiler->CompileAll();

    for ( auto m : modules ) {
        if ( ! CompileHiltiModule(m) )
            return false;
    }

    return true;
}

bool Manager::CompileHiltiModule(std::shared_ptr<::hilti::Module> m)
{
    // TODO: Add caching.
    auto lm = pimpl->hilti_context->compile(m);

    if ( ! lm ) {
        reporter::error(::util::fmt("compiling script module %s failed", m->id()->name()));
        return false;
    }

    if ( pimpl->save_llvm ) {
        ofstream out(::util::fmt("bro.%s.ll", m->id()->name()));
        pimpl->hilti_context->printBitcode(lm.get(), out);
        out.close();
    }

    pimpl->llvm_modules.push_back(std::move(lm));
    pimpl->hilti_modules.push_back(m);
    return true;
}

bool Manager::RunJIT(std::unique_ptr<llvm::Module> llvm_module)
{
    PLUGIN_DBG_LOG(HiltiPlugin, "Initializing HILTI runtime");

    hlt_config cfg = *hlt_config_get();
    cfg.fiber_stack_size = 5000 * 1024;
    cfg.profiling = pimpl->profile;
    cfg.num_workers = pimpl->hilti_workers;
    hlt_config_set(&cfg);

    PLUGIN_DBG_LOG(HiltiPlugin, "Running JIT on LLVM module");

    auto jit = pimpl->hilti_context->jit(std::move(llvm_module));

    if ( ! jit ) {
        reporter::error("jit failed");
        return false;
    }

    pimpl->jit = std::move(jit);

    PLUGIN_DBG_LOG(HiltiPlugin, "Retrieving spicy_parsers() function");

#ifdef BRO_PLUGIN_HAVE_PROFILING
    profile_update(PROFILE_JIT_LAND, PROFILE_START);
#endif

    typedef hlt_list* (*spicy_parsers_func)(hlt_exception * *excpt, hlt_execution_context * ctx);
    auto spicy_parsers = (spicy_parsers_func)pimpl->jit->nativeFunction("spicy_parsers");

#ifdef BRO_PLUGIN_HAVE_PROFILING
    profile_update(PROFILE_JIT_LAND, PROFILE_STOP);
#endif

    if ( ! spicy_parsers ) {
        reporter::error("no function spicy_parsers()");
        return false;
    }

    PLUGIN_DBG_LOG(HiltiPlugin, "Calling spicy_parsers() function");

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    hlt_list* parsers = (*spicy_parsers)(&excpt, ctx);

    // Record them with our analyzers.
    ExtractParsers(parsers);

    // Cache the native functions for compiled script code.
    if ( pimpl->compile_scripts ) {
        PLUGIN_DBG_LOG(HiltiPlugin, "Caching native script functions");

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_JIT_LAND, PROFILE_START);
#endif

        for ( auto i : pimpl->compiler->HiltiFunctionSymbolMap() ) {
            auto symbol = i.first;
            auto func = i.second;

            auto native = pimpl->jit->nativeFunction(symbol);
            auto id = func->GetUniqueFuncID();

            if ( pimpl->native_functions.size() <= id )
                pimpl->native_functions.resize(id + 1);

            pimpl->native_functions[id] = native;

            PLUGIN_DBG_LOG(HiltiPlugin, "    %s -> %s at %p", func->Name(), symbol.c_str(), native);
        }

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_JIT_LAND, PROFILE_STOP);
#endif
    }

    // Done, print out debug summary if requested.

    if ( pimpl->dump_debug )
        DumpDebug();

    if ( pimpl->dump_code || pimpl->dump_code_all )
        DumpCode(pimpl->dump_code_all);

    // Need to delay the following until here. If we compile scripts, this
    // will require the compiled register_protocol_analyzer function to be
    // available.

    auto register_func = internal_func("Bro::register_protocol_analyzer");

    if ( ! register_func )
        reporter::fatal_error("Bro::register_protocol_analyzer() not available");

    for ( auto a : pimpl->spicy_file_analyzers ) {
        val_list* args = new val_list;
        args->append(a->tag.AsEnumVal()->Ref());
        register_func->Call(args);
    }

    return true;
}

bool Manager::LoadSpicyModule(const string& path)
{
    std::ifstream in(path);

    if ( ! in ) {
        reporter::error(::util::fmt("cannot open %s", path));
        return false;
    }

    PLUGIN_DBG_LOG(HiltiPlugin, "Loading grammar from from %s", path.c_str());

    // ctx->enableDebug(dbg_scanner, dbg_parser, dbg_scopes, dbg_grammars);

    reporter::push_location(path, 0);
    auto ctx = std::make_shared<::spicy::CompilerContext>(pimpl->spicy_options);
    ctx->hiltiContext()->setLLVMContext(&pimpl->hilti_context->llvmContext());

    auto module = ctx->load(path);
    reporter::pop_location();

    if ( ! module ) {
        reporter::error(::util::fmt("Error reading %s", path));
        return false;
    }

    auto name = module->id()->name();

    auto minfo = std::make_shared<SpicyModuleInfo>();
    minfo->path = path;
    minfo->context = ctx;
    minfo->module = module;

    minfo->spicy_module = std::make_shared<::spicy::Module>(pimpl->spicy_context.get(),
                                                            std::make_shared<::spicy::ID>(
                                                                ::util::fmt("BroHooks_%s", name)));
    minfo->spicy_module->import("Bro");

    minfo->hilti_context = std::make_shared<::hilti::CompilerContext>(pimpl->hilti_options);
    minfo->hilti_context->setLLVMContext(&pimpl->hilti_context->llvmContext());

    minfo->hilti_mbuilder =
        std::make_shared<compiler::ModuleBuilder>(pimpl->compiler, minfo->hilti_context,
                                                  ::util::fmt("BroFuncs_%s", name));
    minfo->hilti_mbuilder->importModule("Hilti");
    minfo->hilti_mbuilder->importModule("LibBro");
    minfo->value_converter =
        std::make_shared<ValueConverter>(minfo->hilti_mbuilder.get(), pimpl->type_converter.get());

    pimpl->spicy_modules.push_back(minfo);

    pimpl->spicy_ast->process(minfo, module);

    // Make exported enum types available to Bro.
    for ( auto t : module->exportedTypes() ) {
        if ( ! ast::rtti::isA<::spicy::type::Enum>(t) )
            continue;

        auto htype = pimpl->spicy_context->hiltiType(t, &minfo->dep_types);
        pimpl->type_converter->Convert(htype, t);
    }

    return true;
}

static void eat_spaces(const string& chunk, size_t* i)
{
    while ( *i < chunk.size() && isspace(chunk[*i]) )
        ++*i;
}

static bool is_id_char(const string& chunk, size_t i)
{
    char c = chunk[i];

    if ( isalnum(c) )
        return true;

    if ( strchr("_$%", c) != 0 )
        return true;

    char prev = (i > 0) ? chunk[i - 1] : '\0';
    char next = (i + 1 < chunk.size()) ? chunk[i + 1] : '\0';

    if ( c == ':' && next == ':' )
        return true;

    if ( c == ':' && prev == ':' )
        return true;

    return false;
}

static bool extract_id(const string& chunk, size_t* i, string* id)
{
    eat_spaces(chunk, i);

    size_t j = *i;

    while ( j < chunk.size() && is_id_char(chunk, j) )
        ++j;

    if ( *i == j )
        goto error;

    *id = chunk.substr(*i, j - *i);
    *i = j;

    return true;

error:
    reporter::error(::util::fmt("expected id"));
    return false;
}

static bool is_path_char(const string& chunk, size_t i)
{
    char c = chunk[i];
    return (! isspace(c)) && c != ';';
}

static bool extract_path(const string& chunk, size_t* i, string* id)
{
    eat_spaces(chunk, i);

    size_t j = *i;

    while ( j < chunk.size() && is_path_char(chunk, j) )
        ++j;

    if ( *i == j )
        goto error;

    *id = chunk.substr(*i, j - *i);
    *i = j;

    return true;

error:
    reporter::error(::util::fmt("expected path"));
    return false;
}

static bool extract_int(const string& chunk, size_t* i, int* integer)
{
    string sint;

    eat_spaces(chunk, i);

    size_t j = *i;

    if ( j < chunk.size() && (chunk[j] == '-' || chunk[j] == '+') )
        ++j;

    while ( j < chunk.size() && isdigit(chunk[j]) )
        ++j;

    if ( *i == j )
        goto error;

    sint = chunk.substr(*i, j - *i);
    *i = j;
    ::util::atoi_n(sint.begin(), sint.end(), 10, integer);

    return true;

error:
    reporter::error(::util::fmt("expected integer"));
    return false;
}

#if 0
static bool extract_dotted_id(const string& chunk, size_t* i, string* id)
	{
	eat_spaces(chunk, i);

	size_t j = *i;

	while ( j < chunk.size() && (is_id_char(chunk, j) || chunk[j] == '.') )
		++j;

	if ( *i == j )
		goto error;

	*id = chunk.substr(*i, j - *i);
	*i = j;

	return true;

error:
	reporter::error(::util::fmt("expected dotted id"));
	return false;

	}
#endif

static bool extract_expr(const string& chunk, size_t* i, string* expr)
{
    eat_spaces(chunk, i);

    int level = 0;
    bool done = 0;
    size_t j = *i;

    while ( j < chunk.size() ) {
        switch ( chunk[j] ) {
        case '(':
        case '[':
        case '{':
            ++level;
            ++j;
            continue;

        case ')':
            if ( level == 0 ) {
                done = true;
                break;
            }

        // fall-through

        case ']':
        case '}':
            if ( level == 0 )
                goto error;

            --level;
            ++j;
            continue;

        case ',':
            if ( level == 0 ) {
                done = true;
                break;
            }

        // fall-through

        default:
            ++j;
        }

        if ( done )
            break;

        if ( *i == j )
            break;
    }

    *expr = ::util::strtrim(chunk.substr(*i, j - *i));
    *i = j;

    return true;

error:
    reporter::error(::util::fmt("expected Spicy expression"));
    return false;
}

static string::size_type looking_at(const string& chunk, string::size_type i, const char* token)
{
    eat_spaces(chunk, &i);

    for ( string::size_type j = 0; j < strlen(token); ++j ) {
        if ( i >= chunk.size() || chunk[i++] != token[j] )
            return 0;
    }

    return i;
}

static bool eat_token(const string& chunk, string::size_type* i, const char* token)
{
    eat_spaces(chunk, i);

    auto j = looking_at(chunk, *i, token);

    if ( ! j ) {
        reporter::error(::util::fmt("expected token '%s'", token));
        return false;
    }

    *i = j;
    return true;
}

static bool extract_port(const string& chunk, size_t* i, Port* port)
{
    eat_spaces(chunk, i);

    string s;
    size_t j = *i;

    while ( j < chunk.size() && isdigit(chunk[j]) )
        ++j;

    if ( *i == j )
        goto error;

    s = chunk.substr(*i, j - *i);
    ::util::atoi_n(s.begin(), s.end(), 10, &port->port);

    *i = j;

    if ( chunk[*i] != '/' )
        goto error;

    (*i)++;

    if ( looking_at(chunk, *i, "tcp") ) {
        port->proto = TRANSPORT_TCP;
        eat_token(chunk, i, "tcp");
    }

    else if ( looking_at(chunk, *i, "udp") ) {
        port->proto = TRANSPORT_UDP;
        eat_token(chunk, i, "udp");
    }

    else if ( looking_at(chunk, *i, "icmp") ) {
        port->proto = TRANSPORT_ICMP;
        eat_token(chunk, i, "icmp");
    }

    else
        goto error;

    return true;

error:
    reporter::error(::util::fmt("cannot parse expected port specification"));
    return false;
}

bool Manager::LoadSpicyEvents(const string& path)
{
    std::ifstream in(path);

    if ( ! in ) {
        reporter::error(::util::fmt("cannot open %s", path));
        return false;
    }

    PLUGIN_DBG_LOG(HiltiPlugin, "Loading events from %s", path.c_str());

    int lineno = 0;
    string chunk;
    PIMPL::spicy_event_list new_events;

    while ( ! in.eof() ) {
        reporter::push_location(path, ++lineno);

        string line;
        std::getline(in, line);

        // Skip comments and empty lines.
        auto i = line.find_first_not_of(" \t");
        if ( i == string::npos )
            goto next_line;

        if ( line[i] == '#' )
            goto next_line;

        if ( chunk.size() )
            chunk += " ";

        chunk += line.substr(i, string::npos);

        // See if we have a semicolon-terminated chunk now.
        i = line.find_last_not_of(" \t");
        if ( i == string::npos )
            i = line.size() - 1;

        if ( line[i] != ';' )
            // Nope, keep adding.
            goto next_line;

        // Got it, parse the chunk.

        chunk = ::util::strtrim(chunk);

        if ( looking_at(chunk, 0, "protocol") ) {
            auto a = ParseSpicyAnalyzerSpec(chunk);

            if ( ! a )
                goto error;

            pimpl->spicy_analyzers.push_back(a);
            RegisterBroAnalyzer(a);
            PLUGIN_DBG_LOG(HiltiPlugin, "Finished processing analyzer definition for %s",
                           a->name.c_str());
        }


        else if ( looking_at(chunk, 0, "file") ) {
            auto a = ParseSpicyFileAnalyzerSpec(chunk);

            if ( ! a )
                goto error;

            pimpl->spicy_file_analyzers.push_back(a);
            RegisterBroFileAnalyzer(a);
            PLUGIN_DBG_LOG(HiltiPlugin, "Finished processing file analyzer definition for %s",
                           a->name.c_str());
        }

        else if ( looking_at(chunk, 0, "on") ) {
            auto ev = ParseSpicyEventSpec(chunk);

            if ( ! ev )
                goto error;

            ev->file = path;

            new_events.push_back(ev);
            pimpl->spicy_events.push_back(ev);
            HiltiPlugin.AddEvent(ev->name);

            PLUGIN_DBG_LOG(HiltiPlugin, "Finished processing event definition for %s",
                           ev->name.c_str());
        }

        else if ( looking_at(chunk, 0, "grammar") ) {
            size_t i = 0;

            if ( ! eat_token(chunk, &i, "grammar") )
                return 0;

            string spcy;

            if ( ! extract_path(chunk, &i, &spcy) )
                goto error;

            size_t j = spcy.find_last_of("/.");

            if ( j == string::npos || spcy[j] == '/' )
                spcy += ".spicy";

            string dirname;

            j = path.find_last_of("/");
            if ( j != string::npos ) {
                dirname = path.substr(0, j);
                pimpl->spicy_options->libdirs_spicy.push_front(dirname);
            }

            spcy = SearchFile(spcy, dirname);

            if ( ! HiltiPlugin.LoadBroFile(spcy.c_str()) )
                goto error;
        }

        else
            reporter::error("expected 'grammar', '{file,protocol} analyzer', or 'on'");

        chunk = "";

    next_line:
        reporter::pop_location();
        continue;

    error:
        reporter::pop_location();
        return false;
    }

    if ( chunk.size() ) {
        reporter::error("unterminated line at end of file");
        return false;
    }


    for ( auto ev : new_events )
        PopulateEvent(ev);

    return true;
}

shared_ptr<SpicyAnalyzerInfo> Manager::ParseSpicyAnalyzerSpec(const string& chunk)
{
    auto a = std::make_shared<SpicyAnalyzerInfo>();
    a->location = reporter::current_location();
    a->parser_orig = 0;
    a->parser_resp = 0;

    size_t i = 0;

    if ( ! eat_token(chunk, &i, "protocol") )
        return 0;

    if ( ! eat_token(chunk, &i, "analyzer") )
        return 0;

    if ( ! extract_id(chunk, &i, &a->name) )
        return 0;

    a->name = ::util::strreplace(a->name, "::", "_");

    if ( ! eat_token(chunk, &i, "over") )
        return 0;

    string proto;

    if ( ! extract_id(chunk, &i, &proto) )
        return 0;

    proto = ::util::strtolower(proto);

    if ( proto == "tcp" )
        a->proto = TRANSPORT_TCP;

    else if ( proto == "udp" )
        a->proto = TRANSPORT_UDP;

    else if ( proto == "icmp" )
        a->proto = TRANSPORT_ICMP;

    else {
        reporter::error(::util::fmt("unknown transport protocol '%s'", proto));
        return 0;
    }

    if ( ! eat_token(chunk, &i, ":") )
        return 0;

    enum { orig, resp, both } dir;

    while ( true ) {
        if ( looking_at(chunk, i, "parse") ) {
            eat_token(chunk, &i, "parse");

            if ( looking_at(chunk, i, "originator") ) {
                eat_token(chunk, &i, "originator");
                dir = orig;
            }

            else if ( looking_at(chunk, i, "responder") ) {
                eat_token(chunk, &i, "responder");
                dir = resp;
            }

            else if ( looking_at(chunk, i, "with") )
                dir = both;

            else {
                reporter::error("invalid parse-with specification");
                return 0;
            }

            if ( ! eat_token(chunk, &i, "with") )
                return 0;

            string unit;

            if ( ! extract_id(chunk, &i, &unit) )
                return 0;

            switch ( dir ) {
            case orig:
                a->unit_name_orig = unit;
                break;

            case resp:
                a->unit_name_resp = unit;
                break;

            case both:
                a->unit_name_orig = unit;
                a->unit_name_resp = unit;
                break;
            }
        }

        else if ( looking_at(chunk, i, "ports") ) {
            eat_token(chunk, &i, "ports");

            if ( ! eat_token(chunk, &i, "{") )
                return 0;

            while ( true ) {
                Port p;

                if ( ! extract_port(chunk, &i, &p) )
                    return 0;

                a->ports.push_back(p);

                if ( looking_at(chunk, i, "}") ) {
                    eat_token(chunk, &i, "}");
                    break;
                }

                if ( ! eat_token(chunk, &i, ",") )
                    return 0;
            }
        }

        else if ( looking_at(chunk, i, "port") ) {
            eat_token(chunk, &i, "port");

            Port p;

            if ( ! extract_port(chunk, &i, &p) )
                return 0;

            a->ports.push_back(p);
        }

        else if ( looking_at(chunk, i, "replaces") ) {
            eat_token(chunk, &i, "replaces");

            string id;

            if ( ! extract_id(chunk, &i, &a->replaces) )
                return 0;
        }

        else {
            reporter::error("unexpect token");
            return 0;
        }

        if ( looking_at(chunk, i, ";") )
            break; // All done.

        if ( ! eat_token(chunk, &i, ",") )
            return 0;
    }

    return a;
}

shared_ptr<SpicyFileAnalyzerInfo> Manager::ParseSpicyFileAnalyzerSpec(const string& chunk)
{
    auto a = std::make_shared<SpicyFileAnalyzerInfo>();
    a->location = reporter::current_location();
    a->parser = 0;

    size_t i = 0;

    if ( ! eat_token(chunk, &i, "file") )
        return 0;

    if ( ! eat_token(chunk, &i, "analyzer") )
        return 0;

    if ( ! extract_id(chunk, &i, &a->name) )
        return 0;

    a->name = ::util::strreplace(a->name, "::", "_");

    if ( ! eat_token(chunk, &i, ":") )
        return 0;

    while ( true ) {
        if ( looking_at(chunk, i, "parse") ) {
            eat_token(chunk, &i, "parse");

            if ( ! eat_token(chunk, &i, "with") )
                return 0;

            string unit;

            if ( ! extract_id(chunk, &i, &unit) )
                return 0;

            a->unit_name = unit;
        }

        else if ( looking_at(chunk, i, "mime-type") ) {
            eat_token(chunk, &i, "mime-type");

            string mtype;

            if ( ! extract_path(chunk, &i, &mtype) )
                return 0;

            a->mime_types.push_back(mtype);
        }

        else {
            reporter::error("unexpect token");
            return 0;
        }

        if ( looking_at(chunk, i, ";") )
            break; // All done.

        if ( ! eat_token(chunk, &i, ",") )
            return 0;
    }

    return a;
}

shared_ptr<SpicyEventInfo> Manager::ParseSpicyEventSpec(const string& chunk)
{
    auto ev = std::make_shared<SpicyEventInfo>();

    std::list<string> exprs;

    string path;
    string name;
    string expr;
    string cond;

    // We use a quite negative hook here to make sure these run last
    // after anything the grammar defines by default.
    int prio = -1000;

    size_t i = 0;

    if ( ! eat_token(chunk, &i, "on") )
        return 0;

    if ( ! extract_id(chunk, &i, &path) )
        return 0;

    if ( looking_at(chunk, i, "if") ) {
        eat_token(chunk, &i, "if");

        if ( ! eat_token(chunk, &i, "(") )
            return 0;

        if ( ! extract_expr(chunk, &i, &cond) )
            return 0;

        if ( ! eat_token(chunk, &i, ")") )
            return 0;
    }

    if ( ! eat_token(chunk, &i, "->") )
        return 0;

    if ( ! eat_token(chunk, &i, "event") )
        return 0;

    if ( ! extract_id(chunk, &i, &name) )
        return 0;

    if ( ! eat_token(chunk, &i, "(") )
        return 0;

    bool first = true;
    size_t j = 0;

    while ( true ) {
        j = looking_at(chunk, i, ")");

        if ( j ) {
            i = j;
            break;
        }

        if ( ! first ) {
            if ( ! eat_token(chunk, &i, ",") )
                return 0;
        }

        if ( ! extract_expr(chunk, &i, &expr) )
            return 0;

        exprs.push_back(expr);
        first = false;
    }

    if ( looking_at(chunk, i, "&priority") ) {
        eat_token(chunk, &i, "&priority");

        if ( ! eat_token(chunk, &i, "=") )
            return 0;

        if ( ! extract_int(chunk, &i, &prio) )
            return 0;
    }

    if ( ! eat_token(chunk, &i, ";") )
        return 0;

    eat_spaces(chunk, &i);

    if ( i < chunk.size() ) {
        // This shouldn't actually be possible ...
        reporter::error("unexpected characters at end of line");
        return 0;
    }

    ev->name = name;
    ev->path = path;
    ev->exprs = exprs;
    ev->condition = cond;
    ev->priority = prio;
    ev->location = reporter::current_location();

    // These are set later.
    ev->hook = "<unset>";
    ev->hook_local = "<unset>";
    ev->unit = "<unset>";
    ev->unit_type = 0;
    ev->unit_module = 0;

    return ev;
}

void Manager::RegisterBroAnalyzer(shared_ptr<SpicyAnalyzerInfo> a)
{
    analyzer::Tag::subtype_t stype = pimpl->spicy_analyzers_by_subtype.size();
    pimpl->spicy_analyzers_by_subtype.push_back(a);
    a->tag = HiltiPlugin.AddAnalyzer(a->name, a->proto, stype);
}

void Manager::RegisterBroFileAnalyzer(shared_ptr<SpicyFileAnalyzerInfo> a)
{
    file_analysis::Tag::subtype_t stype = pimpl->spicy_file_analyzers_by_subtype.size();
    pimpl->spicy_file_analyzers_by_subtype.push_back(a);
    a->tag = HiltiPlugin.AddFileAnalyzer(a->name, stype);
}

void Manager::BuildBroEventSignature(shared_ptr<SpicyEventInfo> ev)
{
    type_decl_list* types = new type_decl_list();

    EventHandlerPtr handler = event_registry->Lookup(ev->name.c_str());

    if ( handler ) {
        // To enable scoped event names, export their IDs implicitly. For the
        // lookup we pretend to be in the right module so that Bro doesn't
        // tell us the ID isn't exported (doh!).
        auto n = ::util::strsplit(ev->name, "::");
        auto mod = n.size() > 1 ? n.front() : GLOBAL_MODULE_NAME;
        auto id = lookup_ID(ev->name.c_str(), mod.c_str());

        if ( id )
            id->SetExport();
    }

#if 0
	ODesc d;
	if ( handler )
		handler->FType()->Describe(&d);
#endif

    int i = 0;

    for ( auto e : ev->expr_accessors ) {
        BroType* t = nullptr;

        if ( e->expr == "$conn" ) {
            t = internal_type("connection");
            Ref(t);
            types->append(new TypeDecl(t, strdup("c")));
        }

        else if ( e->expr == "$file" ) {
            t = internal_type("fa_file");
            Ref(t);
            types->append(new TypeDecl(t, strdup("f")));
        }

        else if ( e->expr == "$is_orig" )
            types->append(new TypeDecl(base_type(TYPE_BOOL), strdup("is_orig")));

        else {
            string p;
            BroType* t = 0;

            if ( handler ) {
                p = handler->FType()->Args()->FieldName(i);
                t = handler->FType()->Args()->FieldType(i);
                Ref(t);
            }
            else {
                p = util::fmt("arg%d", e->nr);
                t = pimpl->type_converter->Convert(e->htype, e->btype);
            }

            types->append(new TypeDecl(t, strdup(p.c_str())));
        }

        i++;
    }

    auto ftype = new FuncType(new RecordType(types), 0, FUNC_FLAVOR_EVENT);
    ev->bro_event_type = ftype;
}

#if 0
// Have patched Bro to allow field names to not match.
static bool records_compatible(::RecordType* rt1, ::RecordType* rt2)
	{
	// Cannot use same_type() here as that also compares the field names,
	// which won't match our dummy names.

	if ( rt1->NumFields() != rt2->NumFields() )
		return false;

	for ( int i = 0; i < rt1->NumFields(); ++i )
		{
		const TypeDecl* td1 = rt1->FieldDecl(i);
		const TypeDecl* td2 = rt2->FieldDecl(i);

		if ( ! same_type(td1->type, td2->type, 0) )
			return false;
		}

	return true;
	}
#endif

void Manager::InstallTypeMappings(shared_ptr<compiler::ModuleBuilder> mbuilder, ::BroType* t1,
                                  ::BroType* t2)
{
    assert(t1->Tag() == t2->Tag());

    if ( t1->Tag() == TYPE_RECORD ) {
        mbuilder->MapType(t1, t2);
        return;
    }

    if ( t1->Tag() == TYPE_VECTOR )
        InstallTypeMappings(mbuilder, t1->AsVectorType()->YieldType(),
                            t2->AsVectorType()->YieldType());

    if ( t1->Tag() == TYPE_TABLE ) {
        if ( t1->AsTableType()->YieldType() )
            InstallTypeMappings(mbuilder, t1->AsTableType()->YieldType(),
                                t2->AsTableType()->YieldType());
    }
}

void Manager::RegisterBroEvent(shared_ptr<SpicyEventInfo> ev)
{
    assert(ev->bro_event_type);

    EventHandlerPtr handler = event_registry->Lookup(ev->name.c_str());

    if ( handler ) {
        if ( handler->FType() ) {
            // Check if the event types are compatible. Also,
            // for record arguments we want to use the one from
            // the Bro event, rather than ours, which might have
            // just dummy field names. So we go through
            // (recursively for containers) and install mappings
            // as needed.
            auto a1 = ev->bro_event_type->AsFuncType()->Args();
            auto a2 = handler->FType()->Args();

            if ( a1->NumFields() != a2->NumFields() ) {
                EventSignatureMismatch(ev->name, ev->bro_event_type->AsFuncType(), handler->FType(),
                                       -1);
                return;
            }

            for ( int i = 0; i < a1->NumFields(); i++ ) {
                auto t1 = a1->FieldType(i);
                auto t2 = a2->FieldType(i);

                if ( ! same_type(t1, t2, 0, false) ) {
                    EventSignatureMismatch(ev->name, t1, t2, i);
                    return;
                }

                InstallTypeMappings(ev->minfo->hilti_mbuilder, t1, t2);
            }
        }

        ev->bro_event_handler = handler;
    }

    else
        ev->bro_event_handler = 0;

#ifdef DEBUG
    ODesc d;
    d.SetShort();
    ev->bro_event_type->Describe(&d);
    const char* handled = (ev->bro_event_handler ? "has handler" : "no handlers");
    PLUGIN_DBG_LOG(HiltiPlugin, "New Bro event '%s: %s' (%s)", ev->name.c_str(), d.Description(),
                   handled);
#endif
}

void Manager::EventSignatureMismatch(const string& name, const ::BroType* have,
                                     const ::BroType* want, int arg)
{
    ODesc dhave;
    ODesc dwant;

    if ( arg < 0 ) {
        dhave.SetShort();
        dwant.SetShort();
    }

    have->Describe(&dhave);
    want->Describe(&dwant);

    auto sarg = (arg >= 0 ? ::util::fmt("argument %d of ", arg) : "");

    auto l = want->GetLocationInfo();
    reporter::__push_location(l->filename, l->first_line);
    reporter::error(::util::fmt("type mismatch for %sevent handler %s: expected %s, but got %s",
                                sarg, name, dwant.Description(), dhave.Description()));
    reporter::pop_location();
    return;
}

static shared_ptr<::spicy::Expression> id_expr(const string& id)
{
    return std::make_shared<::spicy::expression::ID>(std::make_shared<::spicy::ID>(id));
}

bool Manager::CreateSpicyHook(SpicyEventInfo* ev)
{
    if ( ! WantEvent(ev) )
        return true;

    // Find the Spicy module that this event belongs to.
    PLUGIN_DBG_LOG(HiltiPlugin, "Adding Spicy hook %s for event %s", ev->hook.c_str(),
                   ev->name.c_str());

    ev->minfo->spicy_module->import(ev->unit_module->id());

    auto body =
        std::make_shared<::spicy::statement::Block>(ev->minfo->spicy_module->body()->scope());

    // If the event comes with a condition, evaluate that first.
    if ( ev->condition.size() ) {
        auto cond = pimpl->spicy_context->parseExpression(ev->condition);

        if ( ! cond ) {
            reporter::error(
                ::util::fmt("error parsing conditional expression '%s'", ev->condition));
            return false;
        }

        ::spicy::expression_list args = {cond};
        auto not_ =
            std::make_shared<::spicy::expression::UnresolvedOperator>(::spicy::operator_::Not,
                                                                       args);
        auto return_ = std::make_shared<::spicy::statement::Return>(nullptr);
        auto if_ = std::make_shared<::spicy::statement::IfElse>(not_, return_);

        body->addStatement(if_);
    }

    // Raise the event.

    ::spicy::expression_list args_tuple;

    for ( auto m : ev->unit_type->scope()->map() ) {
        auto n = (m.first != "$$" ? m.first : "__dollardollar");
        auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

        if ( t )
            args_tuple.push_back(id_expr(n));
    }

    auto args = std::make_shared<::spicy::expression::Constant>(
        std::make_shared<::spicy::constant::Tuple>(args_tuple));

    auto raise_name =
        ::util::fmt("%s::raise_%s_%p", ev->minfo->hilti_mbuilder->module()->id()->name(),
                    ::util::strreplace(ev->name, "::", "_"), ev);
    ::spicy::expression_list op_args = {id_expr(raise_name), args};
    auto call =
        std::make_shared<::spicy::expression::UnresolvedOperator>(::spicy::operator_::Call,
                                                                   op_args);
    auto stmt = std::make_shared<::spicy::statement::Expression>(call);

    body->addStatement(stmt);

    auto hook = std::make_shared<::spicy::Hook>(body, ::spicy::Hook::PARSE, ev->priority);
    auto hdecl =
        std::make_shared<::spicy::declaration::Hook>(std::make_shared<::spicy::ID>(ev->hook),
                                                      hook);

    auto raise_result =
        std::make_shared<::spicy::type::function::Result>(std::make_shared<::spicy::type::Void>(),
                                                           true);
    ::spicy::parameter_list raise_params;

    for ( auto m : ev->unit_type->scope()->map() ) {
        auto n = (m.first != "$$" ? m.first : "__dollardollar");
        auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

        if ( t ) {
            auto id = std::make_shared<::spicy::ID>(n);
            auto p = std::make_shared<::spicy::type::function::Parameter>(id, t->type(), false,
                                                                           false, nullptr);
            raise_params.push_back(p);
        }
    }

    auto raise_type =
        std::make_shared<::spicy::type::Function>(raise_result, raise_params,
                                                   ::spicy::type::function::SPICY_HILTI);
    auto raise_func =
        std::make_shared<::spicy::Function>(std::make_shared<::spicy::ID>(raise_name), raise_type,
                                             ev->minfo->spicy_module);
    auto rdecl = std::make_shared<::spicy::declaration::Function>(raise_func,
                                                                   ::spicy::Declaration::IMPORTED);

    ev->minfo->spicy_module->body()->addDeclaration(hdecl);
    ev->minfo->spicy_module->body()->addDeclaration(rdecl);

    ev->spicy_hook = hdecl;

    return true;
}

bool Manager::CreateExpressionAccessors(shared_ptr<SpicyEventInfo> ev)
{
    int nr = 0;

    for ( auto e : ev->exprs ) {
        auto acc = std::make_shared<SpicyExpressionAccessor>();
        acc->nr = ++nr;
        acc->expr = e;
        acc->dollar_id = util::startsWith(e, "$");

        if ( ! acc->dollar_id )
            // We set the other fields below in a second loop.
            acc->spicy_func = CreateSpicyExpressionAccessor(ev, acc->nr, e);

        else {
            if ( e != "$conn" && e != "$is_orig" && e != "$file" ) {
                reporter::error(::util::fmt("unsupported parameters %s", e));
                return false;
            }
        }

        ev->expr_accessors.push_back(acc);
    }

    // Resolve the code as far possible.
    ev->minfo->spicy_module->import(ev->unit_module->id());
    pimpl->spicy_context->partialFinalize(ev->minfo->spicy_module);

    for ( auto acc : ev->expr_accessors ) {
        if ( acc->dollar_id )
            continue;

        acc->btype =
            acc->spicy_func ? acc->spicy_func->function()->type()->result()->type() : nullptr;
        acc->htype = acc->btype ?
                         pimpl->spicy_context->hiltiType(acc->btype, &ev->minfo->dep_types) :
                         nullptr;
        acc->hlt_func = DeclareHiltiExpressionAccessor(ev, acc->nr, acc->htype);
    }

    return true;
}

shared_ptr<spicy::declaration::Function> Manager::CreateSpicyExpressionAccessor(
    shared_ptr<SpicyEventInfo> ev, int nr, const string& expr)
{
    auto fname =
        ::util::fmt("accessor_%s_arg%d_%p", ::util::strreplace(ev->name, "::", "_"), nr, ev);

    PLUGIN_DBG_LOG(HiltiPlugin, "Defining Spicy function %s for parameter %d of event %s",
                   fname.c_str(), nr, ev->name.c_str());

    auto spicy_expr = pimpl->spicy_context->parseExpression(expr);

    if ( ! spicy_expr ) {
        reporter::error(::util::fmt("error parsing expression '%s'", expr));
        return nullptr;
    }

    auto stmt = std::make_shared<::spicy::statement::Return>(spicy_expr);
    auto body =
        std::make_shared<::spicy::statement::Block>(ev->minfo->spicy_module->body()->scope());
    body->addStatement(stmt);

    auto unknown = std::make_shared<::spicy::type::Unknown>();
    auto func_result = std::make_shared<::spicy::type::function::Result>(unknown, true);

    ::spicy::parameter_list func_params;

    for ( auto m : ev->unit_type->scope()->map() ) {
        auto n = (m.first != "$$" ? m.first : "__dollardollar");
        auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

        if ( t ) {
            auto id = std::make_shared<::spicy::ID>(n);
            auto p = std::make_shared<::spicy::type::function::Parameter>(id, t->type(), false,
                                                                           false, nullptr);
            func_params.push_back(p);
        }
    }

    auto ftype = std::make_shared<::spicy::type::Function>(func_result, func_params,
                                                            ::spicy::type::function::SPICY_HILTI);
    auto func = std::make_shared<::spicy::Function>(std::make_shared<::spicy::ID>(fname), ftype,
                                                     ev->minfo->spicy_module, body);
    auto fdecl =
        std::make_shared<::spicy::declaration::Function>(func, ::spicy::Declaration::EXPORTED);

    ev->minfo->spicy_module->body()->addDeclaration(fdecl);

    return fdecl;
}


shared_ptr<::hilti::declaration::Function> Manager::DeclareHiltiExpressionAccessor(
    shared_ptr<SpicyEventInfo> ev, int nr, shared_ptr<::hilti::Type> rtype)
{
    auto fname = ::util::fmt("%s::accessor_%s_arg%d_%p", ev->minfo->spicy_module->id()->name(),
                             ::util::strreplace(ev->name, "::", "_"), nr, ev);

    PLUGIN_DBG_LOG(HiltiPlugin, "Declaring HILTI function %s for parameter %d of event %s",
                   fname.c_str(), nr, ev->name.c_str());

    auto result = ::hilti::builder::function::result(rtype);

    ::hilti::builder::function::parameter_list func_params;

    for ( auto m : ev->unit_type->scope()->map() ) {
        auto n = (m.first != "$$" ? m.first : "__dollardollar");
        auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

        if ( t ) {
            auto id = ::hilti::builder::id::node(n);
            auto htype = pimpl->spicy_context->hiltiType(t->type(), &ev->minfo->dep_types);
            auto p = ::hilti::builder::function::parameter(id, htype, false, nullptr);
            func_params.push_back(p);
        }
    }

    auto cookie =
        ::hilti::builder::function::parameter("cookie",
                                              ::hilti::builder::type::byName("LibBro::SpicyCookie"),
                                              false, nullptr);
    func_params.push_back(cookie);

    auto func = ev->minfo->hilti_mbuilder->declareFunction(fname, result, func_params);
    ev->minfo->hilti_mbuilder->exportID(fname);

    return func;
}

void Manager::AddHiltiTypesForModule(shared_ptr<SpicyModuleInfo> minfo)
{
    for ( auto id : minfo->dep_types ) {
        if ( minfo->hilti_mbuilder->declared(id) )
            continue;

        assert(minfo->spicy_hilti_module);
        auto t = minfo->spicy_hilti_module->body()->scope()->lookup(id, true);
        assert(t.size() == 1);

        auto type = ast::rtti::checkedCast<::hilti::expression::Type>(t.front())->typeValue();
        minfo->hilti_mbuilder->addType(id, type);
    }
}

void Manager::AddHiltiTypesForEvent(shared_ptr<SpicyEventInfo> ev)
{
    auto uid = ::hilti::builder::id::node(ev->unit);

    if ( ev->minfo->hilti_mbuilder->declared(uid) )
        return;

    assert(ev->minfo->spicy_hilti_module);
    auto t = ev->minfo->spicy_hilti_module->body()->scope()->lookup(uid, true);
    assert(t.size() == 1);

    auto unit_type = ast::rtti::checkedCast<::hilti::expression::Type>(t.front())->typeValue();
    ev->minfo->hilti_mbuilder->addType(ev->unit, unit_type);
}

bool Manager::CreateHiltiEventFunction(SpicyEventInfo* ev)
{
    if ( ! WantEvent(ev) )
        return true;

    string fname = ::util::fmt("raise_%s_%p", ::util::strreplace(ev->name, "::", "_"), ev);

    PLUGIN_DBG_LOG(HiltiPlugin, "Adding HILTI function %s for event %s", fname.c_str(),
                   ev->name.c_str());

    auto result = ::hilti::builder::function::result(::hilti::builder::void_::type());

    ::hilti::builder::function::parameter_list args;

    for ( auto m : ev->unit_type->scope()->map() ) {
        auto n = (m.first != "$$" ? m.first : "__dollardollar");
        auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

        if ( t ) {
            auto id = ::hilti::builder::id::node(n);
            auto htype = pimpl->spicy_context->hiltiType(t->type(), &ev->minfo->dep_types);
            auto p = ::hilti::builder::function::parameter(id, htype, false, nullptr);
            args.push_back(p);
        }
    }

    auto cookie =
        ::hilti::builder::function::parameter("cookie",
                                              ::hilti::builder::type::byName("LibBro::SpicyCookie"),
                                              false, nullptr);
    args.push_back(cookie);

    auto mbuilder = ev->minfo->hilti_mbuilder;

    pimpl->compiler->pushModuleBuilder(mbuilder.get());

    auto func = mbuilder->pushFunction(fname, result, args);
    mbuilder->exportID(fname);

    mbuilder->builder()->addInstruction(::hilti::instruction::profiler::Start,
                                        ::hilti::builder::string::create(string("bro/") + fname));

    if ( pimpl->compile_scripts && pimpl->spicy_to_compiler )
        CreateHiltiEventFunctionBodyForHilti(ev);
    else
        CreateHiltiEventFunctionBodyForBro(ev);

    mbuilder->builder()->addInstruction(::hilti::instruction::profiler::Stop,
                                        ::hilti::builder::string::create(string("bro/") + fname));

    mbuilder->popFunction();

    // pimpl->compiler->popModuleBuilder();

    ev->hilti_raise = func;

    return true;
}

bool Manager::CreateHiltiEventFunctionBodyForBro(SpicyEventInfo* ev)
{
    auto mbuilder = ev->minfo->hilti_mbuilder;

    pimpl->hilti_context->resolveTypes(mbuilder->module());

    ::hilti::builder::tuple::element_list vals;

    int i = 0;

    for ( auto e : ev->expr_accessors ) {
        auto val = mbuilder->addTmp("val", ::hilti::builder::type::byName("LibBro::BroVal"));

        if ( e->expr == "$conn" ) {
            mbuilder->builder()->addInstruction(val, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_conn_val"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));
        }

        else if ( e->expr == "$file" ) {
            mbuilder->builder()->addInstruction(val, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_file_val"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));
        }

        else if ( e->expr == "$is_orig" ) {
            mbuilder->builder()->addInstruction(val, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_is_orig"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));
        }

        else {
            auto tmp = mbuilder->addTmp("t", e->htype);
            auto func_id =
                e->hlt_func ? e->hlt_func->id() : ::hilti::builder::id::node("null-function>");

            auto args = ::hilti::builder::tuple::element_list();

            for ( auto m : ev->unit_type->scope()->map() ) {
                auto n = (m.first != "$$" ? m.first : "__dollardollar");
                auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

                if ( t )
                    args.push_back(::hilti::builder::id::create(n));
            }

            args.push_back(::hilti::builder::id::create("cookie"));

            mbuilder->builder()->addInstruction(tmp, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(func_id),
                                                ::hilti::builder::tuple::create(args));

            ev->minfo->value_converter->Convert(tmp, val, e->btype,
                                                ev->bro_event_type->AsFuncType()->Args()->FieldType(
                                                    i));
        }

        vals.push_back(val);
        i++;
    }

    auto canon_name = ::util::strreplace(ev->name, "::", "_");
    auto handler = mbuilder->addGlobal(util::fmt("__bro_handler_%s_%p", canon_name, ev),
                                       ::hilti::builder::type::byName("LibBro::BroEventHandler"));

    auto cond = mbuilder->addTmp("no_handler", ::hilti::builder::boolean::type());
    mbuilder->builder()->addInstruction(cond, ::hilti::instruction::operator_::Equal, handler,
                                        ::hilti::builder::caddr::create());


    auto blocks = mbuilder->builder()->addIf(cond);
    auto block_true = std::get<0>(blocks);
    auto block_cont = std::get<1>(blocks);

    mbuilder->pushBuilder(block_true);
    mbuilder->builder()->addInstruction(handler, ::hilti::instruction::flow::CallResult,
                                        ::hilti::builder::id::create("LibBro::get_event_handler"),
                                        ::hilti::builder::tuple::create(
                                            {::hilti::builder::bytes::create(ev->name)}));
    mbuilder->builder()->addInstruction(::hilti::instruction::flow::Jump, block_cont->block());
    mbuilder->popBuilder(block_true);

    mbuilder->pushBuilder(block_cont);

    mbuilder->builder()->addInstruction(::hilti::instruction::flow::CallVoid,
                                        ::hilti::builder::id::create("LibBro::raise_event"),
                                        ::hilti::builder::tuple::create(
                                            {handler, ::hilti::builder::tuple::create(vals)}));

    return true;
}

bool Manager::CreateHiltiEventFunctionBodyForHilti(SpicyEventInfo* ev)
{
    assert(ev->bro_event_handler->LocalHandler());

    auto mbuilder = ev->minfo->hilti_mbuilder;
    auto conversion_builder = mbuilder->ConversionBuilder();

    ::hilti::builder::tuple::element_list args;

    int i = 0;

    for ( auto e : ev->expr_accessors ) {
        if ( e->expr == "$conn" ) {
            auto conn_val =
                mbuilder->addTmp("conn_val", ::hilti::builder::type::byName("LibBro::BroVal"));

            mbuilder->builder()->addInstruction(conn_val, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_conn_val"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));

            auto arg = conversion_builder->ConvertBroToHilti(conn_val, ::connection_type, false);
            args.push_back(arg);
        }

        else if ( e->expr == "$file" ) {
            auto fa_val =
                mbuilder->addTmp("file_val", ::hilti::builder::type::byName("LibBro::BroVal"));

            mbuilder->builder()->addInstruction(fa_val, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_file_val"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));

            auto arg = conversion_builder->ConvertBroToHilti(fa_val, ::fa_file_type, false);
            args.push_back(arg);
        }

        else if ( e->expr == "$is_orig" ) {
            auto arg = mbuilder->addTmp("is_orig", ::hilti::builder::boolean::type());

            mbuilder->builder()->addInstruction(arg, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(
                                                    "LibBro::cookie_to_is_orig_boolean"),
                                                ::hilti::builder::tuple::create(
                                                    {::hilti::builder::id::create("cookie")}));

            args.push_back(arg);
        }

        else {
            // TODO: If Spicy's and Bro's mapping to HILTI
            // don't match, we need to do more conversion here
            // ... Update: first example below for integers ...
            auto arg = mbuilder->addTmp("t", e->htype);
            auto func_id =
                e->hlt_func ? e->hlt_func->id() : ::hilti::builder::id::node("null-function>");

            auto eargs = ::hilti::builder::tuple::element_list();

            for ( auto m : ev->unit_type->scope()->map() ) {
                auto n = (m.first != "$$" ? m.first : "__dollardollar");
                auto t = ::ast::rtti::tryCast<::spicy::expression::ParserState>(m.second->front());

                if ( t )
                    eargs.push_back(::hilti::builder::id::create(n));
            }

            eargs.push_back(::hilti::builder::id::create("cookie"));

            mbuilder->builder()->addInstruction(arg, ::hilti::instruction::flow::CallResult,
                                                ::hilti::builder::id::create(func_id),
                                                ::hilti::builder::tuple::create(eargs));

            if ( auto itype = ::ast::rtti::tryCast<::hilti::type::Integer>(e->htype) ) {
                if ( itype->width() != 64 ) {
                    auto arg64 = mbuilder->addTmp("t", ::hilti::builder::integer::type(64));
                    mbuilder->builder()->addInstruction(arg64, ::hilti::instruction::integer::ZExt,
                                                        arg);
                    arg = arg64;
                }
            }

            args.push_back(arg);
        }

        i++;
    }

    auto targs = ::hilti::builder::tuple::create(args);
    mbuilder->HiltiCallEventHilti(ev->bro_event_handler->LocalHandler(), targs, nullptr);

    return true;
}

void Manager::ExtractParsers(hlt_list* parsers)
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    std::map<string, spicy_parser*> parser_map;

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        spicy_parser* p = *(spicy_parser**)hlt_iterator_list_deref(i, &excpt, ctx);
        char* name = hlt_string_to_native(p->name, &excpt, ctx);
        parser_map.insert(std::make_pair(name, p));
        hlt_free(name);

        i = hlt_iterator_list_incr(i, &excpt, ctx);
    }

    for ( auto a : pimpl->spicy_analyzers ) {
        if ( a->unit_name_orig.size() ) {
            auto i = parser_map.find(a->unit_name_orig);

            if ( i != parser_map.end() ) {
                a->parser_orig = i->second;
                GC_CCTOR(a->parser_orig, hlt_SpicyHilti_Parser, ctx);
            }
        }

        if ( a->unit_name_resp.size() ) {
            auto i = parser_map.find(a->unit_name_resp);

            if ( i != parser_map.end() ) {
                a->parser_resp = i->second;
                GC_CCTOR(a->parser_resp, hlt_SpicyHilti_Parser, ctx);
            }
        }
    }

    for ( auto a : pimpl->spicy_file_analyzers ) {
        if ( a->unit_name.empty() )
            continue;

        auto i = parser_map.find(a->unit_name);

        if ( i != parser_map.end() ) {
            a->parser = i->second;
            GC_CCTOR(a->parser, hlt_SpicyHilti_Parser, ctx);
        }
    }

    for ( auto p : parser_map ) {
        GC_DTOR(p.second, hlt_SpicyHilti_Parser, ctx);
    }
}

string Manager::AnalyzerName(const analyzer::Tag& tag)
{
    return pimpl->spicy_analyzers_by_subtype[tag.Subtype()]->name;
}

string Manager::FileAnalyzerName(const file_analysis::Tag& tag)
{
    return pimpl->spicy_file_analyzers_by_subtype[tag.Subtype()]->name;
}

struct __spicy_parser* Manager::ParserForAnalyzer(const analyzer::Tag& tag, bool is_orig)
{
    if ( is_orig )
        return pimpl->spicy_analyzers_by_subtype[tag.Subtype()]->parser_orig;
    else
        return pimpl->spicy_analyzers_by_subtype[tag.Subtype()]->parser_resp;
}

struct __spicy_parser* Manager::ParserForFileAnalyzer(const file_analysis::Tag& tag)
{
    return pimpl->spicy_file_analyzers_by_subtype[tag.Subtype()]->parser;
}

analyzer::Tag Manager::TagForAnalyzer(const analyzer::Tag& tag)
{
    analyzer::Tag replaces = pimpl->spicy_analyzers_by_subtype[tag.Subtype()]->replaces_tag;
    return replaces ? replaces : tag;
}

std::pair<bool, Val*> Manager::RuntimeCallFunction(const Func* func, Frame* parent, val_list* args)
{
    Val* result = 0;

    if ( ! (func->HasBodies() || HaveCustomHandler(func)) )
        // For events raised directly, rather than being queued, we
        // arrive here. Ignore if we don't have a handler.
        //
        // TODO: Should these direct events have a separate entry
        // point in the plugin API?
        result = new Val(0, TYPE_VOID);
    else
        result = RuntimeCallFunctionInternal(func, args);

    if ( result ) {
        // We use a VOID value internally to be able to pass around a
        // non-null pointer. However, Bro's plugin API has changed,
        // it now wants null for a void function.
        if ( result->Type()->Tag() == TYPE_VOID ) {
            Unref(result);
            result = 0;
        }

        return std::pair<bool, Val*>(true, result);
    }

    else
        return std::pair<bool, Val*>(false, 0);
}


bool Manager::HaveCustomHandler(const ::Func* ev)
{
    return pimpl->compiler->HaveCustomHandler(ev);
}

bool Manager::HaveCustomHiltiCode()
{
    return pimpl->hlt_files.size();
}

bool Manager::RuntimeRaiseEvent(Event* event)
{
    auto efunc = event->Handler()->LocalHandler();

    if ( efunc && (efunc->HasBodies() || HaveCustomHandler(efunc)) ) {
        auto result = RuntimeCallFunctionInternal(efunc, event->Args());
        Unref(result);
    }

    val_list* args = event->Args();

    loop_over_list(*args, i) Unref((*args)[i]);

    Unref(event);

    return true;
}

::Val* Manager::RuntimeCallFunctionInternal(const ::Func* func, val_list* args)
{
    // TODO: We should cache the nativeFunction() lookup here between
    // calls to speed it up, unless that method itself is already as fast
    // as we would that get that way.

    assert(pimpl->jit);

    auto id = func->GetUniqueFuncID();

    void* native = 0;

    if ( pimpl->native_functions.size() <= id )
        pimpl->native_functions.resize(id + 1);

    native = pimpl->native_functions[id];

    if ( ! native ) {
        // First try again to get it, it could be a custom user
        // function that we haven't used yet.
        auto symbol = pimpl->compiler->HiltiStubSymbol(func, nullptr, true);
        native = pimpl->jit->nativeFunction(symbol);

        if ( native )
            pimpl->native_functions[id] = native;

        else {
            reporter::warning(::util::fmt("no HILTI function %s for %s", symbol, func->Name()));
            return new ::Val(0, ::TYPE_ERROR);
        }
    }

    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();
    __hlt_context_clear_exception(ctx); // TODO: HILTI should take care of this.

#ifdef BRO_PLUGIN_HAVE_PROFILING
    profile_update(PROFILE_HILTI_LAND, PROFILE_START);
#endif

    ::Val* result = nullptr;

    switch ( args->length() ) {
    case 0:
        typedef ::Val* (*e0)(hlt_exception**, hlt_execution_context*);
        result = (*(e0)native)(&excpt, ctx);
        break;

    case 1:
        typedef ::Val* (*e1)(Val*, hlt_exception**, hlt_execution_context*);
        result = (*(e1)native)((*args)[0], &excpt, ctx);
        break;

    case 2:
        typedef ::Val* (*e2)(Val*, Val*, hlt_exception**, hlt_execution_context*);
        result = (*(e2)native)((*args)[0], (*args)[1], &excpt, ctx);
        break;

    case 3:
        typedef ::Val* (*e3)(Val*, Val*, Val*, hlt_exception**, hlt_execution_context*);
        result = (*(e3)native)((*args)[0], (*args)[1], (*args)[2], &excpt, ctx);
        break;

    case 4:
        typedef ::Val* (*e4)(Val*, Val*, Val*, Val*, hlt_exception**, hlt_execution_context*);
        result = (*(e4)native)((*args)[0], (*args)[1], (*args)[2], (*args)[3], &excpt, ctx);
        break;

    case 5:
        typedef ::Val* (*e5)(Val*, Val*, Val*, Val*, Val*, hlt_exception**, hlt_execution_context*);
        result =
            (*(e5)native)((*args)[0], (*args)[1], (*args)[2], (*args)[3], (*args)[4], &excpt, ctx);
        break;

    default:
        reporter::error(::util::fmt(
            "function/event %s with %d parameters not yet supported in RuntimeCallFunction()",
            func->Name(), args->length()));
        return new ::Val(0, ::TYPE_ERROR);
    }

#ifdef BRO_PLUGIN_HAVE_PROFILING
    profile_update(PROFILE_HILTI_LAND, PROFILE_STOP);
#endif

    if ( excpt ) {
        hlt_exception* etmp = 0;
        char* e = hlt_exception_to_asciiz(excpt, &etmp, ctx);
        reporter::error(::util::fmt("event/function %s raised exception: %s", func->Name(), e));
        hlt_free(e);
        GC_DTOR(excpt, hlt_exception, ctx);
    }

    return result ? result : new ::Val(0, ::TYPE_VOID);
}

void Manager::RegisterNativeFunction(const ::Func* func, void* native)
{
    auto id = func->GetUniqueFuncID();
    pimpl->native_functions[id] = native;
}

bool Manager::WantEvent(SpicyEventInfo* ev)
{
    EventHandlerPtr handler = event_registry->Lookup(ev->name.c_str());
    return handler || pimpl->compile_all;
}

bool Manager::WantEvent(shared_ptr<SpicyEventInfo> ev)
{
    return WantEvent(ev.get());
}

void Manager::DumpDebug()
{
    std::cerr << "Spicy analyzer summary:" << std::endl;
    std::cerr << std::endl;

    std::cerr << "  Modules" << std::endl;

    for ( PIMPL::spicy_module_list::iterator i = pimpl->spicy_modules.begin();
          i != pimpl->spicy_modules.end(); i++ ) {
        auto minfo = *i;
        std::cerr << "    " << minfo->module->id()->name() << " (from " << minfo->path << ")"
                  << std::endl;
    }

    std::cerr << std::endl;
    std::cerr << "  Protocol Analyzers" << std::endl;

    string location;       // Location where the analyzer was defined.
    string name;           // Name of the analyzer.
    std::list<Port> ports; // The ports associated with the analyzer.
    string
        unit_name_orig; // The fully-qualified name of the unit type to parse the originator side.
    string
        unit_name_resp; // The fully-qualified name of the unit type to parse the originator side.
    shared_ptr<spicy::type::Unit> unit_orig; // The type of the unit to parse the originator side.
    shared_ptr<spicy::type::Unit> unit_resp; // The type of the unit to parse the originator side.

    for ( auto i = pimpl->spicy_analyzers.begin(); i != pimpl->spicy_analyzers.end(); i++ ) {
        auto a = *i;

        string proto = transportToString(a->proto);

        std::list<string> ports;
        for ( auto p : a->ports )
            ports.push_back(p);

        std::cerr << "    " << a->name << " [" << proto << ", subtype " << a->tag.Subtype() << "] ["
                  << a->location << "]" << std::endl;
        std::cerr << "        Ports      : "
                  << (ports.size() ? ::util::strjoin(ports, ", ") : "none") << std::endl;
        std::cerr << "        Orig parser: "
                  << (a->unit_orig ? a->unit_orig->unit_type->id()->pathAsString() : "none") << " ";

        string desc = "not compiled";

        if ( a->parser_orig ) {
            assert(a->parser_orig);
            hlt_exception* excpt = 0;
            auto s = hlt_string_to_native(a->parser_orig->description, &excpt,
                                          hlt_global_execution_context());
            desc = ::util::fmt("compiled; unit description: \"%s\"", s);
            hlt_free(s);
        }

        std::cerr << "[" << desc << "]" << std::endl;

        std::cerr << "        Resp parser: "
                  << (a->unit_resp ? a->unit_resp->unit_type->id()->pathAsString() : "none") << " ";

        desc = "not compiled";

        if ( a->parser_resp ) {
            assert(a->parser_resp);
            hlt_exception* excpt = 0;
            auto s = hlt_string_to_native(a->parser_resp->description, &excpt,
                                          hlt_global_execution_context());
            desc = ::util::fmt("compiled; unit description: \"%s\"", s);
            hlt_free(s);
        }

        std::cerr << "[" << desc << "]" << std::endl;

        std::cerr << std::endl;
    }

    std::cerr << std::endl;
    std::cerr << "  File Analyzers" << std::endl;

    for ( auto i = pimpl->spicy_file_analyzers.begin(); i != pimpl->spicy_file_analyzers.end();
          i++ ) {
        auto a = *i;

        std::cerr << "    " << a->name << " ["
                  << "subtype " << a->tag.Subtype() << "] [" << a->location << "]" << std::endl;
        std::cerr << "        MIME types : "
                  << (a->mime_types.size() ? ::util::strjoin(a->mime_types, ", ") : "none")
                  << std::endl;
        std::cerr << "        Parser     : "
                  << (a->unit ? a->unit->unit_type->id()->pathAsString() : "none") << " ";

        string desc = "not compiled";

        if ( a->parser ) {
            hlt_exception* excpt = 0;
            auto s = hlt_string_to_native(a->parser->description, &excpt,
                                          hlt_global_execution_context());
            desc = ::util::fmt("compiled; unit description: \"%s\"", s);
            hlt_free(s);
        }

        std::cerr << "[" << desc << "]" << std::endl;

        std::cerr << std::endl;
    }

    std::cerr << std::endl;
    std::cerr << "  Units" << std::endl;

    for ( SpicyAST::unit_map::const_iterator i = pimpl->spicy_ast->Units().begin();
          i != pimpl->spicy_ast->Units().end(); i++ ) {
        auto uinfo = i->second;
        std::cerr << "    " << uinfo->name << std::endl;
    }

    std::cerr << std::endl;
    std::cerr << "  Events" << std::endl;

    for ( PIMPL::spicy_event_list::iterator i = pimpl->spicy_events.begin();
          i != pimpl->spicy_events.end(); i++ ) {
        auto ev = *i;

        string args;

        for ( auto e : ev->expr_accessors ) {
            if ( args.size() )
                args += ", ";

            if ( e->btype )
                args += ::util::fmt("arg%d: %s", e->nr, e->btype->render());
            else
                args += e->expr;
        }

        std::cerr << "    * " << ev->unit << "/" << ev->hook_local << " -> " << ev->name << '('
                  << args << ") [unit: " << ev->unit_module->id()->name() << "] " << ev->location
                  << "]" << std::endl;

        std::cerr << "      - [Bro]      " << ev->name << ": ";

        if ( ev->bro_event_type ) {
            ODesc d;
            d.SetShort();
            ev->bro_event_type->Describe(&d);
            std::cerr << d.Description() << " ["
                      << (ev->bro_event_handler ? "has handler" : "no handler") << "]" << std::endl;
        }

        else
            std::cerr << "(not created)" << std::endl;

        std::cerr << "      - [Spicy] ";

        if ( ev->spicy_hook ) {
            auto id = ev->spicy_hook->id();
            auto hook = ev->spicy_hook->hook();
            std::cerr << id->pathAsString() << std::endl;
        }
        else
            std::cerr << "(not created)" << std::endl;

        std::cerr << "      - [HILTI]    ";

        if ( ev->hilti_raise ) {
            auto id = ev->hilti_raise->id();
            auto func = ev->hilti_raise->function();
            std::cerr << id->pathAsString() << ": " << func->type()->render() << std::endl;
        }
        else
            std::cerr << "(not created)" << std::endl;

        std::cerr << std::endl;
    }

    std::cerr << std::endl;
}

void Manager::DumpCode(bool all)
{
#if 0
	std::cerr << ::util::fmt("\n=== Final code: %s.spicy\n", pimpl->spicy_module->id()->name()) << std::endl;

	if ( pimpl->spicy_context && pimpl->spicy_module )
		pimpl->spicy_context->print(pimpl->spicy_module, std::cerr);
	else
		std::cerr << "(No Spicy code generated.)" << std::endl;

	if ( ! all )
		{
		std::cerr << ::util::fmt("\n=== Final code: %s.hlt\n", pimpl->hilti_module->id()->name()) << std::endl;

		if ( pimpl->hilti_context && pimpl->hilti_module )
			pimpl->hilti_context->print(pimpl->hilti_module, std::cerr);
		else
			std::cerr << "(No HILTI code generated.)" << std::endl;
		}

	else
		{
		for ( auto m : pimpl->hilti_modules )
			{
			std::cerr << ::util::fmt("\n=== Final code: %s.hlt\n", m->id()->name()) << std::endl;
			pimpl->hilti_context->print(m, std::cerr);
			}
		}
#endif
}

void Manager::DumpMemoryStatistics()
{
    hlt_memory_stats stats = hlt_memory_statistics();
    uint64_t heap = stats.size_heap / 1024 / 1024;
    uint64_t alloced = stats.size_alloced / 1024 / 1024;
    uint64_t size_stacks = stats.size_stacks / 1024 / 1024;
    uint64_t num_stacks = stats.num_stacks;
    uint64_t total_refs = stats.num_refs;
    uint64_t current_allocs = stats.num_allocs - stats.num_deallocs;

    fprintf(stderr, "%" PRIu64
                    "M heap, "
                    "%" PRIu64
                    "M alloced, "
                    "%" PRIu64 "M in %" PRIu64
                    " stacks, "
                    "%" PRIu64
                    " allocations, "
                    "%" PRIu64
                    " totals refs"
                    "\n",
            heap, alloced, size_stacks, num_stacks, current_allocs, total_refs);
}
