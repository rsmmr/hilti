// $Id$
//
// Matcher interface, for interpreting a compiled DFA.

#ifndef JRX_DFA_MATCHER_H
#define JRX_DFA_MATCHER_H

#include "jrx-intern.h"
#include "dfa.h"

typedef struct {
    jrx_accept_id aid;
    jrx_offset* tags;
} jrx_match_accept;


static inline int _jrx_cmp_match_accept(jrx_match_accept a, jrx_match_accept b)
{ 
    return a.aid != b.aid ? SET_STD_EQUAL(a.aid, b.aid)
                          : SET_STD_EQUAL(a.tags, b.tags); // ptr comparision ok.
};

DECLARE_SET(match_accept, jrx_match_accept, uint32_t, _jrx_cmp_match_accept);

typedef struct {
    set_match_accept* accepts;  // Accepts we have encountered so far.
    
    jrx_offset offset;        // Offset of next input byte.
    jrx_char previous;        // Previous code point seen (valid iff offset > 0)
    
    jrx_dfa* dfa;             // The DFA we're matching with.
    jrx_dfa_state_id state;   // Current state.
    
    int current_tags;         // Current set of position of tags (0 or 1).
    jrx_offset* tags1;        // 1st & 2nd set of position of tags; (we use
    jrx_offset* tags2;        // a double-bufferinf scheme here).
    
} jrx_match_state;

extern jrx_match_state* jrx_match_state_init(jrx_match_state* ms, jrx_dfa* dfa);
extern void jrx_match_state_done(jrx_match_state* ms);
extern int jrx_match_state_advance(jrx_match_state* ms, jrx_char cp, jrx_assertion assertions);
extern jrx_offset* jrx_match_state_copy_tags(jrx_match_state* ms, jrx_tag_group_id tid);

#endif
