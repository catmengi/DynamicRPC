#include "drpc_queue.h"
#include "queue.h"
#include "drpc_struct.h"
#include "drpc_array.h"
#include "drpc_types.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct d_queue* new_d_queue(){
    struct d_queue* dque = malloc(sizeof(*dque)); assert(dque);

    dque->que = queue_create();

    return dque;
}

void d_queue_push(struct d_queue* dqueue, void* native_type, enum drpc_types type,...){
    assert(dqueue); assert(native_type);
    struct d_struct_element* element = calloc(1,sizeof(*element)); assert(element);

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
            return;

    }
    queue_push(dqueue->que,element);
}

int d_queue_pop(struct d_queue* dqueue, void* native_type, enum drpc_types type,...){
    assert(native_type);
    if(dqueue == NULL) return 1;
    struct d_struct_element* element = queue_pop(dqueue->que);
    if(element == NULL) return 1;
    if(element->type != type){
        queue_push(dqueue->que,element);
        return 1;
    }

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
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint8:
            *(uint8_t*)native_type = drpc_to_uint8(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int16:
            *(int16_t*)native_type = drpc_to_int16(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint16:
            *(uint16_t*)native_type = drpc_to_uint16(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int32:
            *(int32_t*)native_type = drpc_to_int32(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint32:
            *(uint32_t*)native_type = drpc_to_uint32(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int64:
            *(int64_t*)native_type = drpc_to_int64(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint64:
            *(uint64_t*)native_type = drpc_to_uint64(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_float:
            *(float*)native_type = drpc_to_float(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_double:
            *(double*)native_type = drpc_to_double(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
    }
    free(element);
    return 0;
}

int d_queue_pop_with_type(struct d_queue* dqueue, void* native_type, enum drpc_types* out_type,...){
    assert(native_type);
    if(dqueue == NULL) return 1;
    struct d_struct_element* element = queue_pop(dqueue->que);
    if(element == NULL) return 1;
    enum drpc_types type = element->type;
    switch(type){
        default:
            *(void**)native_type = element->data;
            if(type == d_sizedbuf){
                va_list varargs;
                va_start(varargs,out_type);
                size_t* sizedbuf_len = va_arg(varargs,size_t*);
                *sizedbuf_len = element->sizedbuf_len;
            }
            break;
        case d_int8:
            *(int8_t*)native_type = drpc_to_int8(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint8:
            *(uint8_t*)native_type = drpc_to_uint8(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int16:
            *(int16_t*)native_type = drpc_to_int16(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint16:
            *(uint16_t*)native_type = drpc_to_uint16(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int32:
            *(int32_t*)native_type = drpc_to_int32(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint32:
            *(uint32_t*)native_type = drpc_to_uint32(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_int64:
            *(int64_t*)native_type = drpc_to_int64(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_uint64:
            *(uint64_t*)native_type = drpc_to_uint64(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_float:
            *(float*)native_type = drpc_to_float(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
        case d_double:
            *(double*)native_type = drpc_to_double(element->data);
            drpc_type_free(element->data);
            free(element->data);
            break;
    }
    if(out_type != NULL)
        *out_type = type;
    free(element);
    return 0;
}

void d_queue_free_internals(struct d_queue* dqueue){
    if(dqueue == NULL) return;
    if(dqueue->que == NULL) return;
    size_t que_len = queue_count(dqueue->que);

    for(size_t i = 0; i < que_len; i++){
        struct d_struct_element* element = queue_pop(dqueue->que);
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

    queue_free(dqueue->que);
}
void d_queue_free(struct d_queue* dqueue){
    if(dqueue == NULL) return;
    d_queue_free_internals(dqueue);
    free(dqueue);
}
char* d_queue_buf(struct d_queue* dqueue,size_t* buflen){
    size_t dqueue_len = d_queue_len(dqueue);
    size_t alloc_len = (dqueue_len == 0 ? 1 : dqueue_len);
    struct drpc_type* packed_types = calloc(alloc_len,sizeof(*packed_types)); assert(packed_types);
    for(size_t i = 0; i < dqueue_len; i++){
        struct d_struct_element* element = queue_pop(dqueue->que);
        if(element == NULL){
            packed_types[i].type = d_void;
            packed_types[i].len = 0;
            packed_types[i].packed_data = NULL;
        } else {
            if(element->is_packed == 0){
                struct drpc_type* packed_type = malloc(sizeof(*packed_type)); assert(packed_type);
                switch(element->type){
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
                size_t packed_len = drpc_type_buflen(packed_type);
                char* buf = malloc(packed_len); assert(buf);
                drpc_buf(packed_type,buf);

                packed_types[i].type = element->type;
                packed_types[i].len = packed_len;
                packed_types[i].packed_data = buf;

                drpc_type_free(packed_type);
                free(packed_type);
            } else {
                packed_types[i].type = element->type;
                packed_types[i].len = drpc_type_buflen(element->data);
                packed_types[i].packed_data = malloc(packed_types[i].len); assert(packed_types[i].packed_data);
                drpc_buf(element->data,packed_types[i].packed_data);
            }
        }
        queue_push(dqueue->que,element);
    }
    *buflen = drpc_types_buflen(packed_types,dqueue_len);
    char* outbuf = malloc(*buflen); assert(outbuf);

    drpc_types_buf(packed_types,dqueue_len,outbuf);

    drpc_types_free(packed_types,dqueue_len);
    return outbuf;
}
void buf_d_queue(char* buf, struct d_queue* dqueue){
    size_t packed_types_len = 0;
    struct drpc_type* packed_types = buf_drpc_types(buf,&packed_types_len);

    for(size_t i = 0; i < packed_types_len; i++){
        if(packed_types[i].type == d_void) continue;

        void* type_packed = packed_types[i].packed_data;
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
                element->data =  drpc_to_d_queue(type);
                drpc_type_free(type);free(type);
                break;
            default:
                element->is_packed = 1;
                element->type = packed_types[i].type;
                element->data = malloc(sizeof(struct drpc_type)); assert(element->data);
                buf_drpc(element->data,packed_types[i].packed_data);
                break;
        }
        queue_push(dqueue->que,element);
    }
    drpc_types_free(packed_types,packed_types_len);
}

size_t d_queue_len(struct d_queue* dqueue){
    size_t len = queue_count(dqueue->que);
    return len;
}
