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

#include "deque.h"
#include "impl_queue.h"

/**
 * Initializes a impl_queue.
 *
 * @param data_size the size of each element; must be positive
 *
 * @return the newly-initialized impl_queue, or NULL if it was not successfully
 *         initialized due to either invalid input arguments or memory
 *         allocation error
 */
queue impl_queue_init(const size_t data_size)
{
    return deque_init(data_size);
}

/**
 * Determines the size of the impl_queue.
 *
 * @param me the impl_queue to get size of
 *
 * @return the impl_queue size
 */
size_t impl_queue_size(queue me)
{
    return deque_size(me);
}

/**
 * Determines if the impl_queue is empty. The impl_queue is empty if it contains no
 * elements.
 *
 * @param me the impl_queue to check if empty
 *
 * @return BK_TRUE if the impl_queue is empty, otherwise BK_FALSE
 */
bk_bool impl_queue_is_empty(queue me)
{
    return deque_is_empty(me);
}

/**
 * Frees the unused memory in the impl_queue.
 *
 * @param me the impl_queue to trim
 *
 * @return  BK_OK     if no error
 * @return -BK_ENOMEM if out of memory
 */
bk_err impl_queue_trim(queue me)
{
    return deque_trim(me);
}

/**
 * Copies the impl_queue to an array. Since it is a copy, the array may be modified
 * without causing side effects to the impl_queue data structure. Memory is not
 * allocated, thus the array being used for the copy must be allocated before
 * this function is called.
 *
 * @param arr the initialized array to copy the impl_queue to
 * @param me  the impl_queue to copy to the array
 */
void impl_queue_copy_to_array(void *const arr, queue me)
{
    deque_copy_to_array(arr, me);
}

/**
 * Adds an element to the impl_queue. The pointer to the data being passed in should
 * point to the data type which this impl_queue holds. For example, if this impl_queue
 * holds integers, the data pointer should be a pointer to an integer. Since the
 * data is being copied, the pointer only has to be valid when this function is
 * called.
 *
 * @param me   the impl_queue to add an element to
 * @param data the data to add to the impl_queue
 *
 * @return  BK_OK     if no error
 * @return -BK_ENOMEM if out of memory
 * @return -BK_ERANGE if size has reached representable limit
 */
bk_err impl_queue_push(queue me, void *const data)
{
    return deque_push_back(me, data);
}

/**
 * Removes the next element in the impl_queue and copies the data. The pointer to the
 * data being obtained should point to the data type which this impl_queue holds. For
 * example, if this impl_queue holds integers, the data pointer should be a pointer
 * to an integer. Since this data is being copied from the array to the data
 * pointer, the pointer only has to be valid when this function is called.
 *
 * @param data the data to have copied from the impl_queue
 * @param me   the impl_queue to pop the next element from
 *
 * @return BK_TRUE if the impl_queue contained elements, otherwise BK_FALSE
 */
bk_bool impl_queue_pop(void *const data, queue me)
{
    return deque_pop_front(data, me) == 0;
}

/**
 * Gets the front element of the impl_queue. The pointer to the data being obtained
 * should point to the data type which this impl_queue holds. For example, if this
 * impl_queue holds integers, the data pointer should be a pointer to an integer.
 * Since this data is being copied from the array to the data pointer, the
 * pointer only has to be valid when this function is called.
 *
 * @param data the copy of the front element of the impl_queue
 * @param me   the impl_queue to copy from
 *
 * @return BK_TRUE if the impl_queue contained elements, otherwise BK_FALSE
 */
bk_bool impl_queue_front(void *const data, queue me)
{
    return deque_get_first(data, me) == 0;
}

/**
 * Gets the back element of the impl_queue. The pointer to the data being obtained
 * should point to the data type which this impl_queue holds. For example, if this
 * impl_queue holds integers, the data pointer should be a pointer to an integer.
 * Since this data is being copied from the array to the data pointer, the
 * pointer only has to be valid when this function is called.
 *
 * @param data the copy of the back element of the impl_queue
 * @param me   the impl_queue to copy from
 *
 * @return BK_TRUE if the impl_queue contained elements, otherwise BK_FALSE
 */
bk_bool impl_queue_back(void *const data, queue me)
{
    return deque_get_last(data, me) == 0;
}

/**
 * Clears the impl_queue and sets it to the original state from initialization.
 *
 * @param me the impl_queue to clear
 *
 * @return  BK_OK     if no error
 * @return -BK_ENOMEM if out of memory
 */
bk_err impl_queue_clear(queue me)
{
    return deque_clear(me);
}

/**
 * Destroys the impl_queue. Performing further operations after calling this function
 * results in undefined behavior. Freeing NULL is legal, and causes no operation
 * to be performed.
 *
 * @param me the impl_queue to destroy
 *
 * @return NULL
 */
queue impl_queue_destroy(queue me)
{
    return deque_destroy(me);
}
