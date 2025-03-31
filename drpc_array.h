#pragma once

#include "queue.h"
#include "drpc_types.h"

#include <pthread.h>
#include <sys/types.h>
struct d_array{
    size_t lookup_size;
    struct d_struct_element** lookup_table;
    pthread_mutex_t lock;
};

struct d_array* new_d_array(size_t start_cappacity);

void d_array_set(struct d_array* darray,size_t index, void* native_type, enum drpc_types type,...);
                //serializes native_type and store it. If pointer type was set (d_str,d_sizedbuf,d_array,d_struct,d_queue)
                //it CAN be changed without re-setting it because it is stored as pointer until _buf serialization happens. POINTER TYPES WILL BE FREED WHEN STRUCTURE IS FREED
                //IF IT WASNT UNLINKED(OR IN CASE of d_queue POPPED)
                /*
                * index       -- index where element will be placed
                * native_type -- pointer to native C type that will be pushed. If it is a NON pointer type you should pass this: &input else just pass: input
                * type        -- type of native_type, used to serilialize-deserialize
                * ...         -- used with d_sizedbuf type, you should provide len of d_sizedbuf here
                *
                * RETURN: 0 on success*
                */

int d_array_get(struct d_array* darray,size_t index, void* native_type, enum drpc_types type,...);
                //get element the unserialize it and store into native_type. In case of pointer types just store retrieved pointer into native_type
                /*
                * index       -- index of element to get
                * native_type -- pointer to memory where this type will be written. Generaly you should pass something like this to it: &output
                * type        -- type of native_type, used to serilialize-deserialize them
                * ...         -- used with d_sizedbuf type, you should provide pointer to size_t, it will store d_sizedbuf length there
                *
                * RETURN: 0 on success
                */
void d_array_remove(struct d_array* darray, size_t index);              //remove element at index and free it's data RETURN: 0 on success
int d_array_unlink(struct d_array* darray, size_t index);               //remove element at index but DOESNT free it's data. RETURN: 0 on success
enum drpc_types d_array_get_type(struct d_array* darray, size_t index); //returns a type of element at index. RETURN: d_void on error
size_t d_array_len(struct d_array* darray);                             //returns d_array len.
void d_array_free(struct d_array* darray);                              //free d_array and all it's data




/*DRPC's internal API's that should not be used in user code*/
void d_array_free_internal(struct d_array* darray);
char* d_array_buf(struct d_array* darray, size_t* buflen);
struct d_array* buf_d_array(char* buf);
/*==========================================================*/


