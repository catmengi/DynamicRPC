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
struct rpcbuff* rpcbuff_create(uint64_t* dimsizes,uint64_t dimsizes_len);  /*Multi-dimensional array
                                                                            Copies and pack type, not just storing pointer; dimsizes is a index(size) of array*/

void rpcbuff_free_internals(struct rpcbuff* rpcbuff); //same as rpcbuff_free but dont free struct rpcbuff* (only internals of it);

void rpcbuff_free(struct rpcbuff* rpcbuff);           //destroys rpcbuff, and free data that was copyed to it;

int rpcbuff_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len,void* otype,uint64_t* otype_len,enum rpctypes type);  //unpacks data from rpcbuff

int rpcbuff_pushto(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len, void* ptype,uint64_t type_len,enum rpctypes type);  //serialise data type and store it in rpcbuff

void rpcbuff_remove(struct rpcbuff* rpcbuff, uint64_t* index, size_t index_len); /*Removes and frees serialized data in rpcbuff*/

char* rpcbuff_to_buf(struct rpcbuff* rpcbuff,uint64_t* buflen);          //rpcbuff serializer(NOT FOR MANUAL CALL);

struct rpcbuff* buf_to_rpcbuff(char* buf);                               //rpcbuff deserializer(NOT FOR MANUAL CALL);

struct rpcbuff_el* rpcbuff_el_getlast_from(struct  rpcbuff* rpcbuff, uint64_t* index, size_t index_len);


struct rpcstruct* rpcstruct_create();                                          /*Hashtable based struct, copies and pack type, not just storing pointer;*/
void rpcstruct_free(struct rpcstruct* rpcstruct);
char** rpcstruct_get_fields(struct rpcstruct* rpcstruct, size_t* fields_len);  /*Get all fields(keys) in rpcstruct, ammount of them will be placed in fields_len*/
int rpcstruct_remove(struct rpcstruct* rpcstruct, char* key);                  /*Remove key and free it*/
int rpcstruct_set_flag(struct rpcstruct* rpcstruct, char* key, char flag);     /*Same as remove but not immediate(will be removed after serialization), 1 - NOT REMOVE, 0 - REMOVE*/
int buf_to_rpcstruct(char* arr, struct rpcstruct* rpcstruct);                  /*Serialize (NOT FOR MANUAL CALL)*/
char* rpcstruct_to_buf(struct rpcstruct* rpcstruct, uint64_t* buflen);         /*Desirialize (NOT FOR MANUAL CALL)*/
int rpcstruct_get(struct rpcstruct* rpcstruct,char* key,enum rpctypes type,void* out,uint64_t* obuflen);  /*serialise data type and store it in rpcstruct*/
int rpcstruct_set(struct rpcstruct* rpcstruct,char* key,enum rpctypes type, void* arg,uint64_t typelen);  /*unpacks data from rpcstruct*/
void rpcstruct_free_internals(struct rpcstruct* rpcstruct);
