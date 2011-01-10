///
/// Support for activating analyzers by their MIME types.
///

#ifndef BINPAC_MIME_H
#define BINPAC_MIME_H

#include "binpac.h"
#include "sink.h"

extern void binpac_mime_register_parser(binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx);
extern void binpac_mime_connect_by_string(binpac_sink* sink, hlt_string mtype, hlt_exception** excpt, hlt_execution_context* ctx);
extern void binpac_mime_connect_by_bytes(binpac_sink* sink, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
