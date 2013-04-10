/// Support functions for binpac++'s "sink" type.

#ifndef LIBBINPAC_SINK_H
#define LIBBINPAC_SINK_H

#include "libbinpac++.h"
#include "filter.h"

typedef struct binpac_sink binpac_sink;

__HLT_DECLARE_RTTI_GC_TYPE(binpac_sink);

/// Instantiates a new sink.
///
/// excpt: &
/// ctx: &
///
/// Returns: The new instance.
extern binpac_sink* binpachilti_sink_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Connects a parser to a sink. Note that there can be only one parser of
/// each type. If there's already one of the same kind, the request is
/// silently ignored.
///
/// sink: The sink to connect the parser to.
/// pobj: The parsing object for the parser to be connected.
/// parser: The parser to which the parsing object belongs.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_connect(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal version which also optionally takes the MIME type for better debugging output.
extern void _binpachilti_sink_connect_intern(binpac_sink* sink, const hlt_type_info* type, void** pobj, binpac_parser* parser, hlt_bytes* mtype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Disconnects a parser from a sink. The parser is first signaled an
/// end-of-data (and thus it might still be doing some work), and then
/// removed from the sink. If the parser is not found, the function does
/// nothing.
///
/// sink: The sink to disconnect from. May be null, in which case the function returns immediately.
/// pobj: The parsing object for the parser to be disconnected.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_disconnect(binpac_sink* sink, const hlt_type_info* type, void** pobj, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes data into a sink.
///
/// sink: The sink to disconnect from. 
/// data: The data to write into the sink. 
/// excpt: &
/// ctx: &
extern void binpachilti_sink_write(binpac_sink* sink, hlt_bytes* data, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Close a sink by disconnecting all parsers.  Afterwards, the sink most no
/// longer be used.
///
/// sink: The sink to close.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_close(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx);

/// Attaches a filter to the sink. All data written to the sink will
/// then be passed through the filter first. Multiple filters can be attached, and the
/// data will then be passed through them in the order they were attached.
///
/// sink: The sink to attach the filter to.
/// filter: The filter type to attach.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_add_filter(binpac_sink* sink, hlt_enum ftype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Returns the number of bytes written to a sink so far. If the sink has
/// filters attached, this returns the size after filtering.
///
/// sink: The sink to report the value for..
/// excpt: &
/// ctx: &
extern uint64_t binpachilti_sink_size(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
