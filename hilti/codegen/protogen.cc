
#include "protogen.h"
#include "module.h"
#include "util.h"

using namespace hilti;
using namespace codegen;

protogen::TypeMapper::~TypeMapper()
{
}

string protogen::TypeMapper::mapType(shared_ptr<Type> type)
{
    setDefaultResult("<no C type defined>");

    string ctype;
    processOne(type, &ctype);
    return ctype;
}

void protogen::TypeMapper::visit(type::Reference* t)
{
    if ( t->wildcard() )
        setResult("void *");
    else
        setResult(mapType(t->argType()));
}

void protogen::TypeMapper::visit(type::Integer* t)
{
    if ( t->width() <= 8 )
        setResult("int8_t");
    else if ( t->width() <= 16 )
        setResult("int16_t");
    else if ( t->width() <= 32 )
        setResult("int32_t");
    else if ( t->width() <= 64 )
        setResult("int64_t");
    else
        assert(false);
}

void protogen::TypeMapper::visit(type::Tuple* t)
{
    // TODO.
}

void protogen::TypeMapper::visit(type::Struct* t)
{
    // TODO.
}

void ProtoGen::generatePrototypes(shared_ptr<hilti::Module> module)
{
    std::ostream& out = output();

    // Generate header.

    auto modname = ::util::strtoupper(module->id()->name());
    modname = ::util::strreplace(modname, "-", "_");
    modname = ::util::strreplace(modname, ".", "_");

    auto ifdefname = ::util::fmt("%s_HLT_H", modname.c_str());

    out << "/* Automatically generated. Do not edit. */" << std::endl;
    out << std::endl;
    out << "#ifndef " << ifdefname << std::endl;
    out << "#define " << ifdefname << std::endl;
    out << std::endl;
    out << "#include <libhilti.h>" << std::endl;
    out << std::endl;

    // Generate body.
    processAllPreOrder(module);

    // Generator footer.
    out << "#endif" << std::endl;
}


void ProtoGen::visit(type::Enum* t)
{
    auto decl = current<declaration::Type>();

    if ( ! decl )
        return;

    std::ostream& out = output();

    for ( auto l : t->labels() ) {

        if ( *l.first == "Undef" )
            continue;

        auto mod = current<Module>()->id()->name();
        auto scope = decl->id()->name();
        auto id = l.first->pathAsString();
        auto value = t->labelValue(l.first);

        auto proto = ::util::fmt("static const hlt_enum %s_%s_%s = { 0, %d };", mod.c_str(), scope.c_str(), id.c_str(), value);

        out << proto.c_str() << std::endl;
    }
}

void ProtoGen::visit(declaration::Function* f)
{
    auto func = f->function();

    if ( ! (f->linkage() == Declaration::EXPORTED && func->type()->callingConvention() == type::function::HILTI) )
        return;

    std::ostream& out = output();

    // These must match what CodeGen::llvmBuildCWrapper() generates.
    auto name1 = util::mangle(func->id(), true, func->module()->id(), "", false);
    auto name2 = util::mangle(func->id()->name() + "_resume", true, func->module()->id(), "", false);

    auto result = func->type()->result()->type();

    if ( _generated.find(name1) != _generated.end() )
        return;

    out << mapType(result) << ' ' << name1 << '(';

    for ( auto p : func->type()->parameters() )
        out << mapType(p->type()) << " " << p->id()->name() << ", ";

    out << "hlt_exception** excpt);" << std::endl;

    out << mapType(result) << ' ' << name2 << "(hlt_exception* yield_excpt, ";
    out << "hlt_exception** excpt);" << std::endl;

    _generated.insert(name1);
}

void ProtoGen::visit(declaration::Type* t)
{
    auto type = t->type();

    if ( ! t->linkage() == Declaration::EXPORTED )
        return;

    // This must match the logic in CodeGen::llvmRttiPtr().
    //
    // TODO: We should factor this out (but we don't have access to a CodeGen
    // here).
    string name = util::mangle(string("hlt_type_info_hlt_") + type->render(), true, nullptr, "", false);
    name = ::util::strreplace(name, "_ref", "");
    name = ::util::strreplace(name, "_any", "");

    if ( _generated.find(name) != _generated.end() )
        return;

    std::ostream& out = output();

    out << "extern const hlt_type_info* " << name << ";" << std::endl;

    _generated.insert(name);
}


