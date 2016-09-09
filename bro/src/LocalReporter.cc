
// Note: This is compiled from the top-level CMakeLists.txt and inherits its
// compiler settingss

#include "Reporter.h"

#include <string>

namespace bro {
namespace hilti {
namespace reporter {

std::list<Location*> locations;

void __push_location(const char* file, int line)
{
    Location* loc = new Location(copy_string(file), line, line, 0, 0);
    loc->delete_data = true;
    locations.push_back(loc);
    ::reporter->PushLocation(loc);
}

void __pop_location()
{
    Location* loc = locations.back();
    delete loc;
    locations.pop_back();
    ::reporter->PopLocation();
}

char* __current_location()
{
    assert(locations.size());
    Location* loc = locations.back();
    ODesc desc;
    loc->Describe(&desc);
    return strdup(desc.Description());
}

extern void __error(const char* msg)
{
    ::reporter->Error("%s", msg);
}

extern void __fatal_error(const char* msg)
{
    ::reporter->FatalError("%s", msg);
}

extern void __warning(const char* msg)
{
    ::reporter->Warning("%s", msg);
}

extern void __internal_error(const char* msg)
{
    ::reporter->InternalError("%s", msg);
}

extern void __weird(Connection* conn, const char* msg)
{
    if ( conn )
        ::reporter->Weird(conn, msg);
    else
        ::reporter->Weird(msg);
}

extern int __errors()
{
    return ::reporter->Errors();
}
}
}
}
