//
// @TEST-EXEC: ${CC} -c %INPUT -o %INPUT.bc
// @TEST-EXEC: hilti-build %INPUT.bc -o a.out
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output

#include <libhilti.h>

extern const hlt_type_info hlt_type_info_port;

int main()
{
    hlt_execution_context* ctx = 0;
    hlt_exception* excpt = 0;
    hlt_string s;

    hlt_init();
    ctx = hlt_global_execution_context();

    hlt_port a;

    a = hlt_port_from_asciiz("80/tcp", &excpt, ctx);
    s = hlt_port_to_string(&hlt_type_info_port, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_port_from_asciiz("53/udp", &excpt, ctx);
    s = hlt_port_to_string(&hlt_type_info_port, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_port_from_asciiz("can't parse", &excpt, ctx);
    assert(excpt);

    return 0;
}
