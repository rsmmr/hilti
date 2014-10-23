//
// @TEST-EXEC: hilti-build %INPUT -o a.out
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output

#include <libhilti.h>

int main()
{
    hlt_execution_context* ctx = 0;
    hlt_exception* excpt = 0;
    hlt_string s;

    hlt_init();
    ctx = hlt_global_execution_context();

    hlt_port a;

    a = hlt_port_from_asciiz("80/tcp", &excpt, ctx);
    s = hlt_object_to_string(&hlt_type_info_hlt_port, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_port_from_asciiz("53/udp", &excpt, ctx);
    s = hlt_object_to_string(&hlt_type_info_hlt_port, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_port_from_asciiz("can't parse", &excpt, ctx);
    assert(excpt);

    GC_DTOR(excpt, hlt_exception, ctx);

    return 0;
}
