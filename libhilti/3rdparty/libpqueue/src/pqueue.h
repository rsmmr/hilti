/*
 * Copyright 2010 Volkan Yazıcı <volkan.yazici@gmail.com>
 * Copyright 2006-2010 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */


/**
 * @file  pqueue.h
 * @brief Priority Queue function declarations
 *
 * @{
 */


#ifndef PQUEUE_H
#define PQUEUE_H

#include "../../../memory.h"
#include "../../../timer.h"

/** priority data type */
typedef hlt_time pqueue_pri_t;

/** the priority queue handle */
typedef struct pqueue_t
{
    size_t size;
    size_t avail;
    size_t step;
    void **d;
} pqueue_t;


/**
 * initialize the queue
 *
 * @param n the initial estimate of the number of queue items for which memory
 *          should be preallocated
 * @param pri the callback function to run to assign a score to a element
 * @param get the callback function to get the current element's position
 * @param set the callback function to set the current element's position
 *
 * @Return the handle or NULL for insufficent memory
 */
pqueue_t *
pqueue_init(size_t n);

/**
 * free all memory used by the queue
 * @param q the queue
 */
void pqueue_free(pqueue_t *q);


/**
 * return the size of the queue.
 * @param q the queue
 */
size_t pqueue_size(pqueue_t *q);


/**
 * insert an item into the queue.
 * @param q the queue
 * @param d the item
 * @return 0 on success
 */
int pqueue_insert(pqueue_t *q, void *d);


/**
 * move an existing entry to a different priority
 * @param q the queue
 * @param old the old priority
 * @param d the entry
 */
void
pqueue_change_priority(pqueue_t *q,
                       pqueue_pri_t new_pri,
                       void *d);


/**
 * pop the highest-ranking item from the queue.
 * @param p the queue
 * @param d where to copy the entry to
 * @return NULL on error, otherwise the entry
 */
void *pqueue_pop(pqueue_t *q);


/**
 * remove an item from the queue.
 * @param p the queue
 * @param d the entry
 * @return 0 on success
 */
int pqueue_remove(pqueue_t *q, void *d);


/**
 * access highest-ranking item without removing it.
 * @param q the queue
 * @param d the entry
 * @return NULL on error, otherwise the entry
 */
void *pqueue_peek(pqueue_t *q);


#if 0
/**
 * print the queue
 * @internal
 * DEBUG function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
pqueue_print(pqueue_t *q, 
             FILE *out, 
             pqueue_print_entry_f print);

/**
 * dump the queue and it's internal structure
 * @internal
 * debug function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
pqueueu_dump(pqueue_t *q, 
             FILE *out,
             pqueue_print_entry_f print);

#endif

/**
 * checks that the pq is in the right order, etc
 * @internal
 * debug function only
 * @param q the queue
 */
int pqueue_is_valid(pqueue_t *q);


#endif /* PQUEUE_H */
/** @} */
