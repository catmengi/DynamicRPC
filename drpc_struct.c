#include "drpc_types.h"
#include "drpc_struct.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct d_struct* new_d_struct(){
    struct d_struct* d_struct = malloc(sizeof(*d_struct)); assert(d_struct);

    hashtable_create(&d_struct->hashtable,16,2);
    d_struct->current_len = 0;

    return d_struct;
}

void d_struct_set(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...){
    assert(dstruct); assert(key); assert(native_type); assert(type > 0);
    struct d_struct_element* element = NULL;

    if(hashtable_get(dstruct->hashtable,key,strlen(key) + 1,(void**)&element) == ENOTFOUND){
        element = malloc(sizeof(*element)); assert(element);
        hashtable_add(dstruct->hashtable,key,strlen(key) + 1,element,0);
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
                case d_array:
                    //d_array_free(element->data);
                    break;
                case d_struct:
                    //d_struct_free(element->data);
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
        case d_array:
            element->type = type;
            element->is_packed = 0;
            element->data = native_type;
            break;
        default: break;
    }
    dstruct->current_len++;
}

int d_struct_get(struct d_struct* dstruct,char* key, void* native_type, enum drpc_types type,...){
    assert(dstruct); assert(key); assert(native_type); assert(type > 0);
    struct d_struct_element* element = NULL;
    int ret = hashtable_get(dstruct->hashtable,key,strlen(key) + 1,(void**)&element);
    if(ret != 0) return ret;
    if(element->type != type) return 1;

    switch(type){
        default:
            *(void**)native_type = element->data;
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
    struct d_struct_element* element = NULL;
    int ret = hashtable_get(dstruct->hashtable,key,strlen(key) + 1,(void**)&element);
    if(ret != 0) return ret;
    if(element->type != type || element->is_packed == 1) return 1;
    element->data = NULL;

    dstruct->current_len--;
    return 0;
}

int d_struct_remove(struct d_struct* dstruct, char* key){
    struct d_struct_element* element = NULL;
    int ret = hashtable_get(dstruct->hashtable,key,strlen(key) + 1,(void**)&element);
    if(ret != 0) return ret;

    dstruct->current_len--;

    if(element->is_packed == 1){
        drpc_type_free(element->data);
        free(element->data);
        hashtable_remove_entry(dstruct->hashtable,key,strlen(key) + 1);
        free(element);
        return 0;
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
        case d_array:
            //d_array_free(element->data);
            break;
    }
    free(element);
    return 0;
}

void d_struct_free_CB(void* element_ptr){
    struct d_struct_element* element = element_ptr;
    if(element->is_packed == 1){
        drpc_type_free(element->data);
        free(element->data);
        free(element);
        return;
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
        case d_array:
            //d_array_free(element->data);
            break;
    }
    free(element);
}

void d_struct_free(struct d_struct* dstruct){
    hashtable_iterate(dstruct->hashtable,d_struct_free_CB);
    hashtable_free(dstruct->hashtable);
    free(dstruct);
}
