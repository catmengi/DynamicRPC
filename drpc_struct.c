#include "drpc_protocol.h"
#include "queue.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "drpc_queue.h"
#include "drpc_array.h"
#include "hashtable.c/hashtable.h"

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct d_struct* new_d_struct(){
    struct d_struct* d_struct = malloc(sizeof(*d_struct)); assert(d_struct);

    d_struct->hashtable = hashtable_create();
    d_struct->current_len = 0;

    d_struct->heap_keys = queue_create();

    return d_struct;
}

void d_struct_set(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...){
    assert(dstruct); assert(key);
    if(native_type == NULL) return;
    struct d_struct_element* element = NULL;

    char* heap_key = strdup(key); assert(heap_key);
    queue_push(dstruct->heap_keys,heap_key);

    if((element = hashtable_get(dstruct->hashtable,heap_key)) == NULL){
        element = malloc(sizeof(*element)); assert(element);
        hashtable_set(dstruct->hashtable,heap_key,element);
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
        dstruct->current_len--;
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
            hashtable_remove(dstruct->hashtable,heap_key);
            free(element);
            return;
    }
    dstruct->current_len++;
}

int d_struct_get(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...){
    assert(dstruct); assert(key); assert(native_type);

    struct d_struct_element* element = NULL;
    element = hashtable_get(dstruct->hashtable,key);
    if(element == NULL) return 1;
    if(element->type != type) return 1;

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

    return 0;
}
int d_struct_unlink(struct d_struct* dstruct, char* key){
    assert(dstruct); assert(key);
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    if(element != NULL && element->is_packed == 0){
        free(element);
        hashtable_remove(dstruct->hashtable,key);
        dstruct->current_len--;
        return 0;
    }
    return 1;
}

int d_struct_remove(struct d_struct* dstruct, char* key){
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    if(element != NULL){
        hashtable_remove(dstruct->hashtable,key);

        dstruct->current_len--;

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
        return 0;
    }
    return 1;
}

enum drpc_types d_struct_get_type(struct d_struct* dstruct, char* key){
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    enum drpc_types ret = d_void;
    if(element != NULL){
        ret = element->type;
    }
    return ret;
}

struct __d_struct_preser_thrd_input{
    char* key;
    struct d_struct_element* el;
};
struct __d_struct_thrd_serialise_param{
    struct d_struct* dstruct;
    struct drpc_type* output_type;
    char** keys;

    sem_t pre_serialise_wait;
    sem_t final_serialise_wait;
};

static void* d_struct_thrd_data_gather(void* param_P){
    struct __d_struct_thrd_serialise_param* param = param_P;
    size_t j = 0;
    for(size_t i = 0; i < param->dstruct->hashtable->capacity; i++){
        if(param->dstruct->hashtable->body[i].value != NULL && param->dstruct->hashtable->body[i].key != NULL && param->dstruct->hashtable->body[i].key != (char*)0xDEAD){

            param->keys[j] = param->dstruct->hashtable->body[i].key; j++;
            sem_post(&param->pre_serialise_wait);
        }
    }
    return NULL;
}

static void* d_struct_thrd_preserialise(void* param_P){
    struct __d_struct_thrd_serialise_param* param = param_P;

    for(size_t i = 0; i < param->dstruct->current_len; i++){
        assert(sem_wait(&param->pre_serialise_wait) == 0);
        char* key = param->keys[i];
        struct d_struct_element* element = hashtable_get(param->dstruct->hashtable,key);

        enum drpc_types type = element->type;
        struct drpc_type* packed_type = NULL;
        if(element->is_packed == 0){
            packed_type = malloc(sizeof(*packed_type)); assert(packed_type);
            switch(type){
                case d_sizedbuf:
                    sizedbuf_to_drpc(packed_type,element->data,element->sizedbuf_len);
                    break;
                case d_struct:
                    d_struct_to_drpc(packed_type,element->data);
                    break;
                case d_queue:
                    d_queue_to_drpc(packed_type,element->data);
                    break;
                case d_array:
                    d_array_to_drpc(packed_type,element->data);
                    break;
                case d_str:
                    str_to_drpc(packed_type,element->data);
                    break;
                default: break;
            }
        } else packed_type = element->data;

        size_t type_len = strlen(key) + 1 + drpc_type_buflen(packed_type);
        char* keyed_buf = malloc(type_len); assert(keyed_buf);
        char* edit_buf = keyed_buf;

        memcpy(edit_buf,key,strlen(key) + 1); edit_buf += strlen(key) + 1;

        drpc_buf(packed_type,edit_buf);

        param->output_type[i].packed_data = keyed_buf;
        param->output_type[i].type = type;
        param->output_type[i].len = type_len;

        sem_post(&param->final_serialise_wait);

        if(element->is_packed == 0) {
            drpc_type_free(packed_type);
            free(packed_type);
        }
    }
    return NULL;
}

static void* d_struct_thrd_finalserialise(void* param_P){
    struct __d_struct_thrd_serialise_param* param = param_P;

    return drpc_types_buf_threaded(param->output_type,&param->final_serialise_wait,param->dstruct->current_len);
}

char* d_struct_buf(struct d_struct* dstruct, size_t* buflen){
    struct __d_struct_thrd_serialise_param param;
    param.dstruct = dstruct;
    size_t alloc_len = (dstruct->current_len == 0 ? 1 : dstruct->current_len); //ESP-IDF fix
    param.output_type = malloc(alloc_len * sizeof(*param.output_type));
    param.keys = malloc(alloc_len * sizeof(char*));
    assert(param.output_type);
    assert(param.keys);

    sem_init(&param.pre_serialise_wait,0,0);
    sem_init(&param.final_serialise_wait,0,0);

    pthread_t data_gather;
    pthread_t pre_serialise;
    pthread_t final_serialise;

    assert(pthread_create(&data_gather,NULL,d_struct_thrd_data_gather,&param) == 0);
    assert(pthread_create(&pre_serialise,NULL,d_struct_thrd_preserialise,&param) == 0);
    assert(pthread_create(&final_serialise,NULL,d_struct_thrd_finalserialise,&param) == 0);

    void* ret = NULL;
    struct drpc_types_buf_threaded_output* output;
    assert(pthread_join(data_gather,NULL) == 0);
    assert(pthread_join(pre_serialise,NULL) == 0);
    assert(pthread_join(final_serialise,&ret) == 0);
    output = ret;

    sem_destroy(&param.pre_serialise_wait);
    sem_destroy(&param.final_serialise_wait);

    drpc_types_free(param.output_type,dstruct->current_len);
    free(param.keys);

    *buflen = output->buflen;
    char* buf = output->buf;
    free(output);
    return buf;
}

void buf_d_struct(char* buf, struct d_struct* dstruct){
    size_t packed_types_len = 0;
    struct drpc_type* packed_types = buf_drpc_types(buf,&packed_types_len);

    for(size_t i = 0; i < packed_types_len; i++){
        if(packed_types[i].type == d_void) continue;
        char* key = strdup(packed_types[i].packed_data);
        queue_push(dstruct->heap_keys,key);

        void* type_packed = packed_types[i].packed_data + strlen(key) + 1;
        struct drpc_type* type = NULL;

        struct d_struct_element* element = calloc(1,sizeof(*element)); assert(element);
        switch(packed_types[i].type){
            case d_str:
                type = malloc(sizeof(*type)); assert(type);
                buf_drpc(type,type_packed);
                element->is_packed = 0;
                element->type = packed_types[i].type;
                element->data = drpc_to_str(type);
                drpc_type_free(type);free(type);
                break;
            case d_sizedbuf:
                type = malloc(sizeof(*type)); assert(type);
                buf_drpc(type,type_packed);
                element->is_packed = 0;
                element->type = packed_types[i].type;
                element->data = drpc_to_sizedbuf(type,&element->sizedbuf_len);
                drpc_type_free(type);free(type);
                break;
            case d_array:
                type = malloc(sizeof(*type)); assert(type);
                buf_drpc(type,type_packed);
                element->is_packed = 0;
                element->type = packed_types[i].type;
                element->data = drpc_to_d_array(type);
                drpc_type_free(type);free(type);
                break;
            case d_struct:
                type = malloc(sizeof(*type)); assert(type);
                buf_drpc(type,type_packed);
                element->is_packed = 0;
                element->type = packed_types[i].type;
                element->data = drpc_to_d_struct(type);
                drpc_type_free(type);free(type);
                break;
            case d_queue:
                type = malloc(sizeof(*type)); assert(type);
                buf_drpc(type,type_packed);
                element->is_packed = 0;
                element->type = packed_types[i].type;
                element->data = drpc_to_d_queue(type);
                drpc_type_free(type);free(type);
                break;
            default:
                element->is_packed = 1;
                element->type = packed_types[i].type;
                element->data = malloc(sizeof(struct drpc_type)); assert(element->data);

                buf_drpc(element->data,type_packed);
                break;
        }

        hashtable_set(dstruct->hashtable,key,element);
        dstruct->current_len++;
    }
    drpc_types_free(packed_types,packed_types_len);
}
char** d_struct_get_fields(struct d_struct* dstruct, size_t* len){
    *len = dstruct->current_len;
    if(*len > 0){
        char** keys = calloc(*len,sizeof(char**)); assert(keys != NULL);
        size_t j = 0;
        for(size_t i = 0; i < dstruct->hashtable->capacity; i++){
            if(dstruct->hashtable->body[i].value != NULL && dstruct->hashtable->body[i].key != NULL && dstruct->hashtable->body[i].key != (char*)0xDEAD){
                keys[j] = dstruct->hashtable->body[i].key; j++;
            }
        }
        return keys;
    }
    return NULL;
}

void d_struct_free_internal(struct d_struct* dstruct){
    for(size_t i = 0; i < dstruct->hashtable->capacity; i++){
        if(dstruct->hashtable->body[i].value != NULL && dstruct->hashtable->body[i].key != NULL && dstruct->hashtable->body[i].key != (char*)0xDEAD){
            struct d_struct_element* element = dstruct->hashtable->body[i].value;
            if(element->is_packed == 1){
                drpc_type_free(element->data);
                free(element->data);
                free(element);
                continue;
            }
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
            free(element);
        }
    }
    hashtable_destroy(dstruct->hashtable);
///////////
    char* heap_key = NULL;
    while((heap_key = queue_pop(dstruct->heap_keys)) != NULL){
        free(heap_key);
    }
    queue_free(dstruct->heap_keys);
///////////
}
void d_struct_free(struct d_struct* dstruct){
    if(dstruct == NULL) return;
    d_struct_free_internal(dstruct);
    free(dstruct);
}
