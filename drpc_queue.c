#include "drpc_queue.h"
#include "drpc_que.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

struct d_queue* new_d_queue(){
    struct d_queue* dque = malloc(sizeof(*dque)); assert(dque);
    assert(pthread_mutex_init(&dque->lock,NULL) == 0);

    dque->que = drpc_que_create();

    return dque;
}

void d_queue_push(struct d_queue* dqueue, void* native_type, enum drpc_types type,...){
    assert(dqueue); assert(native_type);
    pthread_mutex_lock(&dqueue->lock);
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
            pthread_mutex_unlock(&dqueue->lock);
            return;

    }
    drpc_que_push(dqueue->que,element);
    pthread_mutex_unlock(&dqueue->lock);
}

int d_queue_pop(struct d_queue* dqueue, void* native_type, enum drpc_types type,...){
    assert(dqueue); assert(native_type);
    pthread_mutex_lock(&dqueue->lock);
    if(dqueue->que->cur == NULL) {pthread_mutex_unlock(&dqueue->lock);return 1;}

    struct d_struct_element* check = dqueue->que->cur->ptr;
    if(check->type != type) {pthread_mutex_unlock(&dqueue->lock);return 1;}

    struct d_struct_element* element = drpc_que_pop(dqueue->que);
    assert(element != NULL);

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
    pthread_mutex_unlock(&dqueue->lock);
    return 0;
}
void d_queue_free_internals(struct d_queue* dqueue){
    size_t que_len = drpc_que_get_len(dqueue->que);

    for(size_t i = 0; i < que_len; i++){
        struct d_struct_element* element = drpc_que_pop(dqueue->que);
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
                    //d_array_free(element->data);
                    break;
            }
        }
        free(element);
    }

    drpc_que_free(dqueue->que);
}
void d_queue_free(struct d_queue* dqueue){
    d_queue_free_internals(dqueue);
    free(dqueue);
}
char* d_queue_buf(struct d_queue* dqueue,size_t* buflen){
    size_t packed_types_len = drpc_que_get_len(dqueue->que);
    struct drpc_type* packed = calloc(packed_types_len,sizeof(*packed)); assert(packed);

    for(size_t index = 0; index < packed_types_len; index++){
        struct d_struct_element* element = drpc_que_pop(dqueue->que);

        enum drpc_types type = element->type;
        struct drpc_type* packed_type = NULL;
        if(element->is_packed == 1){
            packed_type = element->data;
        }else{
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
        }
        char* buf = malloc(drpc_type_buflen(packed_type)); assert(buf);

        drpc_buf(packed_type,buf);

        packed[index].packed_data = buf;
        packed[index].type = type;
        packed[index].len = drpc_type_buflen(packed_type);

        if(element->is_packed == 0) {
            drpc_type_free(packed_type);
            free(packed_type);
        }
        drpc_que_push(dqueue->que,element);
    }


    *buflen = drpc_types_buflen(packed,packed_types_len);
    char* buf = malloc(*buflen); assert(buf);

    drpc_types_buf(packed,packed_types_len,buf);

    drpc_types_free(packed,packed_types_len);

    return buf;
}
void buf_d_queue(char* buf, struct d_queue* dqueue){
    size_t packed_types_len = 0;
    struct drpc_type* packed_types = buf_drpc_types(buf,&packed_types_len);

    for(size_t i = 0; i < packed_types_len; i++){
        void* type_packed = packed_types[i].packed_data;
        struct drpc_type* type = NULL;

        struct d_struct_element* element = calloc(1,sizeof(*element));
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

                buf_drpc(element->data,type_packed);
                break;
        }
        drpc_que_push(dqueue->que,element);
    }
    drpc_types_free(packed_types,packed_types_len);
}

size_t d_queue_len(struct d_queue* dqueue){
    size_t len = drpc_que_get_len(dqueue->que);
    return len;
}
enum drpc_types d_queue_top_type(struct d_queue* dqueue){
    if(dqueue == NULL) return d_void;
    pthread_mutex_lock(&dqueue->lock);
    if(dqueue->que->cur == NULL) {pthread_mutex_unlock(&dqueue->lock);return d_void;}


    enum drpc_types ret = ((struct d_struct_element*)dqueue->que->cur->ptr)->type;
    pthread_mutex_unlock(&dqueue->lock);
    return ret;
}
