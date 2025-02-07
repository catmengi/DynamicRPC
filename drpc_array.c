#include "drpc_array.h"
#include "drpc_que.h"
#include "drpc_queue.h"
#include "drpc_struct.h"
#include "hashtable.c/hashtable.h"

#include <asm-generic/errno.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

struct d_array* new_d_array(size_t start_cappacity){

    if(start_cappacity == 0) start_cappacity = 1;

    struct d_array* new = malloc(sizeof(*new));
    assert(new);

    new->lookup_size = start_cappacity;

    new->lookup_table = calloc(new->lookup_size,sizeof(*new->lookup_table));
    assert(new->lookup_table);

    return new;
}

static inline void d_array_set_internal(struct d_array* darray, size_t index, struct d_struct_element* el){
    if(index >= darray->lookup_size){
        size_t prev_size = darray->lookup_size;
        void* realloced = realloc(darray->lookup_table,sizeof(*darray->lookup_table) * (index+1));
        assert(realloced);

        darray->lookup_table = realloced;
        darray->lookup_size = index+1;
        for(size_t i = prev_size; i < darray->lookup_size; i++){
            darray->lookup_table[i] = NULL;
        }
    }

    darray->lookup_table[index] = el;
}

static inline struct d_struct_element* d_array_get_internal(struct d_array* darray, size_t index){
    if(index >= darray->lookup_size) return NULL;

    return darray->lookup_table[index];
}

static inline void d_array_del_internal(struct d_array* darray, size_t index){
    if(index >= darray->lookup_size) return;
    if(darray->lookup_table[index] == NULL) return;

    darray->lookup_table[index] = NULL;
}

void d_array_set(struct d_array* darray,size_t index, void* native_type, enum drpc_types type,...){
    assert(darray); assert(native_type);
    pthread_mutex_lock(&darray->lock);
    struct d_struct_element* element = d_array_get_internal(darray,index);
    if(element == NULL){
        element = calloc(1,sizeof(*element)); assert(element);
    }else{
        if(element->is_packed == 1){
            drpc_type_free(element->data);
            free(element->data);
        }else{
            switch(element->type){
                case d_sizedbuf:
                    free(element->data);
                    break;
                case d_str:
                    free(element->data);
                    break;
                case d_queue:
                    d_queue_free(element->data);
                    break;
                case d_array:
                    d_array_free(element->data);
                    break;
                case d_struct:
                    d_struct_free(element->data);
                    break;
            }
        }
    }

    switch(type){
        case d_int8:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            int8_to_drpc(element->data,*(int8_t*)native_type);
            break;
        case d_uint8:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            uint8_to_drpc(element->data,*(uint8_t*)native_type);
            break;
        case d_int16:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            int16_to_drpc(element->data,*(int16_t*)native_type);
            break;
        case d_uint16:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            uint16_to_drpc(element->data,*(uint16_t*)native_type);
            break;
        case d_int32:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            int32_to_drpc(element->data,*(int32_t*)native_type);
            break;
        case d_uint32:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            uint32_to_drpc(element->data,*(uint32_t*)native_type);
            break;
        case d_int64:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            int64_to_drpc(element->data,*(int64_t*)native_type);
            break;
        case d_uint64:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            uint64_to_drpc(element->data,*(uint64_t*)native_type);
            break;

        case d_float:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            float_to_drpc(element->data,*(float*)native_type);
            break;
        case d_double:
            element->is_packed = 1;
            element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
            element->type = type;
            double_to_drpc(element->data,*(double*)native_type);
            break;

        case d_sizedbuf:
            element->type = type;

            element->is_packed = 0;

            va_list varargs;
            va_start(varargs,type);
            element->sizedbuf_len = va_arg(varargs,size_t);

            element->data = malloc(element->sizedbuf_len); assert(element->data);

            memcpy(element->data,native_type,element->sizedbuf_len);
            break;
        case d_str:
            element->type = type;
            element->is_packed = 0;
            element->data = strdup(native_type);
            break;
        case d_struct:
            element->type = type;
            element->is_packed = 0;
            element->data = native_type;
            break;
        case d_queue:
            element->type = type;
            element->is_packed = 0;
            element->data = native_type;
            break;
        case d_array:
            element->type = type;
            element->is_packed = 0;
            element->data = native_type;
            break;
        default:
            free(element);
            pthread_mutex_unlock(&darray->lock);
            return;

    }
    d_array_set_internal(darray,index,element);
    pthread_mutex_unlock(&darray->lock);
}

int d_array_get(struct d_array* darray,size_t index, void* native_type, enum drpc_types type,...){
    assert(darray); assert(native_type);
    pthread_mutex_lock(&darray->lock);

    struct d_struct_element* element = d_array_get_internal(darray,index);
    if(element == NULL) {pthread_mutex_unlock(&darray->lock); ;return 1;}
    if(element->type != type) {pthread_mutex_unlock(&darray->lock); ;return 1;}

    switch(type){
        default:
            *(void**)native_type = element->data;
            if(type == d_sizedbuf){
                va_list varargs;
                va_start(varargs,type);
                size_t* sizedbuf_len = va_arg(varargs,size_t*);
                *sizedbuf_len = element->sizedbuf_len;
            }
            break;
        case d_int8:
            *(int8_t*)native_type = drpc_to_int8(element->data);
            break;
        case d_uint8:
            *(uint8_t*)native_type = drpc_to_uint8(element->data);
            break;
        case d_int16:
            *(int16_t*)native_type = drpc_to_int16(element->data);
            break;
        case d_uint16:
            *(uint16_t*)native_type = drpc_to_uint16(element->data);
            break;
        case d_int32:
            *(int32_t*)native_type = drpc_to_int32(element->data);
            break;
        case d_uint32:
            *(uint32_t*)native_type = drpc_to_uint32(element->data);
            break;
        case d_int64:
            *(int64_t*)native_type = drpc_to_int64(element->data);
            break;
        case d_uint64:
            *(uint64_t*)native_type = drpc_to_uint64(element->data);
            break;
        case d_float:
            *(float*)native_type = drpc_to_float(element->data);
            break;
        case d_double:
            *(double*)native_type = drpc_to_double(element->data);
            break;
    }
    pthread_mutex_unlock(&darray->lock);
    return 0;
}

void d_array_remove(struct d_array* darray, size_t index){
    assert(darray);
    struct d_struct_element* element = d_array_get_internal(darray,index);
    if(element == NULL) return;

    d_array_del_internal(darray,index);

    if(element->is_packed == 1){
        drpc_type_free(element->data);
        free(element->data);
    }else{
        switch(element->type){
            case d_sizedbuf:
                free(element->data);
                break;
            case d_str:
                free(element->data);
                break;
            case d_struct:
                d_struct_free(element->data);
                break;
            case d_queue:
                d_queue_free(element->data);
                break;
            case d_array:
                d_array_free(element->data);
                break;
        }
    }
    free(element);
}

enum drpc_types drpc_array_get_type(struct d_array* darray, size_t index){
    struct d_struct_element* element = d_array_get_internal(darray,index);
    enum drpc_types ret = d_void;
    if(element != NULL){
        ret = element->type;
    }
    return ret;
}

int d_array_unlink(struct d_array* darray, size_t index, enum drpc_types type){
    struct d_struct_element* element = d_array_get_internal(darray,index);

    if(element){
        if(element->is_packed == 0){
            if(element->type == type){
                d_array_del_internal(darray,index);
                free(element);
                return 0;
            }
        }
    }
    return 1;
}

void d_array_free_internal(struct d_array* darray){
    for(size_t i = 0; i < darray->lookup_size; i++){
        if(darray->lookup_table[i] == NULL) continue;

        d_array_remove(darray,i);
    }
    free(darray->lookup_table);
}

void d_array_free(struct d_array* darray){
    d_array_free_internal(darray);
    free(darray);
}

char* d_array_buf(struct d_array* darray, size_t* buflen){
    struct d_struct* packed = new_d_struct();

    for(size_t i = 0; i < darray->lookup_size; i++){
        if(darray->lookup_table[i] == NULL) continue;

        char key[64];
        char* keyp;
        sprintf(key,"%lu",i);
        keyp = strdup(key);

        pthread_mutex_lock(&packed->lock);

        hashtable_set(packed->hashtable,keyp,darray->lookup_table[i]);   //low level hashtable manipulations to set to already existing elements, because they are in the same format
        packed->current_len++;
        drpc_que_push(packed->heap_keys,keyp);

        pthread_mutex_unlock(&packed->lock);

    }
    char* buf = d_struct_buf(packed,buflen);

    void* freep = NULL;
    while((freep = drpc_que_pop(packed->heap_keys)) != NULL){
        free(freep);
    }
    drpc_que_free(packed->heap_keys);
    hashtable_destroy(packed->hashtable);
    free(packed);
    return buf;
}

struct d_array* buf_d_array(char* buf){
    struct d_struct* packed = new_d_struct();
    buf_d_struct(buf,packed);


    char** keys; enum drpc_types* unused;
    size_t keys_ammount = d_struct_fields(packed,&keys,&unused);
    struct d_array* new = new_d_array(keys_ammount);

    for(size_t i = 0; i < keys_ammount; i++){
        size_t set_at = atol(keys[i]);
        d_array_set_internal(new,set_at,hashtable_get(packed->hashtable,keys[i]));
    }
    free(unused);

    void* freep = NULL;
    while((freep = drpc_que_pop(packed->heap_keys)) != NULL){
        free(freep);
    }
    free(keys);

    drpc_que_free(packed->heap_keys);
    hashtable_destroy(packed->hashtable);
    free(packed);

    return new;
}

size_t d_array_len(struct d_array* darray){
    assert(darray);
    return darray->lookup_size;
}
