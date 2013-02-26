///
/// Generates reference information for the manual formatted .ini style.
/// Writes to stdout.
///

#include <hilti.h>
#include <binpac/type.h>
#include <binpac/binpac++.h>
#include <util/util.h>
#include <time.h>

using namespace std;

string fmtHiltiType(shared_ptr<hilti::Type> t)
{
    return t ? t->render() : "";
}

string fmtPacType(shared_ptr<binpac::Type> t)
{
    if ( ! t )
        return "";

    auto ttype = ast::tryCast<binpac::type::TypeType>(t);

    if ( ttype )
        return ttype->typeType() ? ttype->typeType()->render() : "type";

    return t->render();
}

string fmtPacCallArg(std::pair<string, shared_ptr<binpac::Type>> t)
{
    if ( ! t.second )
        return "";

    auto ttype = ast::tryCast<binpac::type::TypeType>(t.second);

    if ( ttype )
        return t.first + ": " + (ttype->typeType() ? ttype->typeType()->render() : "type");

    return t.first + ": " + t.second->render();
}

string fmtHiltiExpr(shared_ptr<hilti::Expression> e)
{
    return e ? e->render() : "";
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

    // Write the global sections.
    cout << "[hilti]" << endl;
    cout << "version=" << hilti::version() << endl;
    cout << "date=" << date << endl;
    cout << endl;

    cout << "[binpac]" << endl;
    cout << "version=" << binpac::version() << endl;
    cout << "date=" << date << endl;
    cout << endl;

    hilti::init();

    // Write one section per HILTI instruction.
    for ( auto i : hilti::instructions() ) {
        auto info = i->info();
        cout << "[hilti-instruction:" << info.mnemonic << "]" << endl;
        cout << "mnemonic="     << info.mnemonic << endl;
        cout << "namespace="    << info.namespace_ << endl;
        cout << "class="        << info.class_ << endl;
        cout << "terminator="   << info.terminator << endl;
        cout << "type_target="  << fmtHiltiType(info.type_target) << endl;
        cout << "type_op1="     << fmtHiltiType(info.type_op1) << endl;
        cout << "type_op2="     << fmtHiltiType(info.type_op2) << endl;
        cout << "type_op3="     << fmtHiltiType(info.type_op3) << endl;
        cout << "default_op1="  << fmtHiltiExpr(info.default_op1) << endl;
        cout << "default_op2="  << fmtHiltiExpr(info.default_op2) << endl;
        cout << "default_op3="  << fmtHiltiExpr(info.default_op3) << endl;
        cout << "description="  << fmtText(info.description) << endl;
        cout << endl;
    }

    binpac::init();

    // Write one section per BinPAC operator.
    for ( auto i : binpac::operators() ) {
        auto info = i->info();

        cout << "[binpac-operator:" << info.kind_txt << ":" << i.get() << "]" << endl;
        cout << "kind="             << info.kind_txt << endl;
        cout << "namespace="        << info.namespace_ << endl;
        cout << "type_op1="         << fmtPacType(info.type_op1) << endl;
        cout << "type_op2="         << fmtPacType(info.type_op2) << endl;
        cout << "type_op3="         << fmtPacType(info.type_op3) << endl;
        cout << "type_result="      << fmtPacType(info.type_result) << endl;
        cout << "type_callarg1="    << fmtPacCallArg(info.type_callarg1) << endl;
        cout << "type_callarg2="    << fmtPacCallArg(info.type_callarg2) << endl;
        cout << "type_callarg3="    << fmtPacCallArg(info.type_callarg3) << endl;
        cout << "type_callarg4="    << fmtPacCallArg(info.type_callarg4) << endl;
        cout << "type_callarg5="    << fmtPacCallArg(info.type_callarg5) << endl;
        cout << "class_op1="        << info.class_op1 << endl;
        cout << "class_op2="        << info.class_op2 << endl;
        cout << "class_op3="        << info.class_op3 << endl;
        cout << "description="      << fmtText(info.description) << endl;
        cout << "render="           << fmtText(info.render) << endl;
        cout << endl;
    }

    return 0;
}
