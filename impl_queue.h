/*
 * Copyright (c) 2017-2020 Bailey Thompson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef BKTHOMPS_CONTAINERS_QUEUE_H
#define BKTHOMPS_CONTAINERS_QUEUE_H

#include "_bk_defines.h"

/**
 * The impl_queue data structure, which adapts a container to provide a impl_queue
 * (first-in first-out). Adapts the deque container.
 */
typedef struct internal_deque *queue;

/* Starting */
queue impl_queue_init(size_t data_size);

/* Utility */
size_t impl_queue_size(queue me);
bk_bool impl_queue_is_empty(queue me);
bk_err impl_queue_trim(queue me);
void impl_queue_copy_to_array(void *arr, queue me);

/* Adding */
bk_err impl_queue_push(queue me, void *data);

/* Removing */
bk_bool impl_queue_pop(void *data, queue me);

/* Getting */
bk_bool impl_queue_front(void *data, queue me);
bk_bool impl_queue_back(void *data, queue me);

/* Ending */
bk_err impl_queue_clear(queue me);
queue impl_queue_destroy(queue me);

#endif /* BKTHOMPS_CONTAINERS_QUEUE_H */
