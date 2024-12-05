#include "drpc_types.h"
#include "drpc_struct.h"
#include "drpc_queue.h"

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>


#define drpc_convert(type,native) type->packed_data = malloc(sizeof(native)); assert(type->packed_data); type->len = sizeof(native); memcpy(type->packed_data,&native, sizeof(native));
#define drpc_deconvert(type,native_type) native_type ret = 0; memcpy(&ret,type->packed_data,sizeof(ret)); return ret;

size_t drpc_proto_to_ffi_len_adjust(enum drpc_types* prototype, size_t prototype_len){
    size_t adjusted_len = prototype_len;
    for(size_t i = 0; i < prototype_len; i++){
        if(prototype[i] == d_sizedbuf) adjusted_len++;
    }
    return adjusted_len;
}

ffi_type** drpc_proto_to_ffi(enum drpc_types* prototype, size_t prototype_len){
    if(prototype == NULL) return NULL;

    size_t adjusted_len = drpc_proto_to_ffi_len_adjust(prototype,prototype_len);
    ffi_type** ffi_proto = calloc(adjusted_len + 1,sizeof(ffi_type*)); assert(ffi_proto);
    size_t j = 0;

    for(size_t i = 0; i < prototype_len; i++,j++){
        if(prototype[i] >= d_return_is) prototype[i] = d_void;

        ffi_proto[j] = drpc_ffi_convert_table[prototype[i]];
        if(prototype[i] == d_sizedbuf){
            j++;
            ffi_proto[j] = &ffi_type_ulong;
        }
    }

    return ffi_proto;
}


size_t drpc_type_buflen(struct drpc_type* type){
    return 1 + sizeof(uint64_t) + type->len;
}

void void_to_drpc(struct drpc_type* type){
    type->type = d_void;
    type->len = 0;
    type->packed_data = NULL;
}

void int8_to_drpc(struct drpc_type* type, int8_t native){
    type->type = d_int8;
    drpc_convert(type,native);
}
void uint8_to_drpc(struct drpc_type* type, uint8_t native){
    type->type = d_uint8;
    drpc_convert(type,native);
}
void int16_to_drpc(struct drpc_type* type, int16_t native){
    type->type = d_int16;
    drpc_convert(type,native);
}
void uint16_to_drpc(struct drpc_type* type, uint16_t native){
    type->type = d_uint16;
    drpc_convert(type,native);
}
void int32_to_drpc(struct drpc_type* type, int32_t native){
    type->type = d_int32;
    drpc_convert(type,native);
}
void uint32_to_drpc(struct drpc_type* type, uint32_t native){
    type->type = d_uint32;
    drpc_convert(type,native);
}
void int64_to_drpc(struct drpc_type* type, int64_t native){
    type->type = d_int64;
    drpc_convert(type,native);
}
void uint64_to_drpc(struct drpc_type* type, uint64_t native){
    type->type = d_uint64;
    drpc_convert(type,native);
}

void float_to_drpc(struct drpc_type* type, float native){
    type->type = d_float;
    drpc_convert(type,native);
}
void double_to_drpc(struct drpc_type* type, double native){
    type->type = d_double;
    drpc_convert(type,native);
}

void return_is_to_drpc(struct drpc_type* type, uint8_t native){
    type->type = d_return_is;
    drpc_convert(type,native);
}

void str_to_drpc(struct drpc_type* type, char* native){
    type->type = d_str;
    type->packed_data = malloc(strlen(native) + 1);
    type->len = strlen(native) + 1;
    memcpy(type->packed_data,native,strlen(native) + 1);
}
void sizedbuf_to_drpc(struct drpc_type* type, char* buf, size_t buflen){
    type->type = d_sizedbuf;
    type->packed_data = malloc(buflen);
    type->len = buflen;
    memcpy(type->packed_data,buf,buflen);
}
void d_array_to_drpc(struct drpc_type* type, void* d_arrayp){
    type->type = d_array;
}
void d_struct_to_drpc(struct drpc_type* type, void* dstruct){
    type->type = d_struct;
    type->packed_data = d_struct_buf(dstruct,&type->len);
}
void d_queue_to_drpc(struct drpc_type* type, void* dqueue){
    type->type = d_queue;
    type->packed_data = d_queue_buf(dqueue,&type->len);
}





int8_t drpc_to_int8(struct drpc_type* type){
    drpc_deconvert(type,int8_t);
}
uint8_t drpc_to_uint8(struct drpc_type* type){
    drpc_deconvert(type,uint8_t);
}
int8_t drpc_to_int16(struct drpc_type* type){
    drpc_deconvert(type,int16_t);
}
uint64_t drpc_to_uint16(struct drpc_type* type){
    drpc_deconvert(type,uint16_t);
}
int64_t drpc_to_int32(struct drpc_type* type){
    drpc_deconvert(type,int32_t);
}
uint64_t drpc_to_uint32(struct drpc_type* type){
    drpc_deconvert(type,uint32_t);
}
int64_t drpc_to_int64(struct drpc_type* type){
    drpc_deconvert(type,int64_t);
}
uint64_t drpc_to_uint64(struct drpc_type* type){
    drpc_deconvert(type,uint64_t);
}
float drpc_to_float(struct drpc_type* type){
    drpc_deconvert(type,float);
}
double drpc_to_double(struct drpc_type* type){
    drpc_deconvert(type,double);
}
uint8_t drpc_to_return_is(struct drpc_type* type){
    drpc_deconvert(type,uint8_t);
}

char* drpc_to_sizedbuf(struct drpc_type* type, size_t* len){
    char* ret = malloc(type->len);
    assert(ret);
    memcpy(ret,type->packed_data,type->len);
    *len = type->len;
    return ret;
}

char* drpc_to_str(struct drpc_type* type){
    char* ret = malloc(type->len);
    assert(ret);
    memcpy(ret,type->packed_data, type->len);
    return ret;
}
void* drpc_to_d_array(struct drpc_type* type){


    return NULL;
}
struct d_struct* drpc_to_d_struct(struct drpc_type* type){
    struct d_struct* ret = new_d_struct();
    buf_d_struct(type->packed_data,ret);

    return ret;
}
struct d_queue* drpc_to_d_queue(struct drpc_type* type){
    struct d_queue* ret = new_d_queue();
    buf_d_queue(type->packed_data,ret);

    return ret;
}


size_t drpc_buflen(struct drpc_type* type){
    size_t len = 1;
    if(type->type == d_void) return len;

    len += sizeof(uint64_t);
    len += type->len;

    return len;
}

size_t drpc_buf(struct drpc_type* type, char* buf){
    *buf = type->type; buf++;
    if(type->type == d_void) goto exit;

    uint64_t len64 = type->len;
    memcpy(buf,&len64,sizeof(uint64_t)); buf += sizeof(uint64_t);
    memcpy(buf,type->packed_data,type->len);

exit:
    return drpc_type_buflen(type);
}

size_t buf_drpc(struct drpc_type* type, char* buf){
    memset(type,0,sizeof(*type));

    type->type = *buf; buf++;
    if(type->type == d_void) goto exit;

    uint64_t len64 = 0;
    memcpy(&len64,buf,sizeof(uint64_t)); buf += sizeof(uint64_t);
    type->len = (size_t)len64;

    type->packed_data = malloc(type->len); assert(type->len);
    memcpy(type->packed_data,buf,type->len);
exit:
    return drpc_type_buflen(type);
}

size_t drpc_types_buflen(struct drpc_type* types, size_t len){
    size_t ret = sizeof(uint64_t);
    for(size_t i = 0; i < len; i++){
        ret += drpc_buflen(&types[i]);
    }
    return ret;
}

void drpc_types_buf(struct drpc_type* types,size_t len,char* buf){
    uint64_t len64 = len;
    memcpy(buf,&len64,sizeof(uint64_t)); buf += sizeof(uint64_t);

    for(size_t i = 0; i < len; i++){
        buf += drpc_buf(&types[i],buf);
    }
}
struct drpc_type* buf_drpc_types(char* buf, size_t *len){
    uint64_t len64 = 0;
    memcpy(&len64,buf,sizeof(uint64_t)); buf += sizeof(uint64_t);

    *len = (size_t)len64;

    struct drpc_type* types = calloc(*len, sizeof(*types));

    for(size_t i = 0; i < *len; i++){
        buf += buf_drpc(&types[i],buf);
    }
    return types;
}

void drpc_type_free(struct drpc_type* type){
    free(type->packed_data);
}

void drpc_types_free(struct drpc_type* types, size_t len){
    for(size_t i = 0; i < len; i++){
        drpc_type_free(&types[i]);
    }
    free(types);
}
