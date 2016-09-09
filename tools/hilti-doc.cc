///
/// Generates reference information for the manual formatted .ini style.
/// Writes to stdout.
///

#include <hilti.h>
#include <time.h>
#include <util/util.h>

using namespace std;

string fmtHiltiType(shared_ptr<hilti::Type> t)
{
    return t ? t->render() : "";
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

    // Write the global section.
    cout << "[hilti]" << endl;
    cout << "version=" << hilti::version() << endl;
    cout << "date=" << date << endl;
    cout << endl;

    hilti::init();

    // Write one section per HILTI instruction.
    for ( auto i : hilti::instructions() ) {
        auto info = i->info();
        cout << "[hilti-instruction:" << info.mnemonic << "]" << endl;
        cout << "mnemonic=" << info.mnemonic << endl;
        cout << "namespace=" << info.namespace_ << endl;
        cout << "class=" << info.class_ << endl;
        cout << "terminator=" << info.terminator << endl;
        cout << "type_target=" << fmtHiltiType(info.type_target) << endl;
        cout << "type_op1=" << fmtHiltiType(info.type_op1) << endl;
        cout << "type_op2=" << fmtHiltiType(info.type_op2) << endl;
        cout << "type_op3=" << fmtHiltiType(info.type_op3) << endl;
        cout << "default_op1=" << fmtHiltiExpr(info.default_op1) << endl;
        cout << "default_op2=" << fmtHiltiExpr(info.default_op2) << endl;
        cout << "default_op3=" << fmtHiltiExpr(info.default_op3) << endl;
        cout << "description=" << fmtText(info.description) << endl;
        cout << endl;
    }

    return 0;
}
