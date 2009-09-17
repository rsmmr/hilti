// $Id$

#ifndef JRX_INTERN_H
#define JRX_INTERN_H

#include <stdint.h>

// Predefined types. 
typedef uint32_t jrx_char;         // A single codepoint. 
typedef uint32_t jrx_offset;       // Offset into input stream.
typedef int16_t jrx_accept_id;     // ID for an accepting state.
typedef uint32_t jrx_nfa_state_id; // ID for an NFA state.
typedef uint32_t jrx_dfa_state_id; // ID for a DFA state.
typedef uint16_t jrx_ccl_id;       // ID for a CCL.

// Predefined constants.
static const jrx_char JRX_CHAR_MAX = UINT32_MAX; // Max. codepoint.
static const jrx_char JRX_OFFSET_MAX = UINT32_MAX; // Max. offset value.

// Matching options.
typedef uint8_t jrx_option;
static const jrx_option JRX_OPTION_NONE = 0;  
static const jrx_option JRX_OPTION_CASE_INSENSITIVE = 1 << 0;    // Match case-insentive.
static const jrx_option JRX_OPTION_LAZY = 1 << 1;                // Compute DFA lazily. 
static const jrx_option JRX_OPTION_DEBUG = 1 << 2;               // Print debug information.
static const jrx_option JRX_OPTION_NO_CAPTURE = 1 << 3;          // Do not capture subgroups.
static const jrx_option JRX_OPTION_DONT_ANCHOR = 1 << 4;         // Add implict .* at beginning & end.
//static const jrx_option OPTIONS_INCREMENTAL_DFA = 1 << 4;  // Build DFA incrementally.

// Zero-width assertions.
typedef uint16_t jrx_assertion;
static const jrx_assertion JRX_ASSERTION_NONE = 0;
static const jrx_assertion JRX_ASSERTION_BOL = 1 << 1;
static const jrx_assertion JRX_ASSERTION_EOL = 1 << 2;
static const jrx_assertion JRX_ASSERTION_BOD = 1 << 3;
static const jrx_assertion JRX_ASSERTION_EOD = 1 << 4;
static const jrx_assertion JRX_ASSERTION_WORD_BOUNDARY = 1 << 5;
static const jrx_assertion JRX_ASSERTION_NOT_WORD_BOUNDARY = 1 << 6;
static const jrx_assertion JRX_ASSERTION_CUSTOM1 = 1 << 12;
static const jrx_assertion JRX_ASSERTION_CUSTOM2 = 1 << 13;
static const jrx_assertion JRX_ASSERTION_CUSTOM3 = 1 << 14;
static const jrx_assertion JRX_ASSERTION_CUSTOM4 = 1 << 15;

// Predefined standard character classes.
typedef enum {
    JRX_STD_CCL_NONE,
    JRX_STD_CCL_EPSILON,
    JRX_STD_CCL_ANY,
    JRX_STD_CCL_LOWER,
    JRX_STD_CCL_UPPER,
    JRX_STD_CCL_WORD,
        
    JRX_STD_CCL_NUM, // Count number of std CCLs.
} jrx_std_ccl;

#endif
