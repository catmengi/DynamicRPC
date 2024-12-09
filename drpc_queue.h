#pragma once
#include "drpc_que.h"
#include "drpc_types.h"

struct d_queue{
    pthread_mutex_t lock;
    struct drpc_que* que;
};

struct d_queue* new_d_queue();

void d_queue_push(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);
                /*
                 * native_type -- pointer to native C type that will be pushed
                 * type        -- type of native_type, used to serilialize-deserialize
                 * ...         -- used with d_sizedbuf type, used as d_sizedbuf len
                */

int d_queue_pop(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);
                /*
                * native_type -- pointer to memory where this type will be written
                * type        -- type of native_type, used to serilialize-deserialize them
                * ...         -- used with d_sizedbuf type, used as d_sizedbuf len output pointer
                */

void d_queue_free(struct d_queue* dqueue);
void d_queue_free_internals(struct d_queue* dqueue);

char* d_queue_buf(struct d_queue* dqueue,size_t* buflen);
void buf_d_queue(char* buf, struct d_queue* dqueue);

size_t d_queue_len(struct d_queue* dqueue);                       //get d_queue len
enum drpc_types d_queue_top_type(struct d_queue* dqueue);         //get type of d_queue top element

