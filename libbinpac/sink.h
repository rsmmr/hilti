/// Support functions for binpac++'s "sink" type.

#ifndef LIBBINPAC_SINK_H
#define LIBBINPAC_SINK_H

#include "libbinpac++.h"
#include "filter.h"

typedef struct binpac_sink binpac_sink;

__HLT_DECLARE_RTTI_GC_TYPE(binpac_sink);

/// Instantiates a new sink. The sink is created with its initial sequence
/// defaulting to zero, its reassembly policy defaulting to
/// BinPAC::ReassemblyPolicy::First, and auto-trimming enabled.
///
/// excpt: &
/// ctx: &
///
/// Returns: The new instance.
extern binpac_sink* binpachilti_sink_new(hlt_exception** excpt, hlt_execution_context* ctx);

/// Sets the initial sequence number for a sink. All sequence numbers given
/// to other functions will be interpreted as relative to this. If not set,
/// it defaults to zero. This must not be called after any data has already
/// been parsed in; doing so will throw a \a ValueError exception.
///
/// sink: The sink to set it for.
///
/// seq: The initial sequence number.
///
/// pobj: The parsing object for the parser to be connected.
///
/// parser: The parser to which the parsing object belongs.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_set_initial_sequence_number(binpac_sink* sink, uint64_t initial_seq, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Sets the reassembly policy for a sink. If not set, the policy defaults
/// to.
///
/// sink: The sink to set it for.
///
/// policy: The policy as the integer corresponding to values of
/// BinPAC::ReassemblyPolicy.
///
/// pobj: The parsing object for the parser to be connected.
///
/// parser: The parser to which the parsing object belongs.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_set_policy(binpac_sink* sink, int64_t policy, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Sets the state of auto-trimming. If on (which is the default), the sink
/// will automatically trim all data that been processed by the parser. If
/// off, even processed data will still be buffered until explicitly trimmed
/// by binpachilti_sink_trim(). Only the latter mode allows to find
/// reassembly ambigituies due to different data being passed in for the same
/// sequence range.
///
/// sink: The sink to set it for.
///
/// enable: True or false for enabling and disable auto-trim, respectively.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_set_auto_trim(binpac_sink* sink, int8_t enable, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

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

/// Writes data into a sink, appending it at the end of the data passed in so
/// far.
///
/// sink: The sink to disconnect from.
/// data: The data to write into the sink.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_append(binpac_sink* sink, hlt_bytes* data, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes data into a sink. The data will be inserted into the byte stream
/// at the given position.
///
/// sink: The sink to write to.
///
/// data: The data to write into the sink.
///
/// seq: The sequence number of first byte of *data*, relative to the initial
///      sequence number set with \a binpachilti_sink_set_initial_sequence().
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_write(binpac_sink* sink, hlt_bytes* data, uint64_t seq, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Writes data into a sink. The data will be inserted into the byte stream
/// at the given position. This version can associate a custom length with
/// the chunk for reassembly. 
///
/// sink: The sink to write to.
///
/// data: The data to write into the sink.
///
/// seq: The sequence number of first byte of *data*, relative to the initial
///      sequence number set with \a binpachilti_sink_set_initial_sequence().
///
/// len: The length to associate with the chunk within the reaseembly
/// sequence space. The caller is in charge of ensuring consistency across
/// different writes.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_write_custom_length(binpac_sink* sink, hlt_bytes* data, uint64_t seq, uint64_t len, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Reports a gap in the input stream, i.e., data that for sure will be not
/// be seen anymore. If processing reaches a gap, it will stop there and
/// report it. However, a gap can be skipped over with \a
/// binpachilti_sink_skip().
///
/// sink: The sink to disconnect from.
///
/// seq: The sequence number of first byte just beyond the gap.
///
/// len: The number of bytes the gap covers.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_gap(binpac_sink* sink, uint64_t seq, uint64_t len, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Skips ahead in the input stream to a given given sequence number. This is
/// useful for skipping over data that's not interesting. Data that has still
/// been buffered at lower sequence numbers will be discarded; and any
/// furture such data will be ignored. It's guaranteed that the next bytes
/// coming out of the sequence will be at the given sequence number. If
/// processing is currenty inside a gap (and thus stopped), fully skipping
/// over it will lead it to continue at the new position.
///
/// sink: The sink to disconnect from.
///
/// seq: The sequence number of the first byte beyond the range to skip.
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_skip(binpac_sink* sink, uint64_t seq, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Trims input data up to a given given sequence number. This means that all
/// still buffered before that point will be discarded. If new data comes in
/// before that point, it will be ignored (i.e., the sink will automatically
/// skip to that sequence number as well). In auto-trim mode, the function
/// has no effect.
///
/// sink: The sink to disconnect from.
///
/// seq: The sequence number of the first byte beyond the range to trim up
/// to..
///
/// excpt: &
/// ctx: &
extern void binpachilti_sink_trim(binpac_sink* sink, uint64_t seq, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

/// Close a sink by disconnecting all parsers.  Afterwards, the sink most no
/// longer be used.
///
/// sink: The sink to close.
/// excpt: &
/// ctx: &
extern void binpachilti_sink_close(binpac_sink* sink, void* user, hlt_exception** excpt, hlt_execution_context* ctx);

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

/// Returns the sequence number of the current position in input stream being
/// parsed.
///
/// sink: The sink to report the value for.
/// excpt: &
/// ctx: &
extern uint64_t binpachilti_sink_sequence(binpac_sink* sink, hlt_exception** excpt, hlt_execution_context* ctx);


#endif
