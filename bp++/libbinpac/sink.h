/// Support functions for binpac++'s "sink" type.

#ifndef BINPAC_SINK_H
#define BINPAC_SINK_H

#include "binpac.h"
#include "filter.h"

typedef struct binpac_sink binpac_sink;

/// Instantiates a new sink.
///
/// excpt: &
/// ctx: &
///
/// Returns: The new instance.
extern binpac_sink* binpac_sink_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Connects a parser to a sink. Note that there can be only one parser of
/// each type. If there's already one of the same kind, the request is
/// silently ignored.
///
/// sink: The sink to connect the parser to.
/// pobj: The parsing object for the parser to be connected.
/// parser: The parser to which the parsing object belongs.
/// excpt: &
/// ctx: &
extern void binpac_sink_connect(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal version which also optionally takes the MIME type for better debugging output.
extern void _binpac_sink_connect_intern(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Disconnects a parser from a sink. The parser is first signaled an
/// end-of-data (and thus it might still be doing some work), and then
/// removed from the sink. If the parser is not found, the function does
/// nothing.
///
/// sink: The sink to disconnect from. 
/// pobj: The parsing object for the parser to be disconnected.
/// excpt: &
/// ctx: &
extern void binpac_sink_disconnect(binpac_sink* sink, const hlt_type_info* type, void** pobj, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes data into a sink.
///
/// sink: The sink to disconnect from. 
/// data: The data to write into the sink. 
/// excpt: &
/// ctx: &
extern void binpac_sink_write(binpac_sink* sink, hlt_bytes* data, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Close a sink by disconnecting all parsers.  Afterwards, the sink will be
/// in a state as if just freshly instantiated.
///
/// sink: The sink to close.
/// excpt: &
/// ctx: &
extern void binpac_sink_close(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx);

/// Attaches a filter to the sink. All data written to the sink will
/// then be passed through the filter first. Multiple filters can be attached, and the
/// data will then be passed through them in the order they were attached.
///
/// sink: The sink to attach the filter to.
/// filter: The filter to attach.
/// excpt: &
/// ctx: &
extern void binpac_sink_filter(binpac_sink* sink, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
