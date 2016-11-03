///
/// Support for activating analyzers by their MIME types.
///

#ifndef LIBSPICY_MIME_H
#define LIBSPICY_MIME_H

#include "libspicy.h"
#include "sink.h"

// TODO: This should probably all move to sink.*, or for the register() maybe libspicy.

extern void spicyhilti_mime_register_parser(spicy_parser* parser, hlt_exception** excpt,
                                             hlt_execution_context* ctx);

extern void spicyhilti_sink_connect_mimetype_string(spicy_sink* sink, hlt_string mtype,
                                                     int8_t try_mode, void* cookie,
                                                     hlt_exception** excpt,
                                                     hlt_execution_context* ctx);
extern void spicyhilti_sink_connect_mimetype_bytes(spicy_sink* sink, hlt_bytes* mtype,
                                                    int8_t try_mode, void* cookie,
                                                    hlt_exception** excpt,
                                                    hlt_execution_context* ctx);

#endif
