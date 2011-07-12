//
// @TEST-EXEC: ${CC} -c %INPUT -o %INPUT.bc
// @TEST-EXEC: hilti-build2 %INPUT.bc -o a.out
// @TEST-EXEC: ./a.out >output
// @TEST-EXEC: btest-diff output

#include <hilti.h>

extern const hlt_type_info hlt_type_info_addr;

int main()
{
    hlt_execution_context* ctx = 0;
    hlt_exception* excpt = 0;
    hlt_string s;

    hlt_init();
    ctx = hlt_global_execution_context();

    hlt_addr a;

    a = hlt_addr_from_asciiz("1.2.3.4", &excpt, ctx);
    s = hlt_addr_to_string(&hlt_type_info_addr, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_addr_from_asciiz("2001:0db8:85a3:0000:0000:8a2e:0370:7334", &excpt, ctx);
    s = hlt_addr_to_string(&hlt_type_info_addr, &a, 0, &excpt, ctx);
    hlt_string_print(stdout, s, 1, &excpt, ctx);
    assert(! excpt);

    a = hlt_addr_from_asciiz("can't parse", &excpt, ctx);
    assert(excpt);

    return 0;
}
