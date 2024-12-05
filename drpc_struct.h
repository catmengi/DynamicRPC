#pragma once

#include "drpc_types.h"
#include "hashtable.c/hashtable.h"


struct d_struct{
    pthread_mutex_t lock;

    hashtable_t hashtable;
    size_t current_len;   //ammount of elements in d_struct currently
};

struct d_struct_element{
    void* data; //may be packed type or may be a pointer(d_sizedbuf, d_struct, d_array, d_struct); d_sizedbuf and d_str are copyed on setting
    char is_packed; //used for simpler detection of pointer types
    char type;

    size_t sizedbuf_len;
};


struct d_struct* new_d_struct();
void d_struct_set(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...);
int d_struct_get(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...);
int d_struct_unlink(struct d_struct* dstruct, char* key, enum drpc_types type);
void d_struct_free(struct d_struct* dstruct);
int d_struct_remove(struct d_struct* dstruct, char* key);
void d_struct_free_internal(struct d_struct* dstruct);

char* d_struct_buf(struct d_struct* dstruct, size_t* buflen);
void buf_d_struct(char* buf, struct d_struct* dstruct);
