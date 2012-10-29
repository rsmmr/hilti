///
/// Filters are pipelines decoding input from one form to another. In
/// BinPAC++, they can be attached to either sinks or parsing objects. 
///
/// Currently, there's predefined list of filters available. In the future, it may be
/// possible to write them in BinPAC++ directly as well.

#ifndef LIBBINPAC_FILTER_H
#define LIBBINPAC_FILTER_H

#include "libbinpac++.h"

struct __binpac_filter_definition;
struct binpac_filter;

typedef struct binpac_filter* (*__binpac_filter_allocate)(hlt_exception** excpt, hlt_execution_context* ctx);
typedef void                  (*__binpac_filter_dtor)(hlt_type_info* ti, struct binpac_filter*);
typedef hlt_bytes*            (*__binpac_filter_decode)(struct binpac_filter*, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx);
typedef void                  (*__binpac_filter_close)(struct binpac_filter*, hlt_exception** excpt, hlt_execution_context* ctx);

// Internal definition of a filter type.
struct __binpac_filter_definition {
    hlt_enum type;
    const char* name;
    __binpac_filter_allocate allocate;
    __binpac_filter_dtor dtor;
    __binpac_filter_decode decode;
    __binpac_filter_close close;
};

typedef struct __binpac_filter_definition __binpac_filter_definition;

/// A filter for decoding input from one representation into another. This is
/// the common header of all filter structs. In filter_*.c, we define one
/// additional struct per pre-defined filter type storing filter-specific
/// information. 
struct binpac_filter {
    __hlt_gchdr __gch;                        /// Header for garbage collection.
    __binpac_filter_definition* def;   /// Type object describing the filter type.
    struct binpac_filter* next;               /// Link to next filter in chain.
};

typedef struct binpac_filter binpac_filter;

__HLT_DECLARE_RTTI_GC_TYPE(binpac_filter)

/// Instantiates and initializes a new filter and adds it to a filter chain.
///
/// head: The head of the chain to add a new filter to. This can be null to create new chain.
/// ftype: The type of the filter, as defined by one of BINPAC_FILTER_* constants in binpac.h.
/// excpt: &
/// ctx: &
///
/// Returns: The new head of the filter chain at refcount +1.
///
/// Raised: FilterUnsupported - If the given filter type is not supported by the run-time system.
extern binpac_filter* binpachilti_filter_add(binpac_filter* head, hlt_enum ftype, hlt_exception** excpt, hlt_execution_context* ctx);

/// Adds an already instantiated filter to a filter chain. For internal use.
///
/// head: The head of the chain to add a new filter to. This can be null to create new chain.
/// filter: The filter.
/// excpt: &
/// ctx: &
///
/// Returns: The new head of the filter chain at refcount +1.
///
/// Raised: FilterUnsupported - If the given filter type is not supported by the run-time system.
extern binpac_filter* __binpachilti_filter_add(binpac_filter* head, binpac_filter* filter, hlt_exception** excpt, hlt_execution_context* ctx);

/// Closes and deletes a filter chain.
///
/// chain: The head of the chain containing the filter.
///
/// Raises: FilterError - If there's a problem with the filtering process.
///
/// Note: When a filter is closed but has still data pending that can't be
/// processed normally anymore (e.g., because it didn't get sufficient input
/// for decoding something completely), it must raise a FilterError
/// exception. The assumption is that if no exception is raised, all input has
/// been fully processed after closing the filter. 
extern void binpachilti_filter_close(binpac_filter* head, hlt_exception** excpt, hlt_execution_context* ctx);

/// Pipes data into a filter chain. If further filters have been chained to
/// this one, the data is passed through all of them.
///
/// chain: The chain to pass the data into.
/// data: The data to be passed in.
/// excpt: &
/// ctx: &
///
/// Returns: A bytes objects with filtered data at refcnt +1. The data being
/// returned does not necessarily correspond directly to the data passed in,
/// but represents the next chunk of data that could now be prepared by the
/// filter. Depending on how the filter operates, this may still include
/// pieces passed in for filtering earlier. The returned data may also be
/// empty if the filter can currently not produce further output. In
/// particular, an empty result does not indicate an error; these are
/// reported via exceptions.
///
/// Raises: FilterError - If there's a problem with the filtering process.
/// After raising this error, this filter must not be used again. 
extern hlt_bytes* binpachilti_filter_decode(binpac_filter* head, hlt_bytes* data, hlt_exception** excpt, hlt_execution_context* ctx);

#endif
