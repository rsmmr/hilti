// $Id$

#ifndef HILTI_CONTINUATION_H
#define HILTI_CONTINUATION_H

/// A continuation instance. We don't define the attributes further as they
/// aren't usable from C. 
typedef struct {
    void *succ;
    void *frame;
} hlt_continuation;

#endif
