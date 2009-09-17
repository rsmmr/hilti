// $Id$
// 
// regcomp/regexec compatible interface (as far as our capabilities allow).

#ifndef JRX_REGEX_H
#define JRX_REGEX_H

#include <stdlib.h>

#include "jrx-intern.h"

struct jrx_nfa;
struct jrx_dfa;

typedef struct {
    size_t re_nsub; // POSIX
    
    int cflags;
    struct jrx_nfa* nfa;
    struct jrx_dfa* dfa;
    const char* errmsg;
} regex_t;

typedef jrx_offset regoff_t;
    
struct regmatch_t { // don't support this at the moment. 
    regoff_t rm_so; // POSIX
    regoff_t rm_eo; // POSIX
    };

typedef struct regmatch_t regmatch_t;

// POSIX options. We use macros here for compatibility with code using
// ifdef's on them and/or expecting integers.
#define REG_BASIC    0         // sic! (but not supported anyway)
#define REG_EXTENDED (1 << 0)
#define REG_ICASE    (1 << 1)
#define REG_NOSUB    (1 << 2)  // mandatory for now.
#define REG_NEWLINE  (1 << 3)
#define REG_NOTBOL   (1 << 4)  
#define REG_NOTEOL   (1 << 5)   

// Non-standard options. 
#define REG_DEBUG    (1 << 6)
#define REG_STD_MATCHER (1 << 7) // force standard matcher even with NOSUB.

// Non-standard error codes..
#define REG_OK           0
#define REG_NOTSUPPORTED 1

    // POSIX error codes.
#define REG_BADPAT       2
#define REG_NOMATCH      3

// We actually do not raise these.
#define REG_ECOLLATE    10
#define REG_ECTYPE      11
#define REG_EESCAPE     12
#define REG_ESUBREG     13
#define REG_EBRACK      14
#define REG_EPAREN      15
#define REG_EBRACE      16
#define REG_BADBR       17
#define REG_ERANGE      18
#define REG_ESPACE      19
#define REG_BADRPT      20
#define REG_ENEWLINE    21
#define REG_ENULL       22
#define REG_ECOUNT      23
#define REG_BADESC      24
#define REG_EMEM        25
#define REG_EHUNG       26
#define REG_EBUS        27
#define REG_EFAULT      28
#define REG_EFLAGS      29
#define REG_EDELIM      30


// These are POSIX compatible. 
extern int regcomp(regex_t *preg, const char *pattern, int cflags);
extern size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size);
extern int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags);
extern void regfree(regex_t *preg);

// These are non-POSIX extensions. 
extern int regset_init(regex_t *preg, int cflags);
extern int regset_add(regex_t *preg, const char *pattern, unsigned int len);
extern int regset_finalize(regex_t *preg);
extern int regexec_partial(regex_t *preg, const char *buffer, unsigned int len, jrx_assertion first, jrx_assertion last);
extern int reggroups(regex_t *preg, size_t nmatch, regmatch_t pmatch[]);

#endif

                           
                           
                           

