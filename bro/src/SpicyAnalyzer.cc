
#include <memory.h>
#include <netinet/in.h>

#include <util/util.h>

extern "C" {
#include <libspicy/libspicy.h>
}

#undef DBG_LOG

#include "LocalReporter.h"
#include "Manager.h"
#include "Plugin.h"
#include "SpicyAnalyzer.h"

using namespace bro::hilti;
using namespace spicy;

using std::shared_ptr;

Spicy_Analyzer::Spicy_Analyzer(analyzer::Analyzer* analyzer)
{
    orig.cookie.type = SpicyCookie::PROTOCOL;
    orig.cookie.protocol_cookie.analyzer = analyzer;
    orig.cookie.protocol_cookie.is_orig = true;

    resp.cookie.type = SpicyCookie::PROTOCOL;
    resp.cookie.protocol_cookie.analyzer = analyzer;
    resp.cookie.protocol_cookie.is_orig = false;
}

Spicy_Analyzer::~Spicy_Analyzer()
{
}

void Spicy_Analyzer::Init()
{
    orig.parser = 0;
    orig.data = 0;
    orig.resume = 0;

    resp.parser = 0;
    resp.data = 0;
    resp.resume = 0;

    orig.cookie.protocol_cookie.tag =
        HiltiPlugin.Mgr()->TagForAnalyzer(orig.cookie.protocol_cookie.analyzer->GetAnalyzerTag());
    resp.cookie.protocol_cookie.tag = orig.cookie.protocol_cookie.tag;
}

void Spicy_Analyzer::Done()
{
    hlt_execution_context* ctx = hlt_global_execution_context();

    GC_DTOR(orig.parser, hlt_SpicyHilti_Parser, ctx);
    GC_DTOR(orig.data, hlt_bytes, ctx);
    GC_DTOR(orig.resume, hlt_exception, ctx);

    GC_DTOR(resp.parser, hlt_SpicyHilti_Parser, ctx);
    GC_DTOR(resp.data, hlt_bytes, ctx);
    GC_DTOR(resp.resume, hlt_exception, ctx);

    Init();
}

static inline void debug_msg(analyzer::Analyzer* analyzer, const char* msg, int len,
                             const u_char* data, bool is_orig)
{
#ifdef DEBUG
    if ( data ) {
        PLUGIN_DBG_LOG(HiltiPlugin, "[%s/%" PRIu32 "/%s] %s: |%s%s|", analyzer->GetAnalyzerName(),
                       analyzer->GetID(), (is_orig ? "orig" : "resp"), msg,
                       fmt_bytes((const char*)data, min(40, len)), len > 40 ? "..." : "");
    }

    else {
        PLUGIN_DBG_LOG(HiltiPlugin, "[%s/%" PRIu32 "/%s] %s", analyzer->GetAnalyzerName(),
                       analyzer->GetID(), (is_orig ? "orig" : "resp"), msg);
    }
#endif
}

int Spicy_Analyzer::FeedChunk(int len, const u_char* data, bool is_orig, bool eod)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    Endpoint* endp = is_orig ? &orig : &resp;

    // If parser is set but not data, a previous parsing process has
    // finished. If so, we ignore all further input.
    if ( endp->parser && ! endp->data ) {
        if ( len )
            debug_msg(endp->cookie.protocol_cookie.analyzer, "further data ignored", len, data,
                      is_orig);

        return 0;
    }

    if ( ! endp->parser ) {
        endp->parser = HiltiPlugin.Mgr()->ParserForAnalyzer(endp->cookie.protocol_cookie.analyzer
                                                                ->GetAnalyzerTag(),
                                                            is_orig);

        if ( ! endp->parser ) {
            debug_msg(endp->cookie.protocol_cookie.analyzer, "no unit specificed for parsing", 0, 0,
                      is_orig);
            return 1;
        }

        GC_CCTOR(endp->parser, hlt_SpicyHilti_Parser, ctx);
    }

    int result = 0;
    bool done = false;
    bool error = false;

    if ( ! endp->data ) {
        // First chunk.
        debug_msg(endp->cookie.protocol_cookie.analyzer, "initial chunk", len, data, is_orig);

        endp->data = hlt_bytes_new_from_data_copy((const int8_t*)data, len, &excpt, ctx);
        GC_CCTOR(endp->data, hlt_bytes, ctx);

        if ( eod )
            hlt_bytes_freeze(endp->data, 1, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_HILTI_LAND, PROFILE_START);
#endif

        void* pobj = (*endp->parser->parse_func)(endp->data, &endp->cookie, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_HILTI_LAND, PROFILE_STOP);
#endif

        GC_DTOR_GENERIC(&pobj, endp->parser->type_info, ctx);
    }

    else {
        // Resume parsing.
        debug_msg(endp->cookie.protocol_cookie.analyzer, "resuming with chunk", len, data, is_orig);

        assert(endp->data && endp->resume);

        if ( len )
            hlt_bytes_append_raw_copy(endp->data, (int8_t*)data, len, &excpt, ctx);

        if ( eod )
            hlt_bytes_freeze(endp->data, 1, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_HILTI_LAND, PROFILE_START);
#endif

        void* pobj = (*endp->parser->resume_func)(endp->resume, &excpt, ctx);

#ifdef BRO_PLUGIN_HAVE_PROFILING
        profile_update(PROFILE_HILTI_LAND, PROFILE_STOP);
#endif

        GC_DTOR_GENERIC(&pobj, endp->parser->type_info, ctx);
        endp->resume = 0;
    }

    if ( excpt ) {
        if ( hlt_exception_is_yield(excpt) ) {
            debug_msg(endp->cookie.protocol_cookie.analyzer, "parsing yielded", 0, 0, is_orig);
            endp->resume = excpt;
            excpt = 0;
            result = -1;
        }

        else {
            // A parse error.
            hlt_exception* excpt2 = 0;
            char* e = hlt_exception_to_asciiz(excpt, &excpt2, ctx);
            assert(! excpt2);
            ParseError(e, is_orig);
            hlt_free(e);
            GC_DTOR(excpt, hlt_exception, ctx);
            excpt = 0;
            error = true;
            result = 0;
        }
    }

    else // No exception.
    {
        done = true;
        result = 1;
    }

    // TODO: For now we just stop on error, later we might attempt to
    // restart parsing.
    if ( eod || done || error )
        GC_CLEAR(endp->data, hlt_bytes, ctx); // Marker that we're done parsing.

    return result;
}

void Spicy_Analyzer::FlipRoles()
{
    Endpoint tmp = orig;
    orig = resp;
    resp = tmp;
}

void Spicy_Analyzer::ParseError(const string& msg, bool is_orig)
{
    Endpoint* endp = is_orig ? &orig : &resp;
    string s = "parse error: " + msg;
    debug_msg(endp->cookie.protocol_cookie.analyzer, s.c_str(), 0, 0, is_orig);
    reporter::weird(endp->cookie.protocol_cookie.analyzer->Conn(), s);
}

analyzer::Analyzer* Spicy_TCP_Analyzer::InstantiateAnalyzer(Connection* conn)
{
    return new Spicy_TCP_Analyzer(conn);
}

Spicy_TCP_Analyzer::Spicy_TCP_Analyzer(Connection* conn)
    : Spicy_Analyzer(this), TCP_ApplicationAnalyzer(conn)
{
    skip_orig = skip_resp = false;
}

Spicy_TCP_Analyzer::~Spicy_TCP_Analyzer()
{
}

void Spicy_TCP_Analyzer::Init()
{
    TCP_ApplicationAnalyzer::Init();
    Spicy_Analyzer::Init();
}

void Spicy_TCP_Analyzer::Done()
{
    TCP_ApplicationAnalyzer::Done();
    Spicy_Analyzer::Done();

    EndOfData(true);
    EndOfData(false);
}

void Spicy_TCP_Analyzer::DeliverStream(int len, const u_char* data, bool is_orig)
{
    TCP_ApplicationAnalyzer::DeliverStream(len, data, is_orig);

    if ( is_orig && skip_orig )
        return;

    if ( (! is_orig) && skip_resp )
        return;

    if ( TCP() && TCP()->IsPartial() )
        return;

    int rc = FeedChunk(len, data, is_orig, false);

    if ( rc >= 0 ) {
        if ( is_orig ) {
            debug_msg(this, ::util::fmt("parsing %s, skipping further originator payload",
                                        (rc > 0 ? "finished" : "failed"))
                                .c_str(),
                      0, 0, is_orig);
            skip_orig = 1;
        }
        else {
            debug_msg(this, ::util::fmt("parsing %s, skipping further responder payload",
                                        (rc > 0 ? "finished" : "failed"))
                                .c_str(),
                      0, 0, is_orig);
            skip_resp = 1;
        }

        if ( skip_orig && skip_resp ) {
            debug_msg(this, "both endpoints finished, skipping all further TCP processing", 0, 0,
                      is_orig);
            SetSkip(1);
        }
    }
}

void Spicy_TCP_Analyzer::Undelivered(uint64 seq, int len, bool is_orig)
{
    TCP_ApplicationAnalyzer::Undelivered(seq, len, is_orig);

    // This mimics the (modified) Bro HTTP analyzer.
    // Otherwise stop parsing the connection
    if ( is_orig ) {
        debug_msg(this, "undelivered data, skipping further originator payload", 0, 0, is_orig);
        skip_orig = 1;
    }
    else {
        debug_msg(this, "undelivered data, skipping further originator payload", 0, 0, is_orig);
        skip_resp = 1;
    }
}

void Spicy_TCP_Analyzer::EndOfData(bool is_orig)
{
    TCP_ApplicationAnalyzer::EndOfData(is_orig);

    if ( is_orig && skip_orig )
        return;

    if ( (! is_orig) && skip_resp )
        return;

    if ( TCP() && TCP()->IsPartial() )
        return;

    FeedChunk(0, (const u_char*)"", is_orig, true);
}

void Spicy_TCP_Analyzer::FlipRoles()
{
    TCP_ApplicationAnalyzer::FlipRoles();
    Spicy_Analyzer::FlipRoles();
}

void Spicy_TCP_Analyzer::EndpointEOF(bool is_orig)
{
    TCP_ApplicationAnalyzer::EndpointEOF(is_orig);
    FeedChunk(0, (const u_char*)"", is_orig, true);
}

void Spicy_TCP_Analyzer::ConnectionClosed(analyzer::tcp::TCP_Endpoint* endpoint,
                                          analyzer::tcp::TCP_Endpoint* peer, int gen_event)
{
    TCP_ApplicationAnalyzer::ConnectionClosed(endpoint, peer, gen_event);
}

void Spicy_TCP_Analyzer::ConnectionFinished(int half_finished)
{
    TCP_ApplicationAnalyzer::ConnectionFinished(half_finished);
}

void Spicy_TCP_Analyzer::ConnectionReset()
{
    TCP_ApplicationAnalyzer::ConnectionReset();
}

void Spicy_TCP_Analyzer::PacketWithRST()
{
    TCP_ApplicationAnalyzer::PacketWithRST();
}

analyzer::Analyzer* Spicy_UDP_Analyzer::InstantiateAnalyzer(Connection* conn)
{
    return new Spicy_UDP_Analyzer(conn);
}

Spicy_UDP_Analyzer::Spicy_UDP_Analyzer(Connection* conn) : Spicy_Analyzer(this), Analyzer(conn)
{
}

Spicy_UDP_Analyzer::~Spicy_UDP_Analyzer()
{
}

void Spicy_UDP_Analyzer::Init()
{
    Analyzer::Init();
    Spicy_Analyzer::Init();
}

void Spicy_UDP_Analyzer::Done()
{
    Analyzer::Done();
    Spicy_Analyzer::Done();
}

void Spicy_UDP_Analyzer::DeliverPacket(int len, const u_char* data, bool is_orig, uint64 seq,
                                       const IP_Hdr* ip, int caplen)
{
    Analyzer::DeliverPacket(len, data, is_orig, seq, ip, caplen);

    FeedChunk(len, data, is_orig, true);
    Spicy_Analyzer::Done();
}

void Spicy_UDP_Analyzer::Undelivered(uint64 seq, int len, bool is_orig)
{
    Analyzer::Undelivered(seq, len, is_orig);
}

void Spicy_UDP_Analyzer::EndOfData(bool is_orig)
{
    Analyzer::EndOfData(is_orig);
}

void Spicy_UDP_Analyzer::FlipRoles()
{
    Analyzer::FlipRoles();
}
