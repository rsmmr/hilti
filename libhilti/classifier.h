///
/// \addtogroup classifier
/// @{
/// Functions for classifiers.
/// @}

#ifndef LIBHILTI_CLASSIFIER_H
#define LIBHILTI_CLASSIFIER_H

#include <stdint.h>

#include "context.h"
#include "exceptions.h"

/// Structure describing one field of a classifier rule. The classifier takes ownership.
typedef struct {
    uint64_t len;   /// Number of bytes of the data part of the field.
    uint64_t bits;  /// Number of bits that are valid in the data.
    uint8_t data[]; /// The data bytes for this field.
} hlt_classifier_field;

typedef struct __hlt_classifier hlt_classifier;

/// Instantiates a new classifier.
///
/// num_fields: The number of fields that rules for this classifier have.
///
/// rtype: The rule type.
///
/// vtype: The result type associated with rules.
///
/// excpt: &
extern hlt_classifier* hlt_classifier_new(int64_t num_fields, const hlt_type_info* rtype,
                                          const hlt_type_info* vtype, hlt_exception** excpt,
                                          hlt_execution_context* ctx);

/// Adds a rule to the classifier.
///
/// This function must not be called after ~~hlt_classifier_compile has been
/// executed.
///
/// c: The classifier to add the rule to.
///
/// fields: The fields for the classifier for the rule. Their number must
/// match with what as passed to ~~hlt_map_new. The classifier takes
/// ownership of the array and its elements.
///
/// priority: The priority associated with the rule. The highest-priority
/// matching rule be returned for lookups.
///
/// vtype: The result type associated with the rule, which must match what
/// was passed to ~~hlt_classifier_new.
///
/// excpt: &
extern void hlt_classifier_add(hlt_classifier* c, hlt_classifier_field** fields, int64_t priority,
                               const hlt_type_info* vtype, void* value, hlt_exception** excpt,
                               hlt_execution_context* ctx);

/// Adds a rule to the classifier. Similar to ~~hlt_classifier_add except that
/// it sets the priority automatically to the highest priority seen so plus one.
///
/// c: The classifier to add the rule to.
///
/// fields: The fields for the classifier for the rule. Their number must
/// match with what as passed to ~~hlt_map_new. The classifier takes
/// ownership of the array and its elements.
///
/// vtype: The result type associated with the rule, which must match what
/// was passed to ~~hlt_classifier_new.
///
/// excpt: &
extern void hlt_classifier_add_no_prio(hlt_classifier* c, hlt_classifier_field** fields,
                                       const hlt_type_info* vtype, void* value,
                                       hlt_exception** excpt, hlt_execution_context* ctx);

/// Fixes the rules compiled so far and enable subsequent lookups.
///
/// c: The classifier.
///
/// excpt: &
extern void hlt_classifier_compile(hlt_classifier* c, hlt_exception** excpt,
                                   hlt_execution_context* ctx);

/// Returns true if their as rule matching the given key. For each key field,
/// this performs a longest-matching-prefix match.
///
/// This function must not be called before ~~hlt_classifier_compile has been
/// executed.
///
/// c: The classifier.
///
/// vals: The key values to look up; their number must match the number of
/// fields the classifier has.
///
/// excpt: &
///
/// Returns: True if there's a matching rule, false if not.
extern int8_t hlt_classifier_matches(hlt_classifier* c, hlt_classifier_field** vals,
                                     hlt_exception** excpt, hlt_execution_context* ctx);

/// Gets the value associated with the first rule matching the given key. For
/// each key field, this performs a longest-matching-prefix match. Of all
/// matching rules, the one with the highest priority will be choosen. If
/// multiple matching rules have the same priority, it is undefined which one
/// will be considered the match.
///
/// This function must not be called before ~~hlt_classifier_compile has been
/// executed.
///
/// c: The classifier.
///
/// vals: The key values to look up; their number must match the number of
/// fields the classifier has.
///
/// excpt: &
///
/// Returns: The value.
///
/// Raises: IndexError - If no matching rule exists.
extern void* hlt_classifier_get(hlt_classifier* c, hlt_classifier_field** vals,
                                hlt_exception** excpt, hlt_execution_context* ctx);

#endif
