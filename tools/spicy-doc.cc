///
/// Generates reference information for the manual formatted .ini style.
/// Writes to stdout.
///

#include <spicy/spicy.h>
#include <spicy/type.h>
#include <time.h>
#include <util/util.h>

using namespace std;

string fmtPacType(shared_ptr<spicy::Type> t)
{
    if ( ! t )
        return "";

    auto ttype = ast::rtti::tryCast<spicy::type::TypeType>(t);

    if ( ttype )
        return ttype->typeType() ? ttype->typeType()->render() : "type";

    return t->render();
}

string fmtPacCallArg(std::pair<string, shared_ptr<spicy::Type>> t)
{
    if ( ! t.second )
        return "";

    auto ttype = ast::rtti::tryCast<spicy::type::TypeType>(t.second);

    if ( ttype )
        return t.first + ": " + (ttype->typeType() ? ttype->typeType()->render() : "type");

    return t.first + ": " + t.second->render();
}

string fmtText(string s)
{
    return s;
}

int main(int argc, char** argv)
{
    if ( argc != 1 ) {
        cerr << "no arguments needed\n" << endl;
        return 1;
    }

    time_t teatime = time(0);
    static char date[128];
    tm* tm = localtime(&teatime);
    strftime(date, sizeof(date), "%B %d, %Y", tm);

    // Write the global section.
    cout << "[spicy]" << endl;
    cout << "version=" << spicy::version() << endl;
    cout << "date=" << date << endl;
    cout << endl;

    spicy::init();

    // Write one section per Spicy operator.
    for ( auto i : spicy::operators() ) {
        auto info = i->info();

        cout << "[spicy-operator:" << info.kind_txt << ":" << i.get() << "]" << endl;
        cout << "kind=" << info.kind_txt << endl;
        cout << "namespace=" << info.namespace_ << endl;
        cout << "type_op1=" << fmtPacType(info.type_op1) << endl;
        cout << "type_op2=" << fmtPacType(info.type_op2) << endl;
        cout << "type_op3=" << fmtPacType(info.type_op3) << endl;
        cout << "type_result=" << fmtPacType(info.type_result) << endl;
        cout << "type_callarg1=" << fmtPacCallArg(info.type_callarg1) << endl;
        cout << "type_callarg2=" << fmtPacCallArg(info.type_callarg2) << endl;
        cout << "type_callarg3=" << fmtPacCallArg(info.type_callarg3) << endl;
        cout << "type_callarg4=" << fmtPacCallArg(info.type_callarg4) << endl;
        cout << "type_callarg5=" << fmtPacCallArg(info.type_callarg5) << endl;
        cout << "class_op1=" << info.class_op1 << endl;
        cout << "class_op2=" << info.class_op2 << endl;
        cout << "class_op3=" << info.class_op3 << endl;
        cout << "description=" << fmtText(info.description) << endl;
        cout << "render=" << fmtText(info.render) << endl;
        cout << endl;
    }

    return 0;
}
