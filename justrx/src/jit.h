//
/// regexp JIT interface

#ifndef JIT_H
#define JIT_H

#include "jrx.h"

struct jrx_jit;
typedef struct jrx_jit jrx_jit;
extern jrx_jit* jit_from_dfa(jrx_dfa* dfa);
extern void jit_delete(jrx_jit* ci);
extern int jit_regexec_partial_min(const jrx_regex_t *preg,
        const char *buffer, unsigned int len,
        jrx_assertion first, jrx_assertion last,
        jrx_match_state* ms, int find_partial_matches);

#endif






