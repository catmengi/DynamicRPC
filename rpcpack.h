#include <stdint.h>
#include <sys/types.h>
#ifndef RPCTYPES_H
#define RPCTYPES_H
#include "rpctypes.h"
#endif
#define RPCPACK_H
struct packdim_arr{
    uint64_t len;
    char* data;
};
int create_rpcstruct_type(struct rpcstruct* rpcstruct, char flag, struct rpctype* type);
struct rpcstruct* unpack_rpcstruct_type(struct rpctype* type);
int create_rpcbuff_type(struct rpcbuff* rpcbuff, char flag,struct rpctype* type);
struct rpcbuff* unpack_rpcbuff_type(struct rpctype* type);
int create_sizedbuf_type(char* buf, uint64_t buflen, char flag,struct rpctype* type);
void* unpack_sizedbuf_type(struct rpctype* type,uint64_t* len);
int create_str_type(char* str,char flag,struct rpctype* type);
char* unpack_str_type(struct rpctype* type);
int type_to_arr(char* out,struct rpctype* type);
struct rpctype* arr_to_type(char* rawarr,struct rpctype* type);
uint64_t type_buflen(struct rpctype* type);

int char_to_type(char c,struct rpctype* type);
int int16_to_type(int16_t i16,struct rpctype* type);
int uint16_to_type(uint16_t i16,struct rpctype* type);
int int32_to_type(int32_t i32,struct rpctype* type);
int uint32_to_type(uint32_t i32,struct rpctype* type);
int int64_to_type(uint64_t i64,struct rpctype* type);
int uint64_to_type(int64_t i64,struct rpctype* type);
int float_to_type(float pfloat,struct rpctype* type);
int double_to_type(double pdouble,struct rpctype* type);

char type_to_char(struct rpctype* type);
int16_t type_to_int16(struct rpctype* type);
uint16_t type_to_uint16(struct rpctype* type);
int32_t type_to_int32(struct rpctype* type);
uint32_t type_to_uint32(struct rpctype* type);
int64_t type_to_int64(struct rpctype* type);
uint64_t type_to_uint64(struct rpctype* type);
float type_to_float(struct rpctype* type);
double type_to_double(struct rpctype* type);
