#include "drpc_que.h"
#include "drpc_types.h"
#include "drpc_struct.h"
#include "drpc_queue.h"
#include "drpc_array.h"
#include "hashtable.c/hashtable.h"

#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

struct d_struct* new_d_struct(){
    struct d_struct* d_struct = malloc(sizeof(*d_struct)); assert(d_struct);

    d_struct->hashtable = hashtable_create();
    d_struct->current_len = 0;

    d_struct->heap_keys = drpc_que_create();

    return d_struct;
}

void d_struct_set(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...){
    assert(dstruct); assert(key);
    if(native_type == NULL) return;
    struct d_struct_element* element = NULL;

    char* heap_key = strdup(key); assert(heap_key);
    drpc_que_push(dstruct->heap_keys,heap_key);

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
int d_struct_unlink(struct d_struct* dstruct, char* key, enum drpc_types type){
    assert(dstruct); assert(key); assert(type > 0);
    int ret = 1;
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    if(element != NULL){
        if(element->type == type && element->is_packed == 0){
            free(element);
            hashtable_remove(dstruct->hashtable,key);
            dstruct->current_len--;
            ret = 0;
        }
    }
    return ret;
}

int d_struct_remove(struct d_struct* dstruct, char* key){
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    int ret = 1;
    if(element != NULL){
        ret = 0;
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
    }
    return ret;
}

enum drpc_types d_struct_get_type(struct d_struct* dstruct, char* key){
    struct d_struct_element* element = hashtable_get(dstruct->hashtable,key);
    enum drpc_types ret = d_void;
    if(element != NULL){
        ret = element->type;
    }
    return ret;
}

char* d_struct_buf(struct d_struct* dstruct, size_t* buflen){
    struct drpc_type* packed = calloc(dstruct->current_len, sizeof(*packed));

    //gathering elements from hashtable
    struct drpc_que* element_queue = drpc_que_create();
    struct drpc_que* key_queue = drpc_que_create();
    for(size_t i = 0; i < dstruct->hashtable->capacity; i++){
        if(dstruct->hashtable->body[i].value != NULL && dstruct->hashtable->body[i].key != NULL && dstruct->hashtable->body[i].key != (char*)0xDEAD){
            drpc_que_push(element_queue,dstruct->hashtable->body[i].value);
            drpc_que_push(key_queue,dstruct->hashtable->body[i].key);
        }
    }
    //=================================

    size_t elements_len = drpc_que_get_len(element_queue);
    for(size_t i = 0 ; i < elements_len; i++){
        struct d_struct_element* element = drpc_que_pop(element_queue);
        char* key = drpc_que_pop(key_queue);

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

        char* keyed_buf = malloc(strlen(key) + 1 + drpc_type_buflen(packed_type)); assert(keyed_buf);
        char* edit_buf = keyed_buf;

        memcpy(edit_buf,key,strlen(key) + 1); edit_buf += strlen(key) + 1;

        drpc_buf(packed_type,edit_buf);

        packed[i].packed_data = keyed_buf;
        packed[i].type = type;
        packed[i].len = strlen(key) + 1 + drpc_type_buflen(packed_type);

        if(element->is_packed == 0) {
            drpc_type_free(packed_type);
            free(packed_type);
        }
    }
    drpc_que_free(element_queue);
    drpc_que_free(key_queue);

    *buflen = drpc_types_buflen(packed,dstruct->current_len);
    char* buf = malloc(*buflen); assert(buf);

    drpc_types_buf(packed,dstruct->current_len,buf);
    drpc_types_free(packed,dstruct->current_len);

    return buf;
}
void buf_d_struct(char* buf, struct d_struct* dstruct){
    size_t packed_types_len = 0;
    struct drpc_type* packed_types = buf_drpc_types(buf,&packed_types_len);

    for(size_t i = 0; i < packed_types_len; i++){
        if(packed_types[i].type == d_void) continue;
        char* key = strdup(packed_types[i].packed_data);
        drpc_que_push(dstruct->heap_keys,key);

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

    char** keys = calloc(*len,sizeof(char**)); assert(keys != NULL);

    struct drpc_que* keys_queue = drpc_que_create();
    for(size_t i = 0; i < dstruct->hashtable->capacity; i++){
        if(dstruct->hashtable->body[i].value != NULL && dstruct->hashtable->body[i].key != NULL && dstruct->hashtable->body[i].key != (char*)0xDEAD){
            drpc_que_push(keys_queue,dstruct->hashtable->body[i].key);
        }
    }

    size_t elements_len = drpc_que_get_len(keys_queue);
    for(size_t i = 0 ; i <elements_len; i++){
       keys[i] = drpc_que_pop(keys_queue);
    }
    drpc_que_free(keys_queue);

    return keys;
}

void d_struct_free_internal(struct d_struct* dstruct){
    struct drpc_que* element_queue = drpc_que_create();
    for(size_t i = 0; i < dstruct->hashtable->capacity; i++){
        if(dstruct->hashtable->body[i].value != NULL && dstruct->hashtable->body[i].key != NULL && dstruct->hashtable->body[i].key != (char*)0xDEAD)
            drpc_que_push(element_queue,dstruct->hashtable->body[i].value);
    }

    size_t elements_len = drpc_que_get_len(element_queue);
    for(size_t i = 0 ; i < elements_len; i++){
        struct d_struct_element* element = drpc_que_pop(element_queue);
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
    hashtable_destroy(dstruct->hashtable);
    drpc_que_free(element_queue);
///////////
    char* heap_key = NULL;
    while((heap_key = drpc_que_pop(dstruct->heap_keys)) != NULL){
        free(heap_key);
    }
    drpc_que_free(dstruct->heap_keys);
///////////

}
void d_struct_free(struct d_struct* dstruct){
    if(dstruct == NULL) return;
    d_struct_free_internal(dstruct);
    free(dstruct);
}
