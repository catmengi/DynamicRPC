#pragma once
#include "drpc_que.h"
#include "drpc_types.h"

struct d_queue{
    pthread_mutex_t lock;
    struct drpc_que* que;
};

struct d_queue* new_d_queue();
void d_queue_push(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);
int d_queue_pop(struct d_queue* dqueue, void* native_type, enum drpc_types type,...);

void d_queue_free(struct d_queue* dqueue);
void d_queue_free_internals(struct d_queue* dqueue);

char* d_queue_buf(struct d_queue* dqueue,size_t* buflen);
void buf_d_queue(char* buf, struct d_queue* dqueue);

size_t d_queue_len(struct d_queue* dqueue);
enum drpc_types d_queue_top_type(struct d_queue* dqueue);

