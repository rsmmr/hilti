///
/// Filters are pipelines decoding input from one form to another. In
/// Spicy, they can be attached to either sinks or parsing objects.
///
/// Currently, there's predefined list of filters available. In the future, it may be
/// possible to write them in Spicy directly as well.

#ifndef LIBSPICY_FILTER_H
#define LIBSPICY_FILTER_H

#include "libspicy.h"

struct __spicy_filter_definition;
struct spicy_filter;

typedef struct spicy_filter* (*__spicy_filter_allocate)(hlt_exception** excpt,
                                                          hlt_execution_context* ctx);
typedef void (*__spicy_filter_dtor)(hlt_type_info* ti, struct spicy_filter*,
                                     hlt_execution_context* ctx);
typedef hlt_bytes* (*__spicy_filter_decode)(struct spicy_filter*, hlt_bytes* data,
                                             hlt_exception** excpt, hlt_execution_context* ctx);
typedef void (*__spicy_filter_close)(struct spicy_filter*, hlt_exception** excpt,
                                      hlt_execution_context* ctx);

// Internal definition of a filter type.
struct __spicy_filter_definition {
    hlt_enum type;
    const char* name;
    __spicy_filter_allocate allocate;
    __spicy_filter_dtor dtor;
    __spicy_filter_decode decode;
    __spicy_filter_close close;
};

typedef struct __spicy_filter_definition __spicy_filter_definition;

/// A filter for decoding input from one representation into another. This is
/// the common header of all filter structs. In filter_*.c, we define one
/// additional struct per pre-defined filter type storing filter-specific
/// information.
struct spicy_filter {
    __hlt_gchdr __gch;               /// Header for garbage collection.
    __spicy_filter_definition* def; /// Type object describing the filter type.
    struct spicy_filter* next;      /// Link to next filter in chain.
};

typedef struct spicy_filter spicy_filter;

__HLT_DECLARE_RTTI_GC_TYPE(spicy_filter);

/// Instantiates and initializes a new filter and adds it to a filter chain.
///
/// head: The head of the chain to add a new filter to. This can be null to create new chain.
/// ftype: The type of the filter, as defined by one of SPICY_FILTER_* constants in spicy.h.
/// excpt: &
/// ctx: &
///
/// Returns: The new head of the filter chain at refcount +1.
///
/// Raised: FilterUnsupported - If the given filter type is not supported by the run-time system.
extern spicy_filter* spicyhilti_filter_add(spicy_filter* head, hlt_enum ftype,
                                             hlt_exception** excpt, hlt_execution_context* ctx);

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
extern spicy_filter* __spicyhilti_filter_add(spicy_filter* head, spicy_filter* filter,
                                               hlt_exception** excpt, hlt_execution_context* ctx);

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
extern void spicyhilti_filter_close(spicy_filter* head, hlt_exception** excpt,
                                     hlt_execution_context* ctx);

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
extern hlt_bytes* spicyhilti_filter_decode(spicy_filter* head, hlt_bytes* data,
                                            hlt_exception** excpt,
                                            hlt_execution_context* ctx); // ref!

#endif
