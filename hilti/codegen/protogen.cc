
#include "protogen.h"
#include "module.h"

using namespace hilti;
using namespace codegen;

void ProtoGen::generatePrototypes(shared_ptr<hilti::Module> module)
{
    std::ostream& out = output();

    // Generate header.

    auto modname = ::util::strtoupper(module->id()->name());
    modname = ::util::strreplace(modname, "-", "_");
    modname = ::util::strreplace(modname, ".", "_");

    auto ifdefname = ::util::fmt("HILTI_%s_HLT_H", modname.c_str());

    out << "/* Automatically generated. Do not edit. */" << std::endl;
    out << std::endl;
    out << "#ifndef " << modname << std::endl;
    out << "#define " << modname << std::endl;
    out << std::endl;
    out << "#include <hilti.h>" << std::endl;
    out << std::endl;

    // Generate body.
    processOne(module);

    // Generator footer.
    out << "#endif" << std::endl;
}


void ProtoGen::visit(type::Enum* t)
{
    if ( ! in<declaration::Type>() )
        return;

    std::ostream& out = output();

    for ( auto l : t->labels() ) {

        if ( *l.first == "Undef" )
            continue;

        auto scope = current<Module>()->id()->name();
        auto id = ::util::strtoupper(l.first->pathAsString());
        auto value = t->labelValue(l.first);

        auto proto = ::util::fmt("static const hlt_enum %s_%s = { 0, %d };", scope.c_str(), id.c_str(), value);

        out << proto.c_str() << std::endl;
    }
}


