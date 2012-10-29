///
/// Support for activating analyzers by their MIME types.
///

#ifndef LIBBINPAC_MIME_H
#define LIBBINPAC_MIME_H

#include "libbinpac++.h"
#include "sink.h"

// TODO: This should probably all move to sink.*, or for the register() maybe libbinpac++.

extern void binpachilti_mime_register_parser(binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx);
extern void binpachilti_mime_unregister_all(hlt_exception** excpt, hlt_execution_context* ctx);

extern void binpachilti_sink_connect_mimetype_string(binpac_sink* sink, hlt_string mtype, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);
extern void binpachilti_sink_connect_mimetype_bytes(binpac_sink* sink, hlt_bytes* mtype, void* cookie, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
