#pragma once
#include <stdint.h>
#include <sys/types.h>
#include "hashtable.c/hashtable.h"
enum rpctypes{
    VOID = 0,
    CHAR = 1,
    STR = 2,
    UINT16 = 3,
    INT16 = 4,
    UINT32 = 5,
    INT32 = 6,
    UINT64 = 7,
    INT64 = 8,
    FLOAT = 9,
    DOUBLE = 10,
    RPCBUFF = 11,
    SIZEDBUF = 12, //1D array that will pass uint64_t typed value as his len in next arg: "void*, uint64_t"
    PSTORAGE = 13,
    INTERFUNC = 14,
    RPCSTRUCT = 15,
    _RPCSTRUCTEL = 16,
    UNIQSTR = 17,   //random str of 64symbols, uniq for client
};

struct rpcbuff_el{
        struct rpcbuff_el* childs;
        enum rpctypes type;
        uint64_t elen;
        char* endpoint;
};
struct rpcbuff{
    struct rpcbuff_el* start;
    uint64_t* dimsizes;
    uint64_t dimsizes_len;
};
struct rpctype{
  char type;
  char flag;
  uint64_t datalen;
  char* data;
};
struct rpcstruct{
  hashtable_t ht;
  uint64_t count;
};
struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len);  /*lastdim_len set default last dimension len(rpcbuff_getlast_from create this size lastdim if
                                                                                                this  endpoint wasnt writen by rpcbuff_pushto), but rpcbuff_pushto will fit bigger data*/
void rpcbuff_free_internals(struct rpcbuff* rpcbuff); //same as _rpcbuff_free but dont free struct rpcbuff* (only internals of it)
void rpcbuff_free(struct rpcbuff* rpcbuff);
int rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len,void* otype,uint64_t* otype_len,enum rpctypes type);
int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len, void* ptype,uint64_t type_len,enum rpctypes type);
char* rpcbuff_to_arr(struct rpcbuff* rpcbuff,uint64_t* buflen);
struct rpcbuff* buf_to_rpcbuff(char* buf);
struct rpcbuff_el* rpcbuff_el_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, uint64_t index_len);


struct rpcstruct* rpcstruct_create();
void rpcstruct_free(struct rpcstruct* rpcstruct);
char** rpcstruct_get_fields(struct rpcstruct* rpcstruct, uint64_t* fields_len);
int rpcstruct_remove(struct rpcstruct* rpcstruct, char* key);
int rpcstruct_set_flag(struct rpcstruct* rpcstruct, char* key, char flag);
int buf_to_rpcstruct(char* arr, struct rpcstruct* rpcstruct);
char* rpcstruct_to_buf(struct rpcstruct* rpcstruct, uint64_t* buflen);
int rpcstruct_get(struct rpcstruct* rpcstruct,char* key,enum rpctypes type,void* out,uint64_t* obuflen);
int rpcstruct_set(struct rpcstruct* rpcstruct,char* key,enum rpctypes type, void* arg,size_t typelen);
void rpcstruct_free_internals(struct rpcstruct* rpcstruct);
