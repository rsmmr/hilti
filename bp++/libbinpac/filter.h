///
/// Filters are pipelines decoding input from one form to another. In
/// BinPAC++, they can be attached to either sinks or parsing objects. 
///
/// Currently, there's predefined list of filters available. In the future, it may be
/// possible to write them in BinPAC++ directly as well.

#ifndef BINPAC_FILTER_H
#define BINPAC_FILTER_H

#include "binpac.h"

struct binpac_filter;

typedef hlt_bytes* (*__binpac_filter_decode)(struct binpac_filter*, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void       (*__binpac_filter_close)(struct binpac_filter*, hlt_exception** excpt, hlt_execution_context* ctx);

// This is the common header of all filter structs. In filter_*.c, we define
// one additional struct per pre-defined filter type storing filter-specific
// information. 
struct binpac_filter {
    const char* name;
    __binpac_filter_decode decode;
    __binpac_filter_close close;
    struct binpac_filter* next;
};

/// A filter for decoding input from one representation into another.
typedef struct binpac_filter binpac_filter;

/// Instantiates and initialized a new filter.
///
/// ftype: The type of the filter, as defined by one of BINPAC_FILTER_* constants in binpac.h.
/// 
/// excpt: &
/// ctx: &
///
/// Returns: The new filter.
///
/// Raised: FilterUnsupported - If the given filter type is not supported by the run-time system.
extern binpac_filter* binpac_filter_new(hlt_enum ftype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Pipes data into a filter. If further filters have been chained to this one,
/// the data is passed through all of them.
///
/// filter: The filter.
/// data: The data to be passed in.
/// excpt: &
/// ctx: &
///
/// Returns: A bytes objects with filtered data. The data being returned does
/// not necessarily correspond directly to the data passed in, but represents
/// the next chunk of data that could now be prepared by the filter. Depending
/// on how the filter operates, this may still include pieces passed in for
/// filtering earlier. The returned data may also be empty if the filter can
/// currently not produce further output. In particular, an empty result does
/// not indicate an error; these are reported via exceptions.
///
/// Raises: FilterError - If there's a problem with the filtering process.
/// After raising this error, this filter must not be used again. 
extern hlt_bytes* binpac_filter_decode(binpac_filter* filter, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx);

/// Closes the filter. If further filters have been chained to this one, all are closed.
///
/// filter: The filter.
/// excpt: &
/// ctx: &
///
/// Raises: FilterError - If there's a problem with the filtering process.
///
/// Note: When a filter is closed but has still data pending that can't be
/// processed normally anymore (e.g., because it didn't get sufficient input
/// for decoding something completely), it must raise a FilterError
/// exception. The assumption is that if no exception is raised, all input has
/// been fully processed after closing the filter. 
extern void binpac_filter_close(binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal functions for managing filters.

// Initializes the common part of the filter struct.
extern void __binpac_filter_init(binpac_filter* filter, const char* name, __binpac_filter_decode decode, __binpac_filter_close close);

/// Appends another filter to an existing filter chain. The filter will be
/// lined into the chain, and the new head node returned.
///
/// head: The first filter in the chain. If null, the existing list is
/// assumed to be empty, and *add* is returned directly in that case.
///
/// add: The filter to append.
///
/// excpt: &
/// ctx: &
///
/// Returns: The new head node of the filter chain.
extern binpac_filter* binpac_filter_add(binpac_filter* head, binpac_filter* add, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
