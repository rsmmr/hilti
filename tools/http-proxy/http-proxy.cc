#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#define URI_LENGTH 512

#include <stdint.h>

#undef DEBUG

#include <hilti.h>
#include <binpac++.h>
#include <jit/libhilti-jit.h>

extern "C" {
    #include <libbinpac++.h>
}

binpac_parser* request = 0;
binpac_parser* reply = 0;

typedef hlt_list* (*binpac_parsers_func)(hlt_exception** excpt, hlt_execution_context* ctx);
binpac_parsers_func JitParsers = nullptr;
binpac::CompilerContext* PacContext = nullptr;
int send_socket = -1;
bool verbose = false;
bool disable_cache = false;

char uri[URI_LENGTH];
char host[URI_LENGTH];
char method[URI_LENGTH];

typedef std::pair<std::string, std::string> cache_key;
typedef std::map<cache_key, std::string> cache_map;

cache_map cache;
std::string current_request;

static void usage(const char* prog)
{
    auto dbglist = binpac::Options().cgDebugLabels();
    auto dbgstr = util::strjoin(dbglist.begin(), dbglist.end(), "/");

    fprintf(stderr, "%s [options]\n\n", prog);
    fprintf(stderr, "  Options:\n\n");
    fprintf(stderr, "    -P <port>     Bind to given port.                  [Default: 8080]\n");
    fprintf(stderr, "    -O            Optimize generated code.             [Default: off].\n");
    fprintf(stderr, "    -d            Enable debug mode.\n");
    fprintf(stderr, "    -D            Disable caching.\n\n");
    fprintf(stderr, "    -v            Enable verbose output.\n\n");

    exit(1);
}

static void check_exception(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);

        if ( hlt_exception_is_yield(excpt) || hlt_exception_is_termination(excpt) )
            exit(0);
        else {
            GC_DTOR(excpt, hlt_exception, ctx);
            GC_DTOR(request, hlt_BinPACHilti_Parser, ctx);
            exit(1);
        }
    }
}

static bool check_exception_recoverable(hlt_exception* excpt)
{
    if ( excpt ) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception_print_uncaught(excpt, ctx);
        GC_DTOR(excpt, hlt_exception, ctx);
        return true;
    }

    return false;
}

extern "C" {

    void httpprod_set_uri(hlt_bytes* hlt_method, hlt_bytes* hlt_uri) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception* excpt = 0;

        memset(method, 0, URI_LENGTH);
        memset(uri, 0, URI_LENGTH);
        hlt_bytes_to_raw((int8_t*) uri, URI_LENGTH - 1, hlt_uri, &excpt, ctx);
        hlt_bytes_to_raw((int8_t*) method, URI_LENGTH - 1, hlt_method, &excpt, ctx);
        //fprintf(stderr, "Uri set to %s\n", uri);

        check_exception(excpt);
    }

    void httpprod_set_host(hlt_bytes* hlt_host) {
        hlt_execution_context* ctx = hlt_global_execution_context();
        hlt_exception* excpt = 0;

        memset(host, 0, URI_LENGTH);
        hlt_bytes_to_raw((int8_t*) host, URI_LENGTH - 1, hlt_host, &excpt, ctx);
        //fprintf(stderr, "Host set to %s\n", host);

        check_exception(excpt);
    }

}


hlt_list* getParsers()
{
    hlt_exception* excpt = 0;
    hlt_execution_context* ctx = hlt_global_execution_context();

    assert(JitParsers);
    return (*JitParsers)(&excpt, ctx);
}

static binpac_parser* findParser(const char* name)
{
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    hlt_list* parsers = getParsers();

    hlt_string hname = hlt_string_from_asciiz(name, &excpt, ctx);

    hlt_iterator_list i = hlt_list_begin(parsers, &excpt, ctx);
    hlt_iterator_list end = hlt_list_end(parsers, &excpt, ctx);

    binpac_parser* result = 0;

    while ( ! (hlt_iterator_list_eq(i, end, &excpt, ctx) || excpt) ) {
        binpac_parser* p = *(binpac_parser**) hlt_iterator_list_deref(i, &excpt, ctx);

        if ( hlt_string_cmp(hname, p->name, &excpt, ctx) == 0 ) {
            result = p;
            break;
        }

        i = hlt_iterator_list_incr(i, &excpt, ctx);
    }

    check_exception(excpt);

    return result;
}

bool jitPac2(const std::list<string>& pac2, std::shared_ptr<binpac::Options> options)
{
    hilti::init();
    binpac::init();

    PacContext = new binpac::CompilerContext(options);

    std::list<llvm::Module* > llvm_modules;

    for ( auto p : pac2 ) {
        auto llvm_module = PacContext->compile(p);

        if ( ! llvm_module ) {
            fprintf(stderr, "compiling %p failed\n", p.c_str());
            return false;
        }

        llvm_modules.push_back(llvm_module);
    }

    auto linked_module = PacContext->linkModules("<jit analyzers>", llvm_modules);

    if ( ! linked_module ) {
        fprintf(stderr, "linking failed\n");
        return false;
    }

    auto ee = PacContext->hiltiContext()->jitModule(linked_module);

    if ( ! ee ) {
        fprintf(stderr, "jit failed\n");
        return false;
    }

    // TODO: This should be done by jitModule, which however then needs to
    // move into hilti-jit.
    hlt_init_jit(PacContext->hiltiContext(), linked_module, ee);
    binpac_init();
    binpac_init_jit(PacContext->hiltiContext(), linked_module, ee);

    auto func = PacContext->hiltiContext()->nativeFunction(linked_module, ee, "binpac_parsers");

    if ( ! func ) {
        fprintf(stderr, "jitBinpacParser error: no function 'binpac_parsers'\n");
        return false;
    }

    JitParsers = (binpac_parsers_func)func;

    return true;
}

void sendOutput(hlt_bytes* data, void** obj, hlt_type_info* type, void* user, hlt_exception** excpt, hlt_execution_context* ctx)
{
    hlt_bytes_block block;
    hlt_iterator_bytes start = hlt_bytes_begin(data, excpt, ctx);
    hlt_iterator_bytes end = hlt_bytes_end(data, excpt, ctx);

    void* cookie = 0;

    do {
        cookie = hlt_bytes_iterate_raw(&block, cookie, start, end, excpt, ctx);
        //fwrite(block.start, 1, (block.end - block.start), stdout);
        assert(send_socket != -1);
        send(send_socket, block.start, (block.end - block.start), 0);
        current_request.append((const char*) block.start, (block.end - block.start));
    } while ( cookie );
}

void sendData(int sock, binpac_parser* p, void* pobj) {
    if ( ! p->compose_func )
        return;

    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;

    // ok, this is unbelievably dirty... get the sendoutput method to use our socket.
    send_socket = sock;

    GC_CCTOR_GENERIC(&pobj, p->type_info, ctx);
    p->compose_func(pobj, sendOutput, 0, &excpt, ctx);
    GC_DTOR_GENERIC(&pobj, p->type_info, ctx);

    send_socket = -1;

    check_exception(excpt);
}

void sendData(int sock, std::string data) {
    send(sock, data.c_str(), data.length(), 0);
}

int connect_serv(const char* host) {
    uint16_t port = 80;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if ( sock == -1 ) {
        fprintf(stderr, "Error while creating listen socket\n");
        return -1;
    }

    struct hostent *server = gethostbyname(host);
    if ( server == nullptr ) {
        fprintf(stderr, "Could not lookup destination host %s\n", host);
        return -1;
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    memcpy(&serveraddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    if ( connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0 ) {
        fprintf(stderr, "Could not connect to %s:%d -- %s\n", host, port, strerror(errno));
        return -1;
    }

    return sock;
}

void ignore_sigpipe()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    act.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &act, NULL);
}

void listen(unsigned int port) {
    hlt_execution_context* ctx = hlt_global_execution_context();
    hlt_exception* excpt = 0;
    struct sockaddr_in destAddr;

    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    destAddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if ( s == -1 ) {
        fprintf(stderr, "Error while creating listen socket\n");
        exit(1);
    }

    if ( bind( s, (struct sockaddr *) &destAddr, sizeof(destAddr) ) != 0 ) {
        fprintf(stderr, "Error while binding to socket\n");
        exit(1);
    }

    if ( listen( s, 100 ) != 0 ) {
        fprintf(stderr, "Could not listen\n");
        exit(1);
    }

    for (;;) {
        if ( verbose )
            fprintf(stderr, "Waiting for connection...\n");

        struct sockaddr_in sourceAddr;
        socklen_t addrLen = sizeof(sourceAddr);
        int conn = accept(s, (struct sockaddr *) &sourceAddr, &addrLen);

        if ( conn < 0 ) {
          fprintf(stderr, "Error when accepting connection\n");
          exit(1);
        }

        if ( verbose )
            fprintf(stderr, "Connection accepted, trying to parse it...\n");

        //while (1) {
        {
            char buffer[1000];
            memset(buffer, 0, 1000);
            memset(uri, 0, URI_LENGTH);
            memset(host, 0, URI_LENGTH);
            memset(method, 0, URI_LENGTH);
            current_request = "";

            int n = read(conn, buffer, 999);
            if ( verbose )
                fprintf(stderr, "Read %d bytes, sending to spicy...\n", n);
            //fprintf(stderr, "%s\n", buffer);
            if ( n < 10 ) {
                // not an http request..
                close(conn);
                continue;
            }
            // I know this is dirty as hell - hope that we get the entire request in the first chunk
            int8_t* copy = (int8_t*) hlt_malloc(n);
            hlt_bytes* input = hlt_bytes_new(&excpt, ctx);
            GC_CCTOR(input, hlt_bytes, ctx);
            memcpy(copy, buffer, n);
            hlt_bytes_append_raw(input, copy, n, &excpt, ctx);
            check_exception(excpt);

            hlt_bytes_freeze(input, 1, &excpt, ctx);

            void *pobj = (*request->parse_func)(input, 0, &excpt, ctx);
            check_exception(excpt);

            // now hilti should be done parsing and we should hopefully have gotten our callback...
            if ( verbose )
                fprintf(stderr, "%s request for http://%s/%s\n", method, host, uri);

            // look if we already have it in our cache - if yes, send back.
            if ( cache.find(cache_key(std::string(host), std::string(uri))) != cache.end() ) {
                if ( verbose )
                    fprintf(stderr, "Found cached value for url, sending to requestor...\n");
                sendData(conn, cache.at(cache_key(std::string(host), std::string(uri))));
                //sendOutput(cache.at(cache_key(std::string(host), std::string(uri))), 0, 0, 0, &excpt, ctx);
                //std::cerr << cache.at(cache_key(std::string(host), std::string(uri))) << std::endl;

                GC_DTOR(input, hlt_bytes, ctx);
                close(conn);
                continue;
            }

            // try connecting to respective server...
            if ( verbose )
                fprintf(stderr, "Opening connection to server...\n");
            int sock = connect_serv(host);
            if ( sock < 0 ) {
                fprintf(stderr, "Could not connect to server, aborting\n");
                close(conn);
                continue;
            }

            if ( verbose )
                fprintf(stderr, "Sending request to server...\n");
            sendData(sock, request, pobj);
            GC_DTOR(input, hlt_bytes, ctx);

            if ( verbose )
                fprintf(stderr, "Parsing server reply...\n");
            // we store the reply in a single hlt_bytes variable before passing it on to the parser
            // for the moment...
            hlt_bytes* reply_bytes = hlt_bytes_new(&excpt, ctx);
            GC_CCTOR(reply_bytes, hlt_bytes, ctx);
            do {
              memset(buffer, 0, 1000);
              n = read(sock, buffer, 999);
              //fprintf(stderr, "%s", buffer);
              int8_t* copy = (int8_t*) hlt_malloc(n);
              memcpy(copy, buffer, n);
              hlt_bytes_append_raw(reply_bytes, copy, n, &excpt, ctx);
              check_exception(excpt);
              //send(conn, buffer, n, 0);
            } while ( n != 0 );
            if ( verbose )
                fprintf(stderr, "Done receiving server reply, sending to parser...\n");

            current_request = "";

            hlt_bytes_freeze(reply_bytes, 1, &excpt, ctx);
            void *robj = (*reply->parse_func)(reply_bytes, 0, &excpt, ctx);
            check_exception(excpt);

            if ( verbose )
                fprintf(stderr, "Done parsing reply, sending to requestor...\n");
            sendData(conn, reply, robj);

            GC_DTOR(reply_bytes, hlt_bytes, ctx);
            if ( strncmp(method, "GET", 3) == 0  && ! disable_cache )
              cache.insert(std::make_pair(cache_key(std::string(host), std::string(uri)), current_request));

            close(sock);
        }

        close(conn);
    }


}

int main(int argc, char** argv)
{
    const char* progname = argv[0];
    const char* parser = 0;

    auto options = std::make_shared<binpac::Options>();
    options->jit = true;
    options->generate_composers = true;
    unsigned int port = 8080;

    char ch;
    while ((ch = getopt(argc, argv, "OP:dvD")) != -1) {

        switch (ch) {

          case 'P':
            port = strtoul(optarg, nullptr, 10);
            break;

         case 'd':
            options->debug = true;
            break;

         case 'D':
            disable_cache = true;
            break;

         case 'v':
            verbose = true;
            break;

         case 'O':
            options->optimize = true;
            break;

          case 'h':
            // Fall-through.
          default:
            usage(progname);
        }
    }

    argc -= optind;
    argv += optind;

    hlt_config cfg = *hlt_config_get();
    hlt_config_set(&cfg);

    std::list<string> pac2;
    pac2.push_back("parsers/httpprod.pac2");

    if ( ! jitPac2(pac2, options) )
        return 1;

    hlt_execution_context* ctx = hlt_global_execution_context();

    request = findParser("HTTPProd::Request");
    reply = findParser("HTTPProd::Reply");

    if ( ! request || ! reply ) {
      fprintf(stderr, "Could not find http request or reply parsers\n");
      exit(1);
    }

    GC_CCTOR(request, hlt_BinPACHilti_Parser, ctx);
    GC_CCTOR(reply, hlt_BinPACHilti_Parser, ctx);

    ignore_sigpipe();

    listen(port);

    exit(0);
}

