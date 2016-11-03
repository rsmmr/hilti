
#ifndef BRO_PLUGIN_HILTI_SPICYANALYZER_H
#define BRO_PLUGIN_HILTI_SPICYANALYZER_H

#include <analyzer/protocol/tcp/TCP.h>
#include <analyzer/protocol/udp/UDP.h>

#include "Cookie.h"

struct __spicy_parser;
struct __hlt_bytes;
struct __hlt_exception;

class Analyzer;

namespace bro {
namespace hilti {

class Spicy_Analyzer {
public:
    Spicy_Analyzer(analyzer::Analyzer* analyzer);
    virtual ~Spicy_Analyzer();

    void Init();
    void Done();

    // Returns:
    //    -1: Parsing yielded waiting for more input.
    //     0: Parsing failed, not more input will be accepted.
    //     1: Parsing finished, not more input will be accepted.
    int FeedChunk(int len, const u_char* data, bool is_orig, bool eod);

    void FlipRoles();

protected:
    virtual void ParseError(const string& msg, bool is_orig);

private:
    struct Endpoint {
        __spicy_parser* parser;
        __hlt_bytes* data;
        __hlt_exception* resume;
        SpicyCookie cookie;
    };

    Endpoint orig;
    Endpoint resp;
};

class Spicy_TCP_Analyzer : public Spicy_Analyzer, public analyzer::tcp::TCP_ApplicationAnalyzer {
public:
    Spicy_TCP_Analyzer(Connection* conn);
    virtual ~Spicy_TCP_Analyzer();

    // Overriden from Analyzer.
    void Init() override;
    void Done() override;
    void DeliverStream(int len, const u_char* data, bool orig) override;
    void Undelivered(uint64 seq, int len, bool orig) override;
    void EndOfData(bool is_orig) override;
    void FlipRoles() override;

    // Overriden from TCP_ApplicationAnalyzer.
    void EndpointEOF(bool is_orig) override;
    void ConnectionClosed(analyzer::tcp::TCP_Endpoint* endpoint, analyzer::tcp::TCP_Endpoint* peer,
                          int gen_event);
    void ConnectionFinished(int half_finished) override;
    void ConnectionReset() override;
    void PacketWithRST() override;

    static Analyzer* InstantiateAnalyzer(Connection* conn);

private:
    bool skip_orig;
    bool skip_resp;
};

class Spicy_UDP_Analyzer : public Spicy_Analyzer, public analyzer::Analyzer {
public:
    Spicy_UDP_Analyzer(Connection* conn);
    virtual ~Spicy_UDP_Analyzer();

    // Overriden from Analyzer.
    void Init() override;
    void Done() override;
    void DeliverPacket(int len, const u_char* data, bool orig, uint64 seq, const IP_Hdr* ip,
                       int caplen) override;
    void Undelivered(uint64 seq, int len, bool orig) override;
    void EndOfData(bool is_orig) override;
    void FlipRoles() override;

    static Analyzer* InstantiateAnalyzer(Connection* conn);
};
}
}


#endif
