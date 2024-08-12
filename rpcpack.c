#include "rpctypes.h"
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <libubox/utils.h>
int create_rpcstruct_type(struct rpcstruct* rpcstruct, char flag, struct rpctype* type){
    assert(rpcstruct);
    uint64_t buflen = 0;
    type->type = RPCSTRUCT;
    type->flag = flag;
    type->data = rpcstruct_to_buf(rpcstruct,&buflen);
    type->datalen = cpu_to_be64(buflen);
    return 0;
}
struct rpcstruct* unpack_rpcstruct_type(struct rpctype* type){
    assert(type);
    if(type->type != RPCSTRUCT) return NULL;
    struct rpcstruct* rpcstruct = malloc(sizeof(*rpcstruct)); assert(rpcstruct);
    buf_to_rpcstruct(type->data,rpcstruct);
    return rpcstruct;
}
int create_rpcbuff_type(struct rpcbuff* rpcbuff, char flag,struct rpctype* type){
    assert(rpcbuff);
    uint64_t buflen = 0;
    type->type = RPCBUFF;
    type->flag = flag;
    type->data = rpcbuff_to_arr(rpcbuff,&buflen);
    type->datalen = cpu_to_be64(buflen);
    return 0;
}
struct rpcbuff* unpack_rpcbuff_type(struct rpctype* type){
    assert(type);
    if(type->type != RPCBUFF) return NULL;
    return buf_to_rpcbuff(type->data);
}
int create_sizedbuf_type(char* buf, uint64_t buflen, char flag,struct rpctype* type){
    type->type = SIZEDBUF;
    type->flag = flag;
    type->data = calloc(buflen,sizeof(char)); assert(type->data);
    memcpy(type->data, buf, buflen);
    type->datalen = cpu_to_be64(buflen);
    return 0;
}
void* unpack_sizedbuf_type(struct rpctype* type,uint64_t* len){
    assert(type);
    if(type->type != SIZEDBUF) return NULL;
    *len = be64_to_cpu(type->datalen);
    return type->data;
}

int create_str_type(char* str,char flag,struct rpctype* type){
    if(str == NULL) return 1;
    uint64_t strfulllen = strlen(str) + 1;
    type->type = STR;
    type->datalen = cpu_to_be64(strfulllen);
    type->data = calloc(strfulllen, sizeof(char));
    if(flag > 3) return 1;
    type->flag = flag;
    assert(type->data);
    memcpy(type->data, str, strfulllen);
    return 0;
}

char* unpack_str_type(struct rpctype* type){
    if(type == NULL) return NULL;
    if(type->type != STR) return NULL;
    char* data = type->data;
    return data;
}
uint64_t type_buflen(struct rpctype* type){
    if(type == NULL) return 0;
    uint64_t len = 0;
    len = (sizeof(char) * 2) + sizeof(uint64_t);
    if(type->type != VOID) len += be64_to_cpu(type->datalen);
    return len;
}
size_t type_to_arr(char* out,struct rpctype* type){
    if(type == NULL) return 0;
    assert(out);
    *out = type->type;
    out++;
    *out = type->flag;
    out++;
    memcpy(out, &type->datalen, sizeof(uint64_t));
    out += sizeof(uint64_t);
    memcpy(out, type->data, be64_to_cpu(type->datalen));
    size_t ret =  type_buflen(type);
    return ret;
}
struct rpctype* arr_to_type(char* rawarr,struct rpctype* type){
    if(!rawarr) return NULL;
    type->type = *rawarr;
    if(type->type == VOID) return NULL;
    rawarr++;
    type->flag = *rawarr;
    rawarr++;
    if(type->type != VOID){
        memcpy(&type->datalen,rawarr, sizeof(uint64_t));
        rawarr += sizeof(uint64_t);
        type->data = calloc(be64_to_cpu(type->datalen), sizeof(char));
        memcpy(type->data, rawarr, be64_to_cpu(type->datalen));
    }else type->datalen = 0;
    return type;
}

int __bytearr_to_type(char* __raw,size_t len,enum rpctypes dtype,struct rpctype* type){
    assert(__raw || len == 0);
    char* raw = __raw;
    type->type = dtype;
    type->flag = 0;
    type->datalen = cpu_to_be64((uint64_t)len);
    if(len > 0)
        type->data = calloc(be64_to_cpu(type->datalen), sizeof(char));
    assert(type->data || len == 0);
    memcpy(type->data, raw, len);
    return 0;
}

char* __type_to_bytearr(struct rpctype* type,size_t len,enum rpctypes ctl_type){
    if(type == NULL) return NULL;
    if((enum rpctypes)type->type != ctl_type) return NULL;
    if(be64_to_cpu(type->datalen) > len || be64_to_cpu(type->datalen) < len) return NULL;
    char* out = malloc(be64_to_cpu(type->datalen));
    memcpy(out,type->data, be64_to_cpu(type->datalen));

    return out;
}
int char_to_type(char c,struct rpctype* type){
    return __bytearr_to_type(&c,sizeof(char),CHAR,type);
}
int int16_to_type(int16_t i16,struct rpctype* type){
    int16_t be16 = cpu_to_be16(i16);
    return __bytearr_to_type((char*)&be16,sizeof(int16_t),INT16,type);
}
int uint16_to_type(uint16_t i16,struct rpctype* type){
    uint16_t be16 = cpu_to_be16(i16);
    return __bytearr_to_type((char*)&be16,sizeof(uint16_t),UINT16,type);
}
int int32_to_type(int32_t i32,struct rpctype* type){
    int32_t be32 = cpu_to_be32(i32);
    return __bytearr_to_type((char*)&be32,sizeof(int32_t),INT32,type);
}
int uint32_to_type(uint32_t i32,struct rpctype* type){
    uint32_t be32 = cpu_to_be32(i32);
    return __bytearr_to_type((char*)&be32,sizeof(uint32_t),UINT32,type);
}
int int64_to_type(int64_t i64,struct rpctype* type){
    int64_t be64 = cpu_to_be64(i64);
    return __bytearr_to_type((char*)&be64,sizeof(int64_t),INT64,type);
}
int uint64_to_type(uint64_t i64,struct rpctype* type){
    uint64_t be64 = cpu_to_be64(i64);
    return __bytearr_to_type((char*)&be64,sizeof(uint64_t),UINT64,type);
}
int float_to_type(float pfloat,struct rpctype* type){
    float be32 = cpu_to_be32(pfloat);
    return __bytearr_to_type((char*)&be32,sizeof(float),FLOAT,type);
}
int double_to_type(double pdouble,struct rpctype* type){
    double be64 = cpu_to_be32(pdouble);
    return __bytearr_to_type((char*)&be64,sizeof(double),DOUBLE,type);
}

char type_to_char(struct rpctype* type){
    char c;
    char* tmp = __type_to_bytearr(type,sizeof(char),CHAR);
    if(!tmp) return 0;
    c = *tmp;
    free(tmp);

    return c;
}

int16_t type_to_int16(struct rpctype* type){
    int16_t c;
    char* tmp = __type_to_bytearr(type,sizeof(int16_t),INT16);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(int16_t));
    c = be16_to_cpu(c);
    free(tmp);

    return c;
}
uint16_t type_to_uint16(struct rpctype* type){
    uint16_t c;
    char* tmp = __type_to_bytearr(type,sizeof(uint16_t),UINT16);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(uint16_t));
    c = be16_to_cpu(c);
    free(tmp);

    return c;
}
int32_t type_to_int32(struct rpctype* type){
    int32_t c;
    char* tmp = __type_to_bytearr(type,sizeof(int32_t),INT32);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(int32_t));
    c = be32_to_cpu(c);
    free(tmp);

    return c;
}
uint32_t type_to_uint32(struct rpctype* type){
    uint32_t c;
    char* tmp = __type_to_bytearr(type,sizeof(uint32_t),UINT32);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(uint32_t));
    c = be32_to_cpu(c);
    free(tmp);

    return c;
}
int64_t type_to_int64(struct rpctype* type){
    int64_t c;
    char* tmp = __type_to_bytearr(type,sizeof(int64_t),INT64);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(int64_t));
    c = be64_to_cpu(c);
    free(tmp);

    return c;
}
uint64_t type_to_uint64(struct rpctype* type){
    uint64_t c;
    char* tmp = __type_to_bytearr(type,sizeof(uint64_t),UINT64);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(uint64_t));
    c = be64_to_cpu(c);
    free(tmp);

    return c;
}
float type_to_float(struct rpctype* type){
    float c;
    char* tmp = __type_to_bytearr(type,sizeof(float),FLOAT);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(float));
    c = be32_to_cpu(c);
    free(tmp);

    return c;
}

double type_to_double(struct rpctype* type){
    double c;
    char* tmp = __type_to_bytearr(type,sizeof(double),DOUBLE);
    if(!tmp) return 0;
    memcpy(&c,tmp,sizeof(double));
    c = be64_to_cpu(c);
    free(tmp);

    return c;
}
