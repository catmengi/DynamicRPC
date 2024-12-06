#pragma once


#include <sys/types.h>
#include <stdint.h>
#include <ffi.h>





enum drpc_types{
/*  simple non resendable types  */
    d_void = 0,
    d_char = 1,
    d_int8 = 1,
    d_uint8,
    d_int16,
    d_uint16,
    d_int32,
    d_uint32,
    d_int64,
    d_uint64,
    d_float,
    d_double,


/*  always resend this types, to emulate same behavior as C pointers  */
    d_str,
    d_sizedbuf,
    d_array,
    d_struct,
    d_queue,

    d_fn_pstorage,
    d_clientinfo,
    d_interfunc,

    d_return_is,   //this type will be used to say to the client that return is SAME as one of arguments, and to be provided to function

};

static ffi_type* drpc_ffi_convert_table[d_return_is] =
{
    &ffi_type_void,     &ffi_type_sint8,
    &ffi_type_uint8,    &ffi_type_sint16,
    &ffi_type_uint16,   &ffi_type_sint32,
    &ffi_type_uint32,   &ffi_type_sint64,
    &ffi_type_uint64,   &ffi_type_float,
    &ffi_type_double,   &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer,  &ffi_type_pointer,
    &ffi_type_pointer

};

struct drpc_type{
    char type;
    size_t len;
    char* packed_data;
};

#include "drpc_struct.h"

size_t drpc_proto_to_ffi_len_adjust(enum  drpc_types* prototype, size_t prototype_len);
ffi_type** drpc_proto_to_ffi(enum drpc_types* prototype, size_t prototype_len);



size_t drpc_type_buflen(struct drpc_type* type);

void void_to_drpc(struct drpc_type* type);

void int8_to_drpc(struct drpc_type* type, int8_t native);
void uint8_to_drpc(struct drpc_type* type, uint8_t native);
void int16_to_drpc(struct drpc_type* type, int16_t native);
void uint16_to_drpc(struct drpc_type* type, uint16_t native);
void int32_to_drpc(struct drpc_type* type, int32_t native);
void uint32_to_drpc(struct drpc_type* type, uint32_t native);
void int64_to_drpc(struct drpc_type* type, int64_t native);
void uint64_to_drpc(struct drpc_type* type, uint64_t native);

void float_to_drpc(struct drpc_type* type, float native);
void double_to_drpc(struct drpc_type* type, double native);

void return_is_to_drpc(struct drpc_type* type, uint8_t native);

void str_to_drpc(struct drpc_type* type, char* native);
void sizedbuf_to_drpc(struct drpc_type* type, char* buf, size_t buflen);
void d_array_to_drpc(struct drpc_type* type, void* d_array);
void d_struct_to_drpc(struct drpc_type* type, void* dstruct);
void d_queue_to_drpc(struct drpc_type* type, void* dqueue);





int8_t drpc_to_int8(struct drpc_type* type);
uint8_t drpc_to_uint8(struct drpc_type* type);
int8_t drpc_to_int16(struct drpc_type* type);
uint64_t drpc_to_uint16(struct drpc_type* type);
int64_t drpc_to_int32(struct drpc_type* type);
uint64_t drpc_to_uint32(struct drpc_type* type);
int64_t drpc_to_int64(struct drpc_type* type);
uint64_t drpc_to_uint64(struct drpc_type* type);
float drpc_to_float(struct drpc_type* type);
double drpc_to_double(struct drpc_type* type);
uint8_t drpc_to_return_is(struct drpc_type* type);

char* drpc_to_sizedbuf(struct drpc_type* type, size_t* len);

char* drpc_to_str(struct drpc_type* type);
void* drpc_to_d_array(struct drpc_type* type);
struct d_struct* drpc_to_d_struct(struct drpc_type* type);
struct d_queue* drpc_to_d_queue(struct drpc_type* type);

size_t drpc_buflen(struct drpc_type* type);
size_t drpc_buf(struct drpc_type* type, char* buf);

size_t buf_drpc(struct drpc_type* type, char* buf);

size_t drpc_types_buflen(struct drpc_type* types, size_t len);

void drpc_types_buf(struct drpc_type* types,size_t len,char* buf);
struct drpc_type* buf_drpc_types(char* buf, size_t *len);

void drpc_types_free(struct drpc_type* types, size_t len);
void drpc_type_free(struct drpc_type* type);
