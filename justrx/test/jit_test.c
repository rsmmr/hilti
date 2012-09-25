#include <regex.h>

int main(int argc, char *argv[])
{
    regex_t re;
    regcomp(&re, "[a-z]+([0-9][a-zA-Z])*", REG_EXTENDED);
    jit_regset_compile(&re);

    return 0;
}
