#pragma once

#include "drpc_que.h"
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
int d_array_get(struct d_array* darray,size_t index, void* native_type, enum drpc_types type,...);
void d_array_remove(struct d_array* darray, size_t index);
int d_array_unlink(struct d_array* darray, size_t index, enum drpc_types type);
enum drpc_types d_array_get_type(struct d_array* darray, size_t index);
size_t d_array_len(struct d_array* darray);

void d_array_free(struct d_array* darray);
void d_array_free_internal(struct d_array* darray);

char* d_array_buf(struct d_array* darray, size_t* buflen);
struct d_array* buf_d_array(char* buf);


