// $Id$
//
// Minimal matcher interface, for interpreting a compiled DFA without capture
// support. This interface is compatible to functions produced by the JIT
// compilation into LLVM code. 

#ifndef JRX_DFA_MIN_MATCHER_H
#define JRX_DFA_MIN_MATCHER_H

#include "jrx-intern.h"
#include "dfa.h"

typedef struct {
    jrx_dfa* dfa;
    jrx_dfa_state_id state;
    int8_t first;
    jrx_char previous;
} jrx_min_match_state;
 

static inline void jrx_min_match_state_init(jrx_min_match_state* ms, jrx_dfa* dfa)
{
    assert(dfa->options & JRX_OPTION_NO_CAPTURE);
    assert(! (dfa->options & JRX_OPTION_LAZY));
    
    ms->dfa = dfa;
    ms->state = dfa->initial;
    ms->first = 1;
    ms->previous = 0;
}

// >0: Match with the return accept ID (if multiple match, undefined which).
//  0: Failure to match, not recoverable.
// -1: Partial match (e.g., no match yet but might still happen).
// *prev must be NULL initially and not modified between calls. 
extern int jrx_min_match_state_advance(jrx_min_match_state* ms, jrx_char cp, jrx_assertion assertions);

#endif
