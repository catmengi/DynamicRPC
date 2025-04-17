#pragma once
#include "queue.h"
#include "drpc_types.h"

struct d_queue{
    queue_t que;
    enum drpc_types last_element_type; //i cannot access to queue raw without changing its order(((
};

struct d_queue* new_d_queue();

//======================================================================================================================================================================================
void d_queue_push(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);
                                                    //serializes native_type and store it. If pointer type was set (d_str,d_sizedbuf,d_array,d_struct,d_queue)
                                                    //it CAN be changed without re-setting it because it is stored as pointer until _buf serialization happens.
                                                    //POINTER TYPES WILL BE FREED WHEN STRUCTURE IS FREED IF IT WASNT UNLINKED(OR IN CASE of d_queue POPPED)

                                                    // native_type -- pointer to native C type that will be pushed. If it is a NON pointer type you should pass this: &input else just pass: input
                                                    // type        -- type of native_type, used to serilialize-deserialize
                                                    // ...         -- used with d_sizedbuf type, you should provide len of d_sizedbuf here

                                                    // RETURN: 0 on success
//======================================================================================================================================================================================

//======================================================================================================================================================================================
int d_queue_pop(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);
                                                    //get element the unserialize it and store into native_type. In case of pointer types just store retrieved pointer into native_type
                                                    //if you tried to pop element with WRONG type that element WILL BE MOVED TO QUEUE'S END

                                                    // native_type -- pointer to memory where this type will be written. Generaly you should pass something like this to it: &output
                                                    // type        -- type of native_type, used to serilialize-deserialize them
                                                    // ...         -- used with d_sizedbuf type, you should provide pointer to size_t, it will store d_sizedbuf length there

                                                    // RETURN: 0 on success
//======================================================================================================================================================================================

//======================================================================================================================================================================================
int d_queue_pop_with_type(struct d_queue* dqueue, void* native_type, enum drpc_types* out_type,...);
                                                    //same as d_queue_pop but does not require type. Instead it outputs elements type in out_type
//======================================================================================================================================================================================


size_t d_queue_len(struct d_queue* dqueue);                       //return d_queue len
void d_queue_free(struct d_queue* dqueue);                        //free d_queue and all it's data

struct d_queue* d_queue_copy(struct d_queue* dqueue);             //return copy of dqueue




/*DRPC's internal API's that should not be used in user code*/
void d_queue_free_internals(struct d_queue* dqueue);

char* d_queue_buf(struct d_queue* dqueue,size_t* buflen);
void buf_d_queue(char* buf, struct d_queue* dqueue);
/*==========================================================*/


