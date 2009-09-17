
#include "regex.h"
#include "dfa-interpreter.h"
#include "dfa-min-interpreter.h"

int regcomp(regex_t *preg, const char *pattern, int cflags)
{
    preg->errmsg = 0;
    
    if ( ! (cflags & REG_EXTENDED) ) 
        preg->errmsg = "REG_BASIC syntax is not supported";
    
    if ( cflags & REG_ICASE )
        preg->errmsg = "REG_ICASE not supported at this time";
    
    if ( cflags & REG_NEWLINE )
        preg->errmsg = "REG_NEWLINE not supported at this time";

    if ( preg->errmsg )
        return REG_NOTSUPPORTED;
    
    jrx_option options = JRX_OPTION_DONT_ANCHOR;

    if ( cflags & REG_DEBUG )
        options |= JRX_OPTION_DEBUG;
    
    if ( cflags & REG_NOSUB )
        options |= JRX_OPTION_NO_CAPTURE;
    
    // Can't tell here how many capture groups we will need.
    jrx_dfa* dfa = dfa_compile(pattern, strlen(pattern), options, -1, &preg->errmsg);
    
    if ( ! dfa )
        return REG_BADPAT;
    
    preg->nfa = 0;
    preg->dfa = dfa;
    preg->cflags = cflags;
    preg->re_nsub = dfa->max_capture;

    return REG_OK;
}

static int _regexec_std(jrx_dfa* dfa, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
    // Special case the empty string, which always matches.
    if ( ! string || ! *string ) {
        if ( nmatch > 0 ) {
            pmatch[0].rm_so = pmatch[0].rm_eo = 0;
            for ( int i = 1; i < nmatch; i++ )
                pmatch[i].rm_so = pmatch[i].rm_eo = -1;
        }
        
        return REG_OK;
    }

    jrx_match_state ms;
    jrx_match_state_init(&ms, dfa);
    
    for ( const char* p = string; *p; p++ ) {
        jrx_assertion assertions = JRX_ASSERTION_NONE;
        
        if ( p == string )
            assertions |= JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;

        if ( ! *(p+1) )
            assertions |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;
        
        if ( ! jrx_match_state_advance(&ms, *p, assertions) ) {
            if ( (dfa->options & JRX_OPTION_DONT_ANCHOR) && set_match_accept_size(ms.accepts) )
                break;
            
            else {
                jrx_match_state_done(&ms);
                return REG_NOMATCH;
            }
        }
    }

    int rc = set_match_accept_size(ms.accepts) ? REG_OK : REG_NOMATCH;
    
    if ( dfa->options & JRX_OPTION_NO_CAPTURE ) {
        pmatch[0].rm_so = pmatch[0].rm_eo = 0;
        for ( int i = 0; i < nmatch; i++ )
            pmatch[i].rm_so = pmatch[i].rm_eo = -1;
        
        jrx_match_state_done(&ms);
        return rc;
    }
    
    if ( set_match_accept_size(ms.accepts) ) {
        assert(set_match_accept_size(ms.accepts) == 1);
        
        set_for_each(match_accept, ms.accepts, first) {
            jrx_offset* tags = first.tags;
        
            for ( int i = 0; i < nmatch; i++ ) {
                if ( i <= dfa->max_capture && tags[i*2] > 0 && tags[i*2 + 1] > 0 ) {
                    pmatch[i].rm_so = tags[i*2] - 1;
                    pmatch[i].rm_eo = tags[i*2 + 1] - 1;
                }
                else
                    pmatch[i].rm_so = pmatch[i].rm_eo = -1;
            }
        }
    }
    
    else
        for ( int i = 0; i < nmatch; i++ )
            pmatch[i].rm_so = pmatch[i].rm_eo = -1;
    
    jrx_match_state_done(&ms);
    return rc;
}

static int _regexec_min(jrx_dfa* dfa, const char *string, int eflags)
{
    // Special case the empty string, which always matches.
    if ( ! string || ! *string )
        return REG_OK;
    
    jrx_min_match_state ms;
    jrx_min_match_state_init(&ms, dfa);

    jrx_accept_id acc = 0;
    
    for ( const char* p = string; *p; p++ ) {
        jrx_assertion assertions = JRX_ASSERTION_NONE;
        
        if ( p == string )
            assertions |= JRX_ASSERTION_BOL | JRX_ASSERTION_BOD;

        if ( ! *(p+1) )
            assertions |= JRX_ASSERTION_EOL | JRX_ASSERTION_EOD;
        
        acc = jrx_min_match_state_advance(&ms, *p, assertions);
        if ( acc < 0 )
            return REG_NOMATCH;
        
        if ( acc > 0 && (dfa->options & JRX_OPTION_DONT_ANCHOR) )
            // Match.
            return 0;

    }

    return acc > 0 ? 0 : REG_NOMATCH;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch, regmatch_t pmatch[], int eflags)
{
    if ( eflags & (REG_NOTEOL | REG_NOTBOL) )
        return REG_NOTSUPPORTED;

    jrx_dfa* dfa = preg->dfa;
    
    if ( (preg->cflags & REG_NOSUB) && ! (preg->cflags & REG_STD_MATCHER) )
        return _regexec_min(dfa, string, eflags);
    else
        return _regexec_std(dfa, string, nmatch, pmatch, eflags);
}

void regfree(regex_t *preg)
{
    dfa_delete(preg->dfa);
}

size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
    char buffer[127];
    
    const char* msg = 0;
    switch ( errcode ) {
      case REG_NOTSUPPORTED:
        msg = "feature not supported";
        break;
        
      case REG_BADPAT:
        msg = "bad pattern";
        break;
        
      case REG_NOMATCH:
        msg = "no match";
        break;
        
      default:
        msg = "unknown error code for regerror()";
    }

    if ( preg->errmsg ) {
        snprintf(buffer, sizeof(buffer), "%s: %s", msg, preg->errmsg);
        msg = buffer;
    }

    if ( errbuf && errbuf_size ) {
        strncpy(errbuf, msg, errbuf_size);
        errbuf[errbuf_size - 1] = '\0';
    }
    
    return strlen(msg);
}

